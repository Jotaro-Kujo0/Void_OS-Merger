#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_SAMPLES         10
#define PROBE_COOLDOWN_MS   2000  
#define STALE_THRESHOLD_MS  10000 

extern void execute_test_suite(void);

typedef enum {
    SAMPLE_FRESH,
    SAMPLE_STALE,
    SAMPLE_UNAVAILABLE
} SampleFreshness;

typedef enum {
    THERMAL_NOMINAL = 0,
    THERMAL_THROTTLED,
    THERMAL_CRITICAL
} ThermalTier;

typedef struct {
    uint64_t timestamp_ms;
    SampleFreshness status;
    float cpu_utilization_pct;
    uint32_t effective_cpu_units_available;
    uint64_t free_ram_mb;
    uint64_t reclaimable_ram_mb;
    uint64_t free_storage_mb;
    uint32_t current_io_read_mbps;
    float battery_percentage;
    ThermalTier thermal_state;
} ResourceSample;

struct vom_monitor_context {
    pthread_mutex_t lock;
    ResourceSample history[MAX_SAMPLES];
    uint32_t head_idx;
    uint32_t sample_count;
    uint32_t reserved_cpu_units;
    uint64_t reserved_memory_mb;
    uint64_t last_heavy_probe_timestamp_ms;
};

typedef struct vom_monitor_context vom_monitor_context_t;

vom_monitor_context_t* vom_monitor_init(void) {
    vom_monitor_context_t* ctx = (vom_monitor_context_t*)calloc(1, sizeof(vom_monitor_context_t));
    if (!ctx) return NULL;
    
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->head_idx = 0;
    ctx->sample_count = 0;
    ctx->reserved_cpu_units = 0;
    ctx->reserved_memory_mb = 0;
    ctx->last_heavy_probe_timestamp_ms = 0;
    
    return ctx;
}

void vom_monitor_destroy(vom_monitor_context_t* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

static void execute_hardware_metrics_probe(vom_monitor_context_t* ctx, ResourceSample* out_sample, uint64_t now_ms) {
    bool execution_cooldown_passed = (now_ms - ctx->last_heavy_probe_timestamp_ms) >= PROBE_COOLDOWN_MS;
    
    if (execution_cooldown_passed) {
        out_sample->cpu_utilization_pct = 22.5f;
        out_sample->free_ram_mb = 4096;
        out_sample->reclaimable_ram_mb = 512;
        out_sample->free_storage_mb = 250000;
        out_sample->current_io_read_mbps = 45;
        out_sample->battery_percentage = 88.0f;
        out_sample->thermal_state = THERMAL_NOMINAL;
        
        ctx->last_heavy_probe_timestamp_ms = now_ms;
    } else {
        uint32_t prev_idx = (ctx->head_idx > 0) ? (ctx->head_idx - 1) : (MAX_SAMPLES - 1);
        if (ctx->sample_count > 0) {
            ResourceSample* last = &ctx->history[prev_idx];
            out_sample->cpu_utilization_pct = last->cpu_utilization_pct;
            out_sample->free_ram_mb = last->free_ram_mb;
            out_sample->reclaimable_ram_mb = last->reclaimable_ram_mb;
            out_sample->free_storage_mb = last->free_storage_mb;
            out_sample->current_io_read_mbps = last->current_io_read_mbps;
            out_sample->battery_percentage = last->battery_percentage;
            out_sample->thermal_state = last->thermal_state;
        } else {
            out_sample->status = SAMPLE_UNAVAILABLE;
            return;
        }
    }
    
    uint32_t physical_cores_free = 4; 
    if (physical_cores_free > ctx->reserved_cpu_units) {
        out_sample->effective_cpu_units_available = physical_cores_free - ctx->reserved_cpu_units;
    } else {
        out_sample->effective_cpu_units_available = 0;
    }
    
    if (out_sample->free_ram_mb > ctx->reserved_memory_mb) {
        out_sample->free_ram_mb -= ctx->reserved_memory_mb;
    } else {
        out_sample->free_ram_mb = 0;
    }
    
    out_sample->timestamp_ms = now_ms;
    out_sample->status = SAMPLE_FRESH;
}

void vom_monitor_record_sample(vom_monitor_context_t* ctx, uint64_t now_ms) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->lock);
    
    ResourceSample sample = {0};
    execute_hardware_metrics_probe(ctx, &sample, now_ms);
    
    ctx->history[ctx->head_idx] = sample;
    ctx->head_idx = (ctx->head_idx + 1) % MAX_SAMPLES;
    if (ctx->sample_count < MAX_SAMPLES) {
        ctx->sample_count++;
    }
    
    pthread_mutex_unlock(&ctx->lock);
}

void vom_monitor_update_reservations(vom_monitor_context_t* ctx, uint32_t cpu_units, uint64_t memory_mb) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->lock);
    ctx->reserved_cpu_units = cpu_units;
    ctx->reserved_memory_mb = memory_mb;
    pthread_mutex_unlock(&ctx->lock);
}

bool vom_monitor_get_latest_availability(vom_monitor_context_t* ctx, uint64_t now_ms, ResourceSample* out_result) {
    if (!ctx || !out_result) return false;
    pthread_mutex_lock(&ctx->lock);
    
    if (ctx->sample_count == 0) {
        out_result->status = SAMPLE_UNAVAILABLE;
        pthread_mutex_unlock(&ctx->lock);
        return false;
    }
    
    uint32_t latest_idx = (ctx->head_idx > 0) ? (ctx->head_idx - 1) : (MAX_SAMPLES - 1);
    *out_result = ctx->history[latest_idx];
    
    if ((now_ms - out_result->timestamp_ms) > STALE_THRESHOLD_MS) {
        out_result->status = SAMPLE_STALE;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return (out_result->status == SAMPLE_FRESH);
}

void execute_monitor_test_suite(void) {
    printf("--- INITIALIZING RESOURCE MONITOR TELEMETRY TESTS ---\n\n");
    vom_monitor_context_t* monitor = vom_monitor_init();
    
    uint64_t simulated_clock_ms = 5000;
    
    vom_monitor_record_sample(monitor, simulated_clock_ms);
    ResourceSample res = {0};
    vom_monitor_get_latest_availability(monitor, simulated_clock_ms, &res);
    printf("[TEST-1] Verification -> Fresh Capture Generation: %s | CPU Available Cores: %u\n", 
           (res.status == SAMPLE_FRESH) ? "PASS" : "FAIL", res.effective_cpu_units_available);
           
    printf("[TEST-2] Verification -> Local Reservation Deduction: \n");
    vom_monitor_update_reservations(monitor, 2, 1024); 
    simulated_clock_ms += 500; 
    vom_monitor_record_sample(monitor, simulated_clock_ms);
    vom_monitor_get_latest_availability(monitor, simulated_clock_ms, &res);
    printf("Result -> Core available capacity delta (Expected 2): %u\n\n", res.effective_cpu_units_available);
    
    printf("[TEST-3] Verification -> Telemetry Expiration / Stale Guard Triggers:\n");
    simulated_clock_ms += 15000; 
    vom_monitor_get_latest_availability(monitor, simulated_clock_ms, &res);
    printf("Result -> Status verification code evaluation: %s\n", 
           (res.status == SAMPLE_STALE) ? "PASS (Stale State Intercepted)" : "FAIL");
           
    vom_monitor_destroy(monitor);
}

int main(void) {
    execute_monitor_test_suite();
    return 0;
}
