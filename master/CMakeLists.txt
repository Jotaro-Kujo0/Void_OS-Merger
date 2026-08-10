# Build cluster-master, the coordinator and control plane of the logical device.
cmake_minimum_required(VERSION 3.16)

set(VOM_MASTER_SOURCES
    src/cluster_state.c   # authoritative logical-device state
    src/registry.c        # connected worker membership and liveness
    src/workload.c        # complete workload lifecycle
    src/chunk_planner.c   # workload-to-chunk planning
    src/scheduler.c       # compatible worker/chunk placement
    src/recovery.c        # failed-worker chunk recovery and reassignment
    src/router.c          # master transport and message dispatch
    src/ui_coordinator.c  # master-owned UI view model
    src/cli.c             # master argument parsing
    src/main.c            # coordinator entrypoint
)

add_executable(cluster-master ${VOM_MASTER_SOURCES})
target_include_directories(cluster-master PRIVATE include)
target_link_libraries(cluster-master PRIVATE vom_common)
target_compile_options(cluster-master PRIVATE ${VOM_C_FLAGS})

target_compile_definitions(cluster-master PRIVATE
    VOM_MASTER_DEFAULT_ENDPOINT="tcp://0.0.0.0:5555"
)

