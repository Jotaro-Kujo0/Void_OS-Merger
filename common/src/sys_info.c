// common/src/sys_info.c — probe and report host hardware.
// TODO: implement vom_sys_info_collect() using a portable strategy:
//         Linux  -> sysconf() + /proc + /sys/class/power_supply
//         Android -> same Linux paths, plus BatteryManager JNI later
//         Other  -> sysctl / win32 fallbacks
// TODO: implement vom_sys_info_to_capnp() that fills the generated
//       WorkerCapabilities struct matching protocol/cluster.capnp.

#include "common/sys_info.h"
