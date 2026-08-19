#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

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
    CAP_RUN_NATIVE = 1,
    CAP_RUN_WASM = 2,
    CAP_RUN_CONTAINER = 4
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
    uint32_t supported_runtimes_bitmask;
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

struct vom_capabilities_manager {
    int platform_fixture_id;
    vom_worker_static_capabilities_t cached_static;
};

typedef struct vom_capabilities_manager vom_capabilities_manager_t;

vom_capabilities_manager_t* vom_capabilities_manager_create(void) {
    vom_capabilities_manager_t *ctx = (vom_capabilities_manager_t*)calloc(1, sizeof(vom_capabilities_manager_t));
    if (!ctx) return NULL;
    ctx->platform_fixture_id = 0; 
    return ctx;
}

void vom_capabilities_manager_destroy(vom_capabilities_manager_t *ctx) {
    if (ctx) free(ctx);
}

void vom_capabilities_manager_set_fixture(vom_capabilities_manager_t *ctx, int fixture_id) {
    if (ctx) ctx->platform_fixture_id = fixture_id;
}

int32_t vom_capabilities_collect_static(vom_capabilities_manager_t *ctx, vom_worker_static_capabilities_t *out_static) {
    if (!ctx || !out_static) return -1;
    
    memset(out_static, 0, sizeof(vom_worker_static_capabilities_t));
    out_static->capability_protocol_version = 1;

    if (ctx->platform_fixture_id == 0) {
        out_static->architecture = CAP_ARCH_X86_64;
        out_static->abi = CAP_ABI_SYSV;
        out_static->supported_runtimes_bitmask = CAP_RUN_NATIVE | CAP_RUN_WASM | CAP_RUN_CONTAINER;
        out_static->physical_cores_total = 16;
        out_static->physical_ram_bytes_total = 34359738368ULL;
        out_static->storage_capacity_bytes_total = 1000204886016ULL;
        out_static->accelerator_type = CAP_ACCEL_CUDA;
        out_static->accelerator_memory_bytes_total = 17179869184ULL;
        out_static->display_count = 2;
        out_static->primary_display.resolution_width = 3840;
        out_static->primary_display.resolution_height = 2160;
        out_static->primary_display.refresh_rate_hz = 144;
        out_static->primary_display.touch_input_supported = false;
        strncpy(out_static->hardware_model_fingerprint, "linux-workstation-x86_64", CAP_STRING_MAX - 1);
    } else if (ctx->platform_fixture_id == 1) {
        out_static->architecture = CAP_ARCH_AARCH64;
        out_static->abi = CAP_ABI_EABI;
        out_static->supported_runtimes_bitmask = CAP_RUN_NATIVE | CAP_RUN_WASM;
        out_static->physical_cores_total = 8;
        out_static->physical_ram_bytes_total = 8589934592ULL;
        out_static->storage_capacity_bytes_total = 256102424576ULL;
        out_static->accelerator_type = CAP_ACCEL_NPU;
        out_static->accelerator_memory_bytes_total = 2147483648ULL;
        out_static->display_count = 1;
        out_static->primary_display.resolution_width = 2400;
        out_static->primary_display.resolution_height = 1080;
        out_static->primary_display.refresh_rate_hz = 120;
        out_static->primary_display.touch_input_supported = true;
        strncpy(out_static->hardware_model_fingerprint, "android-mobile-arm64", CAP_STRING_MAX - 1);
    } else if (ctx->platform_fixture_id == 2) {
        out_static->architecture = CAP_ARCH_ARMV7;
        out_static->abi = CAP_ABI_EABI;
        out_static->supported_runtimes_bitmask = CAP_RUN_WASM;
        out_static->physical_cores_total = 2;
        out_static->physical_ram_bytes_total = 536870912ULL;
        out_static->storage_capacity_bytes_total = 16106127360ULL;
        out_static->accelerator_type = CAP_ACCEL_NONE;
        out_static->accelerator_memory_bytes_total = 0;
        out_static->display_count = 0;
        strncpy(out_static->hardware_model_fingerprint, "lowpower-iot-displayless", CAP_STRING_MAX - 1);
    }
    
    ctx->cached_static = *out_static;
    return 0;
}

int32_t vom_capabilities_collect_dynamic(vom_capabilities_manager_t *ctx, vom_worker_dynamic_resources_t *out_dynamic) {
    if (!ctx || !out_dynamic) return -1;
    
    memset(out_dynamic, 0, sizeof(vom_worker_dynamic_resources_t));
    out_dynamic->timestamp_ms = 1718900000ULL;
    
    if (ctx->platform_fixture_id == 0) {
        out_dynamic->physical_cores_currently_online = 16;
        out_dynamic->physical_ram_bytes_currently_free = 17179869184ULL;
        out_dynamic->storage_bytes_currently_free = 500102443008ULL;
        out_dynamic->accelerator_memory_bytes_currently_free = 8589934592ULL;
        out_dynamic->current_network_link_bandwidth_bps = 1000000000ULL;
        out_dynamic->current_network_latency_us = 150;
        out_dynamic->battery_charge_percentage = 100.0f;
        out_dynamic->is_charging = true;
        out_dynamic->is_docked = true;
        out_dynamic->thermal_state = CAP_THERMAL_NOMINAL;
    } else if (ctx->platform_fixture_id == 1) {
        out_dynamic->physical_cores_currently_online = 8;
        out_dynamic->physical_ram_bytes_currently_free = 3221225472ULL;
        out_dynamic->storage_bytes_currently_free = 64424509440ULL;
        out_dynamic->accelerator_memory_bytes_currently_free = 1073741824ULL;
        out_dynamic->current_network_link_bandwidth_bps = 150000000ULL;
        out_dynamic->current_network_latency_us = 45000;
        out_dynamic->battery_charge_percentage = 74.5f;
        out_dynamic->is_charging = false;
        out_dynamic->is_docked = false;
        out_dynamic->thermal_state = CAP_THERMAL_THROTTLED;
    } else if (ctx->platform_fixture_id == 2) {
        out_dynamic->physical_cores_currently_online = 2;
        out_dynamic->physical_ram_bytes_currently_free = 134217728ULL;
        out_dynamic->storage_bytes_currently_free = 4294967296ULL;
        out_dynamic->accelerator_memory_bytes_currently_free = 0;
        out_dynamic->current_network_link_bandwidth_bps = 10000000ULL;
        out_dynamic->current_network_latency_us = 12000;
        out_dynamic->battery_charge_percentage = 15.2f;
        out_dynamic->is_charging = false;
        out_dynamic->is_docked = false;
        out_dynamic->thermal_state = CAP_THERMAL_CRITICAL;
    }
    
    return 0;
}

int32_t vom_capabilities_build_join_request(vom_capabilities_manager_t *ctx, uint8_t *out_payload_buffer, size_t max_bytes) {
    if (!ctx || !out_payload_buffer || max_bytes < sizeof(vom_worker_static_capabilities_t)) return -1;
    
    vom_worker_static_capabilities_t current_static;
    if (vom_capabilities_collect_static(ctx, &current_static) != 0) return -1;
    
    memcpy(out_payload_buffer, &current_static, sizeof(vom_worker_static_capabilities_t));
    return (int32_t)sizeof(vom_worker_static_capabilities_t);
}

int32_t vom_capabilities_build_heartbeat(vom_capabilities_manager_t *ctx, uint8_t *out_payload_buffer, size_t max_bytes) {
    if (!ctx || !out_payload_buffer || max_bytes < sizeof(vom_worker_dynamic_resources_t)) return -1;
    
    vom_worker_dynamic_resources_t current_dynamic;
    if (vom_capabilities_collect_dynamic(ctx, &current_dynamic) != 0) return -1;
    
    memcpy(out_payload_buffer, &current_dynamic, sizeof(vom_worker_dynamic_resources_t));
    return (int32_t)sizeof(vom_worker_dynamic_resources_t);
}

void execute_capabilities_test_suite(void) {
    vom_capabilities_manager_t *mgr = vom_capabilities_manager_create();
    uint8_t buffer[sizeof(vom_worker_static_capabilities_t)];
    
    vom_capabilities_manager_set_fixture(mgr, 0);
    int32_t size_static_0 = vom_capabilities_build_join_request(mgr, buffer, sizeof(buffer));
    printf("Test Linux Join Request Generation Size: %d\n", size_static_0);
    assert(size_static_0 == sizeof(vom_worker_static_capabilities_t));
    
    vom_capabilities_manager_set_fixture(mgr, 1);
    vom_worker_dynamic_resources_t dyn_1;
    vom_capabilities_collect_dynamic(mgr, &dyn_1);
    printf("Test Android Thermal Telemetry Throttled Flag: %s\n", (dyn_1.thermal_state == CAP_THERMAL_THROTTLED) ? "PASS" : "FAIL");
    assert(dyn_1.thermal_state == CAP_THERMAL_THROTTLED);
    
    vom_capabilities_manager_set_fixture(mgr, 2);
    vom_worker_static_capabilities_t stat_2;
    vom_capabilities_collect_static(mgr, &stat_2);
}