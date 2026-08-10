/*
 * worker/recovery.h — worker recovery planning notes.
 *
 * The base recovery path is safe cancellation and chunk re-execution. Portable
 * checkpointing may be added later for runtimes that explicitly support it.
 *
 * TODO:
 *
 * - Stop accepting new chunks while draining.
 * - Report active chunk state before shutdown.
 * - Cancel local execution when the master revokes a lease.
 * - Preserve or discard partial output according to chunk policy.
 * - Reconnect and reconcile local state with the master's authoritative state.
 * - Never claim a completed result without the matching assignment/lease.
 */
