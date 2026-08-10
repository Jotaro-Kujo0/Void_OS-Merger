// common/log.h — leveled logging shared across master, workers, and CLI.
// TODO: implement a tiny leveled logger (TRACE/DEBUG/INFO/WARN/ERROR/FATAL)
//       with optional colors, timestamps, and a configurable sink (stderr
//       by default; optionally a ring buffer file for postmortem dumps).

#ifndef VOM_COMMON_LOG_H
#define VOM_COMMON_LOG_H

#include <stdio.h>

// TODO: declare the VOM_LOG_LEVEL enum (TRACE < DEBUG < INFO < WARN < ERROR < FATAL).
// TODO: declare vom_log_set_level(int) and vom_log_set_stream(FILE *).
// TODO: declare the VOM_LOG_<LEVEL>(fmt, ...) macro family that prefixes
//       timestamp + severity tag before delegating to fprintf.

#endif /* VOM_COMMON_LOG_H */
