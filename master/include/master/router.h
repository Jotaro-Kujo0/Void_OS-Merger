#ifndef VOM_MASTER_ROUTER_H
#define VOM_MASTER_ROUTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ROUTER_ID_MAX_LEN       64
#define ROUTER_PAYLOAD_MAX_LEN  4096

typedef enum {
    ROUTER_EVENT_WORKER_JOINED,
    ROUTER_EVENT_WORKER_HEARTBEAT,
    ROUTER_EVENT_CHUNK_PROGRESS,
    ROUTER_EVENT_CHUNK_RESULT,
    ROUTER_EVENT_WORKER_FAILURE,
    ROUTER_EVENT_CLI_CONTROL_REQ,
    ROUTER_EVENT_TRANSPORT_FAULT
} VomRouterEvent;

typedef enum {
    ROUTER_MSG_AUTH_REQ     = 1,
    ROUTER_MSG_AUTH_RESP    = 2,
    ROUTER_MSG_JOIN_REQ     = 3,
    ROUTER_MSG_JOIN_RESP    = 4,
    ROUTER_MSG_HEARTBEAT    = 5,
    ROUTER_MSG_CHUNK_ASSIGN = 6,
    ROUTER_MSG_CHUNK_ACK    = 7,
    ROUTER_MSG_CHUNK_REPORT = 8
} VomRouterMsgCode;

typedef enum {
    ROUTER_STATUS_SUCCESS     = 0,
    ROUTER_STATUS_ERR_VERSION = 1,
    ROUTER_STATUS_ERR_SIZE    = 2,
    ROUTER_STATUS_ERR_SOCKET  = 3,
    ROUTER_STATUS_ERR_TIMEOUT = 4
} VomRouterStatus;

typedef struct {
    uint16_t protocol_version;
    uint16_t message_code;
    uint32_t payload_length;
    uint64_t sequence_id;
    uint64_t timestamp_ms;
} vom_router_envelope_t;

typedef struct vom_master_router_context vom_master_router_context_t;

typedef void (*VomRouterEventCallback)(const char *worker_id, VomRouterEvent event, void *user_data);
typedef void (*VomRouterDispatchCallback)(const char *worker_id, const vom_router_envelope_t *envelope, const uint8_t *payload, void *user_data);

vom_master_router_context_t* vom_master_router_create(void);
void vom_master_router_destroy(vom_master_router_context_t *ctx);

VomRouterStatus vom_master_router_start(vom_master_router_context_t *ctx, const char *bind_endpoint);
void vom_master_router_stop(vom_master_router_context_t *ctx);

VomRouterStatus vom_master_router_send_to_worker(vom_master_router_context_t *ctx, const char *worker_id, VomRouterMsgCode code, const uint8_t *payload, size_t length);
VomRouterStatus vom_master_router_run_once(vom_master_router_context_t *ctx, int timeout_ms, VomRouterDispatchCallback msg_cb, VomRouterEventCallback event_cb, void *user_data);

#endif /* VOM_MASTER_ROUTER_H */
