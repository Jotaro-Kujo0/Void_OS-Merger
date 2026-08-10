# Build cluster-worker, the computation-focused node of the logical device.
cmake_minimum_required(VERSION 3.16)

set(VOM_WORKER_SOURCES
    src/main.c
    src/transport.c
    src/capabilities.c
    src/discovery.c
    src/heartbeat.c
    src/resource_monitor.c
    src/executor.c
    src/display_bridge.c
    src/recovery.c
    src/worker_state.c
    src/cli.c
)

add_executable(cluster-worker ${VOM_WORKER_SOURCES})
target_include_directories(cluster-worker PRIVATE include)
target_link_libraries(cluster-worker PRIVATE vom_common)
target_compile_options(cluster-worker PRIVATE ${VOM_C_FLAGS})

target_compile_definitions(cluster-worker PRIVATE
    VOM_WORKER_DEFAULT_MASTER="tcp://127.0.0.1:5555"
)

