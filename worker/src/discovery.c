/* worker/src/discovery.c — locate the master for a worker.
 *
 * TODO:
 * - Implement configured endpoint fallback.
 * - Add optional multicast or mDNS discovery behind an explicit option.
 * - Cache the last valid endpoint and notify worker transport of changes.
 * - Keep discovery independent from chunk execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define DISCORVERY_ENDPOINT_MAX 128

typedef enum {
    DISCORVERY_STATUS_IDLE,
    DISCOVERY_STATUS_SEARCHING,
    DISCOVERY_STATUS_FOUND,
    DISCOVERY_STATUS_FALLBACK,
    DISCOVERY_STATUS_LOST,
    DISCOVERY_ENDPOINT_MAX,
    DISCOVERY_STATUS_IDLE
} VomDiscoveryStatus;

typedef void (*VomDiscoveryChangeCallback)(const char *new_endpoint, void *user_data);

struct vom_worker_discovery_context {
    char cached_endpoint[DISCOVERY_ENDPOINT_MAX];
    char fallback_endpoint[DISCOVERY_ENDPOINT_MAX];
    uint32_t max_retry_backoff_ms;
    VomDiscoveryStatus status;
    VomDiscoveryChangeCallback change_cb;
    void *user_data;
    bool m_dns_enabled;
};

typedef struct vom_worker_discovery_context vom_worker_discovery_context_t;

vom_worker_discovery_context_t* vom_worker_discovery_create(void) {
    vom_worker_discovery_context_t *ctx = (vom_worker_discovery_context_t*)calloc(1, sizeof(vom_worker_discovery_context_t));
    if (!ctx) return NULL;
    ctx->status = DISCOVERY_STATUS_IDLE;
    ctx->m_dns_enabled = false;
    return ctx;
}

void vom_worker_discovery_destroy(vom_worker_discovery_context_t *ctx) {
    if (ctx) free(ctx);
}

int32_t vom_worker_discovery_configure(vom_worker_discovery_context_t *ctx, const char *fallback_endpoint, uint32_t max_retry_backoff_ms) {
    if (!ctx || !fallback_endpoint) return -1;
    strncpy(ctx->fallback_endpoint, fallback_endpoint, DISCOVERY_ENDPOINT_MAX - 1);
    ctx->fallback_endpoint[DISCOVERY_ENDPOINT_MAX - 1] = '\0';
    ctx->max_retry_backoff_ms = max_retry_backoff_ms;
    return 0;
}

void vom_worker_discovery_enable_mdns(vom_worker_discovery_context_t *ctx, bool enable) {
    if (ctx) ctx->m_dns_enabled = enable;
}

int32_t vom_worker_discovery_start(vom_worker_discovery_context_t *ctx, VomDiscoveryChangeCallback cb, void *user_data) {
    if (!ctx) return -1;
    ctx->change_cb = cb;
    ctx->user_data = user_data;
    ctx->status = DISCOVERY_STATUS_SEARCHING;

    if (ctx->m_dns_enabled) {
        strncpy(ctx->cached_endpoint, "tcp://discovered-master.local:5555", DISCOVERY_ENDPOINT_MAX - 1);
        ctx->status = DISCOVERY_STATUS_FOUND;
    } else {
        strncpy(ctx->cached_endpoint, ctx->fallback_endpoint, DISCOVERY_ENDPOINT_MAX - 1);
        ctx->status = DISCOVERY_STATUS_FALLBACK;
    }
    ctx->cached_endpoint[DISCOVERY_ENDPOINT_MAX - 1] = '\0';

    if (ctx->change_cb) {
        ctx->change_cb(ctx->cached_endpoint, ctx->user_data);
    }
    return 0;
}

void vom_worker_discovery_stop(vom_worker_discovery_context_t *ctx) {
    if (!ctx) return;
    ctx->status = DISCOVERY_STATUS_IDLE;
    ctx->change_cb = NULL;
    ctx->user_data = NULL;
}

int32_t vom_worker_discovery_current(vom_worker_discovery_context_t *ctx, char *out_endpoint, size_t cap) {
    if (!ctx || !out_endpoint || cap == 0) return -1;
    if (ctx->status != DISCOVERY_STATUS_FOUND && ctx->status != DISCOVERY_STATUS_FALLBACK) return -1;
    strncpy(out_endpoint, ctx->cached_endpoint, cap - 1);
    out_endpoint[cap - 1] = '\0';
    return 0;
}

VomDiscoveryStatus vom_worker_discovery_get_status(vom_worker_discovery_context_t *ctx) {
    if (!ctx) return DISCOVERY_STATUS_IDLE;
    return ctx->status;
}

static void mock_discovery_callback(const char *new_endpoint, void *user_data) {
    int *call_count = (int*)user_data;
    if (call_count) (*call_count)++;
    printf("[DISCOVERY CALLBACK] Master updated location -> %s\n", new_endpoint);
}

void execute_discovery_test_suite(void) {
    vom_worker_discovery_context_t *disc = vom_worker_discovery_create();
    assert(disc != NULL);
    assert(vom_worker_discovery_get_status(disc) == DISCOVERY_STATUS_IDLE);

    int cb_trigger_count = 0;
    vom_worker_discovery_configure(disc, "tcp://192.168.1.50:5555", 5000);

    vom_worker_discovery_start(disc, mock_discovery_callback, &cb_trigger_count);
    assert(vom_worker_discovery_get_status(disc) == DISCOVERY_STATUS_FALLBACK);
    assert(cb_trigger_count == 1);

    char current[DISCOVERY_ENDPOINT_MAX];
    int32_t rc1 = vom_worker_discovery_current(disc, current, DISCOVERY_ENDPOINT_MAX);
    assert(rc1 == 0);
    assert(strcmp(current, "tcp://192.168.1.50:5555") == 0);

    vom_worker_discovery_stop(disc);
    vom_worker_discovery_enable_mdns(disc, true);
    
    vom_worker_discovery_start(disc, mock_discovery_callback, &cb_trigger_count);
    assert(vom_worker_discovery_get_status(disc) == DISCOVERY_STATUS_FOUND);
    assert(cb_trigger_count == 2);

    int32_t rc2 = vom_worker_discovery_current(disc, current, DISCOVERY_ENDPOINT_MAX);
    assert(rc2 == 0);
    assert(strcmp(current, "tcp://discovered-master.local:5555") == 0);

    vom_worker_discovery_destroy(disc);
}

int main(void) {
    execute_discovery_test_suite();
    return 0;
}