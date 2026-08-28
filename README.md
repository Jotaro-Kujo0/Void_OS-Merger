# Void_OS-Merger

Void_OS-Merger is a scaffold for a universal logical computer built from
multiple connected devices.

- **Master** — the primary control node. It owns logical-device state, user
  control, worker membership, workload planning, scheduling, recovery, and the
  primary UI view model.
- **Worker** — a computation-focused node. It contributes CPU, memory,
  accelerators, storage, network, displays, or input capabilities and may
  expose optional UI surfaces.
- **Workload** — a complete user-requested operation.
- **Chunk** — a schedulable subdivision of a workload.

The master divides workloads into compatible chunks and distributes them
according to worker capacity, availability, requirements, data locality, and
the balance of the complete chunk set.

## Status

Architecture scaffold. The repository contains canonical master/worker,
logical-device, workload, chunk, UI, security, compatibility, and recovery
planning boundaries. Most source files still contain TODOs and explanatory
comments rather than complete implementations.

See `docs/OBJECTIVE.md` for the agreed objective and `docs/ROADMAP.md` for the
planned implementation order.

## Layout

```text
protocol/  — versioned master/worker logical-device message contract
common/    — shared IDs, resources, logging, transport, config, and probes
master/    — logical-device control plane and scheduler
worker/    — computation-focused worker runtime
ui/        — presentation-neutral master UI view model
cli/       — operator client for the master
tests/     — unit and integration test scaffold
docs/      — objective, architecture, workload, chunking, UI, security, and recovery
scripts/   — protocol, testing, and local simulation helpers
```

## Quick start

```bash
make            # build the current configured targets
make worker     # build cluster-worker
make master     # build cluster-master
make test       # run the worker self-test
make sim        # simulate one master and multiple workers (when implemented)
```

## Naming policy

Use only `master`, `worker`, `logical device`, `workload`, `chunk`, and
`recovery` for the corresponding architectural concepts. The canonical binary names are `cluster-master` and `cluster-worker`.

## Web Dashboard

The web dashboard (`webui/`) provides a browser-based cluster terminal.

### Local development

```bash
pip install websockets
python webui/server.py
# Open http://localhost:8080
```

### Deploy to Netlify

1. Push to GitHub/GitLab/Bitbucket
2. Go to [app.netlify.com](https://app.netlify.com)
3. Import your repository
4. Set **Publish directory:** `webui`
5. Deploy

The dashboard works in two modes:
- **Local:** Python server reads the master's WAL file for live cluster state
- **Netlify:** Serverless functions provide mock data for demo/testing