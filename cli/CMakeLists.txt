# Build vom-cli, the operator client for the master control plane.
cmake_minimum_required(VERSION 3.16)

add_executable(vom-cli src/main.c)
target_include_directories(vom-cli PRIVATE include)
target_link_libraries(vom-cli PRIVATE vom_common)
target_compile_options(vom-cli PRIVATE ${VOM_C_FLAGS})
