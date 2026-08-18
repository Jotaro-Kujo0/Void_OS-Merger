#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#define VOM_ZMQ_IDENTITY        6
#define VOM_ZMQ_LINGER          17
#define VOM_ZMQ_SNDHWM          23
#define VOM_ZMQ_RCVHWM          24
#define VOM_ZMQ_RCVMORE         13
#define VOM_ZMQ_CURVE_SERVER    43
#define VOM_ZMQ_CURVE_PUBLICKEY 44
#define VOM_ZMQ_CURVE_SECRETKEY 45
#define VOM_ZMQ_CURVE_SERVERKEY 46

extern void *zmq_ctx_new(void);
extern int   zmq_ctx_set(void *context, int option, int optval);
extern int   zmq_ctx_term(void *context);
extern void *zmq_socket(void *context, int type);
extern int   zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen);
extern int   zmq_getsockopt(void *s, int option, void *optval, size_t *optvallen);
extern int   zmq_bind(void *s, const char *addr);
extern int   zmq_connect(void *s, const char *addr);
extern int   zmq_close(void *s);

typedef struct { void *p; } vom_zmq_msg_t;
extern int   zmq_msg_init(vom_zmq_msg_t *msg);
extern int   zmq_msg_recv(vom_zmq_msg_t *msg, void *s, int flags);
extern void *zmq_msg_data(vom_zmq_msg_t *msg);
extern size_t zmq_msg_size(vom_zmq_msg_t *msg);
extern int   zmq_msg_close(vom_zmq_msg_t *msg);

static void *g_zmq_context = NULL;
static int g_context_ref_count = 0;
static pid_t g_context_owner_pid = 0;
static pthread_mutex_t g_zmq_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

void *vom_zmq_context(void) {
    pthread_mutex_lock(&g_zmq_ctx_mutex);
    pid_t current_pid = getpid();

    if (g_zmq_context != NULL && current_pid != g_context_owner_pid) {
        g_zmq_context = NULL;
        g_context_ref_count = 0;
    }

    if (g_zmq_context == NULL) {
        g_zmq_context = zmq_ctx_new();
        if (g_zmq_context != NULL) {
            zmq_ctx_set(g_zmq_context, 3, 2); 
            g_context_owner_pid = current_pid;
            g_context_ref_count = 1;
        }
    } else {
        g_context_ref_count++;
    }

    void *ret_ctx = g_zmq_context;
    pthread_mutex_unlock(&g_zmq_ctx_mutex);
    return ret_ctx;
}

void vom_zmq_context_term(void) {
    pthread_mutex_lock(&g_zmq_ctx_mutex);
    pid_t current_pid = getpid();

    if (g_zmq_context != NULL && current_pid == g_context_owner_pid) {
        g_context_ref_count--;
        if (g_context_ref_count  255) return false;
    return (zmq_setsockopt(sock, VOM_ZMQ_IDENTITY, id, id_len) == 0);
}

int vom_zmq_recv_identity(void *sock, char *out, size_t cap) {
    if (sock == NULL || out == NULL || cap == 0) return -1;

    vom_zmq_msg_t message;
    if (zmq_msg_init(&message) != 0) return -1;

    int bytes_read = zmq_msg_recv(&message, sock, 0);
    if (bytes_read < 0) {
        zmq_msg_close(&message);
        return -1;
    }

    size_t msg_len = zmq_msg_size(&message);
    size_t copy_bytes = (msg_len < cap - 1) ? msg_len : cap - 1;

    memcpy(out, zmq_msg_data(&message), copy_bytes);
    out[copy_bytes] = '\0';

    int rcvmore = 0;
    size_t rcvmore_size = sizeof(rcvmore);
    zmq_getsockopt(sock, VOM_ZMQ_RCVMORE, &rcvmore, &rcvmore_size);

    zmq_msg_close(&message);
    return rcvmore;
}
