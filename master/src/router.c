#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <common/compat.h>

#define ROUTER_ID_MAX_LEN       64
#define ROUTER_PAYLOAD_MAX_LEN  4096
#define VOM_ZMQ_IDENTITY        6
#define VOM_ZMQ_LINGER          17
#define VOM_ZMQ_SNDHWM          23
#define VOM_ZMQ_RCVHWM          24
#define VOM_ZMQ_RCVMORE         13
#define VOM_ZMQ_DONTWAIT        1

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

typedef void (*VomRouterEventCallback)(const char *worker_id, VomRouterEvent event, void *user_data);
typedef void (*VomRouterDispatchCallback)(const char *worker_id, const vom_router_envelope_t *envelope, const uint8_t *payload, void *user_data);

extern void *zmq_ctx_new(void);
extern void *zmq_socket(void *context, int type);
extern int   zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen);
extern int   zmq_getsockopt(void *s, int option, void *optval, size_t *optvallen);
extern int   zmq_bind(void *s, const char *addr);
extern int   zmq_close(void *s);
extern int   zmq_ctx_destroy(void *context);
extern int   zmq_send(void *s, const void *buf, size_t len, int flags);
extern int   zmq_recv(void *s, void *buf, size_t len, int flags);

struct vom_master_router_context {
    void *zmq_ctx;
    void *zmq_sock;
    uint64_t next_seq_id;
    bool is_running;
    pthread_mutex_t write_lock;
};

typedef struct vom_master_router_context vom_master_router_context_t;

void vom_mock_router_msg_dispatch(const char *worker_id, const vom_router_envelope_t *envelope, const uint8_t *payload, void *user_data) {
    (void)payload; (void)user_data;
    printf("[DISPATCH] Worker: %s, MsgCode: %u, Size: %u\n", worker_id, envelope->message_code, envelope->payload_length);
}

void vom_mock_router_event_dispatch(const char *worker_id, VomRouterEvent event, void *user_data) {
    (void)user_data;
    printf("[EVENT] Worker: %s, EventID: %d\n", worker_id, event);
}

vom_master_router_context_t* vom_master_router_create(void) {
    vom_master_router_context_t *ctx = (vom_master_router_context_t*)calloc(1, sizeof(vom_master_router_context_t));
    if (!ctx) return NULL;
    ctx->zmq_ctx = zmq_ctx_new();
    if (!ctx->zmq_ctx) {
        free(ctx);
        return NULL;
    }
    pthread_mutex_init(&ctx->write_lock, NULL);
    ctx->next_seq_id = 1;
    ctx->is_running = false;
    return ctx;
}

void vom_master_router_destroy(vom_master_router_context_t *ctx) {
    if (!ctx) return;
    if (ctx->zmq_sock) zmq_close(ctx->zmq_sock);
    if (ctx->zmq_ctx) zmq_ctx_destroy(ctx->zmq_ctx);
    pthread_mutex_destroy(&ctx->write_lock);
    free(ctx);
}

VomRouterStatus vom_master_router_start(vom_master_router_context_t *ctx, const char *bind_endpoint) {
    if (!ctx || !bind_endpoint) return ROUTER_STATUS_ERR_SOCKET;
    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, 4);
    if (!ctx->zmq_sock) return ROUTER_STATUS_ERR_SOCKET;
    int linger = 1000;
    int hwm = 5000;
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_LINGER, &linger, sizeof(linger));
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_SNDHWM, &hwm, sizeof(hwm));
    zmq_setsockopt(ctx->zmq_sock, VOM_ZMQ_RCVHWM, &hwm, sizeof(hwm));
    if (zmq_bind(ctx->zmq_sock, bind_endpoint) != 0) {
        zmq_close(ctx->zmq_sock);
        ctx->zmq_sock = NULL;
        return ROUTER_STATUS_ERR_SOCKET;
    }
    ctx->is_running = true;
    return ROUTER_STATUS_SUCCESS;
}

void vom_master_router_stop(vom_master_router_context_t *ctx) {
    if (ctx) ctx->is_running = false;
}

VomRouterStatus vom_master_router_send_to_worker(vom_master_router_context_t *ctx, const char *worker_id, VomRouterMsgCode code, const uint8_t *payload, size_t length) {
    if (!ctx || !ctx->zmq_sock || !ctx->is_running || !worker_id) return ROUTER_STATUS_ERR_SOCKET;
    if (length > ROUTER_PAYLOAD_MAX_LEN) return ROUTER_STATUS_ERR_SIZE;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    vom_router_envelope_t env;
    env.protocol_version = 1;
    env.message_code = (uint16_t)code;
    env.payload_length = (uint32_t)length;
    pthread_mutex_lock(&ctx->write_lock);
    env.sequence_id = ctx->next_seq_id++;
    env.timestamp_ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
    int rc1 = zmq_send(ctx->zmq_sock, worker_id, strlen(worker_id), 2);
    if (rc1 < 0) {
        pthread_mutex_unlock(&ctx->write_lock);
        return ROUTER_STATUS_ERR_SOCKET;
    }
    int rc2 = zmq_send(ctx->zmq_sock, "", 0, 2);
    if (rc2 < 0) {
        pthread_mutex_unlock(&ctx->write_lock);
        return ROUTER_STATUS_ERR_SOCKET;
    }
    uint8_t msg_buffer[sizeof(vom_router_envelope_t) + ROUTER_PAYLOAD_MAX_LEN];
    memcpy(msg_buffer, &env, sizeof(vom_router_envelope_t));
    if (payload && length > 0) {
        memcpy(msg_buffer + sizeof(vom_router_envelope_t), payload, length);
    }
    int rc3 = zmq_send(ctx->zmq_sock, msg_buffer, sizeof(vom_router_envelope_t) + length, 0);
    pthread_mutex_unlock(&ctx->write_lock);
    if (rc3 < 0) return ROUTER_STATUS_ERR_SOCKET;
    return ROUTER_STATUS_SUCCESS;
}

static bool internal_router_check_frames(void *sock) {
    int rcv = 0;
    size_t rcv_sz = sizeof(rcv);
    int res = zmq_getsockopt(sock, VOM_ZMQ_RCVMORE, &rcv, &rcv_sz);
    if (res != 0) return false;
    return (rcv != 0);
}

VomRouterStatus vom_master_router_run_once(vom_master_router_context_t *ctx, int timeout_ms, VomRouterDispatchCallback msg_cb, VomRouterEventCallback event_cb, void *user_data) {
    if (!ctx || !ctx->zmq_sock || !ctx->is_running) return ROUTER_STATUS_ERR_SOCKET;
    (void)timeout_ms;
    
    char id_buf[ROUTER_ID_MAX_LEN];
    int bytes = zmq_recv(ctx->zmq_sock, id_buf, sizeof(id_buf) - 1, VOM_ZMQ_DONTWAIT);
    if (bytes < 0) return ROUTER_STATUS_SUCCESS;
    id_buf[bytes] = '\0';
    
    bool more1 = internal_router_check_frames(ctx->zmq_sock);
    if (!more1) return ROUTER_STATUS_ERR_SIZE;
    
    char dbuf;
    int dbytes = zmq_recv(ctx->zmq_sock, &dbuf, 1, 0);
    if (dbytes < 0) return ROUTER_STATUS_ERR_SIZE;
    
    bool more2 = internal_router_check_frames(ctx->zmq_sock);
    if (!more2) return ROUTER_STATUS_ERR_SIZE;
    
    uint8_t pbuf[sizeof(vom_router_envelope_t) + ROUTER_PAYLOAD_MAX_LEN];
    int pbytes = zmq_recv(ctx->zmq_sock, pbuf, sizeof(pbuf), 0);
    if (pbytes < (int)sizeof(vom_router_envelope_t)) {
        if (event_cb) event_cb(id_buf, ROUTER_EVENT_TRANSPORT_FAULT, user_data);
        return ROUTER_STATUS_ERR_SIZE;
    }
    
    vom_router_envelope_t env;
    memcpy(&env, pbuf, sizeof(vom_router_envelope_t));
    if (env.protocol_version != 1) {
        if (event_cb) event_cb(id_buf, ROUTER_EVENT_TRANSPORT_FAULT, user_data);
        return ROUTER_STATUS_ERR_VERSION;
    }
    if ((int)sizeof(vom_router_envelope_t) + env.payload_length != pbytes) {
        if (event_cb) event_cb(id_buf, ROUTER_EVENT_TRANSPORT_FAULT, user_data);
        return ROUTER_STATUS_ERR_SIZE;
    }
    
    const uint8_t *payload_ptr = pbuf + sizeof(vom_router_envelope_t);
    if (event_cb) {
        if (env.message_code == ROUTER_MSG_JOIN_REQ) event_cb(id_buf, ROUTER_EVENT_WORKER_JOINED, user_data);
        else if (env.message_code == ROUTER_MSG_HEARTBEAT) event_cb(id_buf, ROUTER_EVENT_WORKER_HEARTBEAT, user_data);
        else if (env.message_code == ROUTER_MSG_CHUNK_REPORT) event_cb(id_buf, ROUTER_EVENT_CHUNK_RESULT, user_data);
    }
    if (msg_cb) msg_cb(id_buf, &env, payload_ptr, user_data);
    return ROUTER_STATUS_SUCCESS;
}

void execute_router_test_suite(void) {
    vom_master_router_context_t *broker = vom_master_router_create();
    VomRouterStatus rc_start = vom_master_router_start(broker, "tcp://127.0.0.1:6555");
    printf("Test Init: %s\n", (rc_start == ROUTER_STATUS_SUCCESS) ? "PASS" : "FAIL");
    uint8_t giant_buffer[ROUTER_PAYLOAD_MAX_LEN + 100];
    VomRouterStatus rc_size = vom_master_router_send_to_worker(broker, "worker-node-01", ROUTER_MSG_CHUNK_ASSIGN, giant_buffer, sizeof(giant_buffer));
    printf("Test Limit: %s\n", (rc_size == ROUTER_STATUS_ERR_SIZE) ? "PASS" : "FAIL");
    vom_master_router_run_once(broker, 0, vom_mock_router_msg_dispatch, vom_mock_router_event_dispatch, NULL);
    vom_master_router_stop(broker);
    vom_master_router_destroy(broker);
}

#ifdef VOM_STANDALONE_TEST
int main(void) {
    execute_router_test_suite();
    return 0;
}
#endif
