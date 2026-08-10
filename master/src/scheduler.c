/*
 * master/src/scheduler.c — ready-chunk to compatible-worker placeholder.
 *
 * No implementation is intentionally present.
 *
 * TODO:
 *
 * - Read ready chunks from the workload/chunk graph.
 * - Hard-filter workers by compatibility and available resources.
 * - Score the complete projected assignment for cluster harmony.
 * - Reserve resources and create a lease before dispatch.
 * - Leave chunks pending when no compatible worker is available.
 * - Add deterministic tests for proportional capacity and tie-breakers.
 */

#include "master/scheduler.h"
