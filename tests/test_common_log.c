#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

typedef enum {
    VOM_LOG_LEVEL_TRACE = 0,
    VOM_LOG_LEVEL_DEBUG,
    VOM_LOG_LEVEL_INFO,
    VOM_LOG_LEVEL_WARN,
    VOM_LOG_LEVEL_ERROR,
    VOM_LOG_LEVEL_FATAL,
    VOM_LOG_LEVEL_NONE
} VomLogLevel;

extern void vom_log_set_level(VomLogLevel level);
extern void vom_log_set_stream(FILE *stream);
extern void vom_log_enable_colors(bool enable);
extern void vom_log_emit(VomLogLevel level, const char *file, int line, const char *fmt, ...);

#define VOM_LOG_TRACE(fmt, ...) vom_log_emit(VOM_LOG_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VOM_LOG_DEBUG(fmt, ...) vom_log_emit(VOM_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VOM_LOG_INFO(fmt, ...)  vom_log_emit(VOM_LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VOM_LOG_WARN(fmt, ...) vom_log_emit(VOM_LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VOM_LOG_ERROR(fmt, ...) vom_log_emit(VOM_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VOM_LOG_FATAL(fmt, ...) vom_log_emit(VOM_LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define BUF_SIZE 2048

static void assert_log_contains(FILE *stream, const char *expected_tag, const char *expected_msg) {
    char buf[BUF_SIZE];
    fflush(stream);
    fseek(stream, 0, SEEK_SET);
    bool found_tag = false;
    bool found_msg = false;
    while (fgets(buf, sizeof(buf), stream)) {
        if (strstr(buf, expected_tag) != NULL) found_tag = true;
        if (strstr(buf, expected_msg) != NULL) found_msg = true;
    }
    assert(found_tag);
    assert(found_msg);
}

static void assert_log_empty(FILE *stream) {
    char buf[BUF_SIZE];
    fflush(stream);
    fseek(stream, 0, SEEK_SET);
    assert(fgets(buf, sizeof(buf), stream) == NULL);
}

static void clear_stream(FILE *stream) {
    fflush(stream);
    freopen(NULL, "w+", stream);
    fseek(stream, 0, SEEK_SET);
}

void test_severity_tags_emission(FILE *stream) {
    clear_stream(stream);
    vom_log_set_level(VOM_LOG_LEVEL_TRACE);
    
    VOM_LOG_TRACE("trace_test_msg");
    assert_log_contains(stream, "TRACE", "trace_test_msg");
    
    clear_stream(stream);
    VOM_LOG_DEBUG("debug_test_msg");
    assert_log_contains(stream, "DEBUG", "debug_test_msg");
    
    clear_stream(stream);
    VOM_LOG_INFO("info_test_msg");
    assert_log_contains(stream, "INFO", "info_test_msg");
    
    clear_stream(stream);
    VOM_LOG_WARN("warn_test_msg");
    assert_log_contains(stream, "WARN", "warn_test_msg");
    
    clear_stream(stream);
    VOM_LOG_ERROR("error_test_msg");
    assert_log_contains(stream, "ERROR", "error_test_msg");
    
    clear_stream(stream);
    VOM_LOG_FATAL("fatal_test_msg");
    assert_log_contains(stream, "FATAL", "fatal_test_msg");
}

void test_logging_level_gating(FILE *stream) {
    clear_stream(stream);
    vom_log_set_level(VOM_LOG_LEVEL_WARN);
    
    VOM_LOG_INFO("dropped_msg");
    assert_log_empty(stream);
    
    clear_stream(stream);
    VOM_LOG_WARN("allowed_msg");
    assert_log_contains(stream, "WARN", "allowed_msg");
    
    clear_stream(stream);
    vom_log_set_level(VOM_LOG_LEVEL_NONE);
    VOM_LOG_FATAL("gated_fatal");
    assert_log_empty(stream);
}

int main(void) {
    FILE *capture_stream = tmpfile();
    if (!capture_stream) return EXIT_FAILURE;
    
    vom_log_set_stream(capture_stream);
    vom_log_enable_colors(false);
    
    test_severity_tags_emission(capture_stream);
    test_logging_level_gating(capture_stream);
    
    fclose(capture_stream);
    return EXIT_SUCCESS;
}
