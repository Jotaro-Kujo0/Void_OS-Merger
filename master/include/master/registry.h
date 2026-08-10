/*
 * master/registry.h — authoritative membership and health view of workers.
 *
 * The registry stores worker observations and exposes snapshots to the master
 * scheduler, recovery manager, and UI coordinator. It does not assign chunks.
 *
 * TODO:
 *
 * - Track stable worker ID and transport identity.
 * - Cache static worker capabilities.
 * - Cache timestamped dynamic resource snapshots.
 * - Track active chunk IDs and reservations.
 * - Track display/input surfaces and worker UI mode.
 * - Track approval, health, draining, and disconnect state.
 * - Sweep stale heartbeats and emit worker-unavailable events.
 * - Keep immutable snapshots safe for scheduler/UI readers.
 *
 * Comment-only future API:
 *
 * // int vom_registry_init(void);
 * // int vom_registry_on_worker_join(const char *worker_id, ...);
 * // int vom_registry_on_worker_heartbeat(const char *worker_id, ...);
 * // int vom_registry_on_worker_leave(const char *worker_id);
 * // int vom_registry_sweep_stale(uint64_t now_ms);
 */
#ifndef VOM_MASTER_REGISTRY_H
#define VOM_MASTER_REGISTRY_H

#include "common/sys_info.h"
#include <stdint.h>

#endif /* VOM_MASTER_REGISTRY_H */
