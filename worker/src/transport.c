/*
 * worker/src/transport.c — worker transport placeholder.
 *
 * TODO:
 *
 * - Implement the documented worker DEALER/master ROUTER topology.
 * - Remove the hardcoded endpoint and obtain it from configuration/discovery.
 * - Add identity, authentication, framing, reconnect, timeout, and shutdown
 *   behavior before accepting workload chunks.
 * - Translate transport events into worker state events.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <common/compat.h>

 #define TRANSPORT_ID_MAX_LEN 64
 #define TRANSPORT_KEY_MAX_LEN  40
 #define TRANSPORT_BUFFER_MAX   4096

#define VOM_ZMQ_IDENTITY        6
#define VOM_ZMQ_LINGER          17
#define VOM_ZMQ_SNDHWM          23
#define VOM_ZMQ_RCVHWM          24
#define VOM_ZMQ_RCVMORE         13
#define VOM_ZMQ_CURVE_SERVER    43
#define VOM_ZMQ_CURVE_PUBLICKEY 44
#define VOM_ZMQ_CURVE_SECRETKEY 45
#define VOM_ZMQ_CURVE_SERVERKEY 46
#define VOM_ZMQ_DONTWAIT        1

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

typedef void (*VomTransportEventCallback)(VomTransportEvent event, void *user_data);
typedef void (*VomMessageDispatchCallback)(const vom_transport_envelope_t *envelope, const uint8_t *payload, void *user_data);

extern void *zmq_ctx_new(void);
extern void *zmq_socket(void *context, int type);
extern int   zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen);
extern int   zmq_getsockopt(void *s, int option, void *optval, size_t *optvallen);
extern int   zmq_connect(void *s, const char *addr);
extern int   zmq_close(void *s);
extern int   zmq_ctx_destroy(void *context);
extern int   zmq_send(void *s, const void *buf, size_t len, int flags);
extern int   zmq_recv(void *s, void *buf, size_t len, int flags);

struct vom_worker_transport_context {
    void *zmq_ctx;
    void *zmq_sock;
    vom_transport_config_t config;
    uint64_t next_seq_id;
    bool is_connected;
    bool is_authenticated;
    bool is_joined;
};

typedef struct vom_worker_transport_context vom_worker_transport_context_t;

vom_worker_transport_context_t* vom_worker_transport_create(void) {
    vom_worker_transport_context_t *ctx = (vom_worker_transport_context_t*)calloc(1, sizeof(vom_worker_transport_context_t));
    if (!ctx) return NULL;
    ctx->zmq_ctx = zmq_ctx_new();
    if (!ctx->zmq_ctx) {
        free(ctx);
        return NULL;
    }
    ctx->next_seq_id = 1;
    return ctx;
}

void vom_worker_transport_destroy(vom_worker_transport_context_t *ctx) {
    if (!ctx) return;
    if (ctx->zmq_sock) zmq_close(ctx->zmq_sock);
    if (ctx->zmq_ctx) zmq_ctx_destroy(ctx->zmq_ctx);
    free(ctx);
}

VomTransportStatus vom_worker_transport_configure(vom_worker_transport_context_t *ctx, const vom_transport_config_t *config) {
    if (!ctx || !config) return TRANS_STATUS_ERR_VERSION;
    ctx->config = *config;
    return TRANS_STATUS_SUCCESS;
}

VomTransportStatus vom_worker_transport_connect(vom_worker_transport_context_t *ctx, const char *endpoint) {
    if (!ctx || !endpoint) return TRANS_STATUS_ERR_DISCONNECT;
    
    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, 5);
    if (!ctx->zmq_sock) return TRANS_STATUS_ERR_DISCONNECT;

    int linger = 1000;
    int hwm = 5000;
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_LINGER, &linger, sizeof(linger));
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_SNDHWM, &hwm, sizeof(hwm));
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_RCVHWM, &hwm, sizeof(hwm));

    size_t id_len = strlen(ctx->config.worker_id);
    if (id_len > 0) {
        zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_IDENTITY, ctx->config.worker_id, id_len);
    }

    if (strlen(ctx->config.server_public_key) > 0 && strlen(ctx->config.client_secret_key) > 0) {
        zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_CURVE_SERVERKEY, ctx->config.server_public_key, strlen(ctx->config.server_public_key));
        zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_CURVE_PUBLICKEY, ctx->config.client_public_key, strlen(ctx->config.client_public_key));
        zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_CURVE_SECRETKEY, ctx->config.client_secret_key, strlen(ctx->config.client_secret_key));
    }

    if (zmq_connect(ctx->zmq_sock, endpoint) != 0) {
        zmq_close(ctx->zmq_sock);
        ctx->zmq_sock = NULL;
        return TRANS_STATUS_ERR_DISCONNECT;
    }

    ctx->is_connected = true;
    return TRANS_STATUS_SUCCESS;
}

VomTransportStatus vom_worker_transport_disconnect(vom_worker_transport_context_t *ctx, bool lease_aware) {
    if (!ctx || !ctx->zmq_sock) return TRANS_STATUS_ERR_DISCONNECT;
    (void)lease_aware;
    zmq_close(ctx->zmq_sock);
    ctx->zmq_sock = NULL;
    ctx->is_connected = false;
    ctx->is_authenticated = false;
    ctx->is_joined = false;
    return TRANS_STATUS_SUCCESS;
}

VomTransportStatus vom_worker_transport_send(vom_worker_transport_context_t *ctx, VomMsgCode code, const uint8_t *payload, size_t length) {
    if (!ctx || !ctx->zmq_sock || !ctx->is_connected) return TRANS_STATUS_ERR_DISCONNECT;
    if (length > TRANSPORT_BUFFER_MAX) return TRANS_STATUS_ERR_SIZE;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    vom_transport_envelope_t env;
    env.protocol_version = 1;
    env.message_code = (uint16_t)code;
    env.payload_length = (uint32_t)length;
    env.sequence_id = ctx->next_seq_id++;
    env.timestamp_ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;

    uint8_t buffer[sizeof(vom_transport_envelope_t) + TRANSPORT_BUFFER_MAX];
    memcpy(buffer, &env, sizeof(vom_transport_envelope_t));
    if (payload && length > 0) {
        memcpy(buffer + sizeof(vom_transport_envelope_t), payload, length);
    }

    int rc = zmq_send(ctx->zmq_sock, buffer, sizeof(vom_transport_envelope_t) + length, 0);
    if (rc < 0) return TRANS_STATUS_ERR_DISCONNECT;

    return TRANS_STATUS_SUCCESS;
}

VomTransportStatus vom_worker_transport_poll(vom_worker_transport_context_t *ctx, int timeout_ms, VomMessageDispatchCallback msg_cb, VomTransportEventCallback event_cb, void *user_data) {
    if (!ctx || !ctx->zmq_sock) return TRANS_STATUS_ERR_DISCONNECT;
    (void)timeout_ms;

    uint8_t buffer[sizeof(vom_transport_envelope_t) + TRANSPORT_BUFFER_MAX];
    int rc = zmq_recv(ctx->zmq_sock, buffer, sizeof(buffer), VOM_ZMQ_DONTWAIT);
    
    if (rc < 0) {
        return TRANS_STATUS_SUCCESS;
    }

    if ((size_t)rc < sizeof(vom_transport_envelope_t)) {
        if (event_cb) event_cb(TRANS_EVENT_ERROR, user_data);
        return TRANS_STATUS_ERR_SIZE;
    }

    vom_transport_envelope_t env;
    memcpy(&env, buffer, sizeof(vom_transport_envelope_t));

    if (env.protocol_version != 1) {
        if (event_cb) event_cb(TRANS_EVENT_ERROR, user_data);
        return TRANS_STATUS_ERR_VERSION;
    }

    if (sizeof(vom_transport_envelope_t) + env.payload_length != (size_t)rc) {
        if (event_cb) event_cb(TRANS_EVENT_ERROR, user_data);
        return TRANS_STATUS_ERR_SIZE;
    }

    const uint8_t *payload_ptr = buffer + sizeof(vom_transport_envelope_t);

    if (env.message_code == MSG_CODE_AUTH_RESP) {
        ctx->is_authenticated = true;
        if (event_cb) event_cb(TRANS_EVENT_AUTHENTICATED, user_data);
    } else if (env.message_code == MSG_CODE_JOIN_RESP) {
        ctx->is_joined = true;
        if (event_cb) event_cb(TRANS_EVENT_JOINED, user_data);
    } else if (env.message_code == MSG_CODE_HEARTBEAT) {
        if (event_cb) event_cb(TRANS_EVENT_HEARTBEAT_ACKED, user_data);
    }

    if (msg_cb) {
        msg_cb(&env, payload_ptr, user_data);
    }

    return TRANS_STATUS_SUCCESS;
}

void vom_mock_msg_dispatch(const vom_transport_envelope_t *envelope, const uint8_t *payload, void *user_data) {
    (void)user_data;
    printf("Mock Dispatch Code parsed: %u, Length: %u\n", envelope->message_code, envelope->payload_length);
    if (envelope->message_code == MSG_CODE_CHUNK_ASSIGN && payload) {
        printf("Chunk processing stream assigned.\n");
    }
}

void vom_mock_event_dispatch(VomTransportEvent event, void *user_data) {
    (void)user_data;
    printf("Transport Event Fired: %d\n", event);
}

void execute_transport_test_suite(void) {
    vom_worker_transport_context_t *ctx = vom_worker_transport_create();
    
    vom_transport_config_t cfg;
    strncpy(cfg.worker_id, "wrk-test-fingerprint-01", TRANSPORT_ID_MAX_LEN - 1);
    memset(cfg.server_public_key, 0, TRANSPORT_KEY_MAX_LEN);
    memset(cfg.client_public_key, 0, TRANSPORT_KEY_MAX_LEN);
    memset(cfg.client_secret_key, 0, TRANSPORT_KEY_MAX_LEN);
    
    vom_worker_transport_configure(ctx, &cfg);
    vom_worker_transport_connect(ctx, "tcp://127.0.0.1:5555");
    
    vom_worker_transport_send(ctx, MSG_CODE_AUTH_REQ, (const uint8_t *)"credentials", 11);
    vom_worker_transport_poll(ctx, 0, vom_mock_msg_dispatch, vom_mock_event_dispatch, NULL);
    
    vom_worker_transport_disconnect(ctx, true);
    vom_worker_transport_destroy(ctx);
}

#ifdef VOM_STANDALONE_TEST
int main(void) {
    execute_transport_test_suite();
    return 0;
}
#endif