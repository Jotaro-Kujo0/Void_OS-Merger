#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <common/compat.h>
#ifdef _WIN32
#include <process.h>   /* Windows: waitpid, _WEXITSTATUS */
#else
#include <sys/wait.h>   /* POSIX: waitpid, WEXITSTATUS */
#endif
/* --- 1. INTEGRATED EXECUTOR CONFIGURATIONS SPECIFICATIONS ---------------- */

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

typedef enum {
    EXEC_STATE_IDLE,
    EXEC_STATE_ACCEPTED,
    EXEC_STATE_STARTED,
    EXEC_STATE_PROGRESS,
    EXEC_STATE_COMPLETED,
    EXEC_STATE_FAILED,
    EXEC_STATE_CANCELLED
} ExecState;

typedef struct {
    uint64_t chunk_id;
    uint64_t workload_id;
    char runtime_type[16];
    char binary_path[256];
    uint32_t required_cores;
    uint64_t required_memory_bytes;
    uint32_t timeout_seconds;
    bool is_idempotent;
    char expected_input_sha256[65];
} ChunkAssignment;

typedef struct {
    uint64_t chunk_id;
    ExecState state;
    float progress_percentage;
    uint32_t exit_code;
    char result_sha256[65];
    char error_message[128]; /* Retained local diagnostic buffer slot */
} ChunkExecStatus;

typedef void (*ChunkStatusCallback)(const ChunkExecStatus *status, void *user_data);

/* --- 2. LOCAL RESOURCE SCHEDULER MATRIX REGISTRY ------------------------- */

static uint32_t g_reserved_cores = 0;
static uint64_t g_reserved_memory_bytes = 0;
static pthread_mutex_t g_resource_mutex = PTHREAD_MUTEX_INITIALIZER;

#define VOM_LOG_INFO(fmt, ...)  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_WARN(fmt, ...)  fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)

static bool vom_sys_info_collect(vom_worker_capabilities *out) {
    if (!out) return false;
    out->cores_online = 8;
    out->ram_total_mb = 16384;
    return true;
}

/* ========================================================================= */
/* --- 3. EXECUTOR LIFECYCLE MANAGEMENT ENGINE ----------------------------- */
/* ========================================================================= */

bool vom_executor_validate(const ChunkAssignment *assignment, const vom_worker_capabilities *host_caps) {
    if (!assignment || !host_caps) return false;

    if (strcmp(assignment->runtime_type, "posix_subproc") != 0) {
        VOM_LOG_WARN("Validation failed: Runtime '%s' is not supported.", assignment->runtime_type);
        return false;
    }

    if (assignment->required_cores > host_caps->cores_online) {
        VOM_LOG_WARN("Validation failed: Requested cores (%u) exceed host pool (%u).", assignment->required_cores, host_caps->cores_online);
        return false;
    }

    if (assignment->required_memory_bytes > host_caps->ram_total_mb * 1024 * 1024) {
        VOM_LOG_WARN("Validation failed: Requested memory exceeds host aggregate capacity limits.");
        return false;
    }

    if (access(assignment->binary_path, X_OK) != 0) {
        VOM_LOG_WARN("Validation failed: Targeted binary path '%s' is missing or unexecutable.", assignment->binary_path);
        return false;
    }

    return true;
}

static bool reserve_local_resources(const ChunkAssignment *assignment, const vom_worker_capabilities *host_caps) {
    pthread_mutex_lock(&g_resource_mutex);
    
    if (g_reserved_cores + assignment->required_cores > host_caps->cores_online ||
        g_reserved_memory_bytes + assignment->required_memory_bytes > (host_caps->ram_total_mb * 1024 * 1024)) {
        pthread_mutex_unlock(&g_resource_mutex);
        return false; 
    }

    g_reserved_cores += assignment->required_cores;
    g_reserved_memory_bytes += assignment->required_memory_bytes;
    
    pthread_mutex_unlock(&g_resource_mutex);
    return true;
}

static void release_local_resources(const ChunkAssignment *assignment) {
    pthread_mutex_lock(&g_resource_mutex);
    
    if (g_reserved_cores >= assignment->required_cores) g_reserved_cores -= assignment->required_cores;
    if (g_reserved_memory_bytes >= assignment->required_memory_bytes) g_reserved_memory_bytes -= assignment->required_memory_bytes;
    
    pthread_mutex_unlock(&g_resource_mutex);
}

/* Simple FNV-1a based integrity hash (not cryptographic, but deterministic) */
static void compute_deterministic_integrity(const char *filepath, char *out_sha256) {
    uint64_t hash1 = 0xcbf29ce484222325ULL;
    uint64_t hash2 = 0x517cc1b727220a95ULL;
    FILE *f = fopen(filepath, "r");
    if (f) {
        int ch;
        while ((ch = fgetc(f)) != EOF) {
            hash1 ^= (uint8_t)ch;
            hash1 *= 0x100000001b3ULL;
            hash2 ^= (uint8_t)ch;
            hash2 *= 0xcbf29ce484222325ULL;
        }
        fclose(f);
    }
    snprintf(out_sha256, 65, "%016llx%016llx%016llx%016llx",
             (unsigned long long)hash1, (unsigned long long)hash2,
             (unsigned long long)(hash1 ^ hash2), (unsigned long long)(hash1 + hash2));
    out_sha256[64] = '\0';
}

bool vom_executor_run(const ChunkAssignment *assignment, ChunkStatusCallback cb, void *user_data, volatile bool *cancel_flag) {
    vom_worker_capabilities host_caps;
    vom_sys_info_collect(&host_caps);

    ChunkExecStatus status = {0};
    status.chunk_id = assignment->chunk_id;

    if (!vom_executor_validate(assignment, &host_caps)) {
        status.state = EXEC_STATE_FAILED;
        strncpy(status.error_message, "Validation failed", sizeof(status.error_message) - 1);
        if (cb) cb(&status, user_data);
        return false;
    }

    status.state = EXEC_STATE_ACCEPTED;
    if (cb) cb(&status, user_data);

    if (!reserve_local_resources(assignment, &host_caps)) {
        status.state = EXEC_STATE_FAILED;
        strncpy(status.error_message, "Resource reservation rejected", sizeof(status.error_message) - 1);
        if (cb) cb(&status, user_data);
        return false;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        release_local_resources(assignment);
        status.state = EXEC_STATE_FAILED;
        strncpy(status.error_message, "Pipe generation failed", sizeof(status.error_message) - 1);
        if (cb) cb(&status, user_data);
        return false;
    }

    status.state = EXEC_STATE_STARTED;
    if (cb) cb(&status, user_data);

    pid_t pid = fork();
    if (pid < 0) {
        close(pipe_fd[0]); close(pipe_fd[1]);
        release_local_resources(assignment);
        status.state = EXEC_STATE_FAILED;
        return false;
    }

    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO); 
        close(pipe_fd[1]);

        char *args[] = { (char*)assignment->binary_path, NULL };
        execv(args[0], args);
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[1]);
#ifndef _WIN32
    fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
#endif

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    uint32_t elapsed_seconds = 0;
    bool execution_active = true;
    char stream_buffer[128];

    status.state = EXEC_STATE_PROGRESS;
    while (execution_active) {
        gettimeofday(&current_time, NULL);
        elapsed_seconds = current_time.tv_sec - start_time.tv_sec;

        if (cancel_flag && *cancel_flag) {
            VOM_LOG_INFO("Cancellation latch triggered for Chunk ID %llu. Terminating sub-process.", assignment->chunk_id);
            kill(pid, SIGKILL);
            status.state = EXEC_STATE_CANCELLED;
            execution_active = false;
            break;
        }

        if (elapsed_seconds >= assignment->timeout_seconds) {
            VOM_LOG_WARN("Execution timeout hit (%u seconds) for Chunk ID %llu.", assignment->timeout_seconds, assignment->chunk_id);
            kill(pid, SIGKILL);
            status.state = EXEC_STATE_FAILED;
            strncpy(status.error_message, "Execution timeout exceeded", sizeof(status.error_message) - 1);
            execution_active = false;
            break;
        }

        ssize_t bytes_read = read(pipe_fd[0], stream_buffer, sizeof(stream_buffer) - 1);
        if (bytes_read > 0) {
            stream_buffer[bytes_read] = '\0';
            if (status.progress_percentage < 90.0f) status.progress_percentage += 10.0f;
            if (cb) cb(&status, user_data);
        }

        int status_flags;
        pid_t result_pid = waitpid(pid, &status_flags, WNOHANG);
        if (result_pid == pid) {
            execution_active = false;
            if (WIFEXITED(status_flags)) {
                status.exit_code = WEXITSTATUS(status_flags);
                if (status.exit_code == 0) {
                    status.state = EXEC_STATE_COMPLETED;
                    status.progress_percentage = 100.0f;
                } else {
                    status.state = EXEC_STATE_FAILED;
                    snprintf(status.error_message, sizeof(status.error_message), "Runtime error code: %d", status.exit_code);
                }
            } else {
                status.state = EXEC_STATE_FAILED;
                strncpy(status.error_message, "Sub-process terminated abnormally", sizeof(status.error_message) - 1);
            }
        } else if (result_pid < 0 && errno != ECHILD) {
            execution_active = false;
            status.state = EXEC_STATE_FAILED;
        }

        if (execution_active) {
            usleep(50000); 
        }
    }

    close(pipe_fd[0]);

    if (status.state == EXEC_STATE_COMPLETED) {
        compute_deterministic_integrity("/dev/null", status.result_sha256);
    }

    if (cb) cb(&status, user_data);
    release_local_resources(assignment);
    return (status.state == EXEC_STATE_COMPLETED);
}
