# Void_OS-Merger objective

This document is the current product and architecture baseline.

## Objective

Build a universal logical computer from multiple connected physical devices.
One node is the **master**: it owns the user's primary control surface, the
cluster-wide state, and the decisions that make the devices behave as one
system. The other nodes are **workers**: they primarily contribute compute,
storage, network, display, or input capabilities and may expose optional UI
features when the master enables them.

The master divides a user workload into smaller, schedulable chunks. It assigns
compatible chunks across the workers according to measured capacity, current
load, resource requirements, data locality, and the balance of the complete
chunk set. Equal work does not mean an equal number of chunks: a stronger node
may receive a larger share while weaker or battery-limited nodes receive less.

## Version-one interpretation

- The master is the authoritative coordinator.
- Workers do not make global scheduling decisions.
- A workload is the complete user operation.
- A chunk is the smallest independently schedulable unit.
- Workers report capabilities and dynamic resource state.
- The first implementation should use explicit chunk plans or known workload
  planners; arbitrary programs must not be split blindly.
- Failed or disconnected workers cause unfinished chunks to be retried or
  reassigned when the workload permits it.
- Native process-memory transfer across unlike architectures is not a
  version-one requirement.
- Worker displays are optional capabilities, not proof that a worker owns the
  primary user interface.

## Authority model

The master owns the following global facts:

- logical-device identity and state;
- worker membership and health;
- workload and chunk state;
- chunk ownership and leases;
- aggregate resource summaries;
- user-visible status and policy;
- recovery and reassignment decisions.

A worker owns local facts and local actions:

- hardware capability inspection;
- local resource monitoring;
- accepted chunk execution;
- local output and progress production;
- local display/input availability;
- safe local shutdown and cancellation.

## Comment-only implementation direction

```c
/*
 * The first executable path should eventually follow this shape:
 *
 *   master_start()
 *       -> load configuration
 *       -> initialize logical-device state
 *       -> accept worker joins
 *       -> receive workload descriptions
 *       -> create a chunk graph
 *       -> schedule ready chunks
 *       -> aggregate worker progress/results
 *       -> publish a logical-device UI snapshot
 *
 *   worker_start()
 *       -> inspect local capabilities
 *       -> connect to the master
 *       -> send a join request
 *       -> report heartbeats
 *       -> receive and execute chunks
 *       -> return progress/results
 */
```

## Current non-goals

The following are deliberately outside the first base:

- transparent transfer of arbitrary native process memory;
- leaderless master election;
- automatic splitting of any unknown executable;
- every operating system and architecture at once;
- every alternate transport;
- full shared-desktop compositing;
- treating every worker display as a primary display.
