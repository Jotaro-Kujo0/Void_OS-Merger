#ifndef VOM_UI_MODEL_H
#define VOM_UI_MODEL_H

#include <stdint.h>
#include <stdbool.h>

#define UI_FIELD_MAX 64
#define UI_POOL_WORKERS 32
#define UI_POOL_WORKLOADS 64
#define UI_POOL_CHUNKS 128
#define UI_POOL_ALERTS 16

typedef enum {
    UI_DEV_BOOTSTRAP = 0,
    UI_DEV_OPERATIONAL = 1,
    UI_DEV_DEGRADED = 2,
    UI_DEV_PANIC = 3
} VomUiDeviceStatus;

typedef enum {
    UI_HEALTH_ONLINE = 0,
    UI_HEALTH_DRAINING = 1,
    UI_HEALTH_OFFLINE = 2
} VomUiWorkerHealth;

typedef enum {
    UI_JOB_PENDING = 0,
    UI_JOB_RUNNING = 1,
    UI_JOB_COMPLETED = 2,
    UI_JOB_FAILED = 3,
    UI_JOB_CANCELLED = 4
} VomUiJobState;

typedef enum {
    UI_ALERT_INFO = 0,
    UI_ALERT_WARNING = 1,
    UI_ALERT_CRITICAL = 2
} VomUiAlertSeverity;

typedef struct {
    uint64_t total_memory_bytes;
    uint64_t allocated_memory_bytes;
    uint32_t total_cpu_cores;
    uint32_t allocated_cpu_cores;
} vom_ui_hardware_resource_t;

typedef struct {
    char worker_id[UI_FIELD_MAX];
    VomUiWorkerHealth health_status;
    vom_ui_hardware_resource_t resources;
    uint64_t last_heartbeat_ms;
    bool local_display_attached;
} vom_ui_worker_summary_t;

typedef struct {
    char chunk_id[UI_FIELD_MAX];
    VomUiJobState execution_state;
    float progress_percentage;
    char assigned_worker_id[UI_FIELD_MAX];
} vom_ui_chunk_progress_t;

typedef struct {
    char workload_id[UI_FIELD_MAX];
    VomUiJobState global_state;
    float cumulative_progress;
    uint32_t active_chunks;
    
    /* REDACTION RULE: Never include plain-text input paths, tokens, or raw keys */
    char redacted_workload_name[UI_FIELD_MAX];
} vom_ui_workload_progress_t;

typedef struct {
    uint32_t alert_id;
    VomUiAlertSeverity severity;
    char actionable_component_id[UI_FIELD_MAX];
    
    /* REDACTION RULE: Contain purely systemic operational tokens; wipe user data variables */
    char explanation_token[UI_FIELD_MAX];
    uint64_t raised_timestamp_ms;
} vom_ui_actionable_alert_t;

/* 
 * IMMUTABLE SNAPSHOT VIEW CONCEPT:
 * Represents a presentation-neutral, frozen state record of the logical device.
 * Exposes stable field name boundaries optimized for JSON serialization or IPC consumers.
 */
typedef struct {
    char logical_device_uuid[UI_FIELD_MAX];
    uint64_t snapshot_epoch_ms;
    uint64_t generation_version;
    VomUiDeviceStatus global_status;
    
    vom_ui_hardware_resource_t cluster_aggregate_capacity;
    
    vom_ui_worker_summary_t workers[UI_POOL_WORKERS];
    uint32_t worker_count;
    
    vom_ui_workload_progress_t workloads[UI_POOL_WORKLOADS];
    uint32_t workload_count;
    
    vom_ui_chunk_progress_t chunks[UI_POOL_CHUNKS];
    uint32_t chunk_count;
    
    vom_ui_actionable_alert_t alerts[UI_POOL_ALERTS];
    uint32_t alert_count;
} vom_ui_immutable_snapshot_t;

#endif /* VOM_UI_MODEL_H */
