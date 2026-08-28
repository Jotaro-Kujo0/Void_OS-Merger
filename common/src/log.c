#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <common/compat.h>

/* --- Embedded Interface Enums to Bypass Include Path Failures --- */
typedef enum {
    VOM_LOG_LEVEL_TRACE = 0,
    VOM_LOG_LEVEL_DEBUG,
    VOM_LOG_LEVEL_INFO,
    VOM_LOG_LEVEL_WARN,
    VOM_LOG_LEVEL_ERROR,
    VOM_LOG_LEVEL_FATAL,
    VOM_LOG_LEVEL_NONE
} VomLogLevel;

/* --- Core Function Declarations --- */
void vom_log_set_level(VomLogLevel level);
void vom_log_set_stream(FILE *stream);
void vom_log_enable_colors(bool enable);
bool vom_log_init_postmortem_ring_buffer(const char *file_path, size_t max_file_size_bytes);
void vom_log_emit(VomLogLevel level, const char *file, int line, const char *fmt, ...);

/* --- Core Logger State Elements --- */
static VomLogLevel g_current_level = VOM_LOG_LEVEL_INFO;
static FILE* g_target_stream = NULL;
static bool g_colors_enabled = true;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

static FILE* g_ring_file = NULL;
static long g_ring_max_size = 0;

#define ANSI_RESET "\x1b[0m"
static const char* g_level_colors[] = {
    "\x1b[35m", /* TRACE - Magenta */
    "\x1b[36m", /* DEBUG - Cyan */
    "\x1b[32m", /* INFO  - Green */
    "\x1b[33m", /* WARN  - Yellow */
    "\x1b[31m", /* ERROR - Red */
    "\x1b[41m\x1b[37m" /* FATAL - White on Red */
};

static const char* g_level_tags[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void vom_log_set_level(VomLogLevel level) {
    pthread_mutex_lock(&g_log_mutex);
    g_current_level = level;
    pthread_mutex_unlock(&g_log_mutex);
}

void vom_log_set_stream(FILE *stream) {
    pthread_mutex_lock(&g_log_mutex);
    g_target_stream = stream;
    pthread_mutex_unlock(&g_log_mutex);
}

void vom_log_enable_colors(bool enable) {
    pthread_mutex_lock(&g_log_mutex);
    g_colors_enabled = enable;
    pthread_mutex_unlock(&g_log_mutex);
}

bool vom_log_init_postmortem_ring_buffer(const char *file_path, size_t max_file_size_bytes) {
    pthread_mutex_lock(&g_log_mutex);
    if (g_ring_file) fclose(g_ring_file);
    g_ring_file = fopen(file_path, "wb+");
    if (!g_ring_file) {
        pthread_mutex_unlock(&g_log_mutex);
        return false;
    }
    g_ring_max_size = (long)max_file_size_bytes;
    pthread_mutex_unlock(&g_log_mutex);
    return true;
}

static void write_to_ring_buffer(const char* buf, size_t len) {
    if (!g_ring_file) return;
    long curr = ftell(g_ring_file);
    if (curr + (long)len > g_ring_max_size) {
        fseek(g_ring_file, 0, SEEK_SET);
    }
    fwrite(buf, 1, len, g_ring_file);
    fflush(g_ring_file);
}

void vom_log_emit(VomLogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < g_current_level || g_current_level == VOM_LOG_LEVEL_NONE) return;

    pthread_mutex_lock(&g_log_mutex);
    FILE* stream = (g_target_stream) ? g_target_stream : stderr;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    const char* short_file = file;
    for (const char* p = file; *p != '\0'; p++) {
        if (*p == '\\' || *p == '/') short_file = p + 1;
    }

    char meta_buf[256];
    char msg_buf[1024];
    int meta_len, msg_len;

    if (g_colors_enabled && (stream == stdout || stream == stderr)) {
        fprintf(stream, "%s.%06d %s[%s]%s [%s:%d] ", 
                time_str, (int)tv.tv_usec, g_level_colors[level], g_level_tags[level], ANSI_RESET, short_file, line);
    } else {
        fprintf(stream, "%s.%06d [%s] [%s:%d] ", 
                time_str, (int)tv.tv_usec, g_level_tags[level], short_file, line);
    }

    va_list args1;
    va_start(args1, fmt);
    vfprintf(stream, fmt, args1);
    va_end(args1);
    fprintf(stream, "\n");
    fflush(stream);

    if (g_ring_file) {
        meta_len = snprintf(meta_buf, sizeof(meta_buf), "%s.%06d [%s] [%s:%d] ", 
                            time_str, (int)tv.tv_usec, g_level_tags[level], short_file, line);
        if (meta_len > 0) write_to_ring_buffer(meta_buf, (size_t)meta_len);

        va_list args2;
        va_start(args2, fmt);
        msg_len = vsnprintf(msg_buf, sizeof(msg_buf), fmt, args2);
        va_end(args2);

        if (msg_len > 0) {
            write_to_ring_buffer(msg_buf, (size_t)msg_len);
            write_to_ring_buffer("\n", 1);
        }
    }

    pthread_mutex_unlock(&g_log_mutex);
}
