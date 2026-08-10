// common/config.h — load simple key/value configuration from a TOML or
// INI file so the same binary can run as master, worker, or CLI just by
// pointing at a different config.
// TODO: parse [cluster], [master], [worker], [logging] sections into a
//       flat struct (cluster name, master endpoint, worker id, log level,
//       heartbeat interval, advertised capabilities).
// TODO: honor VOM_CONFIG env var override so systemd units can swap profiles
//       without editing the on-disk file.

#ifndef VOM_COMMON_CONFIG_H
#define VOM_COMMON_CONFIG_H

#include <stdint.h>

// TODO: declare struct vom_config { ... }.
// TODO: declare vom_config_load(const char *path, struct vom_config *out).
// TODO: declare vom_config_defaults(struct vom_config *out).

#endif /* VOM_COMMON_CONFIG_H */
