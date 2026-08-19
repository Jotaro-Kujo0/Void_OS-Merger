#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define MAX_TEST_WORKERS 8
#define NAME_LEN_MAX    64

typedef uint32_t vom_cpu_npu_t;

typedef enum {
    RUN_POSIX_SUBPROC,
    RUN_WASM_SANDBOX,
    RUN_CONTAINER
} WorkerRuntimeKind;

typedef enum {
    HEALTH_UNHEALTHY,
    HEALTH_HEALTHY,
    HEALTH_DRAINING
} WorkerHealthStatus;

typedef struct {
    vom_cpu_npu_t aggregate_cpu_npu;
    uint64_t aggregate_memory_bytes;
    WorkerRuntimeKind supported_runtime;
} worker_hardware_spec_t;

typedef struct {
    vom_cpu_npu_t allocated_cpu_npu;
    uint64_t allocated_memory_bytes;
    float battery_percentage; 
    WorkerHealthStatus health;
    uint32_t current_data_node_id;
} worker_runtime_state_t;

typedef struct {
    char worker_id[NAME_LEN_MAX];
    worker_hardware_spec_t hardware;
    worker_runtime_state_t dynamic_state;
} scheduler_worker_fixture_t;

typedef struct {
    uint64_t chunk_id;
    vom_cpu_npu_t required_cpu_npu;
    uint64_t required_memory_bytes;
    WorkerRuntimeKind required_runtime;
    uint32_t preferred_data_node_id;
} scheduler_chunk_demand_t;

bool vom_scheduler_evaluate_compatibility(const scheduler_worker_fixture_t *worker, const scheduler_chunk_demand_t *chunk) {
    if (!worker || !chunk) return false;
    if (worker->dynamic_state.health != HEALTH_HEALTHY) return false;
    if (worker->hardware.supported_runtime != chunk->required_runtime) return false;

    vom_cpu_npu_t total_allocated_cpu = worker->dynamic_state.allocated_cpu_npu + chunk->required_cpu_npu;
    if (total_allocated_cpu > worker->hardware.aggregate_cpu_npu) return false;

    uint64_t total_allocated_mem = worker->dynamic_state.allocated_memory_bytes + chunk->required_memory_bytes;
    if (total_allocated_mem > worker->hardware.aggregate_memory_bytes) return false;

    return true;
}

double vom_scheduler_calculate_soft_score(const scheduler_worker_fixture_t *worker, const scheduler_chunk_demand_t *chunk) {
    double score = 0.0;

    double cpu_util = (double)worker->dynamic_state.allocated_cpu_npu / (double)worker->hardware.aggregate_cpu_npu;
    score += (1.0 - cpu_util) * 50.0;

    if (worker->dynamic_state.current_data_node_id == chunk->preferred_data_node_id) {
        score += 30.0;
    }

    score += (worker->dynamic_state.battery_percentage / 100.0) * 20.0;

    return score;
}

const scheduler_worker_fixture_t* vom_scheduler_select_placement(const scheduler_worker_fixture_t *workers, int worker_count, const scheduler_chunk_demand_t *chunk) {
    const scheduler_worker_fixture_t *best_worker = NULL;
    double max_score = -1.0;

    for (int i = 0; i < worker_count; i++) {
        const scheduler_worker_fixture_t *w = &workers[i];
        if (vom_scheduler_evaluate_compatibility(w, chunk)) {
            double current_score = vom_scheduler_calculate_soft_score(w, chunk);
            if (current_score > max_score) {
                max_score = current_score;
                best_worker = w;
            }
        }
    }
    return best_worker;
}

void test_hard_constraints_filtering(void) {
    scheduler_worker_fixture_t test_pool[3];
    memset(test_pool, 0, sizeof(test_pool));

    strncpy(test_pool[0].worker_id, "wrk-draining", NAME_LEN_MAX - 1);
    test_pool[0].hardware = (worker_hardware_spec_t){4000, 8192, RUN_POSIX_SUBPROC};
    test_pool[0].dynamic_state = (worker_runtime_state_t){0, 0, 100.0f, HEALTH_DRAINING, 0};

    strncpy(test_pool[1].worker_id, "wrk-wasm-only", NAME_LEN_MAX - 1);
    test_pool[1].hardware = (worker_hardware_spec_t){4000, 8192, RUN_WASM_SANDBOX};
    test_pool[1].dynamic_state = (worker_runtime_state_t){0, 0, 100.0f, HEALTH_HEALTHY, 0};

    strncpy(test_pool[2].worker_id, "wrk-eligible", NAME_LEN_MAX - 1);
    test_pool[2].hardware = (worker_hardware_spec_t){4000, 8192, RUN_POSIX_SUBPROC};
    test_pool[2].dynamic_state = (worker_runtime_state_t){0, 0, 100.0f, HEALTH_HEALTHY, 0};

    scheduler_chunk_demand_t posix_chunk = {101, 1000, 1024, RUN_POSIX_SUBPROC, 0};

    assert(!vom_scheduler_evaluate_compatibility(&test_pool[0], &posix_chunk));
    assert(!vom_scheduler_evaluate_compatibility(&test_pool[1], &posix_chunk));
    assert(vom_scheduler_evaluate_compatibility(&test_pool[2], &posix_chunk));

    const scheduler_worker_fixture_t *selected = vom_scheduler_select_placement(test_pool, 3, &posix_chunk);
    assert(selected != NULL);
    assert(strcmp(selected->worker_id, "wrk-eligible") == 0);
}

void test_proportional_capacity_and_locality(void) {
    scheduler_worker_fixture_t test_pool[2];
    memset(test_pool, 0, sizeof(test_pool));

    strncpy(test_pool[0].worker_id, "wrk-saturated-big", NAME_LEN_MAX - 1);
    test_pool[0].hardware = (worker_hardware_spec_t){8000, 16384, RUN_POSIX_SUBPROC};
    test_pool[0].dynamic_state = (worker_runtime_state_t){6000, 12288, 100.0f, HEALTH_HEALTHY, 1};

    strncpy(test_pool[1].worker_id, "wrk-local-free-small", NAME_LEN_MAX - 1);
    test_pool[1].hardware = (worker_hardware_spec_t){3000, 4096, RUN_POSIX_SUBPROC};
    test_pool[1].dynamic_state = (worker_runtime_state_t){0, 0, 100.0f, HEALTH_HEALTHY, 2};

    scheduler_chunk_demand_t local_chunk = {102, 1000, 1024, RUN_POSIX_SUBPROC, 2};

    const scheduler_worker_fixture_t *selected = vom_scheduler_select_placement(test_pool, 2, &local_chunk);
    assert(selected != NULL);
    assert(strcmp(selected->worker_id, "wrk-local-free-small") == 0);
}

int main(void) {
    test_hard_constraints_filtering();
    test_proportional_capacity_and_locality();
    return 0;
}
