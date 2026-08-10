# Base architecture roadmap

This roadmap is ordered around stable master/worker contracts rather than
individual implementation files.

## Phase 1: vocabulary and contracts

- [x] Define logical device, master, worker, workload, chunk, resource, and UI
      surface.
- [ ] Choose the first workload runtime.
- [ ] Define version-one non-goals.
- [ ] Keep `worker/` as the sole canonical worker source tree.
- [ ] Remove all temporary compatibility names before the first stable release.

## Phase 2: protocol

- [ ] Assign a stable Cap'n Proto file ID.
- [ ] Define worker join/leave and heartbeat messages.
- [ ] Define capability and resource snapshots.
- [ ] Define workload and chunk messages.
- [ ] Define progress, result, cancellation, error, and lease messages.
- [ ] Add protocol versioning and round-trip tests.

## Phase 3: master state

- [ ] Define logical-device state transitions.
- [ ] Implement worker membership and health state.
- [ ] Implement workload/chunk state ownership.
- [ ] Add persistence requirements before recovery work.

## Phase 4: worker base

- [ ] Collect static capabilities.
- [ ] Collect dynamic resources.
- [ ] Connect using the chosen authenticated transport.
- [ ] Accept and validate chunk assignments.
- [ ] Report progress and results.

## Phase 5: planner and scheduler

- [ ] Build explicit or registered chunk planners.
- [ ] Validate dependencies.
- [ ] Apply hard compatibility filters.
- [ ] Calculate proportional capacity and harmony scores.
- [ ] Create reservations and leases.
- [ ] Requeue safe chunks after worker failure.

## Phase 6: user experience

- [ ] Expose a master-owned logical-device view model.
- [ ] Implement the first CLI or local UI.
- [ ] Add workload and chunk progress.
- [ ] Add worker policy controls.
- [ ] Add optional worker display modes.

## Phase 7: validation

- [ ] One master + one worker join test.
- [ ] Multiple heterogeneous worker test.
- [ ] Chunk proportionality test.
- [ ] Compatibility rejection test.
- [ ] Worker failure and reassignment test.
- [ ] Duplicate/stale result test.
- [ ] Master restart reconciliation test.
- [ ] Security and authorization test.
