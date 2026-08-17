 #ifndef VOM_CLI_COMMANDS_H
 #define VOM_CLI_COMMANDS_H

 #include <stdint.h>
 #include <stdbool.h>

 #ifdef __cplusplus
 extern "C" {
#endif


#define CLI_MAX_ARGS_LEN 128
#define CLI_MAX_PATH_LEN    256

//command action categories hell yea
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

//output fromat strats for script process
typedef enum {
    CLI_OUTPUT_HUMAN_READABLE,
    CLI_OUTPUT_JSON_STRUCTURABLE,
    CLI_OUTPUT_PLAIN_SCRIPT_TABLE
} CliOutputFormat;

/* --- Explicit Deterministic Exit Code Registry --- */
typedef enum {
    CLI_EXIT_SUCCESS           = 0,
    CLI_EXIT_ERR_BAD_SYNTAX    = 1,
    CLI_EXIT_ERR_UNKNOWN_CMD   = 2,
    CLI_EXIT_ERR_MISSING_ARG   = 3,
    CLI_EXIT_ERR_INVALID_VAL   = 4,
    CLI_EXIT_ERR_NETWORK_FAULT = 5,
    CLI_EXIT_ERR_INTERNAL      = 6
} CliExitCode;

//Neutral Payload Option Containers
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
    char mode_name[CLI_MAX_ARGS_LEN]; /* e.g., "interactive", "raw-ascii" */
} CmdUiOpts;

/* --- Master Control Command Parameter Wrapper --- */
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

/**
 * Separates raw command string tokens from transport loops.
 * Populates out_cmd structure neutrally and returns a stable exit code asset.
 */
CliExitCode vom_cli_commands_parse(int argc, char **argv, CliParsedCommand *out_cmd);

#ifdef __cplusplus
}
#endif

#endif /* VOM_CLI_COMMANDS_H */