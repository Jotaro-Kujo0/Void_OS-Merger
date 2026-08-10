/* worker/cli.h — command-line parser planning notes for cluster-worker.
 *
 * TODO:
 * - Support --master ENDPOINT, --worker-id ID, --config PATH,
 *   --heartbeat-ms N, --no-discovery, and --test.
 * - Define struct vom_worker_args and vom_worker_cli_parse(...).
 * - Keep CLI parsing separate from worker transport and execution.
 */
#ifndef VOM_WORKER_CLI_H
#define VOM_WORKER_CLI_H

#include "common/config.h"

/*
 * Comment-only future API:
 *
 * // struct vom_worker_args { ... };
 * // int vom_worker_cli_parse(int argc, char **argv,
 * //                          struct vom_config *cfg,
 * //                          struct vom_worker_args *out);
 */

#endif /* VOM_WORKER_CLI_H */
