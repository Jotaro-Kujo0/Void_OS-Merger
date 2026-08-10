/*
 * master/cluster_state.h — authoritative logical-device state planning notes.
 *
 * TODO:
 * - Track logical-device identity, master epoch, and cluster state.
 * - Track worker membership, health, capabilities, and reservations.
 * - Track workload/chunk ownership, leases, and progress.
 * - Track aggregate capacity and optional worker UI surfaces.
 * - Expose immutable snapshots to scheduler, recovery, and UI modules.
 * - Persist required state before implementing master restart recovery.
 */
