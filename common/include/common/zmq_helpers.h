// common/zmq_helpers.h — thin wrappers around libzmq so the rest of the
// codebase does not deal with raw ctx/socket lifetimes directly.
// TODO: implement:
//   * vom_zmq_context() returning a process-wide singleton ZMQ context
//     (with sane IO thread default) plus a vom_zmq_context_term() shutdown
//     hook used by signal handlers.
//   * vom_zmq_make_socket(int type) returning a freshly-typed socket.
//   * helpers for the common connect/bind calls with retry + timeout.
//   * helpers that set ZMQ_CURVE encryption keys from a config file so
//     tablets on a shared Wi-Fi can join a cluster without a VPN.
//   * identity helpers (vom_zmq_set_identity / vom_zmq_recv_identity)
//     so the master can ROUTER-route messages back to specific workers.

#ifndef VOM_COMMON_ZMQ_HELPERS_H
#define VOM_COMMON_ZMQ_HELPERS_H

#include <zmq.h>

// TODO: declare vom_zmq_context(void).
// TODO: declare vom_zmq_make_socket(int type, const char *endpoint,
//                                   int is_bind /* 0=connect, 1=bind */).
// TODO: declare vom_zmq_set_identity(void *sock, const char *id).
// TODO: declare vom_zmq_recv_identity(void *sock, char *out, size_t cap).

#endif /* VOM_COMMON_ZMQ_HELPERS_H */
