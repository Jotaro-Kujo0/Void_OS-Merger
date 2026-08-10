/*
 * master/src/registry.c — worker membership and health placeholder.
 *
 * No implementation is intentionally present.
 *
 * TODO:
 *
 * - Store approved worker records keyed by stable worker ID.
 * - Update static capabilities on join and dynamic resources on heartbeat.
 * - Track chunk reservations and active assignments.
 * - Detect stale workers and notify master recovery.
 * - Publish immutable snapshots to scheduler and UI code.
 */

#include "master/registry.h"
