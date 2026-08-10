# Shared C helpers used by the master, workers, CLI, and future UI frontends.
cmake_minimum_required(VERSION 3.16)

find_library(VOM_ZMQ_LIB zmq REQUIRED)
find_path(VOM_ZMQ_INCLUDE zmq.h REQUIRED)

set(VOM_COMMON_SOURCES
    src/log.c
    src/zmq_helpers.c
    src/sys_info.c
    src/config.c
)

add_library(vom_common STATIC ${VOM_COMMON_SOURCES})
target_include_directories(vom_common
    PUBLIC  include
    PRIVATE ${VOM_ZMQ_INCLUDE}
)
target_link_libraries(vom_common
    PUBLIC
        vom_proto
        ${VOM_ZMQ_LIB}
)

target_compile_options(vom_common PRIVATE ${VOM_C_FLAGS})
