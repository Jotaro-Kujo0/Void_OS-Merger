 #ifndef MASTER_CLUSTER_STATE_H
 #define MASTER_CLUSTER_STATE_H

 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 //System structural limits

 #define CLUSTER_MAX_WORKERS 64
 #define CLUSTER_MAX_ACTIVE_LEASES  1024
 #define CLUSTER_MAX_CAPABILITIES   16
 #define CLUSTER_NAME_MAX_LEN   64

 //enumerations and state machines

 typedef enum {
    CLUSTER_STATUS_BOOSTRAP,
    CLUSTER_STATUS_OPERATIONAL,
    CLUSTER_STATUS_DEGRADED,
    CLUSTER_STATUS_MAINTENANCE,
    CLUSTER_STATUS_PANIC
 }  ClusterStatus;

 typedef enum {
    WORKER_HEALTH_OFFLINE,
    WORKER_HEALTH_SUSPECT,
    WORKER_HEALTH_ONLINE,
    WORKER_HEALTH_DRAINING
} WorkerHealth;

typedef enum {
    LEASE_STATUS_IDLE,
    LEASE_STATUS_ASSIGNED,
    LEASE_STATUS_EXECUTING,
    LEASE_STATUS_COMMITTED,
    LEASE_STATUS_TIMED_OUT,
    LEASE_STATUS_FAILED
} LeaseStatus;

// subcomponents and accounting
//so this part is to track hardware capacity and allocations for scheduling.

typedef struct {
    uint32_t total_cores;
    uint32_t reserved_cores;
    uint64_t total_memory_bytes;
    uint64_t reserved_memory_bytes;
    uint64_t total_storage_bytes;
    uint64_t reserved_storage_bytes;
} ResourceMetrics;

//Worker UI surface definitions for remote diagnostics.

typedef struct {
    char panel_endpoint_url[128];
    uint32_t telemetry_port;
    bool web_Surface_enabled;
} WorkerUiSurface;

//models a single execution target worker and hardware inventory capabilities.

typedef struct{
    uint32_t worker_id;
    char hostname[CLUSTER_NAME_MAX_LEN];
    WorkerHealth health;
    uint64_t last_heartbeat_monotonic_ms;

    //HArdware capabilites and constraints part
    ResourceMetrics resources;
    char capabilites[CLUSTER_MAX_CAPABILITIES][32];
    int capability_count;

    WorkerUiSurface ui_surface;
} WorkerMember;

//workload chunk lease life cycle state tracker
typedef struct{
    uint64_t chunk_id;
    uint64_t workload_id;
    uint32_t assigned_worker_id;
    LeaseStatus status;
    uint64_t lease_expires_timestamp_ms;
    float progress_percentage;
    uint32_t failure_retry_count;
} ChunkLease;

//summed operational metrics
typedef struct{
    uint64_t cluster_aggregate_memory_bytes;
    uint64_t cluster_allocated_memory_bytes;
    uint32_t cluster_aggregate_cores;
    uint32_t cluster_allocated_cores;
    uint32_t active_worker_count;
    uint32_t standart_pending_chunks;
} AggregateCapacity;

//MAster State Container
//structure containing private tracking details

typedef struct ClusterStateContext ClusterStateContext;

//immuteable layout container
//read-only snapshots of this type are provided to external comps.
typedef struct {
    char cluster_uuid[64];
    uint64_t master_epoch;
    uint64_t state_generation_version;
    ClusterStatus global_status;

    //worker registry
    WorkerMember active_workers[CLUSTER_MAX_WORKERS];
    int total_registered_workers;

    //worklaod chunk lease and pipeline
    ChunkLease active_leases[CLUSTER_MAX_ACTIVE_LEASES];
    int total_active_leases;

    AggregateCapacity metrics;
} ClusterStateSnapshot;

//System Transaction management ınterface
//boots authorative cluster state tracking objects

ClusterStateContext* cluster_state_create(const char* cluster_uuid, uint64_t initial_epoch);

//shuts down tracking loops
void cluster_state_destroy(ClusterStateContext* ctx);
bool cluster_state_persist_to_wal(ClusterStateContext* ctx, const char* wal_journal_path);
//this part is to restore state structures from disk
bool cluster_state_recover_from_wal(ClusterStateContext* ctx, const char* wal_journal_path);

//Snapshot pipeline
//obtains immutable, configuration snapshots
const ClusterStateSnapshot* cluster_state_acquire_snapshot(ClusterStateContext* ctx);
//read handles for specific snapshots, releases associated memory
void cluster_state_release_snapshot(ClusterStateContext* ctx, const ClusterStateSnapshot* snapshot);
//registers optional compute entity mode.
bool cluster_state_register_worker(ClusterStateContext* ctx, const WorkerMember* worker);
//update execution metrics for active chunk leases
bool cluster_state_update_lease(ClusterStateContext* ctx, uint64_t chunk_id, LeaseStatus new_status, float progress);
//Trgiggers heart-beat observation, recalc global health
bool cluster_state_process_heartbeat(ClusterStateContext* ctx, uint32_t worker_id, uint64_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif

