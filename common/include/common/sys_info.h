#ifndef VOM_COMMON_SYS_INFO_H
#define VOM_COMMON_SYS_INFO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VOM_CPU_FEAT_AVX2    (1 << 0)
#define VOM_CPU_FEAT_NEON    (1 << 1)
#define VOM_CPU_FEAT_AES_NI  (1 << 2)

typedef enum {
    VOM_ARCH_X86_64,
    VOM_ARCH_AARCH64,
    VOM_ARCH_ARMV7,
    VOM_ARCH_UNKNOWN
} VomCpuArch;

typedef enum {
    VOM_LINK_ETH,
    VOM_LINK_WIFI,
    VOM_LINK_CELLULAR,
    VOM_LINK_UNKNOWN
} VomLinkType;

typedef struct {
    VomCpuArch arch;
    uint32_t cores_online;
    uint32_t cores_total;
    uint32_t feature_bitmap;
    
    uint64_t ram_total_mb;
    uint64_t ram_free_mb;
    
    int32_t battery_percent; 
    bool is_charging;
    bool is_ac_online;
    
    bool display_attached;
    uint32_t display_width;
    uint32_t display_height;
    bool touch_supported;
    
    VomLinkType network_link_type;
    uint64_t network_bandwidth_kbps;
} vom_worker_capabilities;

bool vom_sys_info_collect(vom_worker_capabilities *out);
void vom_sys_info_to_capnp(const vom_worker_capabilities *in, void *out_proto_struct_ptr);

#ifdef __cplusplus
}
#endif

#endif /* VOM_COMMON_SYS_INFO_H */
