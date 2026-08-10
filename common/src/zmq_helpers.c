// common/src/zmq_helpers.c — thin ZeroMQ wrappers used by every binary.
// TODO: implement vom_zmq_context() as a process-wide singleton with
//       ref-counting so a forked worker does not double-terminate it.
// TODO: implement vom_zmq_make_socket() that maps (type, endpoint, is_bind)
//       to the matching zmq_socket / zmq_bind / zmq_connect sequence
//       with sensible ZMQ_LINGER + ZMQ_SNDHWM defaults.
// TODO: implement vom_zmq_set_identity / vom_zmq_recv_identity for the
//       ROUTER-side address book.

#include "common/zmq_helpers.h"
