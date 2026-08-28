#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <common/compat.h>

#define REG_NAME_MAX 64

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

/* --- Private Agent Capabilities State --- */
static vom_worker_capabilities g_cached_caps;
static pthread_mutex_t g_caps_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_probe_thread;
static volatile bool g_probe_thread_active = false;

/* Mock internal implementation of common/sys_info hardware discovery layer */
bool vom_sys_info_collect(vom_worker_capabilities *out) {
    if (!out) return false;
    out->arch = VOM_ARCH_X86_64;
    out->cores_online = 8;
    out->cores_total = 8;
    out->ram_total_mb = 16384;
    out->ram_free_mb = 8192;
    out->battery_percent = 95;
    out->is_charging = true;
    out->is_ac_online = true;
    out->display_attached = true;
    out->display_width = 1920;
    out->display_height = 1080;
    out->touch_supported = true;
    out->network_link_type = VOM_LINK_ETH;
    out->network_bandwidth_kbps = 1000000;
    return true;
}

/* Background worker thread that scans hardware profiles every 30 seconds for hot-plugs */
static void* vom_caps_refresh_worker_loop(void* arg) {
    (void)arg;
    while (g_probe_thread_active) {
        vom_worker_capabilities fresh_caps;
        if (vom_sys_info_collect(&fresh_caps)) {
            pthread_mutex_lock(&g_caps_mutex);
            g_cached_caps = fresh_caps;
            pthread_mutex_unlock(&g_caps_mutex);
        }
        for (int i = 0; i < 30 && g_probe_thread_active; i++) {
            sleep(1);
        }
    }
    return NULL;
}

bool vom_caps_init(void) {
    pthread_mutex_lock(&g_caps_mutex);
    if (g_probe_thread_active) {
        pthread_mutex_unlock(&g_caps_mutex);
        return true;
    }
    vom_sys_info_collect(&g_cached_caps);
    g_probe_thread_active = true;
    pthread_mutex_unlock(&g_caps_mutex);

    if (pthread_create(&g_probe_thread, NULL, vom_caps_refresh_worker_loop, NULL) != 0) {
        g_probe_thread_active = false;
        return false;
    }
    return true;
}

vom_worker_capabilities vom_caps_snapshot(void) {
    vom_worker_capabilities snapshot;
    pthread_mutex_lock(&g_caps_mutex);
    snapshot = g_cached_caps;
    pthread_mutex_unlock(&g_caps_mutex);
    return snapshot;
}

void vom_caps_build_join_req(const char* agent_id, void* out_proto_struct_ptr) {
    if (!agent_id || !out_proto_struct_ptr) return;
    vom_worker_capabilities caps = vom_caps_snapshot();
    printf("[CAPNP BUILDER] Packing JoinReq -> Agent ID: %s | Core Count: %u | Total Memory: %llu MB\n",
           agent_id, caps.cores_online, (unsigned long long)caps.ram_total_mb);
}

void vom_caps_deinit(void) {
    if (g_probe_thread_active) {
        g_probe_thread_active = false;
        pthread_join(g_probe_thread, NULL);
    }
}
