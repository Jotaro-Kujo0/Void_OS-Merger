/*
 * master/ui_coordinator.h — master-owned logical-device UI planning notes.
 *
 * TODO:
 * - Build a stable view model for logical-device status.
 * - Include worker health and resource contribution.
 * - Include workload/chunk progress and recovery alerts.
 * - Accept user intents such as submit, cancel, drain, resume, and worker UI
 *   mode changes.
 * - Keep rendering and formatting separate from the master control plane.
 */

 #ifndef VOM_MASTER_UI_COORDINATOR_H
 #define VOM_MASTER_UI_COORDINATOR_H

 #include <stdint.h>
 #include <stdbool.h>

 #define UI_NAME_MAX    64
 #define UI_LIMIT_WRK   32
 #define UI_LIMIT_CHK   128

 typedef enum {
    UI_ALERT_NONE,
    UI_ALERT_DEGRADED,
    UI_ALERT_RECOVERING,
    UI_ALERT_FAULT
 } VomUiAlertTier;

typedef enum {
    UI_WRK_OFFLINE,
    UI_WRK_ONLINE,
    UI_WRK_DRAINING
} VomUiWrkState;

typedef enum {
    UI_CHK_PENDING,
    UI_CHK_RUNNING,
    UI_CHK_COMPLETED,
    UI_CHK_FAILED
} VomUiChkState;

typedef enum {
    UI_INTENT_SUBMIT,
    UI_INTENT_CANCEL,
    UI_INTENT_DRAIN,
    UI_INTENT_RESUME,
    UI_INTENT_MODE_SET
} VomUiIntentType;

typedef struct {
    uint32_t id;
    VomUiWrkState state;
    uint64_t capacity_memory_bytes;
    uint64_t allocated_memory_bytes;
    uint32_t capacity_cores;
    uint32_t allocated_cores;
    char capabilities[UI_NAME_MAX];
} vom_ui_worker_snapshot_t;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    VomUiChkState state;
    float progress_pct;
    uint32_t assigned_worker_id;
} vom_ui_chunk_snapshot_t;

typedef struct {
    char logical_device_uuid[UI_NAME_MAX];
    uint64_t master_epoch;
    uint64_t state_generation_version;
    VomUiAlertTier recovery_alert;
    
    uint64_t aggregate_memory_bytes;
    uint64_t allocated_memory_bytes;
    uint32_t aggregate_cpu_cores;
    uint32_t allocated_cpu_cores;
    
    vom_ui_worker_snapshot_t workers[UI_LIMIT_WRK];
    int active_worker_count;
    
    vom_ui_chunk_snapshot_t chunks[UI_LIMIT_CHK];
    int active_chunk_count;
} vom_ui_view_model_t;

typedef struct {
    VomUiIntentType type;
    uint64_t target_id;
    char arguments[UI_NAME_MAX];
} vom_ui_user_intent_t;

typedef struct vom_ui_coordinator vom_ui_coordinator_t;

vom_ui_coordinator_t* vom_ui_coordinator_create(bool raw_ascii_mode);
void vom_ui_coordinator_destroy(vom_ui_coordinator_t *ctx);

void vom_ui_coordinator_synchronize(vom_ui_coordinator_t *ctx, const void *opaque_cluster_snapshot_ptr);
void vom_ui_coordinator_extract_view(const vom_ui_coordinator_t *ctx, vom_ui_view_model_t *out_view);

bool vom_ui_coordinator_translate_intent(vom_ui_coordinator_t *ctx, const vom_ui_user_intent_t *intent, void *out_master_command_payload);

#endif