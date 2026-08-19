#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define VOM_ZMQ_IDENTITY        6
#define VOM_ZMQ_LINGER          17
#define VOM_ZMQ_SNDHWM          23
#define VOM_ZMQ_RCVHWM          24
#define VOM_ZMQ_RCVMORE         13
#define VOM_ZMQ_CURVE_SERVER    43
#define VOM_ZMQ_CURVE_PUBLICKEY 44
#define VOM_ZMQ_CURVE_SECRETKEY 45
#define VOM_ZMQ_CURVE_SERVERKEY 46

extern void *vom_zmq_context(void);
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

void *vom_zmq_make_socket(int type, const char *endpoint, int is_bind) {
    void *local_ctx = vom_zmq_context();
    if (local_ctx == NULL || endpoint == NULL) return NULL;
    void *sock = zmq_socket(local_ctx, type);
    if (sock == NULL) return NULL;
    int opt_linger = 1000;
    int opt_sndhwm = 5000;
    int opt_rcvhwm = 5000;
    zmq_setsockopt(sock, VOM_ZMQ_LINGER, &opt_linger, sizeof(opt_linger));
    zmq_setsockopt(sock, VOM_ZMQ_SNDHWM, &opt_sndhwm, sizeof(opt_sndhwm));
    zmq_setsockopt(sock, VOM_ZMQ_RCVHWM, &opt_rcvhwm, sizeof(opt_rcvhwm));
    int status = (is_bind != 0) ? zmq_bind(sock, endpoint) : zmq_connect(sock, endpoint);
    if (status != 0) {
        zmq_close(sock);
        return NULL;
    }
    return sock;
}

bool vom_zmq_apply_curve_security(void *sock, const char *server_public_key, const char *client_public_key, const char *client_secret_key) {
    if (sock == NULL || server_public_key == NULL) return false;
    if (client_public_key != NULL && client_secret_key != NULL) {
        int client_enabled = 1;
        if (zmq_setsockopt(sock, VOM_ZMQ_CURVE_SERVER, &client_enabled, sizeof(client_enabled)) != 0) return false;
        if (zmq_setsockopt(sock, VOM_ZMQ_CURVE_PUBLICKEY, client_public_key, strlen(client_public_key)) != 0) return false;
        if (zmq_setsockopt(sock, VOM_ZMQ_CURVE_SECRETKEY, client_secret_key, strlen(client_secret_key)) != 0) return false;
    } else {
        int server_enabled = 1;
        if (zmq_setsockopt(sock, VOM_ZMQ_CURVE_SERVER, &server_enabled, sizeof(server_enabled)) != 0) return false;
    }
    if (zmq_setsockopt(sock, VOM_ZMQ_CURVE_SERVERKEY, server_public_key, strlen(server_public_key)) != 0) return false;
    return true;
}

bool vom_zmq_set_identity(void *sock, const char *id) {
    if (sock == NULL || id == NULL) return false;
    size_t id_len = strlen(id);
    if (id_len > 255) return false;
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
