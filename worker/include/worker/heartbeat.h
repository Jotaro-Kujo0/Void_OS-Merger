#ifndef VOM_WORKER_HEARTBEAT_H
#define VOM_WORKER_HEARTBEAT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    HEARTBEAT_MODE_NORMAL,
    HEARTBEAT_MODE_THROTTLED,
    HEARTBEAT_MODE_CONSERVATIVE
} VomHeartbeatMode;

typedef struct vom_heartbeat_manager vom_heartbeat_manager_t;

vom_heartbeat_manager_t* vom_worker_heartbeat_create(void);
void vom_worker_heartbeat_destroy(vom_heartbeat_manager_t *ctx);

int32_t vom_worker_heartbeat_start(vom_heartbeat_manager_t *ctx, const char *worker_id, uint32_t interval_ms, void *opaque_transport_ctx, void *opaque_monitor_ctx);
void vom_worker_heartbeat_stop(vom_heartbeat_manager_t *ctx);

int32_t vom_worker_heartbeat_adjust_mode(vom_heartbeat_manager_t *ctx, VomHeartbeatMode mode, uint32_t custom_interval_ms);
int32_t vom_worker_heartbeat_trigger_immediate(vom_heartbeat_manager_t *ctx);

#endif /* VOM_WORKER_HEARTBEAT_H */