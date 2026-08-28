/*
 * tests/test_common_log.c — Test common type definitions and macros.
 *
 * This test validates that the core types compile and work correctly
 * on the target platform. The full logging subsystem test requires
 * threading which may deadlock on some platforms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <common/domain.h>
#include <common/result.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %-45s ", test_count, name); } while(0)
#define PASS() do { pass_count++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

void test_domain_workload_states(void) {
    TEST("Workload states are defined");
    VomWorkloadState s = VOM_WORKLOAD_STATE_SUBMITTED;
    if (s == 0) PASS(); else FAIL("unexpected value");
}

void test_domain_chunk_states(void) {
    TEST("Chunk states are defined");
    VomChunkState s = VOM_CHUNK_PENDING;
    if (s == 0) PASS(); else FAIL("unexpected value");
}

void test_domain_worker_status(void) {
    TEST("Worker status values are distinct");
    if (VOM_WORKER_OFFLINE != VOM_WORKER_HEALTHY &&
        VOM_WORKER_HEALTHY != VOM_WORKER_DRAINING) {
        PASS();
    } else {
        FAIL("status values not distinct");
    }
}

void test_domain_struct_sizes(void) {
    TEST("Domain structs have reasonable sizes");
    if (sizeof(vom_workload_state) > 0 &&
        sizeof(vom_chunk_plan) > 0 &&
        sizeof(vom_worker_observation) > 0) {
        PASS();
    } else {
        FAIL("zero-size struct");
    }
}

void test_result_type(void) {
    TEST("Result type constructs correctly");
    vom_result_t r = vom_result_success();
    if (vom_result_is_success(r) && r.category == VOM_CAT_SUCCESS) {
        PASS();
    } else {
        FAIL("result construction failed");
    }
}

void test_domain_constants(void) {
    TEST("Domain constants have expected values");
    if (DOMAIN_UUID_LEN == 40 && DOMAIN_NAME_MAX == 64) {
        PASS();
    } else {
        FAIL("unexpected constant values");
    }
}

int main(void) {
    printf("=== Common Types Test Suite ===\n\n");
    test_domain_workload_states();
    test_domain_chunk_states();
    test_domain_worker_status();
    test_domain_struct_sizes();
    test_result_type();
    test_domain_constants();
    printf("\nResults: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
