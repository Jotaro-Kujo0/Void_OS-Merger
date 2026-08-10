/* tests/test_capnp_roundtrip.c — protocol round-trip planning notes.
 *
 * TODO:
 * - Build worker join, heartbeat, workload, chunk assignment, progress,
 *   result, recovery, and UI-surface fixtures once the schema is declared.
 * - Serialize and deserialize each ClusterMessage payload.
 * - Assert IDs, epochs, leases, dependencies, requirements, and result
 *   metadata survive the round trip.
 * - Verify unknown/unsupported fields follow the compatibility policy.
 */
