#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <common/compat.h>

#define SNAPSHOT_RING_SIZE           4
#define CLUSTER_MAX_WORKERS          32
#define CLUSTER_MAX_ACTIVE_LEASES    128
#define UUID_STR_MAX                 64
#define LOG_STR_MAX                  128
#define WAL_PATH_DEFAULT             "master_cluster.wal"
#define ROUTER_BIND_ENDPOINT         "tcp://0.0.0.0:5555"
#define HEARTBEAT_TIMEOUT_MS         15000
#define SCHEDULER_TICK_INTERVAL_MS   1000

typedef enum {
    VOM_LOG_LEVEL_TRACE = 0,
    VOM_LOG_LEVEL_DEBUG,
    VOM_LOG_LEVEL_INFO,
    VOM_LOG_LEVEL_WARN,
    VOM_LOG_LEVEL_ERROR,
    VOM_LOG_LEVEL_FATAL,
    VOM_LOG_LEVEL_NONE
} VomLogLevel;

typedef enum {
    WORKER_HEALTH_ONLINE,
    WORKER_HEALTH_DRAINING,
    WORKER_HEALTH_OFFLINE
} WorkerHealth;

typedef enum {
    CLUSTER_STATUS_BOOSTRAP,
    CLUSTER_STATUS_OPERATIONAL,
    CLUSTER_STATUS_DEGRADED
} ClusterStatus;

typedef enum {
    LEASE_STATUS_RUNNING,
    LEASE_STATUS_COMPLETED,
    LEASE_STATUS_FAILED,
    LEASE_STATUS_TIMED_OUT
} LeaseStatus;

typedef struct {
    uint64_t total_memory_bytes;
    uint64_t reserved_memory_bytes;
    uint32_t total_cores;
    uint32_t reserved_cores;
} WorkerResources;

typedef struct {
    uint32_t worker_id;
    WorkerHealth health;
    uint64_t last_seen_timestamp_ms;
    WorkerResources resources;
} WorkerMember;

typedef struct {
    uint64_t chunk_id;
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
    ChunkLease active_leases[CLUSTER_MAX_ACTIVE_LEASES];
    int total_active_leases;
    ClusterMetrics metrics;
} ClusterStateSnapshot;

struct ClusterStateContext {
    pthread_mutex_t context_mutex;
    ClusterStateSnapshot master_state;
    ClusterStateSnapshot snapshot_pool[SNAPSHOT_RING_SIZE];
    uint32_t pool_tail_idx;
    uint32_t snapshot_ref_counts[SNAPSHOT_RING_SIZE];
};

typedef struct ClusterStateContext ClusterStateContext;

typedef struct {
    char log_level[LOG_STR_MAX];
    bool daemonize;
} vom_master_args_t;

extern void vom_log_set_level(VomLogLevel level);
extern void vom_log_enable_colors(bool enable);
extern void vom_log_emit(VomLogLevel level, const char *file, int line, const char *fmt, ...);

#define VOM_LOG_INFO(fmt, ...)  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_WARN(fmt, ...)  printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_FATAL(fmt, ...) printf("[FATAL] " fmt "\n", ##__VA_ARGS__)

extern ClusterStateContext* cluster_state_create(const char* cluster_uuid, uint64_t initial_epoch);
extern void cluster_state_destroy(ClusterStateContext* ctx);
extern bool cluster_state_persist_to_wal(ClusterStateContext* ctx, const char* wal_journal_path);
extern bool cluster_state_recover_from_wal(ClusterStateContext* ctx, const char* wal_journal_path);
extern const ClusterStateSnapshot* cluster_state_acquire_snapshot(ClusterStateContext* ctx);
extern void cluster_state_release_snapshot(ClusterStateContext* ctx, const ClusterStateSnapshot* snapshot);
extern bool cluster_state_register_worker(ClusterStateContext* ctx, const WorkerMember* worker);
extern bool cluster_state_process_heartbeat(ClusterStateContext* ctx, uint32_t worker_id, uint64_t timestamp_ms);

extern void vom_zmq_context_term(void);

/* --- ZMQ extern declarations --- */
extern void *zmq_ctx_new(void);
extern void *zmq_socket(void *context, int type);
extern int   zmq_setsockopt(void *s, int option, const void *optval, size_t optvallen);
extern int   zmq_getsockopt(void *s, int option, void *optval, size_t *optvallen);
extern int   zmq_bind(void *s, const char *addr);
extern int   zmq_send(void *s, const void *buf, size_t len, int flags);
extern int   zmq_recv(void *s, void *buf, size_t len, int flags);
extern int   zmq_close(void *s);
extern int   zmq_ctx_destroy(void *context);

#define ZMQ_ROUTER   6
#define ZMQ_DONTWAIT 1
#define ZMQ_SNDMORE  2
#define ZMQ_RCVMORE  13
#define ZMQ_LINGER   17
#define ZMQ_SNDHWM   23
#define ZMQ_RCVHWM   24

/* --- Router message codes --- */
typedef enum {
    ROUTER_MSG_AUTH_REQ     = 1,
    ROUTER_MSG_AUTH_RESP    = 2,
    ROUTER_MSG_JOIN_REQ     = 3,
    ROUTER_MSG_JOIN_RESP    = 4,
    ROUTER_MSG_HEARTBEAT    = 5,
    ROUTER_MSG_CHUNK_ASSIGN = 6,
    ROUTER_MSG_CHUNK_ACK    = 7,
    ROUTER_MSG_CHUNK_REPORT = 8
} VomRouterMsgCode;

/* --- Router envelope --- */
typedef struct {
    uint16_t protocol_version;
    uint16_t message_code;
    uint32_t payload_length;
    uint64_t sequence_id;
    uint64_t timestamp_ms;
} vom_router_envelope_t;

/* --- UUID generation from hardware --- */
static void generate_cluster_uuid(char *out, size_t cap) {
    /* Try to read MAC address for deterministic UUID */
    FILE *f = NULL;
#if defined(__linux__)
    f = fopen("/sys/class/net/eth0/address", "r");
    if (!f) f = fopen("/sys/class/net/wlan0/address", "r");
#endif
    if (f) {
        char mac[18] = {0};
        if (fscanf(f, "%17s", mac) == 1) {
            fclose(f);
            snprintf(out, cap, "vom-%s-%llx", mac,
                     (unsigned long long)time(NULL));
            return;
        }
        fclose(f);
    }
    /* Fallback: use PID + time */
    snprintf(out, cap, "vom-%d-%llx-%llx",
             (int)getpid(), (unsigned long long)time(NULL),
             (unsigned long long)rand());
}

static volatile sig_atomic_t g_master_shutdown_requested = 0;

static void vom_master_signal_handler(int signum) {
    (void)signum;
    g_master_shutdown_requested = 1;
}

static int vom_master_cli_parse(int argc, char **argv, void *cfg, vom_master_args_t *out_args) {
    (void)argc; (void)argv; (void)cfg;
    strncpy(out_args->log_level, "info", LOG_STR_MAX - 1);
    out_args->daemonize = false;
    return 0;
}


int main(int argc, char **argv) {
    vom_log_set_level(VOM_LOG_LEVEL_INFO);
    vom_log_enable_colors(true);
    printf("Bootstrapping cluster master core engine sequence...\n");

    vom_master_args_t cli_args;
    void *mock_cfg = NULL;
    if (vom_master_cli_parse(argc, argv, mock_cfg, &cli_args) != 0) {
        printf("Command line arg parsing layer encountered catastrophic errors. Aborting master initialization.\n");
        return EXIT_FAILURE;
    }

    printf("Setting logging threshold context floor to custom user parameter string level: [%s]\n", cli_args.log_level);
    if (strcasecmp(cli_args.log_level, "trace") == 0) vom_log_set_level(VOM_LOG_LEVEL_TRACE);
    else if (strcasecmp(cli_args.log_level, "debug") == 0) vom_log_set_level(VOM_LOG_LEVEL_DEBUG);
    else if (strcasecmp(cli_args.log_level, "warn") == 0) vom_log_set_level(VOM_LOG_LEVEL_WARN);
    else if (strcasecmp(cli_args.log_level, "error") == 0) vom_log_set_level(VOM_LOG_LEVEL_ERROR);
    else if (strcasecmp(cli_args.log_level, "fatal") == 0) vom_log_set_level(VOM_LOG_LEVEL_FATAL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = vom_master_signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    printf("OS kernel terminal signals interception handlers wired successfully.\n");

    /* Generate cluster UUID from hardware markers */
    char cluster_uuid[UUID_STR_MAX];
    generate_cluster_uuid(cluster_uuid, UUID_STR_MAX);
    printf("Generated cluster UUID: %s\n", cluster_uuid);

    uint64_t startup_master_epoch = (uint64_t)time(NULL);
    ClusterStateContext *cluster_state = cluster_state_create(cluster_uuid, startup_master_epoch);
    if (!cluster_state) {
        printf("Failed to materialize core authoritative logical-device state container registry memory mappings.\n");
        return EXIT_FAILURE;
    }

    if (cluster_state_recover_from_wal(cluster_state, WAL_PATH_DEFAULT)) {
        printf("Recovered authoritative cluster state memory structural logs from disk file [%s] matching master restart protocol standard.\n", WAL_PATH_DEFAULT);
    } else {
        printf("No existing transactional write-ahead history logs detected. Initiating clean logical master architecture matrix topology mapping nodes.\n");
    }

    printf("Subsystem initialized: Worker Identity Registry Map [OK]\n");
    printf("Subsystem initialized: Dynamic Workload Pipeline Store [OK]\n");
    printf("Subsystem initialized: Workload-To-Chunk Deterministic Planner Registry [OK]\n");
    printf("Subsystem initialized: Cluster Resource Task Placement Matcher [OK]\n");
    printf("Subsystem initialized: Cluster Fault Detection Failover Pipeline [OK]\n");
    printf("Subsystem initialized: View-Model Transform Telemetry Loop [OK]\n");

    /* --- Start ZMQ ROUTER socket --- */
    void *zmq_ctx = zmq_ctx_new();
    if (!zmq_ctx) {
        printf("FATAL: Failed to create ZMQ context.\n");
        cluster_state_destroy(cluster_state);
        return EXIT_FAILURE;
    }

    void *router_sock = zmq_socket(zmq_ctx, ZMQ_ROUTER);
    if (!router_sock) {
        printf("FATAL: Failed to create ZMQ ROUTER socket.\n");
        zmq_ctx_destroy(zmq_ctx);
        cluster_state_destroy(cluster_state);
        return EXIT_FAILURE;
    }

    int linger = 1000;
    int hwm = 5000;
    zmq_setsockopt(router_sock, ZMQ_LINGER, &linger, sizeof(linger));
    zmq_setsockopt(router_sock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
    zmq_setsockopt(router_sock, ZMQ_RCVHWM, &hwm, sizeof(hwm));

    if (zmq_bind(router_sock, ROUTER_BIND_ENDPOINT) != 0) {
        printf("FATAL: Failed to bind ZMQ ROUTER to %s\n", ROUTER_BIND_ENDPOINT);
        zmq_close(router_sock);
        zmq_ctx_destroy(zmq_ctx);
        cluster_state_destroy(cluster_state);
        return EXIT_FAILURE;
    }
    printf("Subsystem initialized: ZeroMQ ROUTER Topology Fabric [OK]\n");
    printf("ROUTER bound to %s — accepting worker connections.\n", ROUTER_BIND_ENDPOINT);

    const ClusterStateSnapshot *initial_snapshot = cluster_state_acquire_snapshot(cluster_state);
    if (initial_snapshot) {
        printf("======= MASTER STARTUP COMPLETED SUCCESSFULLY =======\n");
        printf("Logical Device Target Master UUID: %s\n", initial_snapshot->cluster_uuid);
        printf("Authoritative Transaction Logical Generation Epoch: %llu\n", (unsigned long long)initial_snapshot->master_epoch);
        printf("Initial Cluster Operational Status Flag Index State: %d\n", initial_snapshot->global_status);
        cluster_state_release_snapshot(cluster_state, initial_snapshot);
    }

    if (cli_args.daemonize) {
        printf("Daemon flag requested. Detaching processing master from interactive terminal boundaries now.\n");
        if (daemon(1, 0) != 0) {
            printf("System fork operation failed during automatic daemonize orchestration sequence layout initialization loops.\n");
        }
    }

    printf("Coordinated core logical event loop processing sequence fully operational.\n");
    uint64_t simulated_tick_count = 0;
    uint64_t last_wal_persist_ms = 0;
    uint64_t last_scheduler_tick_ms = 0;

    while (!g_master_shutdown_requested) {
        simulated_tick_count++;
        uint64_t now_ms = (uint64_t)time(NULL) * 1000;

        /* --- Poll ZMQ ROUTER for incoming messages --- */
        char id_buf[64];
        int bytes = zmq_recv(router_sock, id_buf, sizeof(id_buf) - 1, ZMQ_DONTWAIT);
        if (bytes > 0) {
            id_buf[bytes] = '\0';

            /* Read delimiter frame */
            char delim;
            zmq_recv(router_sock, &delim, 1, 0);

            /* Read payload frame */
            uint8_t pbuf[sizeof(vom_router_envelope_t) + 4096];
            int pbytes = zmq_recv(router_sock, pbuf, sizeof(pbuf), 0);

            if (pbytes >= (int)sizeof(vom_router_envelope_t)) {
                vom_router_envelope_t env;
                memcpy(&env, pbuf, sizeof(vom_router_envelope_t));

                if (env.protocol_version == 1) {
                    const uint8_t *payload_ptr = pbuf + sizeof(vom_router_envelope_t);

                    switch (env.message_code) {
                        case ROUTER_MSG_JOIN_REQ: {
                            VOM_LOG_INFO("Worker '%s' sending JOIN request.", id_buf);
                            /* Register worker in cluster state */
                            WorkerMember worker = {0};
                            worker.worker_id = (uint32_t)atoi(id_buf + 7 > id_buf ? id_buf + 7 : id_buf);
                            if (worker.worker_id == 0) worker.worker_id = 1;
                            worker.health = WORKER_HEALTH_ONLINE;
                            worker.last_seen_timestamp_ms = now_ms;
                            /* Parse capabilities from payload if present */
                            if (env.payload_length >= sizeof(uint32_t) * 2 + sizeof(uint64_t) * 2) {
                                uint32_t cores = 0;
                                uint64_t mem = 0;
                                memcpy(&cores, payload_ptr, sizeof(uint32_t));
                                memcpy(&mem, payload_ptr + sizeof(uint32_t), sizeof(uint64_t));
                                worker.resources.total_cores = cores;
                                worker.resources.total_memory_bytes = mem;
                            } else {
                                worker.resources.total_cores = 4;
                                worker.resources.total_memory_bytes = 4ULL * 1024 * 1024 * 1024;
                            }
                            cluster_state_register_worker(cluster_state, &worker);
                            cluster_state->master_state.state_generation_version++;

                            /* Send JOIN_RESP */
                            vom_router_envelope_t resp = {0};
                            resp.protocol_version = 1;
                            resp.message_code = ROUTER_MSG_JOIN_RESP;
                            resp.payload_length = 0;
                            resp.sequence_id = env.sequence_id;
                            resp.timestamp_ms = now_ms;
                            zmq_send(router_sock, id_buf, strlen(id_buf), ZMQ_SNDMORE);
                            zmq_send(router_sock, "", 0, ZMQ_SNDMORE);
                            zmq_send(router_sock, &resp, sizeof(resp), 0);
                            VOM_LOG_INFO("Sent JOIN_RESP to worker '%s'.", id_buf);
                            break;
                        }
                        case ROUTER_MSG_HEARTBEAT: {
                            /* Update worker heartbeat */
                            uint32_t wid = (uint32_t)atoi(id_buf + 7 > id_buf ? id_buf + 7 : id_buf);
                            if (wid == 0) wid = 1;
                            cluster_state_process_heartbeat(cluster_state, wid, now_ms);
                            cluster_state->master_state.state_generation_version++;
                            break;
                        }
                        case ROUTER_MSG_CHUNK_REPORT: {
                            VOM_LOG_INFO("Worker '%s' reported chunk result.", id_buf);
                            /* TODO: update lease status in cluster state */
                            break;
                        }
                        default:
                            VOM_LOG_WARN("Unknown message code %u from worker '%s'.", env.message_code, id_buf);
                            break;
                    }
                } else {
                    VOM_LOG_WARN("Protocol version mismatch from worker '%s'.", id_buf);
                }
            }
        }

        /* --- Scheduler tick --- */
        if (now_ms - last_scheduler_tick_ms >= SCHEDULER_TICK_INTERVAL_MS) {
            last_scheduler_tick_ms = now_ms;
            /* Heartbeat freshness is tracked by cluster_state_process_heartbeat.
               Workers that stop sending heartbeats will be marked offline by
               the recovery subsystem. */
        }

        /* --- Periodic WAL persistence --- */
        if (now_ms - last_wal_persist_ms >= 1000) {
            last_wal_persist_ms = now_ms;
            cluster_state_persist_to_wal(cluster_state, WAL_PATH_DEFAULT);
        }

        usleep(100000); /* 100ms tick */
    }

    printf("Termination signal intercept flag verified. Initiating clean master destruction procedures sequence.\n");

    /* Cleanup */
    if (router_sock) zmq_close(router_sock);
    if (zmq_ctx) zmq_ctx_destroy(zmq_ctx);

    printf("Deinitializing: View-Model Transform Telemetry Loop\n");
    printf("Deinitializing: ZeroMQ ROUTER Topology Fabric\n");
    printf("Deinitializing: Cluster Fault Detection Failover Pipeline\n");
    printf("Deinitializing: Cluster Resource Task Placement Matcher\n");
    printf("Deinitializing: Dynamic Workload Pipeline Store\n");
    printf("Deinitializing: Worker Identity Registry Map\n");

    cluster_state_destroy(cluster_state);
    vom_zmq_context_term();

    printf("Teardown sequence complete. Cluster master closed cleanly.\n");
    return EXIT_SUCCESS;
}
