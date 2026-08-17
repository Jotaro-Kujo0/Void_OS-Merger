#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <sys/time.h>

#define VOM_HEARTBEAT_INTERVAL_MS 5000 
#define REG_NAME_MAX 64

/* --- Complete Typedef Definitions --- */
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

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CpuTicks;

/* --- Private Embedded Capability Tracking State --- */
static vom_worker_capabilities g_cached_caps;
static pthread_mutex_t g_caps_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_probe_thread;
static volatile bool g_probe_thread_active = false;

/* --- Capabilities Discovery Engine Logic --- */
static bool vom_sys_info_collect(vom_worker_capabilities *out) {
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

static bool vom_caps_init(void) {
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

static vom_worker_capabilities vom_caps_snapshot(void) {
    vom_worker_capabilities snapshot;
    pthread_mutex_lock(&g_caps_mutex);
    snapshot = g_cached_caps;
    pthread_mutex_unlock(&g_caps_mutex);
    return snapshot;
}

static void vom_caps_deinit(void) {
    if (g_probe_thread_active) {
        g_probe_thread_active = false;
        pthread_join(g_probe_thread, NULL);
    }
}

/* --- Proc Stat Sampling & Metric Calculations --- */
static bool sample_cpu_ticks(CpuTicks* ticks) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return false;
    char label[16];
    int read_count = fscanf(f, "%15s %llu %llu %llu %llu %llu %llu %llu %llu",
                            label, &ticks->user, &ticks->nice, &ticks->system,
                            &ticks->idle, &ticks->iowait, &ticks->irq, &ticks->softirq, &ticks->steal);
    fclose(f);
    return (read_count == 9);
}

static double calculate_cpu_percentage(const CpuTicks* prev, const CpuTicks* curr) {
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;

    unsigned long long prev_non_idle = prev->user + prev->nice + prev->system + prev->irq + prev->softirq + prev->steal;
    unsigned long long curr_non_idle = curr->user + curr->nice + curr->system + curr->irq + curr->softirq + curr->steal;

    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;

    if (curr_total <= prev_total) return 0.0;
    
    unsigned long long total_delta = curr_total - prev_total;
    unsigned long long idle_delta = curr_idle - prev_idle;

    return ((double)(total_delta - idle_delta) / (double)total_delta) * 100.0;
}

static void vom_heartbeat_emit(double cpu_pct) {
    vom_worker_capabilities caps = vom_caps_snapshot();
    printf("[HEARTBEAT OUT] Telemetry -> CPU Usage: %3.1f%% | Free RAM: %llu MB\n", 
           cpu_pct, (unsigned long long)caps.ram_free_mb);
}

/* --- Main Non-Blocking Cooperative Event Loop --- */
int main(int argc, char const *argv[]) {
    (void)argc; (void)argv;
    
    if (!vom_caps_init()) {
        fprintf(stderr, "Failed to initialize hardware capabilities background scheduler.\n");
        return 1;
    }

    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;

    CpuTicks prev_ticks, curr_ticks;
    sample_cpu_ticks(&prev_ticks);

    struct timeval last_heartbeat_time, current_time;
    gettimeofday(&last_heartbeat_time, NULL);

    printf("Agent heartbeat manager operational. Monitoring standard input...\n");
    int running = 1;

    while (running) {
        gettimeofday(&current_time, NULL);
        long long elapsed_ms = (current_time.tv_sec - last_heartbeat_time.tv_sec) * 1000 +
                               (current_time.tv_usec - last_heartbeat_time.tv_usec) / 1000;

        int timeout_ms = VOM_HEARTBEAT_INTERVAL_MS - (int)elapsed_ms;
        if (timeout_ms < 0) timeout_ms = 0;

        int poll_result = poll(&fds, 1, timeout_ms);

        if (poll_result == -1) {
            perror("poll");
            break;
        } else if (poll_result == 0) {
            if (sample_cpu_ticks(&curr_ticks)) {
                double cpu_pct = calculate_cpu_percentage(&prev_ticks, &curr_ticks);
                vom_heartbeat_emit(cpu_pct);
                prev_ticks = curr_ticks;
            } else {
                vom_heartbeat_emit(0.0);
            }
            gettimeofday(&last_heartbeat_time, NULL);
        } else {
            if (fds.revents & POLLIN) {
                char buf[32];
                ssize_t length = read(STDIN_FILENO, buf, sizeof(buf) - 1);
                if (length > 0) {
                    buf[length] = '\0';
                    printf("Terminal shutdown directive parsed: %s\n", buf);
                    running = 0;
                }
            }
        }
    }

    vom_caps_deinit();
    printf("Agent deallocation sequence completed cleanly.\n");
    return 0;
}
