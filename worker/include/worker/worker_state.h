/*
 * worker/worker_state.h — worker lifecycle planning notes.
 *
 * A worker owns local observations and local execution state. The master owns
 * cluster-wide assignment truth.
 *
 * TODO:
 *
 * - Define worker identity, connection state, approval state, and health.
 * - Track static capabilities separately from dynamic resources.
 * - Track local chunk reservations and execution state.
 * - Track display/input surfaces and their current UI policy.
 * - Define safe startup, reconnect, drain, and shutdown transitions.
 * - Prevent local state from silently overriding master assignment state.
 */
