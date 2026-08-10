/*
 * common/ids.h — identity planning notes.
 *
 * This file intentionally contains no declarations yet. It records the ID
 * contract that must be agreed before master and worker APIs are implemented.
 *
 * TODO:
 *
 * - Define a logical-device ID owned by the master.
 * - Define a master-node ID.
 * - Define a worker-node ID that survives reconnects and, where possible,
 *   device reboots.
 * - Define workload, chunk, session, request, message, and lease IDs.
 * - Decide whether IDs are UUID strings, fixed binary values, or another
 *   portable representation.
 * - Define validation, maximum length, serialization, and comparison rules.
 * - Never use a process ID or socket identity as a long-term workload ID.
 *
 * Comment-only example:
 *
 *   // logical_device_id identifies the one computer presented to the user.
 *   // worker_id identifies one physical participant in that computer.
 *   // chunk_id identifies one schedulable piece of one workload.
 *
 * Example future shape:
 *
 *   // typedef struct { char text[64]; } vom_id_t;
 *   // int vom_id_generate(vom_id_t *out);
 *   // int vom_id_validate(const char *text);
 */
