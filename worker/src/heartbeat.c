/*
 * worker/src/heartbeat.c — worker heartbeat subsystem placeholder.
 *
 * No implementation is intentionally present.
 *
 * TODO:
 *
 * - Keep heartbeat collection inside the worker lifecycle rather than giving
 *   this module a second executable entrypoint.
 * - Collect dynamic resource availability and active chunk summaries.
 * - Serialize timestamped worker heartbeats through worker/transport.c.
 * - Support graceful stop and reconnect without losing worker identity.
 */
