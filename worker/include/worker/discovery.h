#ifndef VOM_WORKER_DISCOVERY_H
#define VOM_WORKER_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DISCOVERY_ENDPOINT_MAX 128

typedef enum {
    DISCOVERY_STATUS_IDLE,
    DISCOVERY_STATUS_SEARCHING,
    DISCOVERY_STATUS_FOUND,
    DISCOVERY_STATUS_FALLBACK,
    DISCOVERY_STATUS_LOST
} VomDiscoveryStatus;

typedef void (*VomDiscoveryChangeCallback)(const char *new_endpoint, void *user_data);

typedef struct vom_worker_discovery_context vom_worker_discovery_context_t;

vom_worker_discovery_context_t* vom_worker_discovery_create(void);
void vom_worker_discovery_destroy(vom_worker_discovery_context_t *ctx);

int32_t vom_worker_discovery_configure(vom_worker_discovery_context_t *ctx, const char *fallback_endpoint, uint32_t max_retry_backoff_ms);
int32_t vom_worker_discovery_start(vom_worker_discovery_context_t *ctx, VomDiscoveryChangeCallback cb, void *user_data);
void vom_worker_discovery_stop(vom_worker_discovery_context_t *ctx);

int32_t vom_worker_discovery_current(vom_worker_discovery_context_t *ctx, char *out_endpoint, size_t cap);
VomDiscoveryStatus vom_worker_discovery_get_status(vom_worker_discovery_context_t *ctx);

#endif /* VOM_WORKER_DISCOVERY_H */