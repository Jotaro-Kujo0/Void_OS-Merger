 #ifndef VOM_WORKER_TRANSPORT_H
 #define VOM_WORKER_TRANSPORT_H

 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>

 #define TRANSPORT_ID_MAX_LEN   64
 #define TRANSPORT_KEY_MAX_LEN  40
 #define TRANSPORT_BUFFER_MAX   4096

 typedef enum {
    TRANS_EVENT_CONNECTED,
    TRANS_EVENT_AUTHENTICATED,
    TRANS_EVENT_JOINED,
    TRANS_EVENT_HEARTBEAT_SENT,
    TRANS_EVENT_HEARTBEAT_ACKED,
    TRANS_EVENT_DISCONNECTED,
    TRANS_EVENT_RECONNECTING,
    TRANS_EVENT_ERROR
} VomTransportEvent;

typedef enum {
    MSG_CODE_AUTH_REQ     = 1,
    MSG_CODE_AUTH_RESP    = 2,
    MSG_CODE_JOIN_REQ     = 3,
    MSG_CODE_JOIN_RESP    = 4,
    MSG_CODE_HEARTBEAT    = 5,
    MSG_CODE_CHUNK_ASSIGN = 6,
    MSG_CODE_CHUNK_ACK    = 7,
    MSG_CODE_CHUNK_REPORT = 8
} VomMsgCode;

typedef enum {
    TRANS_STATUS_SUCCESS        = 0,
    TRANS_STATUS_ERR_VERSION    = 1,
    TRANS_STATUS_ERR_AUTH       = 2,
    TRANS_STATUS_ERR_SIZE       = 3,
    TRANS_STATUS_ERR_TIMEOUT    = 4,
    TRANS_STATUS_ERR_DISCONNECT = 5
} VomTransportStatus;

typedef struct {
    uint16_t protocol_version;
    uint16_t message_code;
    uint32_t payload_length;
    uint64_t sequence_id;
    uint64_t timestamp_ms;
} vom_transport_envelope_t;

typedef struct {
    char worker_id[TRANSPORT_ID_MAX_LEN];
    char server_public_key[TRANSPORT_KEY_MAX_LEN];
    char client_public_key[TRANSPORT_KEY_MAX_LEN];
    char client_secret_key[TRANSPORT_KEY_MAX_LEN];
} vom_transport_config_t;

typedef struct vom_worker_transport_context vom_worker_transport_context_t;

typedef void (*VomTransportEventCallback)(VomTransportEvent event, void *user_data);
typedef void (*VomMessageDispatchCallback)(const vom_transport_envelope_t *envelope, const uint8_t *payload, void *user_data);

vom_worker_transport_context_t* vom_worker_transport_create(void);
void vom_worker_transport_destroy(vom_worker_transport_context_t *ctx);

VomTransportStatus vom_worker_transport_configure(vom_worker_transport_context_t *ctx, const vom_transport_config_t *config);
VomTransportStatus vom_worker_transport_connect(vom_worker_transport_context_t *ctx, const char *endpoint);
VomTransportStatus vom_worker_transport_disconnect(vom_worker_transport_context_t *ctx, bool lease_aware);

VomTransportStatus vom_worker_transport_send(vom_worker_transport_context_t *ctx, VomMsgCode code, const uint8_t *payload, size_t length);
VomTransportStatus vom_worker_transport_poll(vom_worker_transport_context_t *ctx, int timeout_ms, VomMessageDispatchCallback msg_cb, VomTransportEventCallback event_cb, void *user_data);

#endif /* VOM_WORKER_TRANSPORT_H */