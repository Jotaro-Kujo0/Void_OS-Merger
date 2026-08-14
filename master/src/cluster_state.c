#include "master/cluster_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//priv context declaration

#define SNAPSHOT_RING_SIZE 4


struct ClusterStateContext {
    pthread_mutex_t context_mutex;
    
    // Authoritative master internal state
    ClusterStateSnapshot master_state;
    
    // Circular buffer pool of immutable snapshots for zero-copy read paths
    ClusterStateSnapshot snapshot_pool[SNAPSHOT_RING_SIZE];
    uint32_t pool_tail_idx;
    
    //Active read-reference counters to prevent overwriting active snapshots
    uint32_t snapshot_ref_counts[SNAPSHOT_RING_SIZE];
};

// Internal helper to recalculate aggregate system capacity bound
static void internal_recalculate_aggregates(ClusterStateSnapshot* state) {
    state->metrics.cluster_aggregate_memory_bytes = 0;
    state->metrics.cluster_allocated_memory_bytes = 0;
    state->metrics.cluster_aggregate_cores = 0;
    state->metrics.cluster_allocated_cores = 0;
    state->metrics.active_worker_count = 0;
    
    for (int i = 0; i < state->total_registered_workers; i++) {
        WorkerMember* w = &state->active_workers[i];
        if (w->health == WORKER_HEALTH_ONLINE || w->health == WORKER_HEALTH_DRAINING) {
            state->metrics.cluster_aggregate_memory_bytes += w->resources.total_memory_bytes;
            state->metrics.cluster_allocated_memory_bytes += w->resources.reserved_memory_bytes;
            state->metrics.cluster_aggregate_cores += w->resources.total_cores;
            state->metrics.cluster_allocated_cores += w->resources.reserved_cores;
            state->metrics.active_worker_count++;
        }
    }
}

//lifecycle management

ClusterStateContext* cluster_state_create(const char* cluster_uuid, uint64_t initial_epoch) {
    if (!cluster_uuid) return NULL;
    
    ClusterStateContext* ctx = (ClusterStateContext*)calloc(1, sizeof(ClusterStateContext));
    if (!ctx) return NULL;
    
    if (pthread_mutex_init(&ctx->context_mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }
    
    strncpy(ctx->master_state.cluster_uuid, cluster_uuid, sizeof(ctx->master_state.cluster_uuid) - 1);
    ctx->master_state.master_epoch = initial_epoch;
    ctx->master_state.state_generation_version = 1;
    ctx->master_state.global_status = CLUSTER_STATUS_BOOSTRAP;
    
    return ctx;
}

void cluster_state_destroy(ClusterStateContext* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->context_mutex);
    free(ctx);
}

//Write Ahead Log (Wal) stuff

bool cluster_state_persist_to_wal(ClusterStateContext* ctx, const char* wal_journal_path) {
    if (!ctx || !wal_journal_path) return false;
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    FILE* wal_file = fopen(wal_journal_path, "wb");
    if (!wal_file) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return false;
    }

    size_t written = fwrite(&ctx->master_state, sizeof(ClusterStateSnapshot), 1, wal_file);
    fflush(wal_file);
    fclose(wal_file);
    
    pthread_mutex_unlock(&ctx->context_mutex);
    return (written == 1);
}

bool cluster_state_recover_from_wal(ClusterStateContext* ctx, const char* wal_journal_path) {
    if (!ctx || !wal_journal_path) return false;
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    FILE* wal_file = fopen(wal_journal_path, "rb"); 
    if (!wal_file) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return false; 
    }
    
    ClusterStateSnapshot temp_state;
    size_t read_blocks = fread(&temp_state, sizeof(ClusterStateSnapshot), 1, wal_file);
    fclose(wal_file);
    
    if (read_blocks != 1) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return false; 
    }
    
    memcpy(&ctx->master_state, &temp_state, sizeof(ClusterStateSnapshot));
    ctx->master_state.state_generation_version++;
    
    pthread_mutex_unlock(&ctx->context_mutex);
    return true;
}
//Isolation pipeline

const ClusterStateSnapshot* cluster_state_acquire_snapshot(ClusterStateContext* ctx) {
    if (!ctx) return NULL;
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    uint32_t target_slot = ctx->pool_tail_idx;
    int search_count = 0;
    
    /* Find an available snapshot buffer in the ring that has zero active read references */
    while (ctx->snapshot_ref_counts[target_slot] > 0) {
        target_slot = (target_slot + 1) % SNAPSHOT_RING_SIZE;
        search_count++;
        if (search_count >= SNAPSHOT_RING_SIZE) {
            /* Ring buffer saturation fallback: copy over oldest entry directly */
            break;
        }
    }
    
    /* Copy state values into the ring segment to provide consistent reads without locks */
    memcpy(&ctx->snapshot_pool[target_slot], &ctx->master_state, sizeof(ClusterStateSnapshot));
    ctx->snapshot_ref_counts[target_slot]++;
    
    /* Advance track index */
    ctx->pool_tail_idx = (target_slot + 1) % SNAPSHOT_RING_SIZE;
    
    const ClusterStateSnapshot* snapshot_ptr = &ctx->snapshot_pool[target_slot];
    pthread_mutex_unlock(&ctx->context_mutex);
    
    return snapshot_ptr;
}

void cluster_state_release_snapshot(ClusterStateContext* ctx, const ClusterStateSnapshot* snapshot) {
    if (!ctx || !snapshot) return;
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    /* Trace address identity back to its reference tracking bucket slot */
    for (int i = 0; i < SNAPSHOT_RING_SIZE; i++) {
        if (&ctx->snapshot_pool[i] == snapshot) {
            if (ctx->snapshot_ref_counts[i] > 0) {
                ctx->snapshot_ref_counts[i]--;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&ctx->context_mutex);
}

//validated state pipeline

bool cluster_state_register_worker(ClusterStateContext* ctx, const WorkerMember* worker) {
    if (!ctx || !worker) return false;
    
    pthread_mutex_lock(&ctx->context_mutex);
    ClusterStateSnapshot* state = &ctx->master_state;
    
    if (state->total_registered_workers >= CLUSTER_MAX_WORKERS) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return false;
    }
    
    // De-duplicate check to verify node registration profiles unique
    for (int i = 0; i < state->total_registered_workers; i++) {
        if (state->active_workers[i].worker_id == worker->worker_id) {
            pthread_mutex_unlock(&ctx->context_mutex);
            return false;
        }
    }
    
    int index = state->total_registered_workers++;
    memcpy(&state->active_workers[index], worker, sizeof(WorkerMember));
    
    state->state_generation_version++;
    if (state->global_status == CLUSTER_STATUS_BOOSTRAP && state->total_registered_workers >= 1) {
        state->global_status = CLUSTER_STATUS_OPERATIONAL;
    }
    
    internal_recalculate_aggregates(state);
    pthread_mutex_unlock(&ctx->context_mutex);
    return true;
}

bool cluster_state_update_lease(ClusterStateContext* ctx, uint64_t chunk_id, LeaseStatus new_status, float progress) {
    if (!ctx) return false;
    
    pthread_mutex_lock(&ctx->context_mutex);
    ClusterStateSnapshot* state = &ctx->master_state;
    
    int lease_idx = -1;
    for (int i = 0; i < state->total_active_leases; i++) {
        if (state->active_leases[i].chunk_id == chunk_id) {
            lease_idx = i;
            break;
        }
    }
    
    if (lease_idx == -1) {
        if (state->total_active_leases >= CLUSTER_MAX_ACTIVE_LEASES) {
            pthread_mutex_unlock(&ctx->context_mutex);
            return false;
        }
        lease_idx = state->total_active_leases++;
        memset(&state->active_leases[lease_idx], 0, sizeof(ChunkLease));
        state->active_leases[lease_idx].chunk_id = chunk_id;
    }
    
    ChunkLease* lease = &state->active_leases[lease_idx];
    lease->status = new_status;
    lease->progress_percentage = progress;
    
    if (new_status == LEASE_STATUS_FAILED || new_status == LEASE_STATUS_TIMED_OUT) {
        lease->failure_retry_count++;
    }
    
    state->state_generation_version++;
    pthread_mutex_unlock(&ctx->context_mutex);
    return true;
}

bool cluster_state_process_heartbeat(ClusterStateContext* ctx, uint32_t worker_id, uint64_t timestamp_ms) {
    if (!ctx) return false;
    
    pthread_mutex_lock(&ctx->context_mutex);
    ClusterStateSnapshot* state = &ctx->master_state;
    
