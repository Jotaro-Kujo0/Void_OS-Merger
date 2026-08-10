/*
 * worker/capabilities.h — worker capability planning notes.
 *
 * TODO:
 *
 * - Report architecture, platform, ABI, runtime, and instruction features.
 * - Report CPU, memory, storage, accelerator, and network capabilities.
 * - Report display resolution/count and input devices.
 * - Report battery, charging, thermal, and docking state when available.
 * - Distinguish static capabilities from values that change every heartbeat.
 * - Include capability/protocol version information.
 * - Make snapshots safe to serialize and safe for the master to cache.
 *
 * Comment-only future flow:
 *
 *   // collect_static_capabilities()
 *   // collect_dynamic_resources()
 *   // build_worker_join_request()
 *   // periodically_build_heartbeat()
 */
