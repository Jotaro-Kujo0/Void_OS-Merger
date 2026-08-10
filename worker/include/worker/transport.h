/*
 * worker/transport.h — worker transport planning notes.
 *
 * The intended baseline topology is a worker DEALER socket connected to the
 * master's ROUTER socket. The worker must not use an unrelated PULL loop for
 * control traffic.
 *
 * TODO:
 *
 * - Use one stable worker identity per logical worker node.
 * - Establish authenticated/encrypted sessions before workload delivery.
 * - Serialize the versioned protocol envelope.
 * - Reconnect without creating duplicate worker records.
 * - Apply message size, timeout, and backpressure limits.
 * - Separate transport events from domain state transitions.
 * - Provide clean shutdown and lease-aware disconnect behavior.
 *
 * Comment-only flow:
 *
 *   // connect -> authenticate -> join -> heartbeat
 *   // receive chunk assignment -> acknowledge -> execute -> report result
 */
