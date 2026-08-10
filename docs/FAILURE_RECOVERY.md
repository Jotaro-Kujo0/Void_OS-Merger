# Failure and recovery plan

A logical device must remain understandable when individual physical devices
fail. The master should distinguish a worker failure from a workload failure.

## Worker failure

TODO:

- Detect stale heartbeats.
- Mark the worker unavailable before reassigning its leases.
- Identify chunks with committed results versus uncertain results.
- Requeue only chunks whose retry policy permits it.
- Avoid duplicate side effects through idempotency keys or commit protocols.
- Update the logical-device health state.
- Notify the user through the master UI.

## Network interruption

TODO:

- Use message IDs and request IDs.
- Make result delivery retryable.
- Use assignment leases with expiration.
- Reject stale messages after reassignment.
- Reconnect workers without creating duplicate identities.
- Decide whether a disconnected worker may continue locally.

## Master restart

TODO:

- Persist enough cluster state to reconstruct workload and chunk ownership.
- Define recovery of leases after restart.
- Reconcile worker-reported execution with master state.
- Preserve completed outputs.
- Mark uncertain chunks for validation or retry.

## Comment-only recovery sequence

```text
worker disappears
    -> heartbeat timeout
    -> master marks worker unavailable
    -> master finds leased chunks
    -> master validates retry policy
    -> safe chunks return to ready queue
    -> scheduler assigns them to compatible workers
    -> UI reports degraded/recovering state
```
