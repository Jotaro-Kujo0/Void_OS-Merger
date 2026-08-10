#!/usr/bin/env bash
# scripts/gen_proto.sh — regenerate bindings from protocol/cluster.capnp.
#
# The schema is the master/worker logical-device wire contract.

set -euo pipefail

mkdir -p protocol/gen

# TODO: detect missing capnp/capnpc-c and print platform-specific guidance.
# TODO: invoke capnp compile with the repository protocol include path.
# TODO: keep generated output under the build directory when CMake drives it.
# TODO: add an optional watch mode for schema development.
