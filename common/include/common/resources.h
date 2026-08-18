#ifndef VOM_COMMON_RESOURCES_H
#define VOM_COMMON_RESOURCES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t vom_cpu_npu_t;

typedef enum {
    VOM_ACCEL_NONE = 0,
    VOM_ACCEL_CUDA,
    VOM_ACCEL_OPENCL,
    VOM_ACCEL_METAL,
    VOM_ACCEL_NPU_DSP
} VomAcceleratorType;

typedef struct {
    VomAcceleratorType type;
    uint32_t execution_units;
    uint64_t total_memory_bytes;
} vom_accelerator_spec;

typedef struct {
    uint64_t capacity_bytes;
    uint32_t estimated_read_mbps;  
    uint32_t estimated_write_mbps;
} vom_storage_spec;

typedef struct {
    uint64_t link_bandwidth_kbps;
    uint32_t measured_latency_us;  
} vom_network_spec;

typedef enum {
    VOM_THERMAL_NOMINAL = 0,
    VOM_THERMAL_THROTTLED,
    VOM_THERMAL_CRITICAL
} VomThermalState; /* <--- Fixed semicolon syntax */

typedef struct {
    float battery_percentage;       
    bool is_charging;
    VomThermalState thermal_policy; 
} vom_environmental_policy;

typedef struct {
    vom_cpu_npu_t aggregate_cpu_npu;
    uint64_t aggregate_memory_bytes;
    vom_accelerator_spec accelerator;
    vom_storage_spec primary_storage;
    vom_network_spec baseline_network;
} vom_advertised_capacity;

typedef struct {
    vom_cpu_npu_t available_cpu_npu;
    uint64_t available_memory_bytes;
    uint64_t available_accel_memory_bytes;
    uint64_t free_storage_bytes;
    vom_network_spec current_network;
    vom_environmental_policy power_thermal;
} vom_current_availability;

typedef struct {
    vom_cpu_npu_t required_cpu_npu;
    uint64_t required_memory_bytes;
    uint64_t required_accel_memory_bytes;
    uint64_t required_storage_bytes;
    uint32_t max_acceptable_latency_us;
} vom_chunk_resource_demand;

typedef struct {
    uint64_t reservation_id;
    uint64_t chunk_id;
    vom_cpu_npu_t locked_cpu_npu;
    uint64_t locked_memory_bytes;
    uint64_t hold_expires_ms;      
    bool is_committed;
} vom_resource_reservation;

typedef struct {
    uint64_t lease_id;
    uint64_t chunk_id;
    uint64_t workload_id;
    uint64_t lease_expires_timestamp_ms;
    bool is_revoked;
} vom_assignment_resource_lease;

#ifdef __cplusplus
}
#endif

#endif /* VOM_COMMON_RESOURCES_H */
