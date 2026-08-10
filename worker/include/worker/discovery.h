/* worker/discovery.h — find the master on the local worker network.
 *
 * TODO:
 * - Use a configured master endpoint as the deterministic fallback.
 * - Optionally receive a multicast beacon or mDNS/DNS-SD announcement.
 * - Cache the last valid endpoint through temporary network loss.
 * - Notify worker transport when the master endpoint changes.
 * - Use bounded retry/backoff behavior.
 */
#ifndef VOM_WORKER_DISCOVERY_H
#define VOM_WORKER_DISCOVERY_H

/*
 * Comment-only future API:
 *
 * // int vom_worker_discovery_start(void);
 * // int vom_worker_discovery_current(char *out_endpoint, size_t cap);
 * // void vom_worker_discovery_stop(void);
 */

#endif /* VOM_WORKER_DISCOVERY_H */
