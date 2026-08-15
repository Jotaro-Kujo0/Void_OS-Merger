#ifndef VOM_WORKER_EXECUTOR_H
#define VOM_WORKER_EXECUTOR_H

#include "common/sys_info.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EXEC_STATE_IDLE,
    EXEC_STATE_ACCEPTED,
    EXEC_STATE_STARTED,
    EXEC_STATE_PROGRESS,
    EXEC_STATE_COMPLETED,
    EXEC_STATE_FAILED,
    EXEC_STATE_CANCELLED
} ExecState;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    char runtime_type[16];
    char binary_path[256];
    uint32_t required_cores;
    uint64_t required_memory_bytes;
    uint32_t timeout_seconds;
    bool is_idempotent;
    char expected_input_sha256[65];
} ChunkAssignment;

typedef struct {
    uint64_t chunk_id;
    ExecState state;
    float progress_percentage;
    uint32_t exit_code;
    char result_sha256[65];
} ChunkExecStatus;

// Progress reporting signature to pipe back telemetry update
typedef void (*ChunkStatusCallback)(const ChunkExecStatus *status, void *user_data);

//Core Executor Lifecycle API
bool vom_executor_validate(const ChunkAssignment *assignment, const vom_worker_capabilities *host_caps);
bool vom_executor_run(const ChunkAssignment *assignment, ChunkStatusCallback cb, void *user_data, volatile bool *cancel_flag);

#ifdef __cplusplus
}
#endif

#endif /* VOM_WORKER_EXECUTOR_H */
