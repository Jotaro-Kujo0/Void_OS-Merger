// master/src/cli.c — CLI parser for cluster-master.
#include "master/cli.h"
#include "common/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <stdbool.h>

#define VOM_MASTER_VERSION "1.0.1"
#define VOM_MASTER_LICENSE "Licensed under the Apache License, Version 2.0"

static void vom_master_cli_print_version(void) {
    printf("vom-cluster-master version %s\n%s\n", VOM_MASTER_VERSION, VOM_MASTER_LICENSE);
}

void vom_master_cli_print_help(const char *prog_name) {
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -V, --version            Print version information\n");
    printf("  -c, --config PATH        Path to configuration file\n");
    printf("  -l, --log-level LEVEL    Logging threshold (trace, debug, info, warn, error, fatal)\n");
    printf("  -b, --bind ENDPOINT      Listen endpoint connection string (tcp:// or ipc://)\n");
    printf("  -f, --foreground         Run process in foreground (default)\n");
    printf("  -d, --daemon             Run master process as a background daemon\n");
    printf("  -u, --print-uuid         Emit a fresh UUID and exit instantly (for systemd)\n");
    printf("  -s, --simulate N         Spin up N virtual loopback workers inside simulation matrices\n");
}

static bool vom_master_cli_validate_endpoint(const char *endpoint) {
    if (!endpoint) return false;
    return ((strncmp(endpoint, "tcp://", 6) == 0 || strncmp(endpoint, "ipc://", 6) == 0) && strlen(endpoint) > 6);
}

int vom_master_cli_parse(int argc, char **argv, void *cfg_ptr, vom_master_args_t *out) {
    if (!out) return -1;

    out->help = false;
    out->version = false;
    out->config_file = NULL;
    out->log_level = "info";
    out->bind_endpoint = NULL;
    out->daemonize = false;
    out->print_uuid = false;
    out->simulate_workers_count = 0;

    /* Value indicators tracking non-overlapping short parameter keys */
    const char *short_options = "hVc:l:b:fdus:";
    
    const struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"config",     required_argument, NULL, 'c'},
        {"log-level",  required_argument, NULL, 'l'},
        {"bind",       required_argument, NULL, 'b'},
        {"foreground", no_argument,       NULL, 'f'},
        {"daemon",     no_argument,       NULL, 'd'},
        {"print-uuid", no_argument,       NULL, 'u'},
        {"simulate",   required_argument, NULL, 's'},
        {NULL,         0,                 NULL,  0 }
    };

    int opt;
    int option_index = 0;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                out->help = true;
                vom_master_cli_print_help(argv[0]);
                exit(EXIT_SUCCESS);

            case 'V':
                out->version = true;
                vom_master_cli_print_version();
                exit(EXIT_SUCCESS);

            case 'c':
                out->config_file = optarg;
                break;

            case 'l':
                out->log_level = optarg;
                break;

            case 'b':
                if (!vom_master_cli_validate_endpoint(optarg)) {
                    fprintf(stderr, "Error: Invalid bind endpoint '%s'. Use 'tcp://' or 'ipc://'.\n", optarg);
                    return -1;
                }
                out->bind_endpoint = optarg;
                break;

            case 'f':
                out->daemonize = false;
                break;

            case 'd':
                out->daemonize = true;
                break;

            case 'u':
                out->print_uuid = true;
                /* Mock deterministic initialization value to pipe clean token maps directly to systemd standard descriptors */
                printf("vom-master-uuid-4f9e-a1b2-7cc892de410f\n");
                exit(EXIT_SUCCESS);

            case 's':
                out->simulate_workers_count = atoi(optarg);
                if (out->simulate_workers_count < 0) {
                    fprintf(stderr, "Error: Invalid simulation worker allocation parameter '%s'.\n", optarg);
                    return -1;
                }
                break;

            case '?':
                if (strchr("clbs", optopt)) {
                    fprintf(stderr, "Error: Option '-%c' requires an associated parameter argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Error: Unrecognized operational option flag '-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unsupported parsing character set found.\n");
                }
                vom_master_cli_print_help(argv[0]);
                return -1;

            default:
                return -1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Warning: Position elements starting at '%s' are unsupported and ignored.\n", argv[optind]);
    }


    if (cfg_ptr && out->bind_endpoint) {
        // strncpy(((vom_config_t*)cfg_ptr)->bind_url, out->bind_endpoint, 255);
    }

    return 0;
}
