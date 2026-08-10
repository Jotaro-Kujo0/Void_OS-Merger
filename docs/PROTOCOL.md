# Void_OS-Merger protocol plan

`protocol/cluster.capnp` is the planned single source of truth for messages
between the master, workers, and the operator control surface.

## Baseline topology

```text
cluster-worker  -- DEALER -->  cluster-master -- ROUTER
vom-cli         -- control API --> cluster-master
```

The exact CLI/control transport is still a TODO, but it must not bypass
master-owned workload and worker state.

## Message families

| Direction | Family | Purpose |
| --- | --- | --- |
| worker → master | `workerJoin` | Register identity and static capabilities |
| master → worker | `workerJoinAck` | Approve membership and establish epoch |
| worker → master | `workerHeartbeat` | Report dynamic resources and health |
| worker → master | `workerLeave` | Graceful departure |
| master → worker | `chunkAssign` | Assign a ready workload chunk with a lease |
| worker → master | `chunkAccepted` | Confirm local validation and reservation |
| worker → master | `chunkProgress` | Report bounded execution progress |
| worker → master | `chunkResult` | Return verified result metadata/output |
| master → worker | `chunkCancel` | Revoke or cancel an assignment |
| master ↔ worker | `recovery` | Reconcile leases and safe chunk retry |
| master ↔ worker | `uiSurface` | Advertise and control optional display/input surfaces |

## Envelope requirements

Every message should eventually carry:

- protocol version;
- message ID;
- request/correlation ID where applicable;
- sender identity;
- logical-device/master epoch;
- timestamp and freshness information;
- typed payload or stable error information.

## Capability and resource data

Workers should advertise and periodically update:

- architecture, platform, ABI, and supported runtime;
- CPU and memory capacity/availability;
- accelerator capabilities;
- storage and network information;
- battery and thermal state;
- display and input surfaces;
- active chunk reservations.

## Workload and chunk data

A workload is the complete user operation. A chunk is the independently
schedulable unit. Chunk messages must include enough information to validate
compatibility, dependencies, resource demand, input references, lease state,
retry policy, and result integrity.

## Recovery rules

The protocol must support stale-message rejection. A result from an expired
lease must not overwrite a newer assignment. Duplicate result delivery should
be harmless, and safe unfinished chunks should be reassignable after worker
failure.
