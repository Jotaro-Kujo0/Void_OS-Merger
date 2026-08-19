#ifndef VOM_WORKER_WORKER_STATE_H
#define VOM_WORKER_WORKER_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define STATE_ID_MAX 64

typedef enum {
    LIFECYCLE_INITIALIZING,
    LIFECYCLE_DISCOVERING,
    LIFECYCLE_CONNECTING,
    LIFECYCLE_PENDING_APPROVAL,
    LIFECYCLE_OPERATIONAL,
    LIFECYCLE_DRAINING,
    LIFECYCLE_SHUTTING_DOWN
} VomWorkerLifecycleState;

typedef enum {
    CHUNK_EXEC_IDLE,
    CHUNK_EXEC_RESERVED,
    CHUNK_EXEC_RUNNING,
    CHUNK_EXEC_SUSPENDED,
    CHUNK_EXEC_CANCELLING,
    CHUNK_EXEC_COMPLETED,
    CHUNK_EXEC_FAILED
} VomLocalChunkState;

typedef struct {
    uint64_t chunk_id;
    uint64_t lease_id;
    VomLocalChunkState exec_state;
    uint64_t state_transition_timestamp_ms;
    float current_progress;
} vom_local_chunk_record_t;

typedef struct vom_worker_state_manager vom_worker_state_manager_t;

vom_worker_state_manager_t* vom_worker_state_manager_create(void);
void vom_worker_state_manager_destroy(vom_worker_state_manager_t *ctx);

int32_t vom_worker_state_initialize_identity(vom_worker_state_manager_t *ctx, const char *uuid, void *opaque_static_caps_ptr);
VomWorkerLifecycleState vom_worker_state_get_lifecycle(vom_worker_state_manager_t *ctx);

int32_t vom_worker_state_transition_to(vom_worker_state_manager_t *ctx, VomWorkerLifecycleState target_state);
int32_t vom_worker_state_synchronize_dynamic_resources(vom_worker_state_manager_t *ctx, void *opaque_dynamic_res_ptr);

int32_t vom_worker_state_register_reservation(vom_worker_state_manager_t *ctx, uint64_t chunk_id, uint64_t lease_id);
int32_t vom_worker_state_update_chunk_progress(vom_worker_state_manager_t *ctx, uint64_t chunk_id, VomLocalChunkState next_state, float progress);

int32_t vom_worker_state_reconcile_with_master(vom_worker_state_manager_t *ctx, const uint64_t *authoritative_lease_ids, size_t lease_count);
int32_t vom_worker_state_update_ui_policy(vom_worker_state_manager_t *ctx, uint32_t display_index, int32_t ui_mode_enum);

#endif
