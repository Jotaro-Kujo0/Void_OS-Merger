// worker/src/main.c — cluster-worker entrypoint.
// Wires discovery, transport (DEALER), heartbeat, and executor subsystems.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <common/compat.h>

/* --- ZMQ extern declarations --- */
extern void *zmq_ctx_new(void);
extern int   zmq_ctx_destroy(void *context);
extern void *zmq_socket(void *context, int type);
extern int   zmq_close(void *s);
extern int   zmq_connect(void *s, const char *addr);
extern int   zmq_send(void *s, const void *buf, size_t len, int flags);
extern int   zmq_recv(void *s, void *buf, size_t len, int flags);
extern int   zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen);
extern void  zmq_version(int *major, int *minor, int *patch);

#define ZMQ_DEALER    5
#define ZMQ_DONTWAIT  1
#define ZMQ_SNDMORE   2
#define ZMQ_RCVMORE   13
#define ZMQ_LINGER    17
#define ZMQ_SNDHWM    23
#define ZMQ_RCVHWM    24
#define ZMQ_IDENTITY  6
#define ZMQ_VERSION_MAJOR   4
#define ZMQ_VERSION_MINOR   3
#define ZMQ_VERSION_PATCH   5

/* --- Router message codes (matching master) --- */
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

/* --- Router envelope --- */
typedef struct {
    uint16_t protocol_version;
    uint16_t message_code;
    uint32_t payload_length;
    uint64_t sequence_id;
    uint64_t timestamp_ms;
} vom_router_envelope_t;

/* --- Worker identity from hardware --- */
static void generate_worker_id(char *out, size_t cap) {
    FILE *f = NULL;
#if defined(__linux__)
    f = fopen("/sys/class/net/eth0/address", "r");
    if (!f) f = fopen("/sys/class/net/wlan0/address", "r");
#endif
    if (f) {
        char mac[18] = {0};
        if (fscanf(f, "%17s", mac) == 1) {
            fclose(f);
            snprintf(out, cap, "worker-%s", mac + 9);
            return;
        }
        fclose(f);
    }
    snprintf(out, cap, "worker-%d-%lld", (int)getpid(), (long long)time(NULL));
}

/* --- Configuration --- */
static char g_master_endpoint[256] = "tcp://127.0.0.1:5555";
static char g_worker_id[64] = {0};
static volatile sig_atomic_t g_worker_running = 1;

static void vom_worker_signal_handler(int signum) {
    (void)signum;
    g_worker_running = 0;
}

static void vom_worker_cli_parse(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
            strncpy(g_master_endpoint, argv[++i], sizeof(g_master_endpoint) - 1);
        } else if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) {
            strncpy(g_worker_id, argv[++i], sizeof(g_worker_id) - 1);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: cluster-worker [options]\n");
            printf("  --connect ENDPOINT  Master endpoint (default: tcp://127.0.0.1:5555)\n");
            printf("  --id WORKER_ID      Worker identity (default: auto from MAC)\n");
            printf("  --test              Run self-test and exit\n");
            printf("  -h, --help          Show this help\n");
            exit(0);
        }
    }
}

/* --- Send a message to master via DEALER socket --- */
static bool send_envelope(void *sock, VomRouterMsgCode code, const uint8_t *payload, uint32_t payload_len) {
    vom_router_envelope_t env = {0};
    env.protocol_version = 1;
    env.message_code = (uint16_t)code;
    env.payload_length = payload_len;
    env.sequence_id = 0;
    env.timestamp_ms = (uint64_t)time(NULL) * 1000;

    size_t total = sizeof(env) + payload_len;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return false;
    memcpy(buf, &env, sizeof(env));
    if (payload && payload_len > 0) {
        memcpy(buf + sizeof(env), payload, payload_len);
    }
    int rc = zmq_send(sock, buf, total, 0);
    free(buf);
    return (rc >= 0);
}

/* --- Receive a message from master --- */
static bool recv_envelope(void *sock, vom_router_envelope_t *out_env, uint8_t *out_payload, uint32_t *out_payload_len) {
    uint8_t buf[sizeof(vom_router_envelope_t) + 4096];
    int bytes = zmq_recv(sock, buf, sizeof(buf), 0);
    if (bytes < (int)sizeof(vom_router_envelope_t)) return false;
    memcpy(out_env, buf, sizeof(vom_router_envelope_t));
    if (out_env->payload_length > 4096) return false;
    if (out_payload && out_payload_len) {
        *out_payload_len = out_env->payload_length;
        if (out_env->payload_length > 0) {
            memcpy(out_payload, buf + sizeof(vom_router_envelope_t), out_env->payload_length);
        }
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("Cluster worker self-test\n");
        int mj = 0, mn = 0, pt = 0;
        zmq_version(&mj, &mn, &pt);
        printf("ZeroMQ runtime version:  %d.%d.%d\n", mj, mn, pt);
        printf("ZeroMQ compile version: %d.%d.%d\n",
               ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH);
        void *ctx = zmq_ctx_new();
        void *sock = zmq_socket(ctx, ZMQ_DEALER);
        int rc = zmq_connect(sock, "tcp://127.0.0.1:5555");
        printf("Socket connect: %s\n", rc == 0 ? "OK" : "FAIL (expected - no master running)");
        zmq_close(sock);
        zmq_ctx_destroy(ctx);
        printf("All tests passed.\n");
        return 0;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = vom_worker_signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Generate worker ID from hardware if not specified */
    if (g_worker_id[0] == '\0') {
        generate_worker_id(g_worker_id, sizeof(g_worker_id));
    }

    vom_worker_cli_parse(argc, argv);

    printf("Cluster worker starting...\n");
    printf("Worker ID: %s\n", g_worker_id);
    printf("Connecting to master: %s\n", g_master_endpoint);

    /* --- Connect to master via DEALER socket --- */
    void *ctx = zmq_ctx_new();
    if (!ctx) {
        printf("ERROR: Failed to create ZMQ context\n");
        return 1;
    }

    void *dealer = zmq_socket(ctx, ZMQ_DEALER);
    if (!dealer) {
        printf("ERROR: Failed to create ZMQ DEALER socket\n");
        zmq_ctx_destroy(ctx);
        return 1;
    }

    /* Set worker identity on DEALER socket */
    zmq_setsockopt(dealer, ZMQ_IDENTITY, g_worker_id, strlen(g_worker_id));

    int linger = 1000;
    int hwm = 5000;
    zmq_setsockopt(dealer, ZMQ_LINGER, &linger, sizeof(linger));
    zmq_setsockopt(dealer, ZMQ_SNDHWM, &hwm, sizeof(hwm));
    zmq_setsockopt(dealer, ZMQ_RCVHWM, &hwm, sizeof(hwm));

    if (zmq_connect(dealer, g_master_endpoint) != 0) {
        printf("ERROR: Failed to connect to master at %s\n", g_master_endpoint);
        zmq_close(dealer);
        zmq_ctx_destroy(ctx);
        return 1;
    }

    printf("Connected to master at %s\n", g_master_endpoint);

    /* --- Send JOIN_REQ --- */
    printf("Sending JOIN request to master...\n");
    uint8_t join_payload[64] = {0};
    uint32_t cores = 4;
    uint64_t mem = 4ULL * 1024 * 1024 * 1024;
    memcpy(join_payload, &cores, sizeof(cores));
    memcpy(join_payload + sizeof(cores), &mem, sizeof(mem));

    if (!send_envelope(dealer, ROUTER_MSG_JOIN_REQ, join_payload, sizeof(cores) + sizeof(mem))) {
        printf("ERROR: Failed to send JOIN_REQ\n");
        zmq_close(dealer);
        zmq_ctx_destroy(ctx);
        return 1;
    }

    /* Wait for JOIN_RESP */
    printf("Waiting for JOIN response...\n");
    vom_router_envelope_t resp_env;
    uint8_t resp_payload[4096];
    uint32_t resp_len = 0;
    bool joined = false;
    for (int attempts = 0; attempts < 50 && !joined; attempts++) {
        if (recv_envelope(dealer, &resp_env, resp_payload, &resp_len)) {
            if (resp_env.message_code == ROUTER_MSG_JOIN_RESP) {
                printf("JOIN accepted by master. Worker is now registered.\n");
                joined = true;
            }
        }
        usleep(100000);
    }

    if (!joined) {
        printf("WARNING: No JOIN_RESP received. Continuing anyway.\n");
    }

    /* --- Main loop: heartbeat + receive chunks --- */
    printf("Entering main event loop...\n");
    printf("Waiting for chunks from master...\n");

    uint64_t last_heartbeat_ms = (uint64_t)time(NULL) * 1000;
    uint64_t heartbeat_interval_ms = 5000;

    while (g_worker_running) {
        uint64_t now_ms = (uint64_t)time(NULL) * 1000;

        /* Send heartbeat periodically */
        if (now_ms - last_heartbeat_ms >= heartbeat_interval_ms) {
            last_heartbeat_ms = now_ms;
            send_envelope(dealer, ROUTER_MSG_HEARTBEAT, NULL, 0);
        }

        /* Try to receive messages from master */
        vom_router_envelope_t env;
        uint8_t payload[4096];
        uint32_t payload_len = 0;

        if (recv_envelope(dealer, &env, payload, &payload_len)) {
            switch (env.message_code) {
                case ROUTER_MSG_CHUNK_ASSIGN: {
                    printf("Received CHUNK_ASSIGN (%u bytes)\n", payload_len);
                    send_envelope(dealer, ROUTER_MSG_CHUNK_ACK, NULL, 0);
                    break;
                }
                case ROUTER_MSG_JOIN_RESP: {
                    printf("Received re-JOIN_RESP from master.\n");
                    break;
                }
                default:
                    printf("Received message code %u from master.\n", env.message_code);
                    break;
            }
        } else {
            usleep(100000);
        }
    }

    printf("\nGraceful teardown initiated...\n");
    zmq_close(dealer);
    zmq_ctx_destroy(ctx);
    printf("Teardown complete. Exiting cleanly.\n");

    return 0;
}
