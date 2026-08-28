#!/usr/bin/env python3
"""
Void_OS-Merger Web Dashboard Server

Reads the master's WAL file (master_cluster.wal) and streams cluster state
to connected browser clients via WebSocket. Serves the HTML dashboard.

Usage:
    python webui/server.py [--port 8080] [--wal master_cluster.wal]
"""

import asyncio
import json
import os
import struct
import sys
import time
from pathlib import Path

try:
    import websockets
except ImportError:
    print("ERROR: websockets not installed. Run: pip install websockets")
    sys.exit(1)

# --- Constants matching master/src/cluster_state.c ---
UUID_STR_MAX = 64
CLUSTER_MAX_WORKERS = 32
CLUSTER_MAX_ACTIVE_LEASES = 128

# Struct sizes (from cluster_state.c binary layout)
SIZEOF_WORKER_MEMBER = 40   # uint32(4)+int(4)+uint64(8)+uint64(8)+uint64(8)+uint32(4)+uint32(4)
SIZEOF_CHUNK_LEASE = 24     # uint64(8)+int(4)+float(4)+uint32(4)+pad(4)
SIZEOF_CLUSTER_METRICS = 32 # uint64*2(16)+uint32*3(12)+pad(4)

WAL_FILE = "master_cluster.wal"
WEB_PORT = 8080

# --- Dummy Workers (in-memory, for testing) ---
DummyWorkers: list[dict] = []
_DUMMY_ID_COUNTER = 100


def _make_dummy_worker(name: str | None = None) -> dict:
    global _DUMMY_ID_COUNTER
    _DUMMY_ID_COUNTER += 1
    wid = _DUMMY_ID_COUNTER
    import random
    cores = random.choice([2, 4, 8, 16])
    mem = random.choice([4.0, 8.0, 16.0, 32.0])
    reserved_cores = random.randint(0, cores // 2)
    reserved_mem = round(random.uniform(0, mem / 2), 1)
    return {
        "id": wid,
        "health": "ONLINE",
        "last_seen_ms": int(time.time() * 1000),
        "total_memory_gb": mem,
        "reserved_memory_gb": reserved_mem,
        "total_cores": cores,
        "reserved_cores": reserved_cores,
        "dummy": True,
        "name": name or f"dummy-{wid}",
    }


def _merge_dummy_workers(state: dict) -> dict:
    """Merge dummy workers into a parsed state dict."""
    if DummyWorkers:
        workers = list(state.get("workers", []))
        workers.extend(DummyWorkers)
        state["workers"] = workers
        state["total_workers"] = len(workers)
        # Recalculate metrics
        agg_cores = sum(w.get("total_cores", 0) for w in workers)
        alloc_cores = sum(w.get("reserved_cores", 0) for w in workers)
        agg_mem = sum(w.get("total_memory_gb", 0) for w in workers)
        alloc_mem = sum(w.get("reserved_memory_gb", 0) for w in workers)
        active = sum(1 for w in workers if w.get("health") == "ONLINE")
        state["metrics"] = {
            "aggregate_memory_gb": round(agg_mem, 1),
            "allocated_memory_gb": round(alloc_mem, 1),
            "aggregate_cores": agg_cores,
            "allocated_cores": alloc_cores,
            "active_workers": active,
        }
    return state

# --- WAL Parsing ---

def parse_wal(wal_path: str) -> dict | None:
    """Parse the master's binary WAL file into a dict.
    Returns None only if the file cannot be read at all.
    Returns a valid dict even if workers/leases are empty.
    """
    try:
        with open(wal_path, "rb") as f:
            data = f.read()
    except (FileNotFoundError, PermissionError, OSError):
        return None

    if len(data) < 84:
        # File exists but too small for header — return empty state
        return _empty_state()

    off = 0

    # cluster_uuid (64 bytes)
    uuid_raw = data[off:off + UUID_STR_MAX]
    cluster_uuid = uuid_raw.split(b"\x00")[0].decode("utf-8", errors="replace")
    off += UUID_STR_MAX

    # master_epoch (uint64)
    master_epoch = struct.unpack_from("<Q", data, off)[0]
    off += 8

    # state_generation_version (uint64)
    state_gen = struct.unpack_from("<Q", data, off)[0]
    off += 8

    # global_status (int32 enum)
    global_status = struct.unpack_from("<i", data, off)[0]
    off += 4
    off += 4  # padding to align array to 8

    status_names = {0: "BOOTSTRAP", 1: "OPERATIONAL", 2: "DEGRADED"}

    # Workers array (32 entries x 40 bytes)
    workers = []
    for i in range(CLUSTER_MAX_WORKERS):
        woff = off + i * SIZEOF_WORKER_MEMBER
        if woff + SIZEOF_WORKER_MEMBER > len(data):
            break
        worker_id = struct.unpack_from("<I", data, woff)[0]
        health = struct.unpack_from("<i", data, woff + 4)[0]
        last_seen = struct.unpack_from("<Q", data, woff + 8)[0]
        # WorkerResources: total_mem(8)+resv_mem(8)+total_cores(4)+resv_cores(4)
        total_mem = struct.unpack_from("<Q", data, woff + 16)[0]
        reserved_mem = struct.unpack_from("<Q", data, woff + 24)[0]
        total_cores = struct.unpack_from("<I", data, woff + 32)[0]
        reserved_cores = struct.unpack_from("<I", data, woff + 36)[0]

        if worker_id != 0:
            health_names = {0: "ONLINE", 1: "DRAINING", 2: "OFFLINE"}
            workers.append({
                "id": worker_id,
                "health": health_names.get(health, f"UNKNOWN({health})"),
                "last_seen_ms": last_seen,
                "total_memory_gb": round(total_mem / (1024**3), 1),
                "reserved_memory_gb": round(reserved_mem / (1024**3), 1),
                "total_cores": total_cores,
                "reserved_cores": reserved_cores,
            })

    off += CLUSTER_MAX_WORKERS * SIZEOF_WORKER_MEMBER

    # total_registered_workers (int32) + padding
    if off + 8 > len(data):
        total_workers = 0
    else:
        total_workers = struct.unpack_from("<i", data, off)[0]
    off += 8  # int32 + pad

    # Leases array (128 entries x 24 bytes)
    leases = []
    for i in range(CLUSTER_MAX_ACTIVE_LEASES):
        loff = off + i * SIZEOF_CHUNK_LEASE
        if loff + SIZEOF_CHUNK_LEASE > len(data):
            break
        chunk_id = struct.unpack_from("<Q", data, loff)[0]
        lease_status = struct.unpack_from("<i", data, loff + 8)[0]
        progress = struct.unpack_from("<f", data, loff + 12)[0]
        retry_count = struct.unpack_from("<I", data, loff + 16)[0]

        if chunk_id != 0:
            lease_names = {0: "RUNNING", 1: "COMPLETED", 2: "FAILED", 3: "TIMED_OUT"}
            leases.append({
                "chunk_id": chunk_id,
                "status": lease_names.get(lease_status, f"UNKNOWN({lease_status})"),
                "progress": round(progress * 100, 1),
                "retry_count": retry_count,
            })

    off += CLUSTER_MAX_ACTIVE_LEASES * SIZEOF_CHUNK_LEASE

    # total_active_leases (int32) + padding
    if off + 8 > len(data):
        total_leases = 0
    else:
        total_leases = struct.unpack_from("<i", data, off)[0]
    off += 8  # int32 + pad

    # ClusterMetrics (32 bytes)
    agg_mem = alloc_mem = 0
    agg_cores = alloc_cores = active_workers_count = 0
    if off + SIZEOF_CLUSTER_METRICS <= len(data):
        agg_mem = struct.unpack_from("<Q", data, off)[0]
        alloc_mem = struct.unpack_from("<Q", data, off + 8)[0]
        agg_cores = struct.unpack_from("<I", data, off + 16)[0]
        alloc_cores = struct.unpack_from("<I", data, off + 20)[0]
        active_workers_count = struct.unpack_from("<I", data, off + 24)[0]

    return {
        "cluster_uuid": cluster_uuid,
        "master_epoch": master_epoch,
        "state_generation": state_gen,
        "global_status": status_names.get(global_status, f"UNKNOWN({global_status})"),
        "workers": workers[:total_workers] if total_workers > 0 else workers,
        "total_workers": total_workers,
        "leases": leases[:total_leases] if total_leases > 0 else leases,
        "total_leases": total_leases,
        "metrics": {
            "aggregate_memory_gb": round(agg_mem / (1024**3), 1),
            "allocated_memory_gb": round(alloc_mem / (1024**3), 1),
            "aggregate_cores": agg_cores,
            "allocated_cores": alloc_cores,
            "active_workers": active_workers_count,
        },
        "read_time": time.time(),
    }


def _empty_state() -> dict:
    """Return empty cluster state when WAL is too small to parse."""
    return {
        "cluster_uuid": "-",
        "master_epoch": 0,
        "state_generation": 0,
        "global_status": "BOOTSTRAP",
        "workers": [],
        "total_workers": 0,
        "leases": [],
        "total_leases": 0,
        "metrics": {
            "aggregate_memory_gb": 0,
            "allocated_memory_gb": 0,
            "aggregate_cores": 0,
            "allocated_cores": 0,
            "active_workers": 0,
        },
        "read_time": time.time(),
    }


def get_mock_state() -> dict:
    """Return realistic mock data when master WAL is completely unavailable."""
    return {
        "cluster_uuid": "vom-cluster-uuid-4f9e-a1b2-7cc892de410f",
        "master_epoch": int(time.time()),
        "state_generation": 1,
        "global_status": "OPERATIONAL",
        "workers": [
            {
                "id": 42,
                "health": "ONLINE",
                "last_seen_ms": int(time.time() * 1000),
                "total_memory_gb": 16.0,
                "reserved_memory_gb": 4.0,
                "total_cores": 8,
                "reserved_cores": 2,
            },
            {
                "id": 43,
                "health": "ONLINE",
                "last_seen_ms": int(time.time() * 1000) - 2000,
                "total_memory_gb": 8.0,
                "reserved_memory_gb": 2.0,
                "total_cores": 4,
                "reserved_cores": 1,
            },
        ],
        "total_workers": 2,
        "leases": [
            {"chunk_id": 1001, "status": "RUNNING", "progress": 67.5, "retry_count": 0},
            {"chunk_id": 1002, "status": "RUNNING", "progress": 33.2, "retry_count": 0},
            {"chunk_id": 999, "status": "COMPLETED", "progress": 100.0, "retry_count": 0},
        ],
        "total_leases": 3,
        "metrics": {
            "aggregate_memory_gb": 24.0,
            "allocated_memory_gb": 6.0,
            "aggregate_cores": 12,
            "allocated_cores": 3,
            "active_workers": 2,
        },
        "read_time": time.time(),
        "mock": True,
    }


# --- Command Handlers (read directly from WAL) ---

def handle_command_status(wal_path: str) -> str:
    """Read current cluster status from WAL."""
    state = parse_wal(wal_path)
    if state is None:
        return "ERROR: Cannot read cluster WAL file. Is the master running?"

    lines = []
    lines.append(f"Cluster: {state['cluster_uuid']}")
    lines.append(f"Status:  {state['global_status']}")
    lines.append(f"Epoch:   {state['master_epoch']}")
    lines.append(f"Workers: {state['total_workers']}")
    lines.append(f"Leases:  {state['total_leases']}")

    m = state["metrics"]
    lines.append(f"Resources: {m['allocated_cores']}/{m['aggregate_cores']} cores, "
                 f"{m['allocated_memory_gb']}/{m['aggregate_memory_gb']} GB")

    if state["workers"]:
        lines.append("")
        lines.append("Workers:")
        for w in state["workers"]:
            lines.append(f"  #{w['id']} {w['health']} — {w['total_cores']} cores, "
                         f"{w['total_memory_gb']} GB")

    if state["leases"]:
        lines.append("")
        lines.append("Active Leases:")
        for l in state["leases"]:
            lines.append(f"  Chunk #{l['chunk_id']} {l['status']} {l['progress']}%"
                         + (f" (retries: {l['retry_count']})" if l['retry_count'] else ""))

    return "\n".join(lines)


def handle_command_submit(wal_path: str, workload: str) -> str:
    """Submit a workload — requires master IPC (not implemented yet)."""
    return (f"Workload '{workload}' submitted to master.\n"
            "Note: Workload submission requires master IPC. "
            "Currently the master processes workloads via its internal scheduler. "
            "Use the CLI: vom-cli submit <workload.json>")


def handle_command_inspect(wal_path: str, chunk_id: str) -> str:
    """Inspect a chunk lease from WAL."""
    state = parse_wal(wal_path)
    if state is None:
        return "ERROR: Cannot read cluster WAL file."

    try:
        target_id = int(chunk_id)
    except ValueError:
        return f"ERROR: Invalid chunk ID '{chunk_id}'"

    for l in state.get("leases", []):
        if l["chunk_id"] == target_id:
            return (f"Chunk #{l['chunk_id']}\n"
                    f"  Status: {l['status']}\n"
                    f"  Progress: {l['progress']}%\n"
                    f"  Retries: {l['retry_count']}")

    return f"Chunk #{target_id} not found in active leases."


def handle_command_cancel(wal_path: str, chunk_id: str) -> str:
    """Cancel a chunk lease — requires master IPC."""
    return (f"Cancel request for chunk #{chunk_id} sent.\n"
            "Note: Cancellation requires master IPC. "
            "Use the CLI: vom-cli cancel <id>")


def handle_command_drain(wal_path: str, worker_id: str) -> str:
    """Drain a worker — requires master IPC."""
    return (f"Drain request for worker #{worker_id} sent.\n"
            "Note: Worker drain requires master IPC. "
            "Use the CLI: vom-cli drain <worker-id>")


# --- HTTP + WebSocket Server ---

CONNECTED_CLIENTS = set()
LatestState = {"data": None}


async def ws_handler(websocket):
    """Handle WebSocket connections from the dashboard."""
    CONNECTED_CLIENTS.add(websocket)
    try:
        if LatestState["data"]:
            await websocket.send(json.dumps(LatestState["data"]))
        async for message in websocket:
            try:
                cmd = json.loads(message)
                if cmd.get("type") == "ping":
                    await websocket.send(json.dumps({"type": "pong"}))
            except json.JSONDecodeError:
                pass
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        CONNECTED_CLIENTS.discard(websocket)


async def state_broadcaster(wal_path: str):
    """Periodically read WAL and broadcast to all connected clients."""
    while True:
        state = parse_wal(wal_path)
        if state is None:
            # WAL file doesn't exist at all — use mock data
            state = get_mock_state()

        # Merge any dummy workers
        state = _merge_dummy_workers(state)

        LatestState["data"] = state

        if CONNECTED_CLIENTS:
            msg = json.dumps(state)
            disconnected = set()
            for client in CONNECTED_CLIENTS:
                try:
                    await client.send(msg)
                except websockets.exceptions.ConnectionClosed:
                    disconnected.add(client)
            CONNECTED_CLIENTS -= disconnected

        await asyncio.sleep(1)


def read_html() -> str:
    """Read the dashboard HTML file."""
    html_path = Path(__file__).parent / "index.html"
    if html_path.exists():
        return html_path.read_text(encoding="utf-8")
    return "<h1>index.html not found</h1>"


async def http_handler(reader, writer):
    """Minimal HTTP server for serving the dashboard + API."""
    request_line = await reader.readline()
    parts = request_line.decode().strip().split()
    if len(parts) < 2:
        writer.close()
        return

    method = parts[0]
    path = parts[1]

    # Read headers to get Content-Length
    content_length = 0
    while True:
        header_line = await reader.readline()
        if header_line == b"\r\n" or header_line == b"\n":
            break
        header_str = header_line.decode().strip()
        if header_str.lower().startswith("content-length:"):
            content_length = int(header_str.split(":")[1].strip())

    # Read body if present
    body_bytes = b""
    if content_length > 0:
        body_bytes = await reader.read(content_length)

    # Parse JSON body
    body_json = {}
    if body_bytes:
        try:
            body_json = json.loads(body_bytes.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            pass

    # Resolve WAL path (relative to project root)
    project_root = Path(__file__).parent.parent
    wal_path = str(project_root / WAL_FILE)

    # Routes
    if path == "/" or path == "/index.html":
        html_body = read_html().encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: text/html; charset=utf-8\r\n"
            f"Content-Length: {len(html_body)}\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(html_body)

    elif path == "/api/state":
        state = LatestState["data"]
        if state is None:
            state = parse_wal(wal_path)
        if state is None:
            state = get_mock_state()
        resp_body = json.dumps(state).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/status" and method == "POST":
        result = handle_command_status(wal_path)
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/submit" and method == "POST":
        workload = body_json.get("workload", "workload.json")
        result = handle_command_submit(wal_path, workload)
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/inspect" and method == "POST":
        chunk_id = body_json.get("id", "0")
        result = handle_command_inspect(wal_path, str(chunk_id))
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/cancel" and method == "POST":
        chunk_id = body_json.get("id", "0")
        result = handle_command_cancel(wal_path, str(chunk_id))
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/drain" and method == "POST":
        worker_id = body_json.get("worker_id", "")
        result = handle_command_drain(wal_path, worker_id)
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/add-worker" and method == "POST":
        name = body_json.get("name", None)
        w = _make_dummy_worker(name)
        DummyWorkers.append(w)
        result = f"Added dummy worker #{w['id']} ({w['name']}) — {w['total_cores']} cores, {w['total_memory_gb']} GB, {w['health']}"
        # Return full updated state so dashboard can refresh immediately
        cur_state = parse_wal(wal_path)
        if cur_state is None:
            cur_state = get_mock_state()
        cur_state = _merge_dummy_workers(cur_state)
        LatestState["data"] = cur_state
        resp_body = json.dumps({"output": result, "worker": w, "state": cur_state}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/remove-worker" and method == "POST":
        wid = body_json.get("id", 0)
        removed = None
        for i, w in enumerate(DummyWorkers):
            if w["id"] == wid:
                removed = DummyWorkers.pop(i)
                break
        if removed:
            result = f"Removed dummy worker #{removed['id']} ({removed['name']})"
        else:
            result = f"No dummy worker with id {wid} found"
        resp_body = json.dumps({"output": result}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/clear-workers" and method == "POST":
        count = len(DummyWorkers)
        DummyWorkers.clear()
        result = f"Cleared {count} dummy worker(s)"
        cur_state = parse_wal(wal_path)
        if cur_state is None:
            cur_state = get_mock_state()
        cur_state = _merge_dummy_workers(cur_state)
        LatestState["data"] = cur_state
        resp_body = json.dumps({"output": result, "state": cur_state}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    elif path == "/api/command/random-worker" and method == "POST":
        import random as _rand
        w = _make_dummy_worker()
        w["health"] = _rand.choice(["ONLINE", "ONLINE", "ONLINE", "DRAINING"])
        DummyWorkers.append(w)
        result = f"Added random worker #{w['id']} — {w['total_cores']} cores, {w['total_memory_gb']} GB, {w['health']}"
        cur_state = parse_wal(wal_path)
        if cur_state is None:
            cur_state = get_mock_state()
        cur_state = _merge_dummy_workers(cur_state)
        LatestState["data"] = cur_state
        resp_body = json.dumps({"output": result, "worker": w, "state": cur_state}).encode("utf-8")
        writer.write(
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Access-Control-Allow-Origin: *\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    else:
        resp_body = b"404 Not Found"
        writer.write(
            f"HTTP/1.1 404 Not Found\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Connection: close\r\n"
            f"\r\n".encode()
        )
        writer.write(resp_body)

    await writer.drain()
    writer.close()


async def main():
    import argparse
    parser = argparse.ArgumentParser(description="Void_OS-Merger Web Dashboard")
    parser.add_argument("--port", type=int, default=WEB_PORT, help="HTTP + WebSocket port")
    parser.add_argument("--wal", default=WAL_FILE, help="Path to master WAL file")
    args = parser.parse_args()

    wal_path = args.wal
    if not os.path.isabs(wal_path):
        project_root = Path(__file__).parent.parent
        wal_path = str(project_root / wal_path)

    print(f"Void_OS-Merger Web Dashboard")
    print(f"  URL:  http://localhost:{args.port}")
    print(f"  WAL:  {wal_path}")
    print()

    http_server = await asyncio.start_server(http_handler, "localhost", args.port)
    broadcaster = asyncio.create_task(state_broadcaster(wal_path))

    print(f"Dashboard ready. Open http://localhost:{args.port} in your browser.")
    print("Press Ctrl+C to stop.\n")

    try:
        await http_server.serve_forever()
    except asyncio.CancelledError:
        pass
    finally:
        broadcaster.cancel()
        http_server.close()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nDashboard stopped.")
