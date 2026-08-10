// common/sys_info.h — discover the hardware capabilities of the device
// the worker is running on so it can advertise them to the master.
// TODO: probe and cache:
//   * CPU cores (online + offline), architecture (x86_64 / aarch64 / armv7),
//     and a stable feature flag bitmap (AVX2, NEON, AES-NI, ...).
//   * Total + free RAM in MB (read /proc/meminfo on Linux, sysinfo elsewhere).
//   * Battery percent + charging state on devices that expose one.
//   * /sys/class/power_supply AC state for laptops in dock/undocked mode.
//   * Display presence + resolution (handle detach on hybrids like the
//     Acer Iconia W510 that can lose the keyboard half).
//   * Touch input (so the master can route pointer/keyboard events).
//   * Available network interfaces + link type (wifi / eth / cellular),
//     so the master can pick the lowest-latency peer for chunk recovery or data movement.

#ifndef VOM_COMMON_SYS_INFO_H
#define VOM_COMMON_SYS_INFO_H

#include <stdint.h>

// TODO: declare struct vom_worker_capabilities (mirrors the worker capabilities payload in the protocol).
// TODO: declare vom_sys_info_collect(struct vom_worker_capabilities *out).
// TODO: declare vom_sys_info_to_capnp(struct vom_worker_capabilities *in,
//                                     WorkerCapabilities *out_proto).

#endif /* VOM_COMMON_SYS_INFO_H */
