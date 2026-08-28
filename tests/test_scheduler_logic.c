/*
 * tests/test_scheduler_logic.c — Test the chunk-to-worker scheduling logic.
 *
 * Tests:
 *   1. Worker registration
 *   2. Chunk submission
 *   3. Hard filter rejection (insufficient resources)
 *   4. Basic scheduling tick produces a lease
 *   5. Lease release frees worker resources
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Include the module's types by embedding them (avoids include-path issues) */

typedef struct {
    char worker_id[64];
    int platform;
    int runtime;
    uint8_t cpu_units;
    uint8_t reserv_cpu;
    uint64_t free_memory_mb;
    uint64_t reserv_mem_mb;
    float battery_pct;
    int active_chunks;
    int has_touch_input;
    int draining;
    int healthy;
    double reliability_score;
} vom_worker_view;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    int required_platform;
    int required_runtime;
    uint8_t required_cpu;
    uint64_t required_memory_mb;
    int requires_touch;
    bool is_blocked;
    uint32_t preferred_node;
} vom_chunk_plan;

typedef struct {
    uint64_t chunk_id;
    char assigned_worker_id[64];
    uint64_t lease_expires_ms;
    bool is_active;
} vom_assignment_lease;

/* External scheduler functions */
extern void vom_scheduler_init(void);
extern int  vom_scheduler_register_worker(const vom_worker_view *worker);
extern int  vom_scheduler_submit_ready_chunk(const vom_chunk_plan *chunk);
extern int  vom_scheduler_tick(uint64_t current_time_ms, vom_assignment_lease *out_leases, int max_leases);
extern void vom_scheduler_release_lease(uint64_t chunk_id);

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %-45s ", test_count, name); } while(0)
#define PASS() do { pass_count++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

void test_init(void) {
    TEST("Scheduler initialization");
    vom_scheduler_init();
    PASS();
}

void test_register_worker(void) {
    TEST("Register a healthy worker");
    vom_scheduler_init();
    vom_worker_view w = {0};
    strncpy(w.worker_id, "worker-01", 63);
    w.cpu_units = 8;
    w.free_memory_mb = 16384;
    w.healthy = 1;
    w.reliability_score = 0.95;
    int rc = vom_scheduler_register_worker(&w);
    if (rc == 0) PASS(); else FAIL("register_worker returned non-zero");
}

void test_submit_chunk(void) {
    TEST("Submit a ready chunk");
    vom_scheduler_init();
    vom_chunk_plan c = {0};
    c.chunk_id = 1001;
    c.workload_id = 501;
    c.required_cpu = 2;
    c.required_memory_mb = 1024;
    int rc = vom_scheduler_submit_ready_chunk(&c);
    if (rc == 0) PASS(); else FAIL("submit_ready_chunk returned non-zero");
}

void test_tick_assigns_chunk(void) {
    TEST("Tick assigns chunk to compatible worker");
    vom_scheduler_init();

    vom_worker_view w = {0};
    strncpy(w.worker_id, "worker-01", 63);
    w.cpu_units = 8;
    w.free_memory_mb = 16384;
    w.healthy = 1;
    w.reliability_score = 0.9;
    vom_scheduler_register_worker(&w);

    vom_chunk_plan c = {0};
    c.chunk_id = 2001;
    c.workload_id = 601;
    c.required_cpu = 2;
    c.required_memory_mb = 512;
    vom_scheduler_submit_ready_chunk(&c);

    vom_assignment_lease leases[10];
    int dispatched = vom_scheduler_tick(1000, leases, 10);

    if (dispatched == 1 && leases[0].is_active && leases[0].chunk_id == 2001) {
        PASS();
    } else {
        FAIL("Expected 1 dispatched lease for chunk 2001");
    }
}

void test_reject_insufficient_cpu(void) {
    TEST("Reject chunk needing more CPU than available");
    vom_scheduler_init();

    vom_worker_view w = {0};
    strncpy(w.worker_id, "worker-tiny", 63);
    w.cpu_units = 1;
    w.free_memory_mb = 4096;
    w.healthy = 1;
    w.reliability_score = 1.0;
    vom_scheduler_register_worker(&w);

    vom_chunk_plan c = {0};
    c.chunk_id = 3001;
    c.workload_id = 701;
    c.required_cpu = 8;  /* Needs 8, worker only has 1 */
    c.required_memory_mb = 512;
    vom_scheduler_submit_ready_chunk(&c);

    vom_assignment_lease leases[10];
    int dispatched = vom_scheduler_tick(1000, leases, 10);

    if (dispatched == 0) {
        PASS();
    } else {
        FAIL("Expected 0 dispatched (insufficient CPU)");
    }
}

void test_reject_draining_worker(void) {
    TEST("Reject chunk assignment to draining worker");
    vom_scheduler_init();

    vom_worker_view w = {0};
    strncpy(w.worker_id, "worker-drain", 63);
    w.cpu_units = 8;
    w.free_memory_mb = 16384;
    w.healthy = 1;
    w.draining = 1;  /* Worker is draining */
    w.reliability_score = 1.0;
    vom_scheduler_register_worker(&w);

    vom_chunk_plan c = {0};
    c.chunk_id = 4001;
    c.workload_id = 801;
    c.required_cpu = 1;
    c.required_memory_mb = 256;
    vom_scheduler_submit_ready_chunk(&c);

    vom_assignment_lease leases[10];
    int dispatched = vom_scheduler_tick(1000, leases, 10);

    if (dispatched == 0) {
        PASS();
    } else {
        FAIL("Expected 0 dispatched (worker draining)");
    }
}

void test_lease_release(void) {
    TEST("Release lease frees worker resources");
    vom_scheduler_init();

    vom_worker_view w = {0};
    strncpy(w.worker_id, "worker-01", 63);
    w.cpu_units = 4;
    w.free_memory_mb = 4096;
    w.healthy = 1;
    w.reliability_score = 1.0;
    vom_scheduler_register_worker(&w);

    vom_chunk_plan c = {0};
    c.chunk_id = 5001;
    c.workload_id = 901;
    c.required_cpu = 2;
    c.required_memory_mb = 1024;
    vom_scheduler_submit_ready_chunk(&c);

    vom_assignment_lease leases[10];
    vom_scheduler_tick(1000, leases, 10);

    /* Release the lease */
    vom_scheduler_release_lease(5001);

    /* After release, a new chunk should be assignable */
    c.chunk_id = 5002;
    vom_scheduler_submit_ready_chunk(&c);
    int dispatched = vom_scheduler_tick(2000, leases, 10);

    if (dispatched >= 1) {
        PASS();
    } else {
        FAIL("Expected chunk assignment after lease release");
    }
}

int main(void) {
    printf("=== Scheduler Logic Test Suite ===\n\n");

    test_init();
    test_register_worker();
    test_submit_chunk();
    test_tick_assigns_chunk();
    test_reject_insufficient_cpu();
    test_reject_draining_worker();
    test_lease_release();

    printf("\nResults: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
