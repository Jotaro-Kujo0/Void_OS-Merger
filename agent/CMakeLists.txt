cmake_minimum_required(VERSION 3.16)
project(cluster-agent CXX)

find_library(ZMQ_LIB zmq)
find_path(ZMQ_INCLUDE zmq.h)

if(NOT ZMQ_LIB)
    message(FATAL_ERROR "ZeroMQ library not found")
endif()

add_executable(cluster-agent src/main.c)
target_include_directories(cluster-agent PRIVATE ${ZMQ_INCLUDE})
target_link_libraries(cluster-agent PRIVATE ${ZMQ_LIB})
