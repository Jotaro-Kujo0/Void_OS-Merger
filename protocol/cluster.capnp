@0xdeadbeefcafebabe;   # TODO: replace with a stable ID before schema freeze.

# Void_OS-Merger master/worker logical-device protocol plan.
#
# This file is intentionally still a comment-only schema scaffold. It records
# the names and relationships that must be approved before real Cap'n Proto
# declarations are added.
#
# Every message should eventually carry a version, message ID, correlation ID,
# sender identity, master epoch, timestamp, and typed payload.

# TODO: define MessageCode values for:
#   workerJoin, workerJoinAck, workerHeartbeat, workerLeave,
#   workloadSubmit, workloadAccepted, workloadRejected, workloadStatus,
#   chunkAssign, chunkAccepted, chunkProgress, chunkResult, chunkFailed,
#   chunkCancel, recovery, uiSurface, error.
#
# TODO: define WorkerPlatform/WorkerRuntime/ChunkState/WorkloadState enums.
# TODO: define WorkerCapabilities with architecture, resources, accelerators,
#       network, battery, thermal, display, input, and supported runtimes.
# TODO: define WorkerHeartbeat with timestamped available resources,
#       reservations, active chunk summaries, and health.
# TODO: define WorkloadDescription with workload ID, session, kind, input,
#       output policy, priority, deadline, retry policy, and planner version.
# TODO: define WorkChunk with chunk ID, workload ID, sequence, dependencies,
#       input reference, resource demand, compatibility requirements, and
#       idempotency/lease metadata.
# TODO: define ChunkAssign, ChunkProgress, ChunkResult, ChunkCancel, and Error.
# TODO: define optional UISurface data for worker displays and input devices.
# TODO: define ClusterMessage with a union of all payloads above.

# Comment-only future shape:
#
# struct ClusterMessage {
#   protocolVersion @0 :UInt16;
#   messageId @1 :Text;
#   correlationId @2 :Text;
#   senderId @3 :Text;
#   masterEpoch @4 :UInt64;
#   timestampMs @5 :UInt64;
#   payload :union { ... };
# }
