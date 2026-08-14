# Compile protocol/cluster.capnp into C bindings under protocol/gen/.
cmake_minimum_required(VERSION 3.16)

# The `capnp` compiler is required. The C code generator `capnpc-c`
# (capnproto-c) is unmaintained and not installed on this system, so it is
# optional: while it is missing, `vom_proto` is built from an empty stub and
# the comment-only cluster.capnp schema is not compiled. Install the plugin
# to get real bindings once the schema is populated.
find_program(CAPNP_EXECUTABLE capnp REQUIRED)
find_program(CAPNPC_C_EXECUTABLE NAMES capnpc-c capnp-c)

set(VOM_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cluster.capnp
)

set(VOM_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

if(CAPNPC_C_EXECUTABLE)
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
else()
    # capnpc-c unavailable: provide an empty translation unit so targets that
    # link vom_proto still build until the schema gains real content.
    message(WARNING "[Void_OS-Merger] capnpc-c not found; building vom_proto from an empty stub (cluster.capnp is comment-only)")
    set(VOM_PROTO_STUB ${CMAKE_CURRENT_BINARY_DIR}/proto_stub.c)
    file(WRITE ${VOM_PROTO_STUB}
        "/* Generated stub: capnpc-c unavailable, cluster.capnp not compiled. */\n"
    )
    add_library(vom_proto STATIC ${VOM_PROTO_STUB})
endif()
