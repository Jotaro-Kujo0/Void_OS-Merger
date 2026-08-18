#ifndef VOM_COMMON_LOG_H
#define VOM_COMMON_LOG_H

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif 

//log levels and strcuture

typedef enum {
    VOM_LOG_LEVEL_TRACE,
    VOM_LOG_LEVEL_DEBUG,
    VOM_LOG_LEVEL_INFO,
    VOM_LOG_LEVEL_WARN,
    VOM_LOG_LEVEL_ERROR,
    VOM_LOG_LEVEL_FATAL,
    VOM_LOG_LEVEL_NONE
} VomLogLevel;

//extension engine api
//active filtering.
void vom_log_set_level(VomLogLevel level);
//routes text logs to pipeline or file.
void vom_log_set_stream(FILE *stream);

void vom_log_enable_colors(bool enable);
//routes copies of every message to a recorder when a crash happens to check later
bool vom_log_init_postmortem_ring_buffer(const char *file_path, size_t max_file_size_byte);

//unified emission logic
void vom_log_emit(VomLogLevel level, const char *file, int line, const char *fmt, ...);

//Leveled family
//ok so this part is taken from a form and changed for my format
//alongside the code that defines this part above.
// I didn't save it :( 
//I will just pray it works..

#define VOM_LOG_TRACE(fmt, ...) \
    vom_log_emit(VOM_LOG_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VOM_LOG_DEBUG(fmt, ...) \
    vom_log_emit(VOM_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VOM_LOG_INFO(fmt, ...)  \
    vom_log_emit(VOM_LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VOM_LOG_WARN(fmt, ...)  \
    vom_log_emit(VOM_LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VOM_LOG_ERROR(fmt, ...) \
    vom_log_emit(VOM_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VOM_LOG_FATAL(fmt, ...) \
    vom_log_emit(VOM_LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif