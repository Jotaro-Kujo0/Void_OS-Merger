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

 #ifndef VOM_WORKER_CAPABILITIES_H
 #define VOM_WORKER_CAPABILITIES_H

 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>

 #define CAP_STRING_MAX 64

 typedef enum {
    CAP_ARCH_X86_64,
    CAP_ARCH_AARCH64,
    CAP_ARCH_ARMV7,
    CAP_ARCH_RISCV64
} VomCapArchitecture;

typedef enum {
    CAP_ABI_SYSV,
    CAP_ABI_EABI,
    CAP_ABI_MUSL,
    CAP_ABI_WIN64
} VomCapAbi;

typedef enum {
    CAP_RUN_NATIVE,
    CAP_RUN_WASM,
    CAP_RUN_CONTAINER
} VomCapRuntime;

typedef enum {
    CAP_ACCEL_NONE,
    CAP_ACCEL_CUDA,
    CAP_ACCEL_OPENCL,
    CAP_ACCEL_METAL,
    CAP_ACCEL_NPU
} VomCapAccelerator;

typedef enum {
    CAP_THERMAL_NOMINAL,
    CAP_THERMAL_THROTTLED,
    CAP_THERMAL_CRITICAL
} VomCapThermal;

typedef struct {
    uint32_t instruction_set_extensions_bitmask;
    bool has_hardware_virtualization;
    bool has_secure_sandbox_support;
} vom_cpu_features_t;

typedef struct {
    uint32_t resolution_width;
    uint32_t resolution_height;
    uint32_t refresh_rate_hz;
    bool touch_input_supported;
    bool keyboard_attached;
    bool pointer_attached;
} vom_peripheral_spec_t;

typedef struct {
    uint16_t capability_protocol_version;
    VomCapArchitecture architecture;
    VomCapAbi abi;
    VomCapRuntime supported_runtimes_bitmask;
    vom_cpu_features_t cpu_extensions;
    uint32_t physical_cores_total;
    uint64_t physical_ram_bytes_total;
    uint64_t storage_capacity_bytes_total;
    VomCapAccelerator accelerator_type;
    uint64_t accelerator_memory_bytes_total;
    uint32_t display_count;
    vom_peripheral_spec_t primary_display;
    char hardware_model_fingerprint[CAP_STRING_MAX];
} vom_worker_static_capabilities_t;

typedef struct {
    uint64_t timestamp_ms;
    uint32_t physical_cores_currently_online;
    uint64_t physical_ram_bytes_currently_free;
    uint64_t storage_bytes_currently_free;
    uint64_t accelerator_memory_bytes_currently_free;
    uint64_t current_network_link_bandwidth_bps;
    uint32_t current_network_latency_us;
    float battery_charge_percentage;
    bool is_charging;
    bool is_docked;
    VomCapThermal thermal_state;
    uint32_t active_local_reservations_count;
} vom_worker_dynamic_resources_t;

typedef struct {
    vom_worker_static_capabilities_t static_profile;
    vom_worker_dynamic_resources_t dynamic_telemetry;
} vom_worker_capabilities_snapshot_t;

typedef struct vom_capabilities_manager vom_capabilities_manager_t;

vom_capabilities_manager_t* vom_capabilities_manager_create(void);
void vom_capabilities_manager_destroy(vom_capabilities_manager_t *ctx);

int32_t vom_capabilities_collect_static(vom_capabilities_manager_t *ctx, vom_worker_static_capabilities_t *out_static);
int32_t vom_capabilities_collect_dynamic(vom_capabilities_manager_t *ctx, vom_worker_dynamic_resources_t *out_dynamic);

int32_t vom_capabilities_build_join_request(vom_capabilities_manager_t *ctx, uint8_t *out_payload_buffer, size_t max_bytes);
int32_t vom_capabilities_build_heartbeat(vom_capabilities_manager_t *ctx, uint8_t *out_payload_buffer, size_t max_bytes);

#endif
