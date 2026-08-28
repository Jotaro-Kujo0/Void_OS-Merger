// netlify/functions/state.mjs
// Returns cluster state for the dashboard

export default async (request, context) => {
  // Mock cluster state for demo
  const state = {
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

  return new Response(JSON.stringify(state), {
    status: 200,
    headers: {
      "Content-Type": "application/json",
      "Access-Control-Allow-Origin": "*",
    },
  });
};

export const config = {
  path: "/api/state",
  method: "GET",
};
