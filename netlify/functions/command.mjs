// netlify/functions/command.mjs
// Handles all /api/command/* routes

// In-memory state for demo (persists within same function instance)
let dummyWorkers = [];
let nextDummyId = 100;

function makeDummyWorker(name) {
  const cores = [2, 4, 8, 16][Math.floor(Math.random() * 4)];
  const memOptions = [4.0, 8.0, 16.0, 32.0];
  const mem = memOptions[Math.floor(Math.random() * memOptions.length)];
  const reserved_cores = Math.floor(Math.random() * (cores / 2));
  const reserved_mem = Math.round(Math.random() * (mem / 2) * 10) / 10;

  nextDummyId++;
  return {
    id: nextDummyId,
    health: "ONLINE",
    last_seen_ms: Date.now(),
    total_memory_gb: mem,
    reserved_memory_gb: reserved_mem,
    total_cores: cores,
    reserved_cores: reserved_cores,
    dummy: true,
    name: name || `dummy-${nextDummyId}`,
  };
}

function getBaseState() {
  return {
    cluster_uuid: "vom-cluster-uuid-4f9e-a1b2-7cc892de410f",
    master_epoch: Math.floor(Date.now() / 1000),
    state_generation: 1,
    global_status: "OPERATIONAL",
    workers: [
      {
        id: 42,
        health: "ONLINE",
        last_seen_ms: Date.now(),
        total_memory_gb: 16.0,
        reserved_memory_gb: 4.0,
        total_cores: 8,
        reserved_cores: 2,
        dummy: false,
      },
      {
        id: 43,
        health: "ONLINE",
        last_seen_ms: Date.now() - 2000,
        total_memory_gb: 8.0,
        reserved_memory_gb: 2.0,
        total_cores: 4,
        reserved_cores: 1,
        dummy: false,
      },
    ],
    total_workers: 2,
    leases: [
      { chunk_id: 1001, status: "RUNNING", progress: 67.5, retry_count: 0 },
      { chunk_id: 1002, status: "RUNNING", progress: 33.2, retry_count: 0 },
      { chunk_id: 999, status: "COMPLETED", progress: 100.0, retry_count: 0 },
    ],
    total_leases: 3,
    metrics: {
      aggregate_memory_gb: 24.0,
      allocated_memory_gb: 6.0,
      aggregate_cores: 12,
      allocated_cores: 3,
      active_workers: 2,
    },
    read_time: Date.now() / 1000,
  };
}

function mergeDummyWorkers(state) {
  if (dummyWorkers.length > 0) {
    const workers = [...state.workers, ...dummyWorkers];
    state.workers = workers;
    state.total_workers = workers.length;

    const agg_cores = workers.reduce((s, w) => s + (w.total_cores || 0), 0);
    const alloc_cores = workers.reduce((s, w) => s + (w.reserved_cores || 0), 0);
    const agg_mem = workers.reduce((s, w) => s + (w.total_memory_gb || 0), 0);
    const alloc_mem = workers.reduce((s, w) => s + (w.reserved_memory_gb || 0), 0);
    const active = workers.filter((w) => w.health === "ONLINE").length;

    state.metrics = {
      aggregate_memory_gb: Math.round(agg_mem * 10) / 10,
      allocated_memory_gb: Math.round(alloc_mem * 10) / 10,
      aggregate_cores: agg_cores,
      allocated_cores: alloc_cores,
      active_workers: active,
    };
  }
  return state;
}

function response(body, status = 200) {
  return new Response(JSON.stringify(body), {
    status,
    headers: {
      "Content-Type": "application/json",
      "Access-Control-Allow-Origin": "*",
    },
  });
}

export default async (request, context) => {
  // Handle CORS preflight
  if (request.method === "OPTIONS") {
    return new Response(null, {
      status: 204,
      headers: {
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "POST, OPTIONS",
        "Access-Control-Allow-Headers": "Content-Type",
      },
    });
  }

  if (request.method !== "POST") {
    return response({ error: "Method not allowed" }, 405);
  }

  // Parse command from request body
  let body = {};
  try {
    body = await request.json();
  } catch (e) {
    // Empty body is fine
  }

  const command = body.command;

  if (!command) {
    return response({ error: "Missing command in request body" }, 400);
  }

  let state = getBaseState();
  let output = "";

  switch (command) {
    case "status":
      output = [
        `Cluster: ${state.cluster_uuid}`,
        `Status:  ${state.global_status}`,
        `Epoch:   ${state.master_epoch}`,
        `Workers: ${state.total_workers}`,
        `Leases:  ${state.total_leases}`,
        `Resources: ${state.metrics.allocated_cores}/${state.metrics.aggregate_cores} cores, ${state.metrics.allocated_memory_gb}/${state.metrics.aggregate_memory_gb} GB`,
      ].join("\n");
      break;

    case "add-worker": {
      const w = makeDummyWorker(body.name);
      dummyWorkers.push(w);
      state = mergeDummyWorkers(state);
      output = `Added dummy worker #${w.id} (${w.name}) — ${w.total_cores} cores, ${w.total_memory_gb} GB, ${w.health}`;
      break;
    }

    case "random-worker": {
      const w = makeDummyWorker();
      w.health = Math.random() > 0.3 ? "ONLINE" : "DRAINING";
      dummyWorkers.push(w);
      state = mergeDummyWorkers(state);
      output = `Added random worker #${w.id} — ${w.total_cores} cores, ${w.total_memory_gb} GB, ${w.health}`;
      break;
    }

    case "remove-worker": {
      const id = body.id || 0;
      const idx = dummyWorkers.findIndex((w) => w.id === id);
      if (idx >= 0) {
        const removed = dummyWorkers.splice(idx, 1)[0];
        output = `Removed dummy worker #${removed.id} (${removed.name})`;
      } else {
        output = `No dummy worker with id ${id} found`;
      }
      state = mergeDummyWorkers(state);
      break;
    }

    case "clear-workers": {
      const count = dummyWorkers.length;
      dummyWorkers = [];
      state = mergeDummyWorkers(state);
      output = `Cleared ${count} dummy worker(s)`;
      break;
    }

    case "submit":
      output = `Workload '${body.workload || "workload.json"}' submitted to master.\nNote: Workload submission requires master IPC.`;
      break;

    case "inspect":
      output = `Chunk #${body.id} not found in active leases.`;
      break;

    case "cancel":
      output = `Cancel request for chunk #${body.id} sent.\nNote: Cancellation requires master IPC.`;
      break;

    case "drain":
      output = `Drain request for worker #${body.worker_id} sent.\nNote: Worker drain requires master IPC.`;
      break;

    default:
      return response({ error: `Unknown command: ${command}` }, 404);
  }

  return response({ output, state });
};

export const config = {
  path: "/api/command",
  method: "POST",
};
