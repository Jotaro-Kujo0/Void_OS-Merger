# Void_OS-Merger architecture

Void_OS-Merger is a universal logical-device scaffold. A master node controls a
cluster of heterogeneous worker nodes so the collection can be presented as
one user-controlled computer.

## Topology

- **cluster-master** — the coordinator and primary control plane. It owns the
  logical-device state, worker registry, workload/chunk planner, scheduler,
  recovery decisions, and master UI view model.
- **cluster-worker** — a computation-focused participant. It advertises local
  capabilities, reports resources, executes assigned chunks, and may expose
  optional display or input surfaces.
- **vom-cli** — an operator client of the master control plane.

The first transport plan is a worker DEALER session connected to the master
ROUTER session. The CLI/control path must be separately documented and
authenticated before remote execution is enabled.

## Authority model

The master is authoritative for:

- logical-device identity and state;
- worker membership, health, and approval;
- workload and chunk state;
- chunk ownership, reservations, and leases;
- aggregate capacity and scheduling decisions;
- result aggregation and recovery;
- user-visible status and UI policy.

Workers are authoritative only for local observations and local execution:

- hardware capabilities;
- current resource measurements;
- local chunk execution state;
- progress and result production;
- optional display/input availability.

## Directory map

| Path | Purpose |
| --- | --- |
| `protocol/` | Versioned master/worker logical-device message contract |
| `common/` | Shared IDs, resources, logging, transport, config, and probes |
| `master/` | Logical-device control plane, planning, scheduling, recovery |
| `worker/` | Computation-focused worker runtime |
| `ui/` | Presentation-neutral master UI model |
| `cli/` | Operator control client |
| `tests/` | Unit and integration test scaffold |
| `scripts/` | Protocol, test, and local simulation helpers |
| `docs/` | Objective and design contracts |

## Workload lifecycle

```text
user request
    -> master validates workload
    -> chunk planner builds a dependency graph
    -> ready chunks enter the scheduler
    -> scheduler filters compatible workers
    -> scheduler creates reservations and leases
    -> worker executes assigned chunk
    -> worker reports progress/result
    -> master verifies and aggregates output
    -> master publishes logical-device/workload state
```

## Failure behavior

A worker timeout does not automatically mean the whole workload failed. The
master should expire affected leases, preserve committed results, and requeue
only chunks whose retry policy makes that safe. Portable checkpoint support may
be added for compatible runtimes; arbitrary native process-memory transfer is
not part of the base architecture.

## Naming policy

Use `master`, `worker`, `logical device`, `workload`, `chunk`, and `recovery` in
new code and documentation. The canonical binary names are `cluster-master`
and `cluster-worker`.
