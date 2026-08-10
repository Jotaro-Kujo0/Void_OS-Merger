/*
 * master/router.h — master transport and message-dispatch planning notes.
 *
 * The baseline topology is a ROUTER owned by the master and DEALER sessions
 * owned by workers. A separate authenticated control path may serve the CLI.
 *
 * TODO:
 *
 * - Receive worker identity and versioned protocol envelopes.
 * - Dispatch worker join, heartbeat, progress, result, and failure events.
 * - Send chunk assignments, cancellation, lease, and recovery commands.
 * - Route workload/control requests from the master UI or CLI.
 * - Keep transport parsing separate from domain state transitions.
 * - Bound message sizes, timeouts, and backpressure.
 * - Make shutdown interruptible and lease-aware.
 *
 * Comment-only future API:
 *
 * // int vom_router_start(const char *bind_endpoint);
 * // void vom_router_stop(void);
 * // int vom_router_run_once(int timeout_ms);
 */
#ifndef VOM_MASTER_ROUTER_H
#define VOM_MASTER_ROUTER_H
#endif /* VOM_MASTER_ROUTER_H */
