#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define REC_MAX_WORKERS      64
#define REC_MAX_LEASES       128
#define REC_NAME_MAX         64
#define REC_DETAIL_MAX       128

/* --- Inline Core Types Matching System Vocabulary --- */
typedef enum {
    VOM_CAT_SUCCESS = 0,
    VOM_CAT_TRANSPORT,
    VOM_CAT_WORKER_EXECUTION,
    VOM_CAT_TIMEOUT
} VomErrorCategory;

typedef enum {
    VOM_RETRY_NONE = 0,
    VOM_RETRY_IMMEDIATE,
    VOM_RETRY_BACKOFF,
    VOM_RETRY_REPLAN
} VomRetryAction;

typedef enum {
    UI_HEALTH_OK,
    UI_HEALTH_WARN,
    UI_HEALTH_CRITICAL
} UiHealthStatus;

typedef enum {
    LEASE_ACTIVE,
    LEASE_UNCERTAIN,
    LEASE_EXPIRED,
    LEASE_RECONCILED
} LeaseState;

typedef struct {
    uint64_t lease_id;
    uint64_t chunk_id;
    uint64_t workload_id;
    char assigned_worker_id[REC_NAME_MAX];
    uint64_t lease_expires_ms;
    LeaseState state;
    uint32_t retry_count;
    VomRetryAction retry_policy;
} rec_lease_record_t;

typedef struct {
    char worker_id[REC_NAME_MAX];
    uint64_t last_heartbeat_ms;
    bool is_healthy;
} rec_worker_obs_t;

/* Unified Context State Tracking Container */
struct vom_recovery_context {
    pthread_mutex_t lock;
    
    rec_worker_obs_t workers[REC_MAX_WORKERS];
    int worker_count;
    
    rec_lease_record_t leases[REC_MAX_LEASES];
    int lease_count;
    
    /* Shared Global Telemetry State Markers for the UI View-Model */
    UiHealthStatus summary_health;
    char operational_status_text[REC_NAME_MAX];
    uint32_t active_recovery_jobs;
};

typedef struct vom_recovery_context vom_recovery_context_t;

/* ========================================================================= */
/* --- LIFECYCLE INITIALIZATION -------------------------------------------- */
/* ========================================================================= */

vom_recovery_context_t* vom_recovery_init(void) {
    vom_recovery_context_t* ctx = (vom_recovery_context_t*)calloc(1, sizeof(vom_recovery_context_t));
    if (!ctx) return NULL;
    
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->summary_health = UI_HEALTH_OK;
    strncpy(ctx->operational_status_text, "HEALTHY", REC_NAME_MAX - 1);
    
    return ctx;
}

void vom_recovery_destroy(vom_recovery_context_t* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

/* ========================================================================= */
/* --- RECOVERY MANAGEMENT CORE ENGINE ------------------------------------- */
/* ========================================================================= */

void vom_recovery_register_worker_heartbeat(vom_recovery_context_t* ctx, const char* worker_id, uint64_t now_ms) {
    if (!ctx || !worker_id) return;
    pthread_mutex_lock(&ctx->lock);
    
    for (int i = 0; i < ctx->worker_count; i++) {
        if (strcmp(ctx->workers[i].worker_id, worker_id) == 0) {
            ctx->workers[i].last_heartbeat_ms = now_ms;
            ctx->workers[i].is_healthy = true;
            pthread_mutex_unlock(&ctx->lock);
            return;
        }
    }
    
    if (ctx->worker_count < REC_MAX_WORKERS) {
        rec_worker_obs_t* w = &ctx->workers[ctx->worker_count++];
        strncpy(w->worker_id, worker_id, REC_NAME_MAX - 1);
        w->last_heartbeat_ms = now_ms;
        w->is_healthy = true;
    }
    
    pthread_mutex_unlock(&ctx->lock);
}

void vom_recovery_sweep_timeouts(vom_recovery_context_t* ctx, uint64_t now_ms, uint64_t heartbeat_timeout_ms) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->lock);
    
    bool cluster_degraded = false;
    
    for (int i = 0; i < ctx->worker_count; i++) {
        rec_worker_obs_t* w = &ctx->workers[i];
        if (w->is_healthy && (now_ms - w->last_heartbeat_ms) > heartbeat_timeout_ms) {
            w->is_healthy = false;
            printf("[RECOVERY] Worker '%s' missed heartbeat window. Marking transport lost.\n", w->worker_id);
            
            /* Step 1: Mark associated leases as uncertain before triggering reassignment logic */
            for (int j = 0; j < ctx->lease_count; j++) {
                if (strcmp(ctx->leases[j].assigned_worker_id, w->worker_id) == 0 && ctx->leases[j].state == LEASE_ACTIVE) {
                    ctx->leases[j].state = LEASE_UNCERTAIN;
                    ctx->active_recovery_jobs++;
                    printf("[RECOVERY] Chunk lease %llu marked UNCERTAIN due to worker loss.\n", (unsigned long long)ctx->leases[j].lease_id);
                }
            }
        }
        if (!w->is_healthy) {
            cluster_degraded = true;
        }
    }
    
    /* Step 2: Requeue safe chunks whose retry policy permits it */
    for (int i = 0; i < ctx->lease_count; i++) {
        rec_lease_record_t* l = &ctx->leases[i];
        if (l->state == LEASE_UNCERTAIN || (l->state == LEASE_ACTIVE && now_ms > l->lease_expires_ms)) {
            if (l->state == LEASE_ACTIVE) {
                printf("[RECOVERY] Lease %llu hit internal clock expiration threshold.\n", (unsigned long long)l->lease_id);
            }
            
            l->retry_count++;
            if (l->retry_policy != VOM_RETRY_NONE && l->retry_count <= 3) {
                l->state = LEASE_EXPIRED;
                printf("[RECOVERY] Requeuing Chunk %llu for replacement placement execution. Retry attempt: %u\n", 
                       (unsigned long long)l->chunk_id, l->retry_count);
            } else {
                l->state = LEASE_RECONCILED;
                printf("[RECOVERY] Chunk %llu breached max retry limit or has no-retry policy. Terminating branch.\n", 
                       (unsigned long long)l->chunk_id);
            }
            if (ctx->active_recovery_jobs > 0) ctx->active_recovery_jobs--;
        }
    }
    
    /* Step 3: Publish dynamic aggregated status maps up into the Master UI coordinator layer */
    if (cluster_degraded) {
        ctx->summary_health = UI_HEALTH_WARN;
        strncpy(ctx->operational_status_text, "DEGRADED / RECOVERING", REC_NAME_MAX - 1);
    } else {
        ctx->summary_health = UI_HEALTH_OK;
        strncpy(ctx->operational_status_text, "HEALTHY", REC_NAME_MAX - 1);
    }
    
    pthread_mutex_unlock(&ctx->lock);
}

bool vom_recovery_process_incoming_result(vom_recovery_context_t* ctx, uint64_t lease_id, const char* worker_id, const char* result_hash) {
    if (!ctx || !worker_id || !result_hash) return false;
    pthread_mutex_lock(&ctx->lock);
    
    for (int i = 0; i < ctx->lease_count; i++) {
        rec_lease_record_t* l = &ctx->leases[i];
        if (l->lease_id == lease_id) {
            /* Step 4: Reject stale or delayed result messages from previously expired leases */
            if (l->state == LEASE_EXPIRED || l->state == LEASE_RECONCILED) {
                printf("[RECOVERY SECURITY] Rejected STALE result from Lease %llu sent by worker '%s'.\n", 
                       (unsigned long long)lease_id, worker_id);
                pthread_mutex_unlock(&ctx->lock);
                return false;
            }
            
            if (strcmp(l->assigned_worker_id, worker_id) != 0) {
                printf("[RECOVERY SECURITY] Worker alignment error on Lease %llu. Expected '%s', got '%s'. Rejecting.\n",
                       (unsigned long long)lease_id, l->assigned_worker_id, worker_id);
                pthread_mutex_unlock(&ctx->lock);
                return false;
            }
            
            /* Preserve valid results and reconcile contract */
            l->state = LEASE_RECONCILED;
            printf("[RECOVERY] Verified and committed result hash [%s] for Chunk %llu safely.\n", result_hash, (unsigned long long)l->chunk_id);
            pthread_mutex_unlock(&ctx->lock);
            return true;
        }
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return false;
}

void vom_recovery_inject_test_lease(vom_recovery_context_t* ctx, uint64_t lease_id, uint64_t chunk_id, const char* worker_id, uint64_t expires_ms, VomRetryAction policy) {
    pthread_mutex_lock(&ctx->lock);
    if (ctx->lease_count < REC_MAX_LEASES) {
        rec_lease_record_t* l = &ctx->leases[ctx->lease_count++];
        l->lease_id = lease_id;
        l->chunk_id = chunk_id;
        l->workload_id = 999;
        strncpy(l->assigned_worker_id, worker_id, REC_NAME_MAX - 1);
        l->lease_expires_ms = expires_ms;
        l->state = LEASE_ACTIVE;
        l->retry_count = 0;
        l->retry_policy = policy;
    }
    pthread_mutex_unlock(&ctx->lock);
}

void vom_recovery_publish_ui_dashboard(vom_recovery_context_t* ctx) {
    pthread_mutex_lock(&ctx->lock);
    printf("\n ==== MASTER COORD RECOVERY TELEMENTRY VIEW ====\n");
    printf("Logical Device Health Indicator Token: %d\n", ctx->summary_health);
    printf("Operational Status Text Descriptor : [%s]\n", ctx->operational_status_text);
    printf("Active Re-placement Computations   : %u\n", ctx->active_recovery_jobs);
    printf("==================================================\n\n");
    pthread_mutex_unlock(&ctx->lock);
}

/* ========================================================================= */
/* --- COMPLETE SYSTEM TEST RUNNER HARNESS -------------------------------- */
/* ========================================================================= */

int main(void) {
    printf(" -- INITIALIZING WORKER LOSS RECOVERY ENGINE TESTS --\n\n");
    vom_recovery_context_t* engine = vom_recovery_init();
    
    uint64_t simulated_clock_ms = 10000;
    
    /* Scenario 1: Register worker heartbeat tracks */
    vom_recovery_register_worker_heartbeat(engine, "worker-tablet-01", simulated_clock_ms);
    vom_recovery_inject_test_lease(engine, 7001, 501, "worker-tablet-01", simulated_clock_ms + 5000, VOM_RETRY_IMMEDIATE);
    
    /* Scenario 2: Simulate complete worker transport loss via heartbeat timeout */
}