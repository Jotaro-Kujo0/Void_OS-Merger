# Worker runtime

`worker/` is the canonical source tree for computation-focused nodes in the
Void_OS-Merger logical device. The worker connects to the master, advertises
capabilities, receives workload chunks, executes them, and reports progress and
results. It may also expose optional display or input surfaces.

Canonical names:

- source tree: `worker/`;
- executable: `cluster-worker`;
- CMake target: `cluster-worker`;
- default endpoint macro: `VOM_WORKER_DEFAULT_MASTER`;
- public vocabulary: worker, workload, chunk, recovery.

There is one canonical worker implementation. New code and documentation must
not introduce alternate role names or duplicate worker trees.

TODO:

- Choose and document the first workload runtime.
- Add platform-specific worker implementations behind build options.
- Keep exactly one worker `main()` function.
