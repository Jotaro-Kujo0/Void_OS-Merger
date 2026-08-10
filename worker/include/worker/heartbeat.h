/* worker/heartbeat.h — report worker health and dynamic resources.
 *
 * TODO:
 * - Report active chunk count, reserved resources, and measured utilization.
 * - Include battery, charging, thermal, docking, and display state when
 *   available on the platform.
 * - Use the worker transport to send timestamped heartbeat messages.
 * - Support a slower heartbeat mode only when policy permits it.
 * - Keep this as a subsystem; it must not define a second main() function.
 */
#ifndef VOM_WORKER_HEARTBEAT_H
#define VOM_WORKER_HEARTBEAT_H

/*
 * Comment-only future API:
 *
 * // int vom_worker_heartbeat_start(const char *worker_id,
 * //                                int interval_ms);
 * // void vom_worker_heartbeat_stop(void);
 */

#endif /* VOM_WORKER_HEARTBEAT_H */
