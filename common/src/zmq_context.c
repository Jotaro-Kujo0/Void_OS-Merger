#include <unistd.h>
#include <pthread.h>

extern void *zmq_ctx_new(void);
extern int   zmq_ctx_set(void *context, int option, int optval);
extern int   zmq_ctx_term(void *context);

static void *g_zmq_context = NULL;
static int g_context_ref_count = 0;
static pid_t g_context_owner_pid = 0;
static pthread_mutex_t g_zmq_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

void *vom_zmq_context(void) {
    pthread_mutex_lock(&g_zmq_ctx_mutex);
    pid_t current_pid = getpid();
    if (g_zmq_context != NULL) {
        if (current_pid != g_context_owner_pid) {
            g_zmq_context = NULL;
            g_context_ref_count = 0;
        }
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
    if (g_zmq_context != NULL) {
        if (current_pid == g_context_owner_pid) {
            g_context_ref_count--;
            if (g_context_ref_count <= 0) {
                zmq_ctx_term(g_zmq_context);
                g_zmq_context = NULL;
                g_context_owner_pid = 0;
                g_context_ref_count = 0;
            }
        }
    }
    pthread_mutex_unlock(&g_zmq_ctx_mutex);
}
