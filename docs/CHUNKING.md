# Chunking plan

Chunking converts a complete workload into a graph of smaller units that can
be assigned to different workers.

## Planner responsibilities

TODO:

- Validate that the workload kind has a known splitting strategy.
- Choose fixed-size, input-partition, or workload-specific chunks.
- Estimate the cost of every chunk.
- Record dependencies before scheduling.
- Preserve ordering when output requires it.
- Avoid chunks smaller than their transfer and startup overhead.
- Avoid chunks so large that one worker becomes a long-running bottleneck.
- Produce deterministic chunk IDs for reproducible retries.
- Mark workloads that cannot be split instead of guessing.

## Initial strategies

1. **Explicit plan** — the caller supplies chunk boundaries and requirements.
2. **Fixed-size partition** — the planner divides a byte/object range into
   bounded pieces.
3. **Known workload planner** — a registered planner understands a particular
   operation, such as image batches or matrix blocks.

Automatic arbitrary-program splitting is not part of the base system.

## Dependency graph

```text
chunk-A ───────┐
               ├── chunk-C ─── chunk-E
chunk-B ───────┘

chunk-D ───────────────────────┘
```

TODO:

- Keep a chunk ready only when every dependency is complete.
- Reject dependency cycles during planning.
- Preserve dependency output references.
- Recompute downstream chunks when an upstream result is invalidated.
- Let independent branches execute in parallel.

## Chunk sizing feedback

The first plan may use estimates. The master should later record measured
execution time and use it to improve future chunk sizes.

```c
/*
 * Comment-only example:
 *
 * measured_rate = completed_units / elapsed_seconds;
 *
 * If a worker completes its chunk much faster than predicted, future plans
 * may give that worker larger chunks. If transfer overhead dominates, future
 * plans may create fewer, larger chunks or prefer local data placement.
 */
```

## Result aggregation

TODO:

- Define how chunk outputs are identified.
- Verify result checksums or lengths.
- Combine ordered results in sequence order.
- Combine unordered results by a stable chunk key.
- Detect missing, duplicate, or conflicting outputs.
- Publish workload completion only after aggregation succeeds.
