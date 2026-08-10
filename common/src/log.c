// common/src/log.c — leveled logger implementation.
// TODO: implement vom_log_set_level / vom_log_set_stream.
// TODO: implement the VOM_LOG_<LEVEL>(fmt, ...) macros (declared in
//       common/include/common/log.h) using a single internal vom_log_emit()
//       helper that formats timestamp + severity tag + colored output.

#include "common/log.h"
