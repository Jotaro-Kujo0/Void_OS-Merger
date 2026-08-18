#ifndef VOM_COMMON_ZMQ_HELPERS_H
#define VOM_COMMON_ZMQ_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VOM_ZMQ_IDENTITY    6
#define VOM_ZMQ_LINGER      17
#define VOM_ZMQ_SNDHWM      23
#define VOM_ZMQ_RCVHWM      24
#define VOM_ZMQ_RCVMORE     13
#define VOM_ZMQ_CURVE_SERVER    43
#define VOM_ZMQ_CURVE_PUBLICKEY 44
#define VOM_ZMQ_CURVE_SECRETKEY 45
#define VOM_ZMQ_CURVE_SERVERKEY 46

//subsystem core api entrypoints
void *vom_zmq_context(void);
void vom_zmq_context_term(void);

void *vom_zmq_make_socket(int type, const char *endpoint, int is_bind);

bool vom_zmq_apply_curve_security(void *sock, const char *server_public_key,
                                  const char *client_public_key, const char *client_secret_key);

bool vom_zmq_set_identity(void *sock, const char *id);
int vom_zmq_recv_identity(void *sock, char *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* VOM_COMMON_ZMQ_HELPERS_H */
