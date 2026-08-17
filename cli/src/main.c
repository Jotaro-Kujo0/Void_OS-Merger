#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define VOM_LOG_INFO(fmt, ...)  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_WARN(fmt, ...)  fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define VOM_LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define CLI_MAX_ARGS_LEN    128
#define CLI_MAX_PATH_LEN    256

typedef enum {
    CMD_TYPE_CLUSTER_STATUS,
    CMD_TYPE_WORKLOAD_SUBMIT,
    CMD_TYPE_WORKLOAD_INSPECT,
    CMD_TYPE_WORKLOAD_CANCEL,
    CMD_TYPE_WORKER_DRAIN,
    CMD_TYPE_WORKER_RESUME,
    CMD_TYPE_WORKER_APPROVE,
    CMD_TYPE_UI_MODE_SET,
    CMD_TYPE_UNKNOWN
} CliCommandType;

typedef enum {
    CLI_OUTPUT_HUMAN_READABLE,
    CLI_OUTPUT_JSON_STRUCTURABLE,
    CLI_OUTPUT_PLAIN_SCRIPT_TABLE
} CliOutputFormat;

typedef enum {
    CLI_EXIT_SUCCESS           = 0,
    CLI_EXIT_ERR_BAD_SYNTAX    = 1,
    CLI_EXIT_ERR_UNKNOWN_CMD   = 2,
    CLI_EXIT_ERR_MISSING_ARG   = 3,
    CLI_EXIT_ERR_INVALID_VAL   = 4,
    CLI_EXIT_ERR_NETWORK_FAULT = 5,
    CLI_EXIT_ERR_INTERNAL      = 6
} CliExitCode;

typedef struct {
    char workload_path[CLI_MAX_PATH_LEN];
    uint32_t simulated_priority_level;
} CmdSubmitOpts;

typedef struct {
    uint64_t target_id;
} CmdTargetIdOpts;

typedef struct {
    char target_worker_id[CLI_MAX_ARGS_LEN];
} CmdWorkerOpts;

typedef struct {
    char mode_name[CLI_MAX_ARGS_LEN];
} CmdUiOpts;

typedef struct {
    CliCommandType type;
    CliOutputFormat format_strategy;
    
    union {
        CmdSubmitOpts submit;
        CmdTargetIdOpts inspect;
        CmdTargetIdOpts cancel;
        CmdWorkerOpts worker_drain;
        CmdWorkerOpts worker_resume;
        CmdWorkerOpts worker_approve;
        CmdUiOpts ui_mode;
    } options;
} CliParsedCommand;

CliExitCode vom_cli_commands_parse(int argc, char **argv, CliParsedCommand *out_cmd) {
    if (argc < 2 || !out_cmd) {
        return CLI_EXIT_ERR_BAD_SYNTAX;
    }

    memset(out_cmd, 0, sizeof(CliParsedCommand));
    out_cmd->format_strategy = CLI_OUTPUT_HUMAN_READABLE;
    out_cmd->type = CMD_TYPE_UNKNOWN;

    int arg_start_idx = 1;
    if (strcmp(argv[1], "--json") == 0) {
        out_cmd->format_strategy = CLI_OUTPUT_JSON_STRUCTURABLE;
        arg_start_idx++;
    } else if (strcmp(argv[1], "--script") == 0) {
        out_cmd->format_strategy = CLI_OUTPUT_PLAIN_SCRIPT_TABLE;
        arg_start_idx++;
    }

    if (arg_start_idx >= argc) {
        return CLI_EXIT_ERR_MISSING_ARG;
    }

    const char *verb = argv[arg_start_idx];

    if (strcmp(verb, "status") == 0) {
        out_cmd->type = CMD_TYPE_CLUSTER_STATUS;
        return CLI_EXIT_SUCCESS;
    } 
    
    if (strcmp(verb, "submit") == 0) {
        if (arg_start_idx + 1 >= argc) return CLI_EXIT_ERR_MISSING_ARG;
        out_cmd->type = CMD_TYPE_WORKLOAD_SUBMIT;
        strncpy(out_cmd->options.submit.workload_path, argv[arg_start_idx + 1], CLI_MAX_PATH_LEN - 1);
        out_cmd->options.submit.simulated_priority_level = 1;
        return CLI_EXIT_SUCCESS;
    } 
    
    if (strcmp(verb, "inspect") == 0) {
        if (arg_start_idx + 1 >= argc) return CLI_EXIT_ERR_MISSING_ARG;
        out_cmd->type = CMD_TYPE_WORKLOAD_INSPECT;
        out_cmd->options.inspect.target_id = strtoull(argv[arg_start_idx + 1], NULL, 10);
        return CLI_EXIT_SUCCESS;
    }

    if (strcmp(verb, "cancel") == 0) {
        if (arg_start_idx + 1 >= argc) return CLI_EXIT_ERR_MISSING_ARG;
        out_cmd->type = CMD_TYPE_WORKLOAD_CANCEL;
        out_cmd->options.cancel.target_id = strtoull(argv[arg_start_idx + 1], NULL, 10);
        return CLI_EXIT_SUCCESS;
    }

    if (strcmp(verb, "drain") == 0) {
        if (arg_start_idx + 1 >= argc) return CLI_EXIT_ERR_MISSING_ARG;
        out_cmd->type = CMD_TYPE_WORKER_DRAIN;
        strncpy(out_cmd->options.worker_drain.target_worker_id, argv[arg_start_idx + 1], CLI_MAX_ARGS_LEN - 1);
        return CLI_EXIT_SUCCESS;
    }

    return CLI_EXIT_ERR_UNKNOWN_CMD;
}

int main(int argc, char **argv) {
    CliParsedCommand cmd;
    CliExitCode rc = vom_cli_commands_parse(argc, argv, &cmd);

    if (rc != CLI_EXIT_SUCCESS) {
        switch (rc) {
            case CLI_EXIT_ERR_BAD_SYNTAX:
                VOM_LOG_ERROR("Usage: vom-cli [--json|--script] <command> [args...]");
                break;
            case CLI_EXIT_ERR_MISSING_ARG:
                VOM_LOG_ERROR("Error: Command requires missing argument variables.");
                break;
            case CLI_EXIT_ERR_UNKNOWN_CMD:
                VOM_LOG_ERROR("Error: Unrecognized master management command verb verb input.");
                break;
            default:
                VOM_LOG_ERROR("Catastrophic error parsing CLI context inputs.");
                break;
        }
        return rc;
    }

    VOM_LOG_INFO("Successfully parsed command action index: %d [Format Mode: %d]", cmd.type, cmd.format_strategy);
    return CLI_EXIT_SUCCESS;
}
