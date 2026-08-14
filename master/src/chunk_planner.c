#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_STRATEGIES  16
#define MAX_CHUNKS  64
#define MAX_EDGES   128
#define MAX_NAME_LEN    64
#define HASH_SEED   0x9E3779B97F4A7C15ULL

typedef enum {
    STRATEGY_INDEPENDENT_BATCH,
    STRATEGY_SEQUENTIAL_PIPELINE,
    STRATEGY_MAP_REDUCE,
    STRATEGY_UNSUPPORTED,
} StrategyType;

typedef struct {
    char workload_name[MAX_NAME_LEN];
    StrategyType strategy_type;
    size_t total_input_size_bytes;
    size_t compute_intensity_factor;
    bool is_splittable;
    uint32_t raw_data_node_id;
} Workload;

typedef struct {
    uint64_t chunk_id;
    size_t memory_demand_bytes;
    size_t cpu_cores_demand;
    uint32_t target_node_id;
    double data_locality_score;
} ChunkNode;

typedef struct {
    uint64_t src_chunk_id;
    uint64_t dst_chunk_id;
} DependencyEdge;

typedef struct {
    ChunkNode chunks[MAX_CHUNKS];
    int chunk_count;
    DependencyEdge edges[MAX_EDGES];
    int edge_count;
    bool is_valid;
} ExecutionPlan;

typedef bool (*PlanGenerationFunc)(const Workload* wl, ExecutionPlan* out_plan);

typedef struct {
    StrategyType type;
    PlanGenerationFunc plan_fn;
} StrategyRegistry;

static StrategyRegistry g_strategy_registry[MAX_STRATEGIES];
static int g_registered_strategy_count = 0;

static uint64_t generate_deterministic_chunk_id(uint64_t base_hash, int chunk_index) {
    uint64_t hash = base_hash ^ (uint64_t)chunk_index;
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    return hash;
}

static uint64_t compute_workload_base_hash(const Workload* wl) {
    uint64_t hash = HASH_SEED;
    for (int i = 0; wl->workload_name[i] != '\0' && i < MAX_NAME_LEN; i++) {
        hash = (hash ^ wl->workload_name[i]) * 1099511628211ULL;
    }
    hash ^= wl->total_input_size_bytes;
    hash ^= (uint64_t)wl->strategy_type;
    return hash;
}

bool register_planning_strategy(StrategyType type, PlanGenerationFunc fn) {
    if (g_registered_strategy_count >= MAX_STRATEGIES) {
        return false;
    }
    g_strategy_registry[g_registered_strategy_count++] = (StrategyRegistry){type, fn};
    return true;
}

static PlanGenerationFunc find_strategy(StrategyType type) {
    for (int i = 0; i < g_registered_strategy_count; i++) {
        if (g_strategy_registry[i].type == type && g_strategy_registry[i].plan_fn) {
            return g_strategy_registry[i].plan_fn;
        }
    }
    return NULL;
}

bool plan_independent_batch_strategy(const Workload* wl, ExecutionPlan* out_plan) {
    uint64_t base_hash = compute_workload_base_hash(wl);
    out_plan->chunk_count = 4;
    out_plan->edge_count = 0;

    size_t chunk_size = wl->total_input_size_bytes / out_plan->chunk_count;

    for (int i = 0; i < out_plan->chunk_count; i++) {
        uint32_t scheduled_node = i % 4;

        out_plan->chunks[i] = (ChunkNode){
            .chunk_id = generate_deterministic_chunk_id(base_hash, i),
            .memory_demand_bytes = chunk_size + (1024 * 1024 * 64),
            .cpu_cores_demand = wl->compute_intensity_factor * 2,
            .target_node_id = scheduled_node,
            .data_locality_score = (scheduled_node == wl->raw_data_node_id)
        };
    }
    out_plan->is_valid = true;
    return true;
}

bool plan_sequential_pipeline_strategy(const Workload* wl, ExecutionPlan* out_plan) {
    uint64_t base_hash = compute_workload_base_hash(wl);
    out_plan->chunk_count = 4;
    out_plan->edge_count = 3;

    size_t step_memory = wl->total_input_size_bytes / 2;

    for (int i = 0; i < out_plan->chunk_count; i++) {
        out_plan->chunks[i] = (ChunkNode){
            .chunk_id = generate_deterministic_chunk_id(base_hash, i),
            .memory_demand_bytes = step_memory,
            .cpu_cores_demand = wl->compute_intensity_factor,
            .target_node_id = wl->raw_data_node_id,
            .data_locality_score = 1.0
        };
    }

    for (int i = 0; i < out_plan->edge_count; i++) {
        out_plan->edges[i].src_chunk_id = out_plan->chunks[i].chunk_id;
        out_plan->edges[i].dst_chunk_id = out_plan->chunks[i + 1].chunk_id;
    }

    out_plan->is_valid = true;
    return true;
}

bool generate_chunk_graph(const Workload* wl, ExecutionPlan* out_plan) {
    if (out_plan == NULL || wl == NULL) return false;
    memset(out_plan, 0, sizeof(ExecutionPlan));

    if (!wl->is_splittable) {
        printf("[GUARDO] Rejected, unable to split workload '%s'. Aborting execution plan safely.\n", wl->workload_name);
        out_plan->is_valid = false;
        return false;
    }

    PlanGenerationFunc strategy_call = find_strategy(wl->strategy_type);
    if (strategy_call == NULL) {
        printf("[GUARDO] Strategy type %d is not registered.\n", wl->strategy_type);
        out_plan->is_valid = false;
        return false;
    }

    return strategy_call(wl, out_plan);
}

bool verify_plan_topology_is_dag(const ExecutionPlan* plan) {
    if (!plan->is_valid || plan->chunk_count == 0) return false;

    int in_degree[MAX_CHUNKS] = {0};

    for (int i = 0; i < plan->edge_count; i++) {
        uint64_t dest = plan->edges[i].dst_chunk_id;
        for (int j = 0; j < plan->chunk_count; j++) {
            if (plan->chunks[j].chunk_id == dest) {
                in_degree[j]++;
            }
        }
    }

    int processing_queue[MAX_CHUNKS];
    int head = 0, tail = 0;

    for (int i = 0; i < plan->chunk_count; i++) {
        if (in_degree[i] == 0) {
            processing_queue[tail++] = i;
        }
    }

    int processed_nodes_count = 0;
    while (head < tail) {
        int current_idx = processing_queue[head++];
        processed_nodes_count++;

        uint64_t current_id = plan->chunks[current_idx].chunk_id;
        for (int i = 0; i < plan->edge_count; i++) {
            if (plan->edges[i].src_chunk_id == current_id) {
                uint64_t neighbor_id = plan->edges[i].dst_chunk_id;
                for (int j = 0; j < plan->chunk_count; j++) {
                    if (plan->chunks[j].chunk_id == neighbor_id) {
                        in_degree[j]--;
                        if (in_degree[j] == 0) {
                            processing_queue[tail++] = j;
                        }
                    }
                }
            }
        }
    }

    return (processed_nodes_count == plan->chunk_count);
}

void execute_test_suite(void) {
    printf("-- INITIALIZE CHUNK PLANNER HARNESS --\n\n");
    register_planning_strategy(STRATEGY_INDEPENDENT_BATCH, plan_independent_batch_strategy);
    register_planning_strategy(STRATEGY_SEQUENTIAL_PIPELINE, plan_sequential_pipeline_strategy);

    printf("[TEST-1] Testing unsplittable workload rejection...\n");
    Workload monolithic_wl = {
        .workload_name = "LegacyMonolithicDBDump",
        .strategy_type = STRATEGY_SEQUENTIAL_PIPELINE,
        .total_input_size_bytes = 1024ULL * 1024 * 1024 * 50,
        .compute_intensity_factor = 4,
        .is_splittable = false,
        .raw_data_node_id = 0
    };
    ExecutionPlan plan_1;
    bool res_1 = generate_chunk_graph(&monolithic_wl, &plan_1);
    printf("Result -> Verification: %s\n\n", (!res_1 && !plan_1.is_valid) ? "SUCCESS (Rejected)" : "FAIL");

    printf("[TEST-2] Testing Independent batch workload allocation...\n");
    Workload batch_wl = {
        .workload_name = "ParallelETLJob",
        .strategy_type = STRATEGY_INDEPENDENT_BATCH,
        .total_input_size_bytes = 1024ULL * 1024 * 1024 * 8,
        .compute_intensity_factor = 2,
        .is_splittable = true,
        .raw_data_node_id = 1
    };
    ExecutionPlan plan_2;
    bool res_2 = generate_chunk_graph(&batch_wl, &plan_2);
    printf("Result -> Status: %s | Chunks: %d | Edges: %d\n", res_2 ? "SUCCESS" : "FAIL", plan_2.chunk_count, plan_2.edge_count);
    bool is_dag_2 = verify_plan_topology_is_dag(&plan_2);
    printf("Result -> DAG Topology Evaluation: %s\n\n", is_dag_2 ? "SUCCESS" : "FAIL");

    printf("[TEST-3] Testing Sequential pipeline workload allocation...\n");
    Workload pipeline_wl = {
        .workload_name = "OrderedDataTransform",
        .strategy_type = STRATEGY_SEQUENTIAL_PIPELINE,
        .total_input_size_bytes = 1024ULL * 1024 * 1024 * 16,
        .compute_intensity_factor = 3,
        .is_splittable = true,
        .raw_data_node_id = 2
    };
    ExecutionPlan plan_3;
    bool res_3 = generate_chunk_graph(&pipeline_wl, &plan_3);
    printf("Result -> Status: %s\n", res_3 ? "SUCCESS" : "FAIL");
    bool is_dag_3 = verify_plan_topology_is_dag(&plan_3);
    printf("Result -> Chunks Count: %d | Edges Count: %d\n", plan_3.chunk_count, plan_3.edge_count);
    printf("Result -> Edge 0 Layout: [Src: 0x%llX] ---> [Dst: 0x%llX]\n", (unsigned long long)plan_3.edges[0].src_chunk_id, (unsigned long long)plan_3.edges[0].dst_chunk_id);
    printf("Result -> DAG Topology Evaluation: %s\n\n", is_dag_3 ? "SUCCESS" : "FAIL");

    printf("[TEST 4] Testing Malicious Cyclic Topological Deadlock Protection...\n");
    ExecutionPlan cyclic_forged_plan;
    cyclic_forged_plan.is_valid = true;
    cyclic_forged_plan.chunk_count = 2;
    cyclic_forged_plan.chunks[0].chunk_id = 0xDEADE;
    cyclic_forged_plan.chunks[1].chunk_id = 0xBEEFE;
    cyclic_forged_plan.edge_count = 2;
    cyclic_forged_plan.edges[0] = (DependencyEdge){0xDEADE, 0xBEEFE};
    cyclic_forged_plan.edges[1] = (DependencyEdge){0xBEEFE, 0xDEADE};
    bool is_dag_4 = verify_plan_topology_is_dag(&cyclic_forged_plan);
    printf("Result -> Forged Cyclic Graph Status (Should fail DAG validation): %s\n", !is_dag_4 ? "SUCCESS (Deadlock Caught)" : "FAIL");
}

int main(void) {
    execute_test_suite();
    return 0;
}
