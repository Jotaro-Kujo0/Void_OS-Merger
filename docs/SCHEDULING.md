# Scheduling plan

The scheduler assigns **ready chunks** to **compatible workers**. It should
optimize the complete cluster assignment rather than simply choosing the
worker with the largest CPU count.

## Stage one: hard filter

Reject a worker when any required condition fails:

- unsupported architecture;
- unsupported workload runtime;
- insufficient available CPU;
- insufficient available memory;
- missing accelerator;
- insufficient storage;
- unacceptable network path;
- missing display or input capability when required;
- worker is draining, offline, unhealthy, or unauthorized;
- required dependency data is unavailable and cannot be transferred.

```c
/*
 * Comment-only example:
 *
 * bool worker_can_run_chunk(worker, chunk) {
 *     if (!runtime_matches(worker, chunk)) return false;
 *     if (worker->free_cpu < chunk->cpu_required) return false;
 *     if (worker->free_memory_mb < chunk->memory_required_mb) return false;
 *     if (!requirements_match(worker, chunk)) return false;
 *     if (worker->draining || !worker->healthy) return false;
 *     return true;
 * }
 */
```

## Stage two: harmony score

Among compatible workers, score the projected assignment using:

- projected CPU and memory utilization;
- proportional capacity share;
- cluster-wide imbalance after assignment;
- data transfer cost and locality;
- measured worker throughput;
- battery and thermal policy;
- worker reliability;
- dependency locality;
- display/input continuity when applicable.

Lower score should mean a better assignment, or the convention must be
explicitly documented if higher scores are used.

```c
/*
 * Comment-only score outline:
 *
 * score =
 *       projected_load_penalty
 *     + cluster_imbalance_penalty
 *     + transfer_cost_penalty
 *     + battery_penalty
 *     + thermal_penalty
 *     + reliability_penalty;
 *
 * The weights must be configurable or documented. Tests must cover every
 * hard-filter rule before testing score tie-breakers.
 */
```

## Proportional distribution

Equal work means proportional use of available compatible capacity, not equal
chunk counts. A worker with eight effective capacity units should normally
receive more work than one with one unit, while the scheduler still respects
memory, architecture, runtime, battery, and data constraints.

TODO:

- Define how CPU, memory, accelerators, and network are normalized.
- Define whether capacity is measured, configured, or both.
- Define reservations so two scheduling decisions cannot oversubscribe one
  worker.
- Define a lease for every assignment.
- Expire leases when a worker stops reporting.
- Requeue only chunks that are safe to retry.
- Keep the master as the source of truth for assignment state.
