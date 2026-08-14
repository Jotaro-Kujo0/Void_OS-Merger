// Subcommands to support (each is its own TODO block below):
//
//   vom-cli status                           → list agents + load
//   vom-cli submit <payload.bin> [--caps X] → send a task to the master
//   vom-cli migrate <task-id> <to-agent>     → force-move a running task
//   vom-cli drain <agent-id>                 → migrate everything off a node
//   vom-cli watch                            → live tail of cluster events
//
// All of them talk to the master through a ZMQ_PULL/PAIR pair or a tiny
// admin ROUTER socket reserved for CLI traffic.
//
// TODO: implement argv dispatch.
// TODO: reuse common/log.h so log-level flags behave like the binary tools.
// TODO: print results in a colored human format and as --json for scripts.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "common/log.h"

#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED "\033[31m"
#define COLOR_CYAN "\033[36m"

//global flag tarckking
static bool opt_json = false;
static const char *opt_log_level = "info";

static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [global_options] <subcommand> [subcommand_options]\n\n", prog_name);
    fprintf(stderr, "Global Options:\n");
    fprintf(stderr, "  -h, --help               Show this core help screen\n");
    fprintf(stderr, "  --json                   Format output as JSON for programmatic scripting\n");
    fprintf(stderr, "  -l, --log-level <level>  Set log threshold (debug, info, warn, error)\n\n");
    fprintf(stderr, "Supported Subcommands:\n");
    fprintf(stderr, "  status                   List live cluster agents and active loads\n");
    fprintf(stderr, "  submit <payload.bin>     Send a task payload to the master node\n");
    fprintf(stderr, "  migrate <id> <agent>     Force-move an active running task to a new agent\n");
    fprintf(stderr, "  drain <agent-id>         Migrate all running workloads off a target node\n");
    fprintf(stderr, "  watch                    Live tail stream of global cluster events\n");
}

// SubCommand : Status
static int handle_status(int argc, char **argv) {
    if (argc > 0) {
        fprintf(stderr, "Error 1: 'status subcommand accepts no additional position arguments. \n");
        return EXIT_FAILURE;
    }


    if (opt_json) {
        printf("{\n \"status\": \"ok\",\n  \"agents\": []\n}\n");
    }else {
        printf(COLOR_BOLD COLOR_GREEN "Cluster status Summary:\n" COLOR_RESET);
        printf(COLOR_CYAN "No agents currently registered.\n" COLOR_RESET);
    }
    return EXIT_SUCCESS;
}

// SubCommand Submit
static int handle_submit(int argc, char **argv) {
    int caps = 0;
    char *payload_path = NULL;

    static struct option long_options[] = {
        {"caps", required_argument, NULL, 'x'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    opterr =0;
    while ((opt = getopt_long(argc, argv, "x:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'x':

        }
    }

}
// TODO: parse argv, validate subcommand, build the matching admin request.
