# Compatibility plan

Universal device support requires explicit compatibility metadata. A worker
must not receive a chunk merely because it has an open socket and spare CPU.

TODO:

- Define CPU architecture and operating-system identifiers.
- Define supported workload runtimes.
- Define instruction-set and ABI requirements.
- Define accelerator types and versions.
- Define memory and storage requirements.
- Define display and input requirements.
- Define network and data-locality requirements.
- Define battery and thermal policy.
- Define worker software/protocol version compatibility.
- Define capability freshness and heartbeat timestamps.

## Runtime decision

The project must choose what a chunk executes. Candidate approaches include:

- native binaries on compatible architectures;
- containers with compatible images;
- WebAssembly or another portable runtime;
- registered application-specific chunk processors;
- explicit remote jobs that may restart rather than transfer process memory.

The first base should select one approach instead of pretending every native
workload can run everywhere.

```c
/*
 * Comment-only requirement example:
 *
 * struct vom_capability_requirements {
 *     enum vom_architecture architecture;
 *     enum vom_workload_runtime runtime;
 *     uint32_t cpu_units;
 *     uint64_t memory_mb;
 *     bool requires_gpu;
 *     bool requires_display;
 *     bool requires_touch;
 * };
 */
```
