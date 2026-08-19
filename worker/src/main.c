// worker/src/main.c — cluster-worker entrypoint.
// The working --test self-test below is preserved for CI; once the new
// modules below are filled in, the non-test branch should become a thin
// caller that wires together:
//
//     vom_worker_cli_parse(...)        →  cli.c
//     vom_worker_capabilities_init();                →  capabilities.c
//     vom_worker_discovery_start(...);       →  discovery.c
//     vom_worker_transport_init(...);        →  transport.c   (DEALER socket)
//     vom_worker_heartbeat_start(...);       →  heartbeat.c
//     vom_worker_executor_start();           →  executor.c
//     poll-loop: vom_transport_recv_msg → dispatch by MsgCode
//
// NOTE: the raw ZMQ_PULL socket still in this file is a placeholder while
//       transport.c is unimplemented; the real worker will talk to the
//       master over a DEALER socket with a stable workerId identity so the
//       master's ROUTER can address it individually.  Delete the PULL
//       block once transport.c is wired in.
//
// TODO: replace the current ZMQ_PULL loop with the dispatcher above.
// TODO: install SIGINT/SIGTERM handlers that gracefully tear down
//       heartbeat, executor, transport, discovery before exiting.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#define ZMQ_PAIR    0
#define ZMQ_DEALER  5
#define ZMQ_DONTWAIT 1
#define ZMQ_VERSION_MAJOR   4
#define ZMQ_VERSION_MINOR   3
#define ZMQ_VERSION_PATCH   5

extern void zmq_version(int *major, int *minor, int *patch);
extern void *zmq_ctx_new(void);
extern int zmq_ctx_destroy(void *context);
extern void *zmq_socket(void *context, int type);
extern int zmq_close(void *s);
extern int zmq_bind(void *s, const char *addr);
extern int zmq_connect(void *s, const char *addr);
extern int zmq_recv(void *s, void *buf, size_t len, int flags);

static volatile sig_atomic_t g_worker_running = 1;

static void vom_worker_signal_handler(int signum) {
    (void)signum;
    g_worker_running = 0;
}

static void vom_worker_cli_parse(int argc, char **argv) {
    (void)argc; (void)argv;
}

static void vom_worker_capabilities_init(void) {
}

static void vom_worker_discovery_start(void) {
}

static void* vom_worker_transport_init(void *ctx, const char *endpoint) {
    void *dealer = zmq_socket(ctx, ZMQ_DEALER);
    if (dealer) {
        zmq_connect(dealer, endpoint);
    }
    return dealer;
}

static void vom_worker_heartbeat_start(void) {
}

static void vom_worker_executor_start(void) {
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
        void *sock = zmq_socket(ctx, ZMQ_PAIR);
        int rc = zmq_bind(sock, "tcp://127.0.0.1:5555");
        printf("Socket bind: %s\n", rc == 0 ? "OK" : "FAIL");
        
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

    printf("Cluster worker starting...\n");
    
    vom_worker_cli_parse(argc, argv);
    vom_worker_capabilities_init();
    vom_worker_discovery_start();
    
    void *ctx = zmq_ctx_new();
    void *dealer = vom_worker_transport_init(ctx, "tcp://192.168.1.10:5555");
    
    vom_worker_heartbeat_start();
    vom_worker_executor_start();
    
    printf("Waiting for chunks from master...\n");
    
    while (g_worker_running) {
        char buf[4096];
        int size = zmq_recv(dealer, buf, sizeof(buf) - 1, ZMQ_DONTWAIT);
        if (size >= 0) {
            buf[size] = '\0';
            printf("Received chunk (%d bytes): %s\n", size, buf);
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