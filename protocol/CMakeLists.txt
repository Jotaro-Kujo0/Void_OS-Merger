# Compile protocol/cluster.capnp into C bindings under protocol/gen/.
cmake_minimum_required(VERSION 3.16)

# Resolve the `capnp` and `capnpc-c` executables; fail loudly if missing.
find_program(CAPNP_EXECUTABLE capnp REQUIRED)
find_program(CAPNPC_C_EXECUTABLE capnpc-c REQUIRED)

set(VOM_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cluster.capnp
)

set(VOM_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

# Note: the OUTPUT filenames below are derived from the schema filename
# (cluster.capnp -> cluster.capnp.{h,c}).  If you ever rename the schema,
# update OUTPUT <-> source mapping here or generation will silently skip.
add_custom_command(
    OUTPUT  ${VOM_PROTO_GEN_DIR}/cluster.capnp.h
            ${VOM_PROTO_GEN_DIR}/cluster.capnp.c
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VOM_PROTO_GEN_DIR}
    COMMAND ${CAPNP_EXECUTABLE}
            compile
            -o ${CAPNPC_C_EXECUTABLE}
            --src-prefix ${CMAKE_CURRENT_SOURCE_DIR}
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            ${VOM_PROTO_FILES}
    DEPENDS ${VOM_PROTO_FILES}
    COMMENT "[Void_OS-Merger] Generating Cap'n Proto C bindings"
    VERBATIM
)

add_custom_target(vom_proto_gen DEPENDS ${VOM_PROTO_GEN_DIR}/cluster.capnp.h)

# Expose the generated headers to the rest of the build.
target_include_directories(vom_proto_gen INTERFACE ${VOM_PROTO_GEN_DIR})

# Helper so subprojects can link against the generated sources simply by
# saying:  target_link_libraries(<tgt> PRIVATE vom_proto).
add_library(vom_proto STATIC ${VOM_PROTO_GEN_DIR}/cluster.capnp.c)
target_include_directories(vom_proto PUBLIC ${VOM_PROTO_GEN_DIR})
add_dependencies(vom_proto vom_proto_gen)
