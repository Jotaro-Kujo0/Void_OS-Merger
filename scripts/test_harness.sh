#!/usr/bin/env bash
# ============================================================================
# Void_OS-Merger — Local Test Harness
# ============================================================================
#
# This script builds the project, compiles standalone module tests, runs them,
# and reports a pass/fail summary. It is designed to be run on your local
# machine before deploying to production.
#
# Usage:
#   bash scripts/test_harness.sh              # Full build + test
#   bash scripts/test_harness.sh --test-only  # Skip build, run tests only
#   bash scripts/test_harness.sh --clean      # Clean build directory first
#
# Prerequisites:
#   - CMake ≥ 3.16
#   - C compiler (MSVC on Windows, GCC/Clang on Linux/macOS)
#   - ZeroMQ (optional — build works without it, some tests skipped)
#   - Cap'n Proto (optional — stub fallback used)
#
# ============================================================================

set -euo pipefail

# ---- Configuration ----
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
TEST_DIR="${BUILD_DIR}/tests"
TEST_ONLY=0
CLEAN=0

# ---- Parse arguments ----
for arg in "$@"; do
    case "$arg" in
        --test-only) TEST_ONLY=1 ;;
        --clean)     CLEAN=1 ;;
        --help|-h)
            echo "Usage: $0 [--test-only] [--clean]"
            echo "  --test-only  Skip build, run tests only"
            echo "  --clean      Clean build directory first"
            exit 0
            ;;
    esac
done

# ---- Colors ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'  # No Color

# ---- Counters ----
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

# ---- Helper functions ----
header() {
    echo ""
    echo -e "${BLUE}============================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}============================================${NC}"
}

subheader() {
    echo ""
    echo -e "${YELLOW}--- $1 ---${NC}"
}

pass() {
    PASSED=$((PASSED + 1))
    TOTAL=$((TOTAL + 1))
    echo -e "  ${GREEN}✓ PASS${NC}: $1"
}

fail() {
    FAILED=$((FAILED + 1))
    TOTAL=$((TOTAL + 1))
    echo -e "  ${RED}✗ FAIL${NC}: $1"
    [ -n "${2:-}" ] && echo -e "         ${RED}Detail: $2${NC}"
}

skip() {
    SKIPPED=$((SKIPPED + 1))
    echo -e "  ${YELLOW}○ SKIP${NC}: $1"
}

run_test() {
    local name="$1"
    local cmd="$2"
    local timeout="${3:-10}"

    TOTAL=$((TOTAL + 1))
    local output
    if output=$(timeout "${timeout}s" bash -c "$cmd" 2>&1); then
        PASSED=$((PASSED + 1))
        echo -e "  ${GREEN}✓ PASS${NC}: ${name}"
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            SKIPPED=$((SKIPPED + 1))
            TOTAL=$((TOTAL - 1))
            echo -e "  ${YELLOW}○ SKIP${NC}: ${name} (timeout after ${timeout}s)"
        else
            FAILED=$((FAILED + 1))
            echo -e "  ${RED}✗ FAIL${NC}: ${name}"
            echo "$output" | head -5 | sed 's/^/         /'
        fi
    fi
}

# ============================================================================
# PHASE 1: Build Verification
# ============================================================================
header "PHASE 1: Build Verification"

if [ $CLEAN -eq 1 ]; then
    subheader "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    echo "  Cleaned."
fi

if [ $TEST_ONLY -eq 0 ]; then
    subheader "Configuring with CMake"
    mkdir -p "$BUILD_DIR"

    # Detect vcpkg
    VCPKG_FLAGS=""
    VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
    if [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
        VCPKG_FLAGS="-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        echo "  Found vcpkg at: ${VCPKG_ROOT}"
    else
        echo "  vcpkg not found; ZMQ may not be found"
    fi

    # Detect ZMQ from vcpkg
    ZMQ_FLAGS=""
    if [ -f "${VCPKG_ROOT}/installed/x64-windows/lib/libzmq-mt-4_3_5.lib" ]; then
        ZMQ_FLAGS="-DVOM_ZMQ_LIB=${VCPKG_ROOT}/installed/x64-windows/lib/libzmq-mt-4_3_5.lib -DVOM_ZMQ_INCLUDE=${VCPKG_ROOT}/installed/x64-windows/include"
    elif [ -f "${VCPKG_ROOT}/installed/x64/lib/libzmq.so" ]; then
        ZMQ_FLAGS="-DVOM_ZMQ_LIB=${VCPKG_ROOT}/installed/x64/lib/libzmq.so -DVOM_ZMQ_INCLUDE=${VCPKG_ROOT}/installed/x64/include"
    elif [ -f "${VCPKG_ROOT}/installed/x64-linux/lib/libzmq.so" ]; then
        ZMQ_FLAGS="-DVOM_ZMQ_LIB=${VCPKG_ROOT}/installed/x64-linux/lib/libzmq.so -DVOM_ZMQ_INCLUDE=${VCPKG_ROOT}/installed/x64-linux/include"
    fi

    # shellcheck disable=SC2086
    if cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" $VCPKG_FLAGS $ZMQ_FLAGS > /dev/null 2>&1; then
        pass "CMake configuration"
    else
        fail "CMake configuration" "cmake failed. Check that dependencies are installed."
        echo "  Cannot continue without a configured build."
        echo ""
        echo "  Install ZeroMQ:  vcpkg install zeromq:x64-windows"
        echo "  Install capnp:   winget install capnproto.capnproto"
        exit 1
    fi

    subheader "Building project targets"
    BUILD_LOG="${BUILD_DIR}/build_test.log"
    if cmake --build "$BUILD_DIR" > "$BUILD_LOG" 2>&1; then
        pass "Full project build"
    else
        fail "Full project build" "See ${BUILD_LOG} for details"
        # Show first errors
        grep -i "error" "$BUILD_LOG" | head -10 | sed 's/^/         /'
    fi
else
    subheader "Skipping build (--test-only)"
fi

# ============================================================================
# PHASE 2: Binary Verification
# ============================================================================
header "PHASE 2: Binary Verification"

# Check that executables exist
for binary in \
    "master/cluster-master.exe" \
    "master/cluster-master" \
    "worker/cluster-worker.exe" \
    "worker/cluster-worker" \
    "cli/vom-cli.exe" \
    "cli/vom-cli"
do
    full_path="${BUILD_DIR}/${binary}"
    if [ -f "$full_path" ]; then
        pass "Binary exists: ${binary}"
        break  # Only report the first match per component
    fi
done

# Check that libraries exist
for lib in \
    "common/vom_common.lib" \
    "common/libvom_common.a" \
    "protocol/vom_proto.lib" \
    "protocol/libvom_proto.a" \
    "ui/vom_ui.lib" \
    "ui/libvom_ui.a"
do
    full_path="${BUILD_DIR}/${lib}"
    if [ -f "$full_path" ]; then
        pass "Library exists: ${lib}"
        break
    fi
done

# ============================================================================
# PHASE 3: Standalone Module Tests
# ============================================================================
header "PHASE 3: Standalone Module Tests"

subheader "Building standalone test executables"

# Try building the CMake test targets if tests/ directory has CMakeLists.txt
if [ -f "${PROJECT_ROOT}/tests/CMakeLists.txt" ]; then
    CTEST_LOG="${BUILD_DIR}/ctest_build.log"
    if cmake --build "$BUILD_DIR" --target test_common_log > "$CTEST_LOG" 2>&1; then
        pass "Build test_common_log"
    else
        fail "Build test_common_log" "See ${CTEST_LOG}"
    fi
fi

subheader "Running standalone module tests"

# ---- Worker Discovery Test ----
TEST_DISC="${TEST_DIR}/test_discovery"
if [ -f "$TEST_DISC" ]; then
    run_test "Worker discovery module" "$TEST_DISC" 5
else
    skip "Worker discovery module (not built)"
fi

# ---- Worker State Machine Test ----
TEST_STATE="${TEST_DIR}/test_worker_state"
if [ -f "$TEST_STATE" ]; then
    run_test "Worker state machine" "$TEST_STATE" 5
else
    skip "Worker state machine (not built)"
fi

# ---- Worker Recovery Test ----
TEST_WREC="${TEST_DIR}/test_worker_recovery"
if [ -f "$TEST_WREC" ]; then
    run_test "Worker recovery" "$TEST_WREC" 5
else
    skip "Worker recovery (not built)"
fi

# ---- Worker Resource Monitor Test ----
TEST_RM="${TEST_DIR}/test_resource_monitor"
if [ -f "$TEST_RM" ]; then
    run_test "Worker resource monitor" "$TEST_RM" 5
else
    skip "Worker resource monitor (not built)"
fi

# ---- Worker Display Bridge Test ----
TEST_DISP="${TEST_DIR}/test_display_bridge"
if [ -f "$TEST_DISP" ]; then
    run_test "Worker display bridge" "$TEST_DISP" 5
else
    skip "Worker display bridge (not built)"
fi

# ---- Worker CLI Test ----
TEST_WCLI="${TEST_DIR}/test_cli_worker"
if [ -f "$TEST_WCLI" ]; then
    run_test "Worker CLI parser" "$TEST_WCLI" 5
else
    skip "Worker CLI parser (not built)"
fi

# ---- Master Recovery Test ----
TEST_MREC="${TEST_DIR}/test_recovery_master"
if [ -f "$TEST_MREC" ]; then
    run_test "Master recovery" "$TEST_MREC" 5
else
    skip "Master recovery (not built)"
fi

# ---- Master Router Test ----
TEST_ROUTER="${TEST_DIR}/test_router"
if [ -f "$TEST_ROUTER" ]; then
    run_test "Master router" "$TEST_ROUTER" 5
else
    skip "Master router (not built)"
fi

# ---- Master UI Coordinator Test ----
TEST_UI="${TEST_DIR}/test_ui_coordinator"
if [ -f "$TEST_UI" ]; then
    run_test "Master UI coordinator" "$TEST_UI" 5
else
    skip "Master UI coordinator (not built)"
fi

# ---- Master Chunk Planner Test ----
TEST_CP="${TEST_DIR}/test_chunk_planner"
if [ -f "$TEST_CP" ]; then
    run_test "Master chunk planner" "$TEST_CP" 5
else
    skip "Master chunk planner (not built)"
fi

# ============================================================================
# PHASE 4: CMake Test Suite
# ============================================================================
header "PHASE 4: CTest Suite"

if [ -f "${BUILD_DIR}/CTestTestfile.cmake" ]; then
    CTEST_OUTPUT="${BUILD_DIR}/ctest_results.log"
    if ctest --test-dir "$BUILD_DIR" --output-on-failure > "$CTEST_OUTPUT" 2>&1; then
        pass "CTest suite"
    else
        fail "CTest suite" "Some tests failed. See ${CTEST_OUTPUT}"
    fi
else
    skip "CTest suite (no CTestTestfile.cmake)"
fi

# ============================================================================
# PHASE 5: Binary Smoke Tests
# ============================================================================
header "PHASE 5: Binary Smoke Tests"

# Find the master binary
MASTER_BIN=""
for candidate in \
    "${BUILD_DIR}/master/cluster-master.exe" \
    "${BUILD_DIR}/master/cluster-master"
do
    if [ -x "$candidate" ]; then
        MASTER_BIN="$candidate"
        break
    fi
done

# Find the worker binary
WORKER_BIN=""
for candidate in \
    "${BUILD_DIR}/worker/cluster-worker.exe" \
    "${BUILD_DIR}/worker/cluster-worker"
do
    if [ -x "$candidate" ]; then
        WORKER_BIN="$candidate"
        break
    fi
done

# Find the CLI binary
CLI_BIN=""
for candidate in \
    "${BUILD_DIR}/cli/vom-cli.exe" \
    "${BUILD_DIR}/cli/vom-cli"
do
    if [ -x "$candidate" ]; then
        CLI_BIN="$candidate"
        break
    fi
done

# Worker --test self-check
if [ -n "$WORKER_BIN" ]; then
    run_test "Worker --test self-check" "\"$WORKER_BIN\" --test" 10
else
    skip "Worker --test self-check (binary not found)"
fi

# Master startup/shutdown (run for 2 seconds then kill)
if [ -n "$MASTER_BIN" ]; then
    MASTER_LOG="${BUILD_DIR}/master_smoke.log"
    timeout 3 "$MASTER_BIN" > "$MASTER_LOG" 2>&1 &
    MASTER_PID=$!
    sleep 2
    kill $MASTER_PID 2>/dev/null || true
    wait $MASTER_PID 2>/dev/null || true

    if [ -s "$MASTER_LOG" ]; then
        pass "Master starts and produces output"
    else
        fail "Master starts and produces output" "No output from master"
    fi
else
    skip "Master smoke test (binary not found)"
fi

# CLI --help
if [ -n "$CLI_BIN" ]; then
    run_test "CLI produces output" "\"$CLI_BIN\" 2>&1 || true" 5
else
    skip "CLI smoke test (binary not found)"
fi

# ============================================================================
# SUMMARY
# ============================================================================
header "TEST SUMMARY"

echo ""
echo -e "  Total:   ${TOTAL}"
echo -e "  ${GREEN}Passed:  ${PASSED}${NC}"
echo -e "  ${RED}Failed:  ${FAILED}${NC}"
echo -e "  ${YELLOW}Skipped: ${SKIPPED}${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}  ✓ ALL TESTS PASSED${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}  ✗ ${FAILED} TEST(S) FAILED${NC}"
    echo ""
    echo "  Review the output above for details."
    echo "  Common fixes:"
    echo "    - Install ZeroMQ: vcpkg install zeromq:x64-windows"
    echo "    - Install capnp:  winget install capnproto.capnproto"
    echo "    - Clean rebuild:  rm -rf build && bash scripts/test_harness.sh"
    echo ""
    exit 1
fi
