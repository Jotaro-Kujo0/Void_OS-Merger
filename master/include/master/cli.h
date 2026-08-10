// master/cli.h — command-line argument parser for cluster-master.
// TODO: flags to cover:
//         --bind ENDPOINT      (default from config)
//         --config PATH
//         --log-level LEVEL
//         --foreground / --daemon
//         --print-uuid        (emit a UUID and exit — for systemd)
//         --simulate N        (spin up N virtual workers on loopback)

#ifndef VOM_MASTER_CLI_H
#define VOM_MASTER_CLI_H

#include "common/config.h"

// TODO: declare struct vom_master_args { ... }.
// TODO: declare vom_master_cli_parse(int argc, char **argv,
//                                    struct vom_config *cfg,
//                                    struct vom_master_args *out).

#endif /* VOM_MASTER_CLI_H */
