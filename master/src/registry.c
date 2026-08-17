#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define REG_MAX_WORKERS      64
#define REG_MAX_CHUNKS       32
#define REG_RING_SIZE        4
#define REG_NAME_MAX         64

/* --- Inline Required Types to Bypass Include Failures --- */
typedef enum {
    VOM_ARCH_X86_64,
    VOM_ARCH_AARCH64,
    VOM_ARCH_ARMV7,
    VOM_ARCH_UNKNOWN
} VomCpuArch;

typedef enum {
    VOM_LINK_ETH,
    VOM_LINK_WIFI,
    VOM_LINK_CELLULAR,
    VOM_LINK_UNKNOWN
} VomLinkType;

typedef struct {
    VomCpuArch arch;
    uint32_t cores_online;
    uint32_t cores_total;
    uint32_t feature_bitmap;
    uint64_t ram_total_mb;
    uint64_t ram_free_mb;
    int32_t battery_percent;
    bool is_charging;
    bool is_ac_online;
    bool display_attached;
    uint32_t display_width;
    uint32_t display_height;
    bool touch_supported;
    VomLinkType network_link_type;
    uint64_t network_bandwidth_kbps;
} vom_worker_capabilities;

typedef enum {
    REG_STATUS_APPROVED,
    REG_STATUS_HEALTHY,
    REG_STATUS_DRAINING,
    REG_STATUS_DISCONNECTED
} WorkerRegStatus;

typedef struct {
    char worker_id[REG_NAME_MAX];
    char transport_endpoint[REG_NAME_MAX];
    WorkerRegStatus status;
    uint64_t last_seen_ms;
    vom_worker_capabilities capabilities;
    uint64_t active_chunk_ids[REG_MAX_CHUNKS];
    int active_chunk_count;
} WorkerRecord;

typedef struct {
    WorkerRecord records[REG_MAX_WORKERS];
    int worker_count;
    uint64_t snapshot_version;
} RegistrySnapshot;

struct vom_registry_context {
    pthread_mutex_t mutex;
    RegistrySnapshot state;
    RegistrySnapshot pool[REG_RING_SIZE];
    uint32_t pool_tail;
    uint32_t ref_counts[REG_RING_SIZE];
};

typedef struct vom_registry_context vom_registry_context_t;

/* --- Core Function Implementations --- */

vom_registry_context_t* vom_registry_create(void) {
    vom_registry_context_t* ctx = (vom_registry_context_t*)calloc(1, sizeof(vom_registry_context_t));
    if (!ctx) return NULL;
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }
    ctx->state.snapshot_version = 1;
    return ctx;
}

void vom_registry_destroy(vom_registry_context_t* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

bool vom_registry_on_worker_join(vom_registry_context_t* ctx, const char* worker_id, const char* transport, const vom_worker_capabilities* caps, uint64_t now_ms) {
    if (!ctx || !worker_id || !transport || !caps) return false;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < ctx->state.worker_count; i++) {
        if (strcmp(ctx->state.records[i].worker_id, worker_id) == 0) {
            ctx->state.records[i].status = REG_STATUS_APPROVED;
            ctx->state.records[i].last_seen_ms = now_ms;
            ctx->state.records[i].capabilities = *caps;
            ctx->state.snapshot_version++;
            pthread_mutex_unlock(&ctx->mutex);
            return true;
        }
    }

    if (ctx->state.worker_count >= REG_MAX_WORKERS) {
        pthread_mutex_unlock(&ctx->mutex);
        return false;
    }

    WorkerRecord* r = &ctx->state.records[ctx->state.worker_count++];
    strncpy(r->worker_id, worker_id, REG_NAME_MAX - 1);
    strncpy(r->transport_endpoint, transport, REG_NAME_MAX - 1);
    r->status = REG_STATUS_APPROVED;
    r->last_seen_ms = now_ms;
    r->capabilities = *caps;
    r->active_chunk_count = 0;

    ctx->state.snapshot_version++;
    pthread_mutex_unlock(&ctx->mutex);
    return true;
}

bool vom_registry_on_worker_heartbeat(vom_registry_context_t* ctx, const char* worker_id, const vom_worker_capabilities* dynamic_caps, uint64_t now_ms) {
    if (!ctx || !worker_id || !dynamic_caps) return false;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < ctx->state.worker_count; i++) {
        WorkerRecord* r = &ctx->state.records[i];
        if (strcmp(r->worker_id, worker_id) == 0) {
            r->last_seen_ms = now_ms;
            r->capabilities.ram_free_mb = dynamic_caps->ram_free_mb;
            r->capabilities.cores_online = dynamic_caps->cores_online;
            r->capabilities.battery_percent = dynamic_caps->battery_percent;
            r->capabilities.is_charging = dynamic_caps->is_charging;
            r->capabilities.is_ac_online = dynamic_caps->is_ac_online;
            if (r->status == REG_STATUS_DISCONNECTED) {
                r->status = REG_STATUS_HEALTHY;
            }
            ctx->state.snapshot_version++;
            pthread_mutex_unlock(&ctx->mutex);
            return true;
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
    return false;
}

bool vom_registry_on_worker_leave(vom_registry_context_t* ctx, const char* worker_id) {
    if (!ctx || !worker_id) return false;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < ctx->state.worker_count; i++) {
        if (strcmp(ctx->state.records[i].worker_id, worker_id) == 0) {
            ctx->state.records[i].status = REG_STATUS_DISCONNECTED;
            ctx->state.snapshot_version++;
            pthread_mutex_unlock(&ctx->mutex);
            return true;
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
    return false;
}

bool vom_registry_track_chunk(vom_registry_context_t* ctx, const char* worker_id, uint64_t chunk_id) {
    if (!ctx || !worker_id) return false;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < ctx->state.worker_count; i++) {
        WorkerRecord* r = &ctx->state.records[i];
        if (strcmp(r->worker_id, worker_id) == 0) {
            if (r->active_chunk_count >= REG_MAX_CHUNKS) {
                pthread_mutex_unlock(&ctx->mutex);
                return false;
            }
            r->active_chunk_ids[r->active_chunk_count++] = chunk_id;
            ctx->state.snapshot_version++;
            pthread_mutex_unlock(&ctx->mutex);
            return true;
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
    return false;
}

bool vom_registry_untrack_chunk(vom_registry_context_t* ctx, const char* worker_id, uint64_t chunk_id) {
    if (!ctx || !worker_id) return false;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < ctx->state.worker_count; i++) {
        WorkerRecord* r = &ctx->state.records[i];
        if (strcmp(r->worker_id, worker_id) == 0) {
            for (int j = 0; j < r->active_chunk_count; j++) {
                if (r->active_chunk_ids[j] == chunk_id) {
                    if (j < r->active_chunk_count - 1) {
                        memmove(&r->active_chunk_ids[j], &r->active_chunk_ids[j + 1], sizeof(uint64_t) * (r->active_chunk_count - j - 1));
                    }
                    r->active_chunk_count--;
                    ctx->state.snapshot_version++;
                    pthread_mutex_unlock(&ctx->mutex);
                    return true;
                }
            }
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
    return false;
}

int vom_registry_sweep_stale(vom_registry_context_t* ctx, uint64_t now_ms, uint64_t timeout_ms, char out_dead_workers[][REG_NAME_MAX], int max_out) {
    if (!ctx || !out_dead_workers || max_out <= 0) return 0;
    pthread_mutex_lock(&ctx->mutex);

    int dead_count = 0;
    for (int i = 0; i < ctx->state.worker_count; i++) {
        WorkerRecord* r = &ctx->state.records[i];
        if (r->status != REG_STATUS_DISCONNECTED && (now_ms - r->last_seen_ms) > timeout_ms) {
            r->status = REG_STATUS_DISCONNECTED;
            if (dead_count < max_out) {
                strncpy(out_dead_workers[dead_count++], r->worker_id, REG_NAME_MAX - 1);
            }
            ctx->state.snapshot_version++;
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
    return dead_count;
}

const RegistrySnapshot* vom_registry_acquire_snapshot(vom_registry_context_t* ctx) {
    if (!ctx) return NULL;
    pthread_mutex_lock(&ctx->mutex);

    uint32_t slot = ctx->pool_tail;
    for (int i = 0; i < REG_RING_SIZE; i++) {
        if (ctx->ref_counts[slot] == 0) break;
        slot = (slot + 1) % REG_RING_SIZE;
    }

    memcpy(&ctx->pool[slot], &ctx->state, sizeof(RegistrySnapshot));
    ctx->ref_counts[slot]++;
    ctx->pool_tail = (slot + 1) % REG_RING_SIZE;

    const RegistrySnapshot* snapshot = &ctx->pool[slot];
    pthread_mutex_unlock(&ctx->mutex);
    return snapshot;
}

void vom_registry_release_snapshot(vom_registry_context_t* ctx, const RegistrySnapshot* snapshot) {
    if (!ctx || !snapshot) return;
    pthread_mutex_lock(&ctx->mutex);

    for (int i = 0; i < REG_RING_SIZE; i++) {
        if (&ctx->pool[i] == snapshot) {
            if (ctx->ref_counts[i] > 0) ctx->ref_counts[i]--;
            break;
        }
    }

    pthread_mutex_unlock(&ctx->mutex);
}
