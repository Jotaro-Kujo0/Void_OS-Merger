/*
 * ui/model.h — UI view-model planning notes.
 *
 * This file must remain presentation-neutral. It should describe the logical
 * device, workers, workloads, chunks, resources, and alerts without importing
 * a terminal, desktop, web, or mobile UI toolkit.
 *
 * TODO:
 *
 * - Define immutable snapshot concepts.
 * - Include aggregate logical-device status.
 * - Include worker summaries and optional UI surfaces.
 * - Include workload and chunk progress.
 * - Include actionable alerts and recovery explanations.
 * - Define redaction rules for sensitive workload data.
 * - Define stable field names for future JSON or IPC consumers.
 *
 * Comment-only example:
 *
 *   // UI reads a snapshot:
 *   // logical device -> resources -> workers -> workloads -> alerts
 *   // UI sends intents:
 *   // submit, cancel, drain, resume, approve-worker, change-ui-mode
 */
