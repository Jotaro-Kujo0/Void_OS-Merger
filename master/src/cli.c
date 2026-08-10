// master/src/cli.c — CLI parser for cluster-master.
#include "master/cli.h"
#include "stdlib.h"
#include "getopt.h"
#include "stdbool.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Included to wire up log levels and core configuration hooks
#include "common/log.h"
#include "common/config.h"

typedef struct {
    bool help;
    const char *config_file;
    const char *log_level;
    const char *bind_endpoint;
    int port;
} vom_master_cli_t;

#define VOM_MASTER_VERSION "1.0.1."
#define VOM_MASTER_LICENSE "Licensed under the Apache License, Version 2.0 (the \"License\");\n" \
                           "you may not use this file except in compliance with the License."

// Prints a friendly version banner and license information
static void vom_master_cli_print_version(void) {
    printf("vom-cluster-master version %s\n", VOM_MASTER_VERSION);
    printf("%s\n", VOM_MASTER_LICENSE);
}

void vom_master_cli_print_help(const char *prog_name) {
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help                   Display this help message and exit\n");
    printf("  -V, --version                Print version and license information then exit\n");
    printf("  -c, --config <path>          Path to the master configuration file\n");
    printf("  -l, --log-level <level>      Set logging threshold (debug, info, warn, error)\n");
    printf("  -b, --bind <endpoint>        Listen endpoint connection string (tcp:// or ipc://)\n");
    printf("  -p, --port <port>            Port number for network communications (default: 8080)\n");
}

// Validates that --bind matches either a tcp:// or ipc:// format string 
static bool vom_master_cli_validate_endpoint(const char *endpoint) {
    if (endpoint == NULL) {
        return false;
    }
    if (strncmp(endpoint, "tcp://", 6) == 0 && strlen(endpoint) > 6) {
        return true;
    }
    if (strncmp(endpoint, "ipc://", 6) == 0 && strlen(endpoint) > 6) {
        return true;
    }
    return false;
}

int vom_master_cli_parse(vom_master_cli_t *cli, int argc, char *argv[]) {
    // Initialize standard defaults
    cli->help = false;
    cli->config_file = NULL;
    cli->log_level = "info"; 
    cli->bind_endpoint = NULL;
    cli->port = 8080;

    // Short options array layout matching flags
    const char *short_options = "hVc:l:b:p:";

    // Extended option mappings matching the underlying structure
    const struct option long_options[] = {
        {"help",      no_argument,       NULL, 'h'},
        {"version",   no_argument,       NULL, 'V'},
        {"config",    required_argument, NULL, 'c'},
        {"log-level", required_argument, NULL, 'l'},
        {"bind",      required_argument, NULL, 'b'},
        {"port",      required_argument, NULL, 'p'},
        {NULL,        0,                 NULL,  0 } 
    };

    int opt;
    int option_index = 0;

    // Suppress default getopt prints to issue clean, contextual engine errors
    opterr = 0; 

    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                cli->help = true;
                vom_master_cli_print_help(argv[0]);
                exit(EXIT_SUCCESS);

            case 'V':
                vom_master_cli_print_version();
                exit(EXIT_SUCCESS);

            case 'c':
                cli->config_file = optarg;
                break;

            case 'l':
                cli->log_level = optarg;
                // Optional: hook directly into common/log.h if an instant global mutation is desired
                // log_set_level_from_str(optarg); 
                break;

            case 'b':
                if (!vom_master_cli_validate_endpoint(optarg)) {
                    fprintf(stderr, "Error: Invalid bind endpoint '%s'. Must start with 'tcp://' or 'ipc://'.\n", optarg);
                    return -1;
                }
                cli->bind_endpoint = optarg;
                break;

            case 'p':
                cli->port = atoi(optarg);
                if (cli->port <= 0 || cli->port > 65535) {
                    fprintf(stderr, "Error: Invalid port number '%s'. Out of standard TCP range (1-65535).\n", optarg);
                    return -1;
                }
                break;

            case '?':
                if (optopt == 'c' || optopt == 'l' || optopt == 'b' || optopt == 'p') {
                    fprintf(stderr, "Error: Option '-%c' requires a matching parameter argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Error: Unrecognized option option flag '-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unrecognized command line symbol encountered.\n");
                }
                vom_master_cli_print_help(argv[0]);
                return -1;

            default:
                return -1;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Warning: Positional arguments are unsupported. Ignoring elements starting at '%s'\n", argv[optind]);
    }

    return 0;
}
