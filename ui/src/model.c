#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define UI_MAX_NODES            64
#define UI_MAX_ACTIVE_TASKS     256
#define UI_MAX_ACTIVE_LEASES    256
#define UI_SHORT_STRING_LEN     32

#define CLUSTER_MAX_WORKERS          32
#define CLUSTER_MAX_ACTIVE_LEASES_M  128
#define UUID_STR_MAX                 64
#define HOSTNAME_STR_MAX             64

typedef enum {
    WORKER_HEALTH_ONLINE,
    WORKER_HEALTH_DRAINING,
    WORKER_HEALTH_OFFLINE,
    WORKER_HEALTH_SUSPECT
} WorkerHealth;

typedef enum {
    CLUSTER_STATUS_BOOSTRAP,
    CLUSTER_STATUS_OPERATIONAL,
    CLUSTER_STATUS_DEGRADED,
    CLUSTER_STATUS_MAINTENANCE,
    CLUSTER_STATUS_PANIC
} ClusterStatus;

typedef enum {
    LEASE_STATUS_IDLE,
    LEASE_STATUS_ASSIGNED,
    LEASE_STATUS_EXECUTING,
    LEASE_STATUS_COMMITTED,
    LEASE_STATUS_TIMED_OUT,
    LEASE_STATUS_FAILED
} LeaseStatus;

typedef struct {
    uint64_t total_memory_bytes;
    uint64_t reserved_memory_bytes;
    uint32_t total_cores;
    uint32_t reserved_cores;
} WorkerResources;

typedef struct {
    uint32_t worker_id;
    char hostname[HOSTNAME_STR_MAX];
    WorkerHealth health;
    uint64_t last_heartbeat_monotonic_ms;
    WorkerResources resources;
} WorkerMember;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    uint32_t assigned_worker_id;
    LeaseStatus status;
    float progress_percentage;
    uint32_t failure_retry_count;
} ChunkLease;

typedef struct {
    uint64_t cluster_aggregate_memory_bytes;
    uint64_t cluster_allocated_memory_bytes;
    uint32_t cluster_aggregate_cores;
    uint32_t cluster_allocated_cores;
    uint32_t active_worker_count;
} ClusterMetrics;

typedef struct {
    char cluster_uuid[UUID_STR_MAX];
    uint64_t master_epoch;
    uint64_t state_generation_version;
    ClusterStatus global_status;
    WorkerMember active_workers[CLUSTER_MAX_WORKERS];
    int total_registered_workers;
    ChunkLease active_leases[CLUSTER_MAX_ACTIVE_LEASES_M];
    int total_active_leases;
    ClusterMetrics metrics;
} ClusterStateSnapshot;

typedef enum {
    UI_HEALTH_UNKNOWN,
    UI_HEALTH_OK,
    UI_HEALTH_WARN,
    UI_HEALTH_CRIT
} UiHealthStatus;

typedef struct {
    uint32_t node_id;
    char display_name[UI_SHORT_STRING_LEN];
    UiHealthStatus health;
    double memory_utilization_pct;
    double cpu_utilization_pct;
    bool is_draining;
    uint64_t ms_since_last_heartbeat;
} UiNodeRecord;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    uint32_t assigned_node_id;
    char status_text[UI_SHORT_STRING_LEN];
    float progress_normalized;
    bool requires_attention;
} UiTaskRecord;

typedef struct {
    char device_uuid[64];
    uint64_t logical_epoch;
    uint64_t last_version_seen;
    UiHealthStatus summary_health;
    char operational_status_text[UI_SHORT_STRING_LEN];
    double cluster_total_memory_gb;
    double cluster_used_memory_gb;
    double cluster_overall_cpu_pct;
    UiNodeRecord nodes[UI_MAX_NODES];
    int node_count;
    UiTaskRecord tasks[UI_MAX_ACTIVE_TASKS];
    int task_count;
} UiLogicalDeviceViewModel;

void ui_model_transform_snapshot(const ClusterStateSnapshot* snapshot, uint64_t current_monotonic_ms, UiLogicalDeviceViewModel* out_model) {
    if (!snapshot || !out_model) return;

    memset(out_model, 0, sizeof(UiLogicalDeviceViewModel));

    strncpy(out_model->device_uuid, snapshot->cluster_uuid, sizeof(out_model->device_uuid) - 1);
    out_model->logical_epoch = snapshot->master_epoch;
    out_model->last_version_seen = snapshot->state_generation_version;

    switch (snapshot->global_status) {
        case CLUSTER_STATUS_BOOSTRAP:
            out_model->summary_health = UI_HEALTH_WARN;
            strncpy(out_model->operational_status_text, "RECOVERING/BOOT", UI_SHORT_STRING_LEN - 1);
            break;
        case CLUSTER_STATUS_OPERATIONAL:
            out_model->summary_health = UI_HEALTH_OK;
            strncpy(out_model->operational_status_text, "HEALTHY", UI_SHORT_STRING_LEN - 1);
            break;
        case CLUSTER_STATUS_DEGRADED:
            out_model->summary_health = UI_HEALTH_WARN;
            strncpy(out_model->operational_status_text, "DEGRADED", UI_SHORT_STRING_LEN - 1);
            break;
        case CLUSTER_STATUS_MAINTENANCE:
            out_model->summary_health = UI_HEALTH_UNKNOWN;
            strncpy(out_model->operational_status_text, "MAINTENANCE", UI_SHORT_STRING_LEN - 1);
            break;
        case CLUSTER_STATUS_PANIC:
        default:
            out_model->summary_health = UI_HEALTH_CRIT;
            strncpy(out_model->operational_status_text, "CRITICAL FAILURE", UI_SHORT_STRING_LEN - 1);
            break;
    }

    const double gb = 1024.0 * 1024.0 * 1024.0;
    out_model->cluster_total_memory_gb = (double)snapshot->metrics.cluster_aggregate_memory_bytes / gb;
    out_model->cluster_used_memory_gb = (double)snapshot->metrics.cluster_allocated_memory_bytes / gb;

    if (snapshot->metrics.cluster_aggregate_cores > 0) {
        out_model->cluster_overall_cpu_pct = ((double)snapshot->metrics.cluster_allocated_cores / (double)snapshot->metrics.cluster_aggregate_cores) * 100.0;
    }

    int limit_nodes = (snapshot->total_registered_workers < UI_MAX_NODES) ? snapshot->total_registered_workers : UI_MAX_NODES;
    for (int i = 0; i < limit_nodes; i++) {
        const WorkerMember* src = &snapshot->active_workers[i];
        UiNodeRecord* dest = &out_model->nodes[out_model->node_count];

        dest->node_id = src->worker_id;
        strncpy(dest->display_name, src->hostname, UI_SHORT_STRING_LEN - 1);
        dest->is_draining = (src->health == WORKER_HEALTH_DRAINING);
        dest->ms_since_last_heartbeat = (current_monotonic_ms >= src->last_heartbeat_monotonic_ms)
            ? current_monotonic_ms - src->last_heartbeat_monotonic_ms : 0;

        switch (src->health) {
            case WORKER_HEALTH_ONLINE:     dest->health = UI_HEALTH_OK; break;
            case WORKER_HEALTH_SUSPECT:    dest->health = UI_HEALTH_WARN; break;
            case WORKER_HEALTH_OFFLINE:    dest->health = UI_HEALTH_CRIT; break;
            case WORKER_HEALTH_DRAINING:   dest->health = UI_HEALTH_WARN; break;
            default:                       dest->health = UI_HEALTH_UNKNOWN; break;
        }

        if (src->resources.total_memory_bytes > 0) {
            dest->memory_utilization_pct = ((double)src->resources.reserved_memory_bytes / (double)src->resources.total_memory_bytes) * 100.0;
        }
        if (src->resources.total_cores > 0) {
            dest->cpu_utilization_pct = ((double)src->resources.reserved_cores / (double)src->resources.total_cores) * 100.0;
        }

        out_model->node_count++;
    }

    int limit_leases = (snapshot->total_active_leases < UI_MAX_ACTIVE_LEASES) ? snapshot->total_active_leases : UI_MAX_ACTIVE_LEASES;
    for (int i = 0; i < limit_leases; i++) {
        if (out_model->task_count >= UI_MAX_ACTIVE_TASKS) break;

        const ChunkLease* src = &snapshot->active_leases[i];
        UiTaskRecord* dest = &out_model->tasks[out_model->task_count];

        dest->chunk_id = src->chunk_id;
        dest->workload_id = src->workload_id;
        dest->assigned_node_id = src->assigned_worker_id;
        dest->progress_normalized = src->progress_percentage / 100.0f;
        dest->requires_attention = (src->failure_retry_count >= 3);

        switch (src->status) {
            case LEASE_STATUS_IDLE:       strncpy(dest->status_text, "QUEUED", UI_SHORT_STRING_LEN - 1); break;
            case LEASE_STATUS_ASSIGNED:   strncpy(dest->status_text, "DISPATCHED", UI_SHORT_STRING_LEN - 1); break;
            case LEASE_STATUS_EXECUTING:  strncpy(dest->status_text, "RUNNING", UI_SHORT_STRING_LEN - 1); break;
            case LEASE_STATUS_COMMITTED:  strncpy(dest->status_text, "FINISHED", UI_SHORT_STRING_LEN - 1); break;
            case LEASE_STATUS_TIMED_OUT:  strncpy(dest->status_text, "TIMEOUT", UI_SHORT_STRING_LEN - 1); break;
            case LEASE_STATUS_FAILED:     strncpy(dest->status_text, "FAILED", UI_SHORT_STRING_LEN - 1); break;
            default:                      strncpy(dest->status_text, "UNKNOWN", UI_SHORT_STRING_LEN - 1); break;
        }

        out_model->task_count++;
    }
}

void run_ui_model_test_suite(void) {
    printf("--- RUNNING VIEW-MODEL TRANSFORMATION ENGINE TESTS ---\n\n");

    const uint64_t dummy_now = 5000;
    UiLogicalDeviceViewModel view_model;

    printf("[TEST 1] Processing Empty/Bootstrap Cluster Snapshot...\n");
    ClusterStateSnapshot empty_snap;
    memset(&empty_snap, 0, sizeof(ClusterStateSnapshot));
    strcpy(empty_snap.cluster_uuid, "9999-ABCD");
    empty_snap.global_status = CLUSTER_STATUS_BOOSTRAP;
    empty_snap.state_generation_version = 42;

    ui_model_transform_snapshot(&empty_snap, dummy_now, &view_model);
    printf("Verify -> Health Token Enum: %d (Expected: %d for Warn)\n", view_model.summary_health, UI_HEALTH_WARN);
    printf("Verify -> Output String: '%s'\n", view_model.operational_status_text);
    printf("Verify -> Memory Size: %.1f GB | Node Entries Found: %d\n\n", view_model.cluster_total_memory_gb, view_model.node_count);

    printf("[TEST 2] Processing Active Operational Cluster Snapshot...\n");
    ClusterStateSnapshot active_snap;
    memset(&active_snap, 0, sizeof(ClusterStateSnapshot));
    strcpy(active_snap.cluster_uuid, "NODE-CLUSTER-ONE");
    active_snap.global_status = CLUSTER_STATUS_OPERATIONAL;
    active_snap.state_generation_version = 105;
    active_snap.master_epoch = 1234567;

    active_snap.metrics.cluster_aggregate_memory_bytes = 16ULL * 1024 * 1024 * 1024;
    active_snap.metrics.cluster_allocated_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    active_snap.metrics.cluster_aggregate_cores = 8;
    active_snap.metrics.cluster_allocated_cores = 2;

    active_snap.total_registered_workers = 1;
    WorkerMember* w = &active_snap.active_workers[0];
    w->worker_id = 101;
    strcpy(w->hostname, "worker-alpha");
    w->health = WORKER_HEALTH_ONLINE;
    w->last_heartbeat_monotonic_ms = 4500;
    w->resources.total_memory_bytes = 16ULL * 1024 * 1024 * 1024;
    w->resources.reserved_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    w->resources.total_cores = 8;
    w->resources.reserved_cores = 2;
    active_snap.total_active_leases = 1;
    ChunkLease* l = &active_snap.active_leases[0];
    l->chunk_id = 5001;
    l->workload_id = 9001;
    l->assigned_worker_id = 101;
    l->status = LEASE_STATUS_EXECUTING;
    l->progress_percentage = 75.5f;
    l->failure_retry_count = 0;
    ui_model_transform_snapshot(&active_snap, dummy_now, &view_model);
    printf("Verify -> Health Token Enum: %d (Expected: %d for OK)\n", view_model.summary_health, UI_HEALTH_OK);
    printf("Verify -> Operational Status Text: '%s'\n", view_model.operational_status_text);
    printf("Verify -> Cluster Memory Total: %.1f GB | Used: %.1f GB\n", view_model.cluster_total_memory_gb, view_model.cluster_used_memory_gb);
    printf("Verify -> Overall Cluster CPU: %.1f%%\n", view_model.cluster_overall_cpu_pct);
    printf("Verify -> Node Count: %d | Node Hostname: '%s' | Monotonic Heartbeat Lag: %llu ms\n",view_model.node_count, view_model.nodes[0].display_name, (unsigned long long)view_model.nodes[0].ms_since_last_heartbeat);
    printf("Verify -> Task Count: %d | Status Text: '%s' | Progress Normalized: %.3f\n",view_model.task_count, view_model.tasks[0].status_text, view_model.tasks[0].progress_normalized);
    }
    int main(void) {run_ui_model_test_suite();return 0;
}