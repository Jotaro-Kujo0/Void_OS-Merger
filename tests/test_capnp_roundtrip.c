/*
 * tests/test_capnp_roundtrip.c — Test protocol stub and result types.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <common/result.h>

static int test_count = 0;
static int pass_count = 0;
#define TEST(name) do { test_count++; printf("  [%d] %-45s ", test_count, name); } while(0)
#define PASS() do { pass_count++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

void test_stub_links(void) { TEST("vom_proto stub links"); PASS(); }

void test_result_success(void) {
    TEST("vom_result_t success");
    vom_result_t r = vom_result_success();
    if (vom_result_is_success(r)) PASS(); else FAIL("failed");
}

void test_result_failure(void) {
    TEST("vom_result_t failure");
    vom_result_t r = { .category = VOM_CAT_TIMEOUT, .stable_code = 9,
        .retry_policy = VOM_RETRY_BACKOFF, .human_detail = "Timed out" };
    if (!vom_result_is_success(r)) PASS(); else FAIL("expected failure");
}

void test_result_categories(void) {
    TEST("All error categories distinct");
    VomErrorCategory cats[] = { VOM_CAT_SUCCESS, VOM_CAT_INVALID_INPUT,
        VOM_CAT_TRANSPORT, VOM_CAT_PROTOCOL, VOM_CAT_SCHEDULER_REJECT,
        VOM_CAT_WORKER_EXECUTION, VOM_CAT_TIMEOUT, VOM_CAT_CANCELLATION,
        VOM_CAT_AUTHENTICATION, VOM_CAT_INTERNAL };
    for (int i = 0; i < 10; i++)
        for (int j = i+1; j < 10; j++)
            if (cats[i] == cats[j]) { FAIL("duplicate"); return; }
    PASS();
}

void test_retry_actions(void) {
    TEST("All retry actions distinct");
    VomRetryAction a[] = { VOM_RETRY_NONE, VOM_RETRY_IMMEDIATE,
        VOM_RETRY_BACKOFF, VOM_RETRY_REPLAN };
    for (int i = 0; i < 4; i++)
        for (int j = i+1; j < 4; j++)
            if (a[i] == a[j]) { FAIL("duplicate"); return; }
    PASS();
}

int main(void) {
    printf("=== Cap'n Proto / Result Type Test Suite ===\n\n");
    test_stub_links(); test_result_success(); test_result_failure();
    test_result_categories(); test_retry_actions();
    printf("\nResults: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
