/*
 * cli/commands.h — master control command planning notes.
 *
 * The CLI should be a client of the master's control API, not a second
 * scheduler or a direct editor of worker state.
 *
 * TODO:
 *
 * - Define status, workload submit, workload inspect, cancel, worker drain,
 *   worker resume, worker approve, and UI-mode commands.
 * - Add structured output suitable for scripts and future accessibility tools.
 * - Keep command parsing separate from transport and view formatting.
 * - Return stable error categories and exit codes.
 * - Show workloads and chunks using the master UI view model.
 */
