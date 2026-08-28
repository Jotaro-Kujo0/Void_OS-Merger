#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <common/compat.h>

/* --- Embedded Interface Types to Bypass Include Failures --- */
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

int vom_master_cli_parse(int argc, char **argv, void *cfg_ptr, vom_master_args_t *out);
void vom_master_cli_print_help(const char *prog_name);

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
                {
                    /* Generate UUID from hardware MAC address */
                    char uuid_buf[64] = {0};
                    FILE *mf = NULL;
#if defined(__linux__)
                    mf = fopen("/sys/class/net/eth0/address", "r");
                    if (!mf) mf = fopen("/sys/class/net/wlan0/address", "r");
#endif
                    if (mf) {
                        char mac[18] = {0};
                        if (fscanf(mf, "%17s", mac) == 1) {
                            snprintf(uuid_buf, sizeof(uuid_buf), "vom-%s-%lld", mac, (long long)time(NULL));
                        }
                        fclose(mf);
                    }
                    if (uuid_buf[0] == '\0') {
                        snprintf(uuid_buf, sizeof(uuid_buf), "vom-%d-%lld", (int)getpid(), (long long)time(NULL));
                    }
                    printf("%s\n", uuid_buf);
                }
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

    return 0;
}
