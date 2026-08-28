# tests/CMakeLists.txt — unit and integration tests for Void_OS-Merger.

# ---- Standalone worker module tests (VOM_STANDALONE_TEST) ----
set(VOM_STANDALONE_TEST_FILES
    ${CMAKE_SOURCE_DIR}/worker/src/discovery.c
    ${CMAKE_SOURCE_DIR}/worker/src/worker_state.c
    ${CMAKE_SOURCE_DIR}/worker/src/resource_monitor.c
    ${CMAKE_SOURCE_DIR}/worker/src/display_bridge.c
    ${CMAKE_SOURCE_DIR}/worker/src/recovery.c
)

foreach(test_src ${VOM_STANDALONE_TEST_FILES})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(target_name test_standalone_${test_name})
    add_executable(${target_name} ${test_src})
    target_compile_definitions(${target_name} PRIVATE VOM_STANDALONE_TEST)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/master/include
        ${CMAKE_SOURCE_DIR}/worker/include
    )
    target_link_libraries(${target_name} PRIVATE vom_common)
    target_compile_options(${target_name} PRIVATE ${VOM_C_FLAGS})
    add_test(NAME standalone_${test_name} COMMAND ${target_name})
endforeach()

# ---- Standalone master module tests ----
set(VOM_MASTER_STANDALONE_TEST_FILES
    ${CMAKE_SOURCE_DIR}/master/src/recovery.c
    ${CMAKE_SOURCE_DIR}/master/src/chunk_planner.c
)

foreach(test_src ${VOM_MASTER_STANDALONE_TEST_FILES})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(target_name test_standalone_master_${test_name})
    add_executable(${target_name} ${test_src})
    target_compile_definitions(${target_name} PRIVATE VOM_STANDALONE_TEST)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/master/include
        ${CMAKE_SOURCE_DIR}/worker/include
    )
    target_link_libraries(${target_name} PRIVATE vom_common)
    target_compile_options(${target_name} PRIVATE ${VOM_C_FLAGS})
    add_test(NAME standalone_master_${test_name} COMMAND ${target_name})
endforeach()

# Test: common logging
add_executable(test_common_log test_common_log.c)
target_include_directories(test_common_log PRIVATE ${CMAKE_SOURCE_DIR}/common/include)
target_link_libraries(test_common_log PRIVATE vom_common)
target_compile_options(test_common_log PRIVATE ${VOM_C_FLAGS})
add_test(NAME test_common_log COMMAND test_common_log)

# Test: Cap'n Proto stub / result types
add_executable(test_capnp_roundtrip test_capnp_roundtrip.c)
target_include_directories(test_capnp_roundtrip PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/master/include
)
target_link_libraries(test_capnp_roundtrip PRIVATE vom_common)
target_compile_options(test_capnp_roundtrip PRIVATE ${VOM_C_FLAGS})
add_test(NAME test_capnp_roundtrip COMMAND test_capnp_roundtrip)

# Test: scheduler logic (compile scheduler.c directly since it's not a library)
add_executable(test_scheduler_logic
    test_scheduler_logic.c
    ${CMAKE_SOURCE_DIR}/master/src/scheduler.c
)
target_include_directories(test_scheduler_logic PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/master/include
)
target_link_libraries(test_scheduler_logic PRIVATE vom_common)
target_compile_options(test_scheduler_logic PRIVATE ${VOM_C_FLAGS})
add_test(NAME test_scheduler_logic COMMAND test_scheduler_logic)
