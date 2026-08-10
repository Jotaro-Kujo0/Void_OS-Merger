/* worker/src/cli.c — argument parsing for cluster-worker.
 *
 * TODO:
 * - Implement vom_worker_cli_parse() using the selected argument parser.
 * - Support master endpoint, worker identity, configuration, heartbeat,
 *   discovery, logging, and self-test options.
 * - Keep the parser independent from the worker event loop.
 */

#include "worker/cli.h"
