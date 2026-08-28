#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <common/compat.h>

typedef enum {
    WRK_STATE_OFFLINE,
    WRK_STATE_CONNECTED,
    WRK_STATE_REGISTERED,
    WRK_STATE_RUNNING,
    WRK_STATE_DRAINING,
    WRK_STATE_SHUTDOWN
} WorkerState;

typedef enum {
    WRK_EVENT_CONNECT,
    WRK_EVENT_REGISTER_SUCCESS,
    WRK_EVENT_START_DRAIN,
    WRK_EVENT_STOP_DRAIN,
    WRK_EVENT_DISCONNECT,
    WRK_EVENT_TERMINATE
} WorkerEvent;

typedef struct {
    pthread_mutex_t lock;
    WorkerState current_state;
    uint64_t last_transition_ms;
    uint32_t active_tasks_count;
    char master_endpoint[128];
} vom_worker_state_t;

vom_worker_state_t* vom_worker_state_init(void) {
    vom_worker_state_t* ctx = (vom_worker_state_t*)calloc(1, sizeof(vom_worker_state_t));
    if (!ctx) return NULL;
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->current_state = WRK_STATE_OFFLINE;
    ctx->last_transition_ms = 0;
    ctx->active_tasks_count = 0;
    return ctx;
}

void vom_worker_state_destroy(vom_worker_state_t* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

WorkerState vom_worker_state_get(vom_worker_state_t* ctx) {
    pthread_mutex_lock(&ctx->lock);
    WorkerState s = ctx->current_state;
    pthread_mutex_unlock(&ctx->lock);
    return s;
}

bool vom_worker_state_transition(vom_worker_state_t* ctx, WorkerEvent event, uint64_t now_ms, const char* endpoint) {
    if (!ctx) return false;
    pthread_mutex_lock(&ctx->lock);

    bool allowed = false;
    WorkerState next = ctx->current_state;

    switch (ctx->current_state) {
        case WRK_STATE_OFFLINE:
            if (event == WRK_EVENT_CONNECT) {
                next = WRK_STATE_CONNECTED;
                allowed = true;
                if (endpoint) {
                    strncpy(ctx->master_endpoint, endpoint, sizeof(ctx->master_endpoint) - 1);
                }
            }
            break;

        case WRK_STATE_CONNECTED:
            if (event == WRK_EVENT_REGISTER_SUCCESS) {
                next = WRK_STATE_REGISTERED;
                allowed = true;
            } else if (event == WRK_EVENT_DISCONNECT) {
                next = WRK_STATE_OFFLINE;
                allowed = true;
            }
            break;

        case WRK_STATE_REGISTERED:
            if (event == WRK_EVENT_START_DRAIN) {
                next = WRK_STATE_DRAINING;
                allowed = true;
            } else if (event == WRK_EVENT_DISCONNECT) {
                next = WRK_STATE_OFFLINE;
                allowed = true;
            } else if (event == WRK_EVENT_TERMINATE) {
                next = WRK_STATE_SHUTDOWN;
                allowed = true;
            }
            break;

        case WRK_STATE_RUNNING:
            if (event == WRK_EVENT_START_DRAIN) {
                next = WRK_STATE_DRAINING;
                allowed = true;
            } else if (event == WRK_EVENT_DISCONNECT) {
                next = WRK_STATE_OFFLINE;
                allowed = true;
            } else if (event == WRK_EVENT_TERMINATE) {
                next = WRK_STATE_SHUTDOWN;
                allowed = true;
            }
            break;

        case WRK_STATE_DRAINING:
            if (event == WRK_EVENT_STOP_DRAIN) {
                next = WRK_STATE_REGISTERED;
                allowed = true;
            } else if (event == WRK_EVENT_DISCONNECT) {
                next = WRK_STATE_OFFLINE;
                allowed = true;
            } else if (event == WRK_EVENT_TERMINATE) {
                next = WRK_STATE_SHUTDOWN;
                allowed = true;
            }
            break;

        case WRK_STATE_SHUTDOWN:
            break;
    }

    if (allowed) {
        ctx->current_state = next;
        ctx->last_transition_ms = now_ms;
    }

    pthread_mutex_unlock(&ctx->lock);
    return allowed;
}

void vom_worker_state_update_tasks(vom_worker_state_t* ctx, int delta) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->lock);
    if (delta > 0) {
        ctx->active_tasks_count += (uint32_t)delta;
        if (ctx->current_state == WRK_STATE_REGISTERED) {
            ctx->current_state = WRK_STATE_RUNNING;
        }
    } else if (delta < 0) {
        uint32_t u_delta = (uint32_t)(-delta);
        if (ctx->active_tasks_count >= u_delta) {
            ctx->active_tasks_count -= u_delta;
        } else {
            ctx->active_tasks_count = 0;
        }
        if (ctx->active_tasks_count == 0 && ctx->current_state == WRK_STATE_RUNNING) {
            ctx->current_state = WRK_STATE_REGISTERED;
        }
    }
    pthread_mutex_unlock(&ctx->lock);
}

void execute_worker_state_test_suite(void) {
    vom_worker_state_t* ws = vom_worker_state_init();
    uint64_t clock_ms = 1000;

    bool r1 = vom_worker_state_transition(ws, WRK_EVENT_CONNECT, clock_ms, "tcp://127.0.0.1:5555");
    printf("Test Connect: %s | State: %d\n", r1 ? "PASS" : "FAIL", vom_worker_state_get(ws));

    clock_ms += 100;
    bool r2 = vom_worker_state_transition(ws, WRK_EVENT_REGISTER_SUCCESS, clock_ms, NULL);
    printf("Test Register: %s | State: %d\n", r2 ? "PASS" : "FAIL", vom_worker_state_get(ws));

    vom_worker_state_update_tasks(ws, 2);
    printf("Test Tasks Added | State: %d\n", vom_worker_state_get(ws));

    clock_ms += 100;
    bool r3 = vom_worker_state_transition(ws, WRK_EVENT_START_DRAIN, clock_ms, NULL);
    printf("Test Start Drain: %s | State: %d\n", r3 ? "PASS" : "FAIL", vom_worker_state_get(ws));

    clock_ms += 100;
    bool r4 = vom_worker_state_transition(ws, WRK_EVENT_DISCONNECT, clock_ms, NULL);
    printf("Test Reconnect/Disconnect Transition: %s | State: %d\n", r4 ? "PASS" : "FAIL", vom_worker_state_get(ws));

    clock_ms += 100;
    vom_worker_state_transition(ws, WRK_EVENT_CONNECT, clock_ms, "tcp://127.0.0.1:5555");
    vom_worker_state_transition(ws, WRK_EVENT_REGISTER_SUCCESS, clock_ms + 50, NULL);
    bool r5 = vom_worker_state_transition(ws, WRK_EVENT_TERMINATE, clock_ms + 100, NULL);
    printf("Test Shutdown Termination: %s | State: %d\n", r5 ? "PASS" : "FAIL", vom_worker_state_get(ws));

    vom_worker_state_destroy(ws);
}

#ifdef VOM_STANDALONE_TEST
int main(void) {
    execute_worker_state_test_suite();
    return 0;
}
#endif
