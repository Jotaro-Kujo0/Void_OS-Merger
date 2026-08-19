#ifndef VOM_WORKER_CLI_H
#define VOM_WORKER_CLI_H

#include <stdint.h>
#include <stdbool.h>

#define CLI_STR_MAX 128

typedef struct {
    char master_endpoint[CLI_STR_MAX];
    char worker_id[CLI_STR_MAX];
    char config_path[CLI_STR_MAX];
    uint32_t heartbeat_interval_ms;
    bool disable_discovery;
    bool execute_self_test;
} vom_worker_args_t;

int vom_worker_cli_parse(int argc, char **argv, void *opaque_config_ctx, vom_worker_args_t *out_args);

#endif
