#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <common/compat.h>

/* Independent Mock Logger Macro Implementations */
#define VOM_LOG_INFO(fmt, ...)  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_WARN(fmt, ...)  fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

#define MAX_SWORKERS 64
#define MAX_SCHUNKS  128

typedef struct {
    char worker_id[64];
    int platform;
    int runtime;
    uint8_t cpu_units;
    uint8_t reserv_cpu;
    uint64_t free_memory_mb;
    uint64_t reserv_mem_mb;
    float battery_pct;
    int active_chunks;
    int has_touch_input;
    int draining;
    int healthy;
    double reliability_score;
} vom_worker_view;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    int required_platform;
    int required_runtime;
    uint8_t required_cpu;
    uint64_t required_memory_mb;
    int requires_touch;
    bool is_blocked;
    uint32_t preferred_node;
} vom_chunk_plan;

typedef struct {
    uint64_t chunk_id;
    char assigned_worker_id[64];
    uint64_t lease_expires_ms;
    bool is_active;
} vom_assignment_lease;

static vom_worker_view g_workers[MAX_SWORKERS];
static int g_worker_count = 0;

static vom_chunk_plan g_chunks[MAX_SCHUNKS];
static int g_chunk_count = 0;

static vom_assignment_lease g_leases[MAX_SCHUNKS];
static int g_lease_count = 0;

static pthread_mutex_t g_sched_mutex = PTHREAD_MUTEX_INITIALIZER;

void vom_scheduler_init(void) {
    pthread_mutex_lock(&g_sched_mutex);
    g_worker_count = 0;
    g_chunk_count = 0;
    g_lease_count = 0;
    memset(g_workers, 0, sizeof(g_workers));
    memset(g_chunks, 0, sizeof(g_chunks));
    memset(g_leases, 0, sizeof(g_leases));
    pthread_mutex_unlock(&g_sched_mutex);
}

int vom_scheduler_register_worker(const vom_worker_view *worker) {
    if (!worker) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    for (int i = 0; i < g_worker_count; i++) {
        if (strcmp(g_workers[i].worker_id, worker->worker_id) == 0) {
            g_workers[i] = *worker;
            pthread_mutex_unlock(&g_sched_mutex);
            return 0;
        }
    }
    if (g_worker_count >= MAX_SWORKERS) {
        pthread_mutex_unlock(&g_sched_mutex);
        return -1;
    }
    g_workers[g_worker_count++] = *worker;
    pthread_mutex_unlock(&g_sched_mutex);
    return 0;
}

int vom_scheduler_submit_ready_chunk(const vom_chunk_plan *chunk) {
    if (!chunk) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    for (int i = 0; i < g_chunk_count; i++) {
        if (g_chunks[i].chunk_id == chunk->chunk_id) {
            g_chunks[i] = *chunk;
            pthread_mutex_unlock(&g_sched_mutex);
            return 0;
        }
    }
    if (g_chunk_count >= MAX_SCHUNKS) {
        pthread_mutex_unlock(&g_sched_mutex);
        return -1;
    }
    g_chunks[g_chunk_count++] = *chunk;
    pthread_mutex_unlock(&g_sched_mutex);
    return 0;
}

static bool evaluate_hard_filters(const vom_chunk_plan *chunk, const vom_worker_view *worker) {
    if (!worker->healthy || worker->draining) return false;
    if (worker->platform != chunk->required_platform) return false;
    if (worker->runtime != chunk->required_runtime) return false;
    if (chunk->requires_touch && !worker->has_touch_input) return false;
    
    uint32_t available_cpu = (worker->cpu_units > worker->reserv_cpu) ? (worker->cpu_units - worker->reserv_cpu) : 0;
    if (available_cpu < chunk->required_cpu) return false;
    
    uint64_t available_mem = (worker->free_memory_mb > worker->reserv_mem_mb) ? (worker->free_memory_mb - worker->reserv_mem_mb) : 0;
    if (available_mem < chunk->required_memory_mb) return false;
    
    return true;
}

static double calculate_worker_score(const vom_chunk_plan *chunk, const vom_worker_view *worker) {
    double score = 0.0;
    double total_cpu_capacity = (double)worker->cpu_units;
    double free_cpu_ratio = (double)(worker->cpu_units - worker->reserv_cpu) / total_cpu_capacity;
    score += free_cpu_ratio * 40.0;
    score += worker->reliability_score * 30.0;

    uint32_t worker_node_numeric_id = (uint32_t)atoi(worker->worker_id + 7);
    if (worker_node_numeric_id == chunk->preferred_node) {
        score += 20.0;
    }
    if (worker->battery_pct >= 0.0f) {
        score += (worker->battery_pct / 100.0) * 10.0;
    } else {
        score += 10.0;
    }
    return score;
}

int vom_scheduler_tick(uint64_t current_time_ms, vom_assignment_lease *out_leases, int max_leases) {
    pthread_mutex_lock(&g_sched_mutex);
    int dispatched_count = 0;

    for (int c = 0; c < g_chunk_count; c++) {
        if (dispatched_count >= max_leases) break;
        vom_chunk_plan *chunk = &g_chunks[c];
        if (chunk->is_blocked) continue;

        bool already_leased = false;
        for (int l = 0; l < g_lease_count; l++) {
            if (g_leases[l].chunk_id == chunk->chunk_id && g_leases[l].is_active) {
                if (current_time_ms > g_leases[l].lease_expires_ms) {
                    VOM_LOG_WARN("Lease expired for chunk %llu. Requeuing.", chunk->chunk_id);
                    g_leases[l].is_active = false;
                    for (int w = 0; w < g_worker_count; w++) {
                        if (strcmp(g_workers[w].worker_id, g_leases[l].assigned_worker_id) == 0) {
                            if (g_workers[w].reserv_cpu >= chunk->required_cpu) g_workers[w].reserv_cpu -= chunk->required_cpu;
                            if (g_workers[w].reserv_mem_mb >= chunk->required_memory_mb) g_workers[w].reserv_mem_mb -= chunk->required_memory_mb;
                            g_workers[w].active_chunks--;
                            break;
                        }
                    }
                } else {
                    already_leased = true;
                }
                break;
            }
        }
        if (already_leased) continue;

        int best_worker_idx = -1;
        double highest_score = -1.0;

        for (int w = 0; w < g_worker_count; w++) {
            if (evaluate_hard_filters(chunk, &g_workers[w])) {
                double score = calculate_worker_score(chunk, &g_workers[w]);
                if (score > highest_score) {
                    highest_score = score;
                    best_worker_idx = w;
                }
            }
        }

        if (best_worker_idx != -1) {
            vom_worker_view *target_worker = &g_workers[best_worker_idx];
            target_worker->reserv_cpu += chunk->required_cpu;
            target_worker->reserv_mem_mb += chunk->required_memory_mb;
            target_worker->active_chunks++;

            vom_assignment_lease new_lease = {
                .chunk_id = chunk->chunk_id,
                .lease_expires_ms = current_time_ms + 30000,
                .is_active = true
            };
            strncpy(new_lease.assigned_worker_id, target_worker->worker_id, sizeof(new_lease.assigned_worker_id) - 1);

            bool slot_found = false;
            for (int i = 0; i < g_lease_count; i++) {
                if (!g_leases[i].is_active) {
                    g_leases[i] = new_lease;
                    slot_found = true;
                    out_leases[dispatched_count++] = new_lease;
                    break;
                }
            }
            if (!slot_found && g_lease_count < MAX_SCHUNKS) {
                g_leases[g_lease_count++] = new_lease;
                out_leases[dispatched_count++] = new_lease;
            }
            VOM_LOG_INFO("Scheduled chunk %llu -> %s [Match Score: %.2f]", chunk->chunk_id, target_worker->worker_id, highest_score);
        } else {
            VOM_LOG_DEBUG("Chunk %llu left pending: No worker matched requirements.", chunk->chunk_id);
        }
    }
    pthread_mutex_unlock(&g_sched_mutex);
    return dispatched_count;
}

void vom_scheduler_release_lease(uint64_t chunk_id) {
    pthread_mutex_lock(&g_sched_mutex);
    for (int l = 0; l < g_lease_count; l++) {
        if (g_leases[l].chunk_id == chunk_id && g_leases[l].is_active) {
            g_leases[l].is_active = false;
            for (int c = 0; c < g_chunk_count; c++) {
                if (g_chunks[c].chunk_id == chunk_id) {
                    for (int w = 0; w < g_worker_count; w++) {
                        if (strcmp(g_workers[w].worker_id, g_leases[l].assigned_worker_id) == 0) {
                            if (g_workers[w].reserv_cpu >= g_chunks[c].required_cpu) g_workers[w].reserv_cpu -= g_chunks[c].required_cpu;
                            if (g_workers[w].reserv_mem_mb >= g_chunks[c].required_memory_mb) g_workers[w].reserv_mem_mb -= g_chunks[c].required_memory_mb;
                            g_workers[w].active_chunks--;
                            break;
                        }
                    }
                    if (c < g_chunk_count - 1) {
                        memmove(&g_chunks[c], &g_chunks[c + 1], sizeof(vom_chunk_plan) * (g_chunk_count - c - 1));
                    }
                    g_chunk_count--;
                    break;
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&g_sched_mutex);
}
