# Security plan

Workers execute work selected by the master, so cluster communication and
membership cannot be treated as an unauthenticated local convenience.

TODO:

- Authenticate workers before accepting them into the logical device.
- Authenticate the master from the worker side.
- Encrypt transport traffic.
- Give every worker a stable identity and revocable credential.
- Require explicit approval for a new worker unless configured otherwise.
- Validate message sizes, identifiers, enums, and state transitions.
- Validate workload and chunk input before execution.
- Isolate worker execution with the selected runtime and resource limits.
- Prevent a worker from claiming another worker's chunk identity.
- Protect result and checkpoint data in transit and at rest.
- Define operator roles for submitting, canceling, draining, and removing.
- Record security-relevant events in an audit log.

Security must be designed before remote execution is exposed outside a trusted
single-machine simulation.

```c
/*
 * Comment-only trust flow:
 *
 *   worker -> master: join request + public identity
 *   master -> operator: pending-worker approval event
 *   operator/master -> worker: signed approval or cluster credential
 *   worker -> master: authenticated session establishment
 *   master -> worker: only then send workload chunks
 */
```
