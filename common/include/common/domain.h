 #ifndef VOM_COMMON_DOMAIN_H
 #define VOM_COMMON_DOMAIN_H

 #include <cstdint>
#include <stdint.h>
 #include <stdbool.h>

 #define DOMAIN_UUID_LEN    40
 #define DOMAIN_NAME_MAX    64
 #define DOMAIN_CHECKSUM_LEN    65

 //global error status stuff

 typedef enum {
    VOM_STATUS_SUCCESS      = 0,
    VOM_STAT_ERR_SYNTAX   = 1,
    VOM_STAT_ERR_UNKNOWN_VERB   =2,
    VOM_STAT_ERR_MISSING_ARG    =3,
    VOM_STAT_ERR_INVALID_VAL  =4,
    VOM_STAT_ERR_NETWORK_FAULT  =5,
    VOM_STATUS_ERR_VALIDATION    = 6,
    VOM_STATUS_ERR_UNSUPPORTED   = 7,
    VOM_STATUS_ERR_CAPACITY      = 8,
    VOM_STATUS_ERR_TIMEOUT       = 9,
    VOM_STATUS_ERR_REJECTED      = 10,
    VOM_STATUS_ERR_INTERNAL      = 11

 } VomStatus;

//workload layer (durable/seperate macro state)

typedef enum {
    VOM_STAT_ERR_VALIDATION    = 6,
    VOM_STAT_ERR_UNSUPPORTED   = 7,
    VOM_STAT_ERR_CAPACITY      = 8,
    VOM_STAT_ERR_TIMEOUT       = 9,
    VOM_STAT_ERR_REJECTED      = 10,
    VOM_STAT_ERR_INTERNAL      = 11
} VomWorkloadState;

typedef struct {
    uint64_t workload_id;
    char name[DOMAIN_NAME_MAX];
    VomWorkloadState state;
    uint64_t total_size_bytes;
    uint32_t deadline_seconds;
    uint64_t start_time_ms;
    char final_aggregated_hash[DOMAIN_CHECKSUM_LEN];
} vom_workload_state;

//chunk layer (dervied sched plan nodes)

typedef enum {
    VOM_CHUNK_PENDING,
    VOM_CHUNK_READY,
    VOM_CHUNK_RUNNING,
    VOM_CHUNK_COMPLETED,
    VOM_CHUNK_FAILED
} VomChunkState;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    VomChunkState state;
    int platform_required;
    int runtime_required;

    //predict resource requests
    uint32_t required_cpu_units;
    uint64_t required_memory_mb;
    bool requires_touch_input;
    uint32_t preffered_data_node;
} vom_chunk_plan;

//worker layer (observation telemetry vs leaes)

typedef enum {
    VOM_WORKER_OFFLINE,
    VOM_WORKER_APPROVED,
    VOM_WORKER_HEALTHY,
    VOM_WORKER_HAELTHY,
    VOM_WORKER_DRAINING,
    VOM_WORKER_DISCONNECTED
} VomWorkerStatus;

//worker surface cpabillities map
typedef struct {
    bool display_attached;
    uint32_t display_width;
    uint32_t display_height;
    bool touch_supported;
    uint32_t ui_mode_token;
} vom_ui_surface;

//dynamic observations input
typedef struct {
    uint64_t last_seen_ms;
    uint32_t cores_online;
    uint64_t ram_free_mb;
    float battery_pct;
    bool is_charging;
    bool is_ac_online;
    uint64_t network_bandwidth_kbps;
    vom_ui_surface peripheral_ui;
} vom_worker_observation;

//auth master decisions output struct
typedef struct {
    uint64_t chunk_id;
    char assigned_worker_id[DOMAIN_NAME_MAX];
    uint64_t lease_expires_ms;
    bool is_active;
} vom_chunk_lease;

//logic device integrated container

typedef struct {
    char cluster_uuid[DOMAIN_UUID_LEN];
    uint64_t master_epoch;
    uint64_t snapshot_version;
    VomWorkerStatus global_status;

    //master resource metric state cache
    uint64_t aggregate_memory_bytes;
    uint64_t allocated_memory_bytes;
    uint32_t aggregate_cpu_units;
    uint32_t allocate_cpu_units;
    uint32_t active_workers_count;
} vom_logical_device_state;

#endif //VOM_COMMON_DOMAIN_H