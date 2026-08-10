/*
 * master/workload.h — master-owned workload lifecycle planning notes.
 *
 * TODO:
 * - Accept and validate a complete user workload.
 * - Assign a workload ID and invoke a registered chunk planner.
 * - Store the dependency graph and ready/running/completed chunk state.
 * - Aggregate verified chunk outputs.
 * - Apply workload cancellation, deadline, and retry policy.
 * - Expose stable workload summaries to the master UI.
 *
 * Comment-only future API:
 *
 * // int vom_workload_submit(const struct vom_workload_description *input);
 * // int vom_workload_cancel(const char *workload_id);
 * // int vom_workload_get_summary(const char *workload_id, ...);
 */
