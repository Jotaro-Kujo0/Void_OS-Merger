# Protocol scaffold

`cluster.capnp` is the planned versioned wire contract for the master,
workers, and the master control surface.

Required message families:

```text
membership:
  workerJoin, workerJoinAck, workerHeartbeat, workerLeave

workload:
  workloadSubmit, workloadAccepted, workloadRejected, workloadStatus,
  workloadCancel

chunk:
  chunkAssign, chunkAccepted, chunkStarted, chunkProgress, chunkResult,
  chunkFailed, chunkCancel, chunkLeaseExpired

ui:
  uiSurfaceAdvertise, uiSurfaceChanged, uiModeChanged, inputEvent

control:
  workerDrain, workerResume, workerApprove, workerRemove, protocolError
```

TODO:

- Replace the comment-only schema with real versioned declarations.
- Assign a stable Cap'n Proto file ID.
- Preserve append-only field numbering.
- Define message/request IDs, timestamps, master epochs, and sender identity.
- Define size limits and error behavior.
- Add schema round-trip and compatibility tests.
