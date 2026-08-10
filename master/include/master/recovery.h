/*
 * master/recovery.h — failed-worker chunk recovery planning notes.
 *
 * The base system recovers by expiring leases and reassigning safe unfinished
 * chunks. Portable checkpointing may be added for runtimes that support it.
 *
 * TODO:
 * - Detect stale worker heartbeats and transport loss.
 * - Mark leases uncertain before reassignment.
 * - Apply idempotency and retry policy.
 * - Preserve committed results and reject stale result messages.
 * - Publish degraded/recovering state through the master UI.
 */
