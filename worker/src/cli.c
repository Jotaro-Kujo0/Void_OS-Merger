#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define CLI_STR_MAX 128

typedef struct {
    char master_endpoint[CLI_STR_MAX];
    char worker_id[CLI_STR_MAX];
    char config_path[CLI_STR_MAX];
    uint32_t heartbeat_interval_ms;
    bool disable_discovery;
    bool execute_self_test;
} vom_worker_args_t;

int vom_worker_cli_parse(int argc, char **argv, void *opaque_config_ctx, vom_worker_args_t *out_args) {
    (void)opaque_config_ctx;
    if (!out_args) return -1;

    memset(out_args, 0, sizeof(vom_worker_args_t));
    strncpy(out_args->master_endpoint, "tcp://127.0.0.1:5555", CLI_STR_MAX - 1);
    out_args->heartbeat_interval_ms = 1000;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--master") == 0 && i + 1 < argc) {
            strncpy(out_args->master_endpoint, argv[++i], CLI_STR_MAX - 1);
        } else if (strcmp(argv[i], "--worker-id") == 0 && i + 1 < argc) {
            strncpy(out_args->worker_id, argv[++i], CLI_STR_MAX - 1);
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            strncpy(out_args->config_path, argv[++i], CLI_STR_MAX - 1);
        } else if (strcmp(argv[i], "--heartbeat-ms") == 0 && i + 1 < argc) {
            out_args->heartbeat_interval_ms = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--no-discovery") == 0) {
            out_args->disable_discovery = true;
        } else if (strcmp(argv[i], "--test") == 0) {
            out_args->execute_self_test = true;
        }
    }
    return 0;
}

void execute_cli_test_suite(void) {
    vom_worker_args_t args;
    
    char *mock_argv1[] = {"worker_bin", "--test"};
    int rc1 = vom_worker_cli_parse(2, mock_argv1, NULL, &args);
    assert(rc1 == 0);
    assert(args.execute_self_test == true);
    assert(args.disable_discovery == false);
    assert(strcmp(args.master_endpoint, "tcp://127.0.0.1:5555") == 0);

    char *mock_argv2[] = {"worker_bin", "--master", "tcp://10.0.0.1:6666", "--worker-id", "node-42", "--heartbeat-ms", "3000", "--no-discovery"};
    int rc2 = vom_worker_cli_parse(8, mock_argv2, NULL, &args);
    assert(rc2 == 0);
    assert(args.execute_self_test == false);
    assert(args.disable_discovery == true);
    assert(strcmp(args.master_endpoint, "tcp://10.0.0.1:6666") == 0);
    assert(strcmp(args.worker_id, "node-42") == 0);
    assert(args.heartbeat_interval_ms == 3000);
}

#ifdef VOM_STANDALONE_TEST
int main(void) {
    execute_cli_test_suite();
    return 0;
}
#endif
