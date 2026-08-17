#ifndef VOM_MASTER_SCHEDULER_H
#define VOM_MASTER_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

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

void vom_scheduler_init(void);
int vom_scheduler_register_worker(const vom_worker_view *worker);
int vom_scheduler_submit_ready_chunk(const vom_chunk_plan *chunk);
int vom_scheduler_tick(uint64_t current_time_ms, vom_assignment_lease *out_leases, int max_leases);
void vom_scheduler_release_lease(uint64_t chunk_id);

#endif
