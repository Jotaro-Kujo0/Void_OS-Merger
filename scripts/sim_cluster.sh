#!/usr/bin/env bash
# scripts/sim_cluster.sh — boot 1 master + N workers on loopback.
#
# Usage:
#   bash scripts/sim_cluster.sh           # 1 master + 3 workers
#   N=5 bash scripts/sim_cluster.sh       # 1 master + 5 workers

set -euo pipefail

N=${N:-3}
BUILD_DIR=${BUILD_DIR:-build/linux}

if [[ ! -x "$BUILD_DIR/cluster-master" ]] || [[ ! -x "$BUILD_DIR/cluster-worker" ]]; then
    echo "[sim] building first…" >&2
    make -s all
fi

# TODO: derive per-worker loopback endpoints from the worker index.
# TODO: launch cluster-master in the background and capture its PID.
# TODO: launch N cluster-worker processes with stable worker IDs.
# TODO: forward each worker's stdout/stderr to build/sim_logs/.
# TODO: tail combined logical-device events and clean up on Ctrl-C.
