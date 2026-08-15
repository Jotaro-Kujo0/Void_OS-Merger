// master/cli.h — command-line argument parser for cluster-master.
// TODO: flags to cover:
//         --bind ENDPOINT      (default from config)
//         --config PATH
//         --log-level LEVEL
//         --foreground / --daemon
//         --print-uuid        (emit a UUID and exit — for systemd)
//         --simulate N        (spin up N virtual workers on loopback)
// TODO: declare struct vom_master_args { ... }.
// TODO: declare vom_master_cli_parse(int argc, char **argv,
//                                    struct vom_config *cfg,
//                                    struct vom_master_args *out).

#ifndef VOM_MASTER_CLI_H
#define VOM_MASTER_CLI_H

#include "common/config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vom_master_args {
    bool help;
    bool version;
    const char *config_file;
    const char *log_level;
    const char *bind_endpoint;
    bool daemonize;
    bool print_uuid;
    int simulate_workers_count;
} vom_master_args_t;

//parses argv matrices into unfied runtime param

int vom_master_cli_parse(int argc, char **argv, void *cfg_ptr, vom_master_args_t *out);

void vom_master_cli_print_help(const char *prog_name);

#ifdef __cplusplus
}
#endif

#endif /* VOM_MASTER_CLI_H */
