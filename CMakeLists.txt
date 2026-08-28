# Void_OS-Merger — universal logical device built from a master and workers.
cmake_minimum_required(VERSION 3.16)

set(VOM_SUBPROJECTS
    protocol
    common
    master
    worker
    cli
    ui
)

project(void_os_merger C CXX)

if(MSVC)
    set(VOM_WARN_FLAGS /W4 /wd4100)
else()
    set(VOM_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()
set(VOM_C_FLAGS   "${VOM_WARN_FLAGS}")
set(VOM_CXX_FLAGS "${VOM_WARN_FLAGS}")

find_package(Threads)

include_directories(${CMAKE_SOURCE_DIR})

foreach(sub ${VOM_SUBPROJECTS})
    add_subdirectory(${sub})
endforeach()

option(VOM_BUILD_TESTS "Build the integration/unit tests" OFF)
if(VOM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
