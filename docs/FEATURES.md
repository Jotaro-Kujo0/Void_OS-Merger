# Void_OS-Merger — feature inventory

This living catalogue tracks the current master/worker logical-device scaffold
and future capabilities for heterogeneous devices and users.

Legend:

- ✅ = structure or scaffold exists;
- 🟡 = designed in TODOs but not implemented;
- ❌ = not yet scaffolded or intentionally postponed.

## 1. Base logical-device features

### 1.1 Protocol

| Feature | Status |
| --- | --- |
| Cap'n Proto as the master/worker wire contract | 🟡 comment-only schema scaffold |
| CMake-driven binding generation | ✅ `protocol/CMakeLists.txt` |
| Versioned `ClusterMessage` envelope | 🟡 `protocol/cluster.capnp` |
| Worker join and approval | 🟡 protocol TODOs |
| Worker heartbeat and health reporting | 🟡 protocol TODOs |
| Workload and chunk descriptions | 🟡 protocol TODOs |
| Chunk assignment, lease, progress, result, and cancellation | 🟡 protocol TODOs |
| Recovery and stale-message handling | 🟡 protocol TODOs |
| Optional worker UI-surface messages | 🟡 protocol TODOs |
| Worker platform/runtime/capability enums | 🟡 protocol TODOs |

### 1.2 Shared support (`common/`)

| Feature | Status |
| --- | --- |
| Stable ID vocabulary | 🟡 `common/include/common/ids.h` |
| Resource capacity and reservation vocabulary | 🟡 `common/include/common/resources.h` |
| Shared domain vocabulary | 🟡 `common/include/common/domain.h` |
| Shared result/error vocabulary | 🟡 `common/include/common/result.h` |
| Process-wide ZeroMQ context planning | 🟡 `common/zmq_helpers.{h,c}` |
| Worker identity and master ROUTER addressing | 🟡 `common/zmq_helpers.h` |
| Transport authentication/encryption planning | 🟡 `docs/SECURITY.md` |
| CPU and architecture probing | 🟡 `common/sys_info.{h,c}` |
| Memory and storage probing | 🟡 `common/sys_info.{h,c}` |
| Battery, charging, docking, and thermal probing | 🟡 `common/sys_info.{h,c}` |
| Display and input capability probing | 🟡 `common/sys_info.{h,c}` |
| Network capability and locality probing | 🟡 `common/sys_info.{h,c}` |
| Configuration loading | 🟡 `common/config.{h,c}` |
| Leveled shared logging | 🟡 `common/log.{h,c}` |

### 1.3 Master (`master/`)

| Feature | Status |
| --- | --- |
| Logical-device state ownership | 🟡 `master/cluster_state.{h,c}` |
| Worker membership and health registry | 🟡 `master/registry.{h,c}` |
| Workload lifecycle | 🟡 `master/workload.{h,c}` |
| Workload-to-chunk planning | 🟡 `master/chunk_planner.{h,c}` |
| Dependency graph management | 🟡 `master/chunk_planner.{h,c}` |
| Hard compatibility filtering | 🟡 `master/scheduler.{h,c}` |
| Proportional capacity balancing | 🟡 `master/scheduler.{h,c}` |
| Harmony scoring using projected cluster state | 🟡 `docs/SCHEDULING.md` |
| Resource reservations and assignment leases | 🟡 `docs/SCHEDULING.md` |
| Failed-worker chunk recovery/reassignment | 🟡 `master/recovery.{h,c}` |
| Master ROUTER transport boundary | 🟡 `master/router.{h,c}` |
| Master-owned UI view model | 🟡 `master/ui_coordinator.{h,c}` |
| Master argument parsing | 🟡 `master/cli.{h,c}` |
| Graceful shutdown and persistence planning | 🟡 `master/src/main.c`, `docs/FAILURE_RECOVERY.md` |

### 1.4 Worker (`worker/`)

| Feature | Status |
| --- | --- |
| Canonical worker runtime boundary | ✅ `worker/` |
| Worker lifecycle state | 🟡 `worker/worker_state.{h,c}` |
| Worker capability reporting | 🟡 `worker/capabilities.{h,c}` |
| Dynamic resource monitoring | 🟡 `worker/resource_monitor.{h,c}` |
| Worker DEALER transport | 🟡 `worker/transport.{h,c}` |
| Master endpoint discovery | 🟡 `worker/discovery.{h,c}` |
| Worker heartbeat subsystem | 🟡 `worker/heartbeat.{h,c}` |
| Workload chunk execution | 🟡 `worker/executor.{h,c}` |
| Local resource reservation/enforcement | 🟡 `worker/executor.{h,c}` |
| Chunk progress and result reporting | 🟡 `worker/executor.{h,c}` |
| Optional display/input surfaces | 🟡 `worker/display_bridge.{h,c}` |
| Worker drain, cancellation, and reconnect | 🟡 `worker/recovery.{h,c}` |
| Exactly one worker entrypoint | 🟡 `worker/src/main.c` |

### 1.5 Master control and UI

| Feature | Status |
| --- | --- |
| Logical-device status | 🟡 `cli/src/main.c`, `ui/` |
| Worker health and resource status | 🟡 `ui/model.h` |
| Workload and chunk progress | 🟡 `ui/model.h` |
| Workload submission | 🟡 CLI command scaffold |
| Workload inspection and cancellation | 🟡 CLI command scaffold |
| Worker approval, drain, and resume | 🟡 CLI command scaffold |
| Worker UI-mode control | 🟡 CLI/protocol planning |
| Structured JSON output | 🟡 UI/CLI TODOs |
| Accessibility-friendly output | 🟡 UI/CLI TODOs |
| Future graphical master UI | ❌ view-model boundary first |

### 1.6 Operational tooling and tests

| Feature | Status |
| --- | --- |
| Protocol-generation helper | 🟡 `scripts/gen_proto.sh` |
| Test runner | 🟡 `scripts/run_tests.sh` |
| Master/worker loopback simulation | 🟡 `scripts/sim_cluster.sh` |
| Shared logging tests | 🟡 `tests/test_common_log.c` |
| Protocol round-trip tests | 🟡 `tests/test_capnp_roundtrip.c` |
| Chunk scheduling tests | 🟡 `tests/test_scheduler_logic.c` |
| Worker join integration test | ❌ planned |
| Worker failure/reassignment test | ❌ planned |
| Master restart reconciliation test | ❌ planned |
| Security/authorization test | ❌ planned |

## 2. Device compatibility roadmap

### 2.1 Planned worker platform classes

| Worker class | Status | Purpose |
| --- | --- | --- |
| Standard Linux | 🟡 first platform candidate | Desktop/server computation |
| Android tablet | 🟡 NDK path exists | Mobile and detachable computation |
| Hybrid convertible | ❌ planned | Dock/undock-aware workers |
| Low-power IoT | ❌ planned | Small ARM and gateway workers |
| ChromeOS | ❌ planned | Chromebook/tablet workers |
| Apple Silicon | ❌ planned | Requires a separate platform runtime |
| Apple Intel | ❌ planned | Requires a separate platform runtime |
| Windows tablet | ❌ planned | Requires Win32 worker runtime |
| Steam Deck/handheld PC | ❌ planned | Portable Linux workers |
| Smart TV | ❌ planned | WebOS/Tizen/Android TV workers |
| RISC-V board | ❌ planned | RISC-V-compatible runtime |
| WASI runtime | ❌ planned | Portable sandboxed worker |
| Browser tab | ❌ planned | WebAssembly/WebSocket worker |

A worker class must not be added to the protocol until its runtime and
capability semantics are understood.

### 2.2 Capability probes

| Capability | Status |
| --- | --- |
| CPU architecture and instruction features | 🟡 planned |
| CPU effective capacity | 🟡 planned |
| Memory capacity and pressure | 🟡 planned |
| Storage free space and throughput | 🟡 planned |
| GPU/CUDA/Vulkan/Metal/ROCm | ❌ planned |
| NPU/edge accelerator | ❌ planned |
| Webcam and microphone | ❌ planned |
| Touch and multi-touch | 🟡 planned |
| Stylus/pen | ❌ planned |
| Keyboard, pointer, and gamepad | 🟡 planned |
| Display count, resolution, and connection | 🟡 planned |
| Audio input/output | ❌ planned |
| Cellular modem and signal quality | ❌ planned |
| Bluetooth radio | ❌ planned |
| GPS/location sensor | ❌ planned |
| Battery, charging, docking, and thermal state | 🟡 planned |
| Network interfaces, latency, and locality | 🟡 planned |

## 3. Transport and discovery roadmap

| Capability | Status |
| --- | --- |
| Master ROUTER / worker DEALER topology | 🟡 planned |
| Authenticated transport | 🟡 security requirement |
| Encrypted transport | 🟡 security requirement |
| Multicast master discovery | ❌ planned |
| mDNS/DNS-SD discovery | ❌ planned |
| QR endpoint handoff | ❌ planned |
| NFC join handoff | ❌ planned |
| Bluetooth discovery | ❌ planned |
| Wi-Fi Direct | ❌ planned |
| QUIC transport | ❌ planned |
| WebRTC transport | ❌ planned |
| WebSocket bridge | ❌ planned |
| Shared memory/vsock transport | ❌ planned |
| USB tether transport | ❌ planned |
| SSH tunnel transport | ❌ planned |
| Cloud relay | ❌ planned |

Version one should stabilize one authenticated LAN transport before adding
alternate transports.

## 4. Distribution roadmap

| Channel | Status |
| --- | --- |
| Linux source/CMake build | ✅ scaffolded |
| Debian package | ❌ planned |
| RPM package | ❌ planned |
| Arch/AUR package | ❌ planned |
| Alpine package | ❌ planned |
| Snap/Flatpak | ❌ planned |
| Android APK/NDK worker | ❌ planned |
| Windows package | ❌ planned |
| macOS package | ❌ planned |
| Docker image/Compose | ❌ planned |
| Kubernetes/Helm deployment | ❌ planned |
| Browser/PWA worker | ❌ planned |

## 5. Protocol language bindings

The protocol is a contract for future workers and control clients beyond C.

| Binding | Status |
| --- | --- |
| C | 🟡 current planned binding |
| Rust | ❌ planned |
| Go | ❌ planned |
| Python | ❌ planned |
| TypeScript/WebAssembly | ❌ planned |
| Swift | ❌ planned |
| Kotlin | ❌ planned |
| Zig | ❌ planned |

## 6. Logical-device and accessibility roadmap

### 6.1 Logical-device features

| Feature | Status |
| --- | --- |
| Aggregate resource dashboard | 🟡 planned |
| Worker contribution view | 🟡 planned |
| Chunk progress visualization | 🟡 planned |
| Worker display status mode | 🟡 planned |
| Secondary worker UI surface | ❌ planned |
| Shared virtual display | ❌ planned |
| Input forwarding | ❌ planned |
| Keyboard/touch/pen routing | ❌ planned |
| Clipboard synchronization | ❌ planned |
| Drag-and-drop between surfaces | ❌ planned |
| Audio output/input routing | ❌ planned |
| Follow-user UI policy | ❌ planned |
| Cluster-aware settings | ❌ planned |

### 6.2 Accessibility for operators

| Feature | Status |
| --- | --- |
| Keyboard-only CLI operation | 🟡 planned |
| Screen-reader-friendly output | 🟡 planned |
| High-contrast output | 🟡 planned |
| Color-independent status indicators | 🟡 planned |
| Structured JSON output | 🟡 planned |
| Plain-language status explanations | 🟡 planned |
| Scheduling explanation mode | ❌ planned |
| Exportable audit log | ❌ planned |
| Internationalization | ❌ planned |
| Locale-aware timestamps/numbers | ❌ planned |
| Right-to-left presentation support | ❌ planned |
| Sonification/haptic feedback | ❌ planned |
| Voice control hooks | ❌ planned |

## 7. Intentionally postponed from the base

- Arbitrary native process-memory transfer;
- automatic splitting of unknown programs;
- master failover and leader election;
- every operating system and architecture at once;
- alternate transports before the first transport is secure and stable;
- full shared-desktop compositing;
- treating a worker display as the primary user interface;
- GPU/NPU scheduling before the first workload runtime is defined;
- broad packaging and language bindings before the protocol is stable.
