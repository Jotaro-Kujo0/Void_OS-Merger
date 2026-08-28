# Build vom-ui, the presentation boundary for the master control plane.
cmake_minimum_required(VERSION 3.16)

set(VOM_UI_SOURCES
    src/model.c
)

add_library(vom_ui STATIC ${VOM_UI_SOURCES})
target_include_directories(vom_ui PRIVATE include)
target_link_libraries(vom_ui PRIVATE vom_common)
target_compile_options(vom_ui PRIVATE ${VOM_C_FLAGS})