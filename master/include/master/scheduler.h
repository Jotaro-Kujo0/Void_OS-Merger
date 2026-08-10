/*
 * master/scheduler.h — assign ready workload chunks to compatible workers.
 *
 * The scheduler is not the chunk planner. The planner creates a validated
 * workload graph; this module chooses worker placement for ready chunks.
 *
 * TODO:
 *
 * 1. Hard-filter workers by runtime, architecture, resources, accelerator,
 *    storage, network, display/input requirements, health, and drain state.
 * 2. Score compatible assignments by projected utilization, cluster balance,
 *    data locality, measured throughput, battery, thermal state, and worker
 *    reliability.
 * 3. Reserve resources and create a lease before dispatch.
 * 4. Requeue safe chunks when a lease expires or a worker fails.
 * 5. Preserve workload dependencies and never dispatch a blocked chunk.
 *
 * Proportional distribution means stronger workers may receive more chunks;
 * equal chunk counts are not the goal.
 */
#ifndef VOM_MASTER_SCHEDULER_H
#define VOM_MASTER_SCHEDULER_H

#include <stdint.h>

/*
 * Comment-only future domain objects:
 *
 * // struct vom_chunk_plan { ... };
 * // struct vom_worker_view { ... };
 * // struct vom_assignment_lease { ... };
 * // int vom_scheduler_submit_ready_chunk(const struct vom_chunk_plan *chunk);
 * // int vom_scheduler_register_worker(const struct vom_worker_view *worker);
 * // int vom_scheduler_tick(void);
 */

struct vom_worker_view {
    char     worker_id[64];
    int      platform;
    int      runtime;
    uint8_t  cpu_units;
    uint64_t free_memory_mb;
    float    battery_pct;
    int      active_chunks;
    int      has_touch_input;
    int      draining;
    int      healthy;
};

#endif /* VOM_MASTER_SCHEDULER_H */
