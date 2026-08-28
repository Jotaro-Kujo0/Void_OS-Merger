#!/usr/bin/env bash
# webui/start.sh — Launch master + web dashboard
#
# Usage:
#   bash webui/start.sh              # Start both
#   bash webui/start.sh --web-only   # Dashboard only (master must be running)
#   bash webui/start.sh --master-only # Master only

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
WEB_PORT="${WEB_PORT:-8080}"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

WEB_ONLY=0
MASTER_ONLY=0

for arg in "$@"; do
    case "$arg" in
        --web-only) WEB_ONLY=1 ;;
        --master-only) MASTER_ONLY=1 ;;
        --help|-h)
            echo "Usage: $0 [--web-only|--master-only]"
            echo "  --web-only     Start dashboard only (master must be running)"
            echo "  --master-only  Start master only"
            echo "  (default)      Start both master + dashboard"
            exit 0
            ;;
    esac
done

# Find binaries
MASTER_BIN=""
WORKER_BIN=""
for candidate in "${BUILD_DIR}/master/cluster-master.exe" "${BUILD_DIR}/master/cluster-master"; do
    [ -x "$candidate" ] && MASTER_BIN="$candidate" && break
done

# Check Python
if ! command -v python &>/dev/null; then
    echo -e "${RED}ERROR: Python not found. Install Python 3.10+${NC}"
    exit 1
fi

# Check websockets
if ! python -c "import websockets" 2>/dev/null; then
    echo -e "${RED}ERROR: websockets not installed. Run: pip install websockets${NC}"
    exit 1
fi

cleanup() {
    echo ""
    echo -e "${BLUE}Shutting down...${NC}"
    [ -n "${MASTER_PID:-}" ] && kill "$MASTER_PID" 2>/dev/null
    [ -n "${WEB_PID:-}" ] && kill "$WEB_PID" 2>/dev/null
    wait 2>/dev/null
    echo -e "${GREEN}Clean shutdown.${NC}"
}
trap cleanup EXIT INT TERM

# Start master
if [ "$WEB_ONLY" -eq 0 ]; then
    if [ -z "$MASTER_BIN" ]; then
        echo -e "${RED}ERROR: cluster-master not found in ${BUILD_DIR}/${NC}"
        echo "Build first: cmake --build build"
        exit 1
    fi

    echo -e "${GREEN}Starting cluster-master...${NC}"
    "$MASTER_BIN" &
    MASTER_PID=$!
    sleep 1

    if ! kill -0 "$MASTER_PID" 2>/dev/null; then
        echo -e "${RED}ERROR: Master failed to start${NC}"
        exit 1
    fi
    echo -e "${GREEN}  Master running (PID: $MASTER_PID)${NC}"
fi

# Start web dashboard
if [ "$MASTER_ONLY" -eq 0 ]; then
    echo -e "${GREEN}Starting web dashboard on port ${WEB_PORT}...${NC}"
    python "${PROJECT_ROOT}/webui/server.py" --port "$WEB_PORT" --wal "${BUILD_DIR}/master_cluster.wal" &
    WEB_PID=$!
    sleep 1

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Dashboard: http://localhost:${WEB_PORT}${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Press Ctrl+C to stop everything."
fi

# Wait
wait
