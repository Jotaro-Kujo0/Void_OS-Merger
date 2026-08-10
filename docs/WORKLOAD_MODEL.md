# Workload model

The scheduler must not treat every operation as an unnamed unit. The base
model has two levels:

```text
Workload
└── Chunk 0
└── Chunk 1
└── Chunk 2
    └── dependencies, requirements, input, progress, result
```

## Workload

A workload is a complete user-requested operation. It owns the user-visible
lifecycle and the final output. A workload may contain one chunk or many
chunks.

TODO:

- Define a stable workload ID.
- Associate the workload with a user/session ID.
- Record workload kind and version.
- Record input and output references.
- Record priority, deadline, and retry policy.
- Record whether chunks can execute concurrently.
- Record whether output ordering matters.
- Record whether the workload supports retry or checkpoint recovery.
- Record whether an optional display or input surface is required.

## Chunk

A chunk is an independently assignable unit of work. It must have enough
metadata for the master to decide whether a worker can execute it and enough
metadata for the worker to validate and run it.

TODO:

- Define a stable chunk ID and parent workload ID.
- Define sequence/order information.
- Define input range, object reference, or payload reference.
- Define dependency chunk IDs.
- Define estimated CPU, memory, storage, and network use.
- Define runtime and architecture requirements.
- Define optional display/input requirements.
- Define result reference and integrity metadata.
- Define lease, timeout, retry, and idempotency metadata.

## State transitions

```text
created -> planned -> ready -> leased -> running
running -> completed
running -> failed
running -> cancelled
leased  -> expired -> ready
failed  -> retrying -> ready
```

TODO:

- Define which transitions are master-authoritative.
- Define which worker messages are observations versus state commits.
- Reject stale results from expired leases.
- Make duplicate result delivery harmless.
- Define when a workload becomes complete.

## Comment-only C model

```c
/*
 * Illustrative planning types only; these are not declarations yet.
 *
 * struct vom_workload_plan {
 *     char workload_id[64];
 *     char session_id[64];
 *     enum vom_workload_kind kind;
 *     enum vom_workload_state state;
 *     uint32_t chunk_count;
 *     bool chunks_parallel;
 *     bool ordered_output;
 *     bool retryable;
 * };
 *
 * struct vom_chunk_plan {
 *     char chunk_id[64];
 *     char workload_id[64];
 *     uint32_t sequence_index;
 *     struct vom_resource_request resources;
 *     struct vom_capability_requirements requirements;
 *     uint64_t input_offset;
 *     uint64_t input_length;
 *     uint32_t retry_count;
 *     uint64_t lease_expiry_ms;
 * };
 */
```

## Important constraint

A generic byte stream is not automatically splittable. Each workload must
provide an explicit plan or use a registered planner for a known workload kind.
For example, image batches, independent records, and matrix row blocks may be
splittable. An opaque interactive process may not be.
