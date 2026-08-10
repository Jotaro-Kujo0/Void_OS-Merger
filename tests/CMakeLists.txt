# tests/CMakeLists.txt — unit and integration tests for Void_OS-Merger.
#
# Tests remain a scaffold until the protocol, domain APIs, and implementations
# are filled in.

set(VOM_TEST_SOURCES
    test_common_log.c          # shared logging behavior
    test_capnp_roundtrip.c     # master/worker envelope round trip
    test_scheduler_logic.c     # ready-chunk to worker scoring
)

foreach(t ${VOM_TEST_SOURCES})
    get_filename_component(name ${t} NAME_WE)
    add_executable(${name} ${t})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/master/include
        ${CMAKE_SOURCE_DIR}/worker/include
    )
    target_link_libraries(${name} PRIVATE vom_common)
    target_compile_options(${name} PRIVATE ${VOM_C_FLAGS})
    add_test(NAME ${name} COMMAND ${name})
endforeach()
