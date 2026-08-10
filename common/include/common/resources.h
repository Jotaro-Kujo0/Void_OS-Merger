/*
 * common/resources.h — resource accounting planning notes.
 *
 * The master cannot balance workers using CPU core count alone. This file is
 * reserved for a shared vocabulary for capacity, availability, demand, and
 * reservations.
 *
 * TODO:
 *
 * - Define CPU capacity units and how they are normalized across platforms.
 * - Define memory capacity and currently available memory.
 * - Define accelerator capacity and accelerator memory.
 * - Define storage capacity, free space, and optional throughput estimates.
 * - Define network bandwidth/latency observations.
 * - Define battery and thermal policy inputs.
 * - Define reservations so concurrent scheduler decisions cannot oversubscribe
 *   a worker.
 * - Define an assignment lease and its expiration timestamp.
 * - Distinguish advertised capacity from currently available capacity.
 *
 * Comment-only example:
 *
 *   // worker_capacity = measured or configured maximum contribution.
 *   // worker_available = capacity minus reservations and local usage.
 *   // chunk_demand = predicted resource use for one chunk.
 *   // reservation = temporary promise made by the master before dispatch.
 *
 * A future scheduler should calculate projected availability before accepting
 * an assignment, then release or revise the reservation as telemetry arrives.
 */
