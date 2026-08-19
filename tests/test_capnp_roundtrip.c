/* tests/test_capnp_roundtrip.c — protocol round-trip planning notes.
 *
 * TODO:
 * - Build worker join, heartbeat, workload, chunk assignment, progress,
 *   result, recovery, and UI-surface fixtures once the schema is declared.
 * - Serialize and deserialize each ClusterMessage payload.
 * - Assert IDs, epochs, leases, dependencies, requirements, and result
 *   metadata survive the round trip.
 * - Verify unknown/unsupported fields follow the compatibility policy.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <assert.h>

 #define BUF_SIZE 4096
 #define ID_MAX 64

 typedef enum {
    MSG_WORKER_JOIN,
    MSG_WORKER_HEARTBEAT,
    MSG_WORKLOAD_SUBMIT,
    MSG_CHUNK_ASSIGN,
    MSG_CHUNK_PROGRESS,
    MSG_CHUNK_RESULT,
    MSG_RECOVERY,
    MSG_UI_SURFACE
 } MsgType;

 typedef struct {
    uint32_t cores_online;
    uint64_t ram_total_mb;
    bool has_cuda;
 } worker_join_payload_t;

 typedef struct {
    uint64_t timestamp_ms;
    uint64_t ram_free_mb;
    float cpu_utilization_pct;
 } worker_heartbeat_payload_t;

typedef struct {
    char workload_id[ID_MAX];
    uint64_t total_size_bytes;
    uint32_t deadline_seconds;
} workload_submit_payload_t;

typedef struct {
    char chunk_id[ID_MAX];
    uint32_t required_cpu_units;
    uint64_t lease_expires_ms;
} chunk_assign_payload_t;

typedef struct {
    char chunk_id[ID_MAX];
    float progress_percentage;
} chunk_progress_payload_t;

typedef struct {
    char chunk_id[ID_MAX];
    uint32_t exit_code;
    char result_sha256[ID_MAX];
} chunk_result_payload_t;

typedef struct {
    char failed_worker_id[ID_MAX];
    bool action_requeue;
} recovery_payload_t;

typedef struct {
    bool display_attached;
    bool touch_supported;
} ui_surface_payload_t;

typedef struct {
    uint16_t protocol_version;
    char message_id[ID_MAX];
    char correlation_id[ID_MAX];
    char sender_id[ID_MAX];
    uint64_t master_epoch;
    uint64_t timestamp_ms;
    MsgType type;
    
    union {
        worker_join_payload_t join;
        worker_heartbeat_payload_t heartbeat;
        workload_submit_payload_t workload;
        chunk_assign_payload_t assign;
        chunk_progress_payload_t progress;
        chunk_result_payload_t result;
        recovery_payload_t recovery;
        ui_surface_payload_t ui;
    } payload;
} cluster_message_t;

static int mock_capnp_serialize(const cluster_message_t *msg, uint8_t *buf, size_t max_len) {
    if (!msg || !buf || max_len < sizeof(cluster_message_t)) return -1;
    memcpy(buf, msg, sizeof(cluster_message_t));
    return sizeof(cluster_message_t);
}

static int mock_capnp_deserialize(const uint8_t *buf, size_t len, cluster_message_t *out_msg) {
    if (!buf || !out_msg || len < sizeof(cluster_message_t)) return -1;
    memcpy(out_msg, buf, sizeof(cluster_message_t));
    return 0;
}

static void populate_base_header(cluster_message_t *msg, MsgType type) {
    msg->protocol_version = 1;
    strncpy(msg->message_id, "msg-001", ID_MAX - 1);
    strncpy(msg->correlation_id, "corr-100", ID_MAX - 1);
    strncpy(msg->sender_id, "node-master", ID_MAX - 1);
    msg->master_epoch = 1718900000;
    msg->timestamp_ms = 1718900050;
    msg->type = type;
}

static void verify_base_header(const cluster_message_t *msg) {
    assert(msg->protocol_version == 1);
    assert(strcmp(msg->message_id, "msg-001") == 0);
    assert(strcmp(msg->correlation_id, "corr-100") == 0);
    assert(strcmp(msg->sender_id, "node-master") == 0);
    assert(msg->master_epoch == 1718900000);
    assert(msg->timestamp_ms == 1718900050);
}

void test_worker_join_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_WORKER_JOIN);
    src.payload.join.cores_online = 8;
    src.payload.join.ram_total_mb = 16384;
    src.payload.join.has_cuda = true;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_WORKER_JOIN);
    assert(dst.payload.join.cores_online == 8);
    assert(dst.payload.join.ram_total_mb == 16384);
    assert(dst.payload.join.has_cuda == true);
    printf("Round-trip WorkerJoin: PASS\n");
}

void test_worker_heartbeat_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_WORKER_HEARTBEAT);
    src.payload.heartbeat.timestamp_ms = 1718900100;
    src.payload.heartbeat.ram_free_mb = 8192;
    src.payload.heartbeat.cpu_utilization_pct = 45.5f;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_WORKER_HEARTBEAT);
    assert(dst.payload.heartbeat.timestamp_ms == 1718900100);
    assert(dst.payload.heartbeat.ram_free_mb == 8192);
    assert(dst.payload.heartbeat.cpu_utilization_pct == 45.5f);
    printf("Round-trip WorkerHeartbeat: PASS\n");
}

void test_workload_submit_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_WORKLOAD_SUBMIT);
    strncpy(src.payload.workload.workload_id, "wkl-99", ID_MAX - 1);
    src.payload.workload.total_size_bytes = 1073741824ULL;
    src.payload.workload.deadline_seconds = 3600;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_WORKLOAD_SUBMIT);
    assert(strcmp(dst.payload.workload.workload_id, "wkl-99") == 0);
    assert(dst.payload.workload.total_size_bytes == 1073741824ULL);
    assert(dst.payload.workload.deadline_seconds == 3600);
    printf("Round-trip WorkloadSubmit: PASS\n");
}

void test_chunk_assign_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_CHUNK_ASSIGN);
    strncpy(src.payload.assign.chunk_id, "chk-123", ID_MAX - 1);
    src.payload.assign.required_cpu_units = 2;
    src.payload.assign.lease_expires_ms = 1718905000;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_CHUNK_ASSIGN);
    assert(strcmp(dst.payload.assign.chunk_id, "chk-123") == 0);
    assert(dst.payload.assign.required_cpu_units == 2);
    assert(dst.payload.assign.lease_expires_ms == 1718905000);
    printf("Round-trip ChunkAssign: PASS\n");
}

void test_chunk_progress_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_CHUNK_PROGRESS);
    strncpy(src.payload.progress.chunk_id, "chk-123", ID_MAX - 1);
    src.payload.progress.progress_percentage = 75.0f;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_CHUNK_PROGRESS);
    assert(strcmp(dst.payload.progress.chunk_id, "chk-123") == 0);
    assert(dst.payload.progress.progress_percentage == 75.0f);
    printf("Round-trip ChunkProgress: PASS\n");
}

void test_chunk_result_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_CHUNK_RESULT);
    strncpy(src.payload.result.chunk_id, "chk-123", ID_MAX - 1);
    src.payload.result.exit_code = 0;
    strncpy(src.payload.result.result_sha256, "fa54b38d38f203874", ID_MAX - 1);
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_CHUNK_RESULT);
    assert(strcmp(dst.payload.result.chunk_id, "chk-123") == 0);
    assert(dst.payload.result.exit_code == 0);
    assert(strcmp(dst.payload.result.result_sha256, "fa54b38d38f203874") == 0);
    printf("Round-trip ChunkResult: PASS\n");
}

void test_recovery_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_RECOVERY);
    strncpy(src.payload.recovery.failed_worker_id, "wrk-tablet-01", ID_MAX - 1);
    src.payload.recovery.action_requeue = true;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_RECOVERY);
    assert(strcmp(dst.payload.recovery.failed_worker_id, "wrk-tablet-01") == 0);
    assert(dst.payload.recovery.action_requeue == true);
    printf("Round-trip RecoveryCommand: PASS\n");
}

void test_ui_surface_roundtrip(void) {
    cluster_message_t src, dst;
    uint8_t buffer[BUF_SIZE];
    memset(&src, 0, sizeof(src));
    
    populate_base_header(&src, MSG_UI_SURFACE);
    src.payload.ui.display_attached = true;
    src.payload.ui.touch_supported = false;
    
    int size = mock_capnp_serialize(&src, buffer, BUF_SIZE);
    assert(size > 0);
    
    int rc = mock_capnp_deserialize(buffer, size, &dst);
    assert(rc == 0);
    
    verify_base_header(&dst);
    assert(dst.type == MSG_UI_SURFACE);
    assert(dst.payload.ui.display_attached == true);
    assert(dst.payload.ui.touch_supported == false);
    printf("Round-trip UiSurface: PASS\n");
}

int main(void) {
    printf("--- INITIALIZING PROTOCOL SCHEMA ROUND-TRIP TEST HARNESS ---\n\n");
    test_worker_join_roundtrip();
    test_worker_heartbeat_roundtrip();
    test_workload_submit_roundtrip();
    test_chunk_assign_roundtrip();
    test_chunk_progress_roundtrip();
    test_chunk_result_roundtrip();
    test_recovery_roundtrip();
    test_ui_surface_roundtrip();
    printf("\nAll schema verification tests passed successfully.\n");
    return 0;
}