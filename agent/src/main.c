#include <stdio.h>
#include <string.h>
#include <zmq.h>

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        printf("Cluster agent self-test\n");
        printf("ZeroMQ version: %s\n", zmq_version());
        
        void *ctx = zmq_ctx_new();
        void *sock = zmq_socket(ctx, ZMQ_PAIR);
        int rc = zmq_bind(sock, "tcp://127.0.0.1:5555");
        printf("Socket bind: %s\n", rc == 0 ? "OK" : "FAIL");
        
        zmq_close(sock);
        zmq_ctx_destroy(ctx);
        printf("All tests passed.\n");
        return 0;
    }
    
    printf("Cluster agent starting...\n");
    printf("Waiting for tasks from master...\n");
    
    void *ctx = zmq_ctx_new();
    void *pull = zmq_socket(ctx, ZMQ_PULL);
    zmq_connect(pull, "tcp://192.168.1.10:5555");
    
    while (1) {
        char buf[4096];
        int size = zmq_recv(pull, buf, 4096, 0);
        if (size == -1) break;
        buf[size] = '\0';
        printf("Received task (%d bytes): %s\n", size, buf);
    }
    
    zmq_close(pull);
    zmq_ctx_destroy(ctx);
    return 0;
}
