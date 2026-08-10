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
#include <zmq.h>

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
    
    printf("Cluster worker starting...\n");
    printf("Waiting for chunks from master...\n");
    
    void *ctx = zmq_ctx_new();
    void *pull = zmq_socket(ctx, ZMQ_PULL);
    zmq_connect(pull, "tcp://192.168.1.10:5555");
    
    while (1) {
        char buf[4096];
        int size = zmq_recv(pull, buf, 4096, 0);
        if (size == -1) break;
        buf[size] = '\0';
        printf("Received chunk (%d bytes): %s\n", size, buf);
    }
    
    zmq_close(pull);
    zmq_ctx_destroy(ctx);
    return 0;
}
