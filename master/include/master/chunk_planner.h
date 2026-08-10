/*
 * master/chunk_planner.h — workload-to-chunk planning notes.
 *
 * The planner creates a validated graph; the scheduler later places ready
 * chunks on compatible workers.
 *
 * TODO:
 * - Register planners by workload kind and version.
 * - Support explicit and known workload-specific chunk strategies.
 * - Estimate resource demand and data locality.
 * - Record dependencies and reject cycles.
 * - Produce deterministic chunk IDs for safe retries.
 * - Reject opaque workloads that cannot be split safely.
 *
 * Comment-only future API:
 *
 * // int vom_chunk_planner_build(const struct vom_workload_description *input,
 * //                             struct vom_chunk_graph *out_graph, ...);
 */
