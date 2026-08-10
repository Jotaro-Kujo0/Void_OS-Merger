/*
 * worker/resource_monitor.h — local resource monitoring planning notes.
 *
 * The scheduler needs current availability, not only hardware maximums.
 *
 * TODO:
 *
 * - Sample CPU utilization and effective available capacity.
 * - Sample memory pressure and reclaimable memory.
 * - Sample storage pressure and optional I/O throughput.
 * - Sample accelerator utilization when supported.
 * - Sample network quality and transfer estimates.
 * - Sample battery and thermal limits.
 * - Account for chunks already reserved by the master.
 * - Avoid expensive probes at heartbeat frequency.
 * - Mark samples with timestamps and freshness information.
 */
