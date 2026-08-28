# Shared C helpers used by the master, workers, CLI, and future UI frontends.
cmake_minimum_required(VERSION 3.16)

find_library(VOM_ZMQ_LIB NAMES zmq libzmq libzmq-mt)
find_path(VOM_ZMQ_INCLUDE zmq.h)

set(VOM_COMMON_SOURCES
    src/log.c
    src/zmq_sockets.c
    src/zmq_context.c
    src/sys_info.c
    src/config.c
)

add_library(vom_common STATIC ${VOM_COMMON_SOURCES})

if(VOM_ZMQ_LIB AND VOM_ZMQ_INCLUDE)
    target_compile_definitions(vom_common PUBLIC VOM_HAS_ZMQ=1)
    target_include_directories(vom_common
        PUBLIC  include
        PRIVATE ${VOM_ZMQ_INCLUDE}
    )
else()
    message(WARNING "[Void_OS-Merger] libzmq not found; building vom_common without ZeroMQ support")
    target_compile_definitions(vom_common PUBLIC VOM_HAS_ZMQ=0)
    target_include_directories(vom_common
        PUBLIC  include
    )
endif()

if(WIN32)
    target_link_libraries(vom_common
        PUBLIC
            vom_proto
            ws2_32
    )
else()
    target_link_libraries(vom_common
        PUBLIC
            vom_proto
    )
endif()

if(VOM_ZMQ_LIB AND VOM_ZMQ_INCLUDE)
    target_link_libraries(vom_common
        PUBLIC
            ${VOM_ZMQ_LIB}
    )
endif()

target_compile_options(vom_common PRIVATE ${VOM_C_FLAGS})
