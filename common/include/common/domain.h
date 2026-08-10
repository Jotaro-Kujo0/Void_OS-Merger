/*
 * common/domain.h — shared domain vocabulary planning notes.
 *
 * This file is intentionally comment-only. It is the place to agree on terms
 * before public C declarations are introduced.
 *
 * Core objects:
 *
 *   logical device
 *       The cluster-wide computer presented to the user.
 *
 *   master
 *       The node that owns global state, user control, and scheduling.
 *
 *   worker
 *       A physical or virtual node contributing compute or optional peripheral
 *       capabilities.
 *
 *   workload
 *       A complete user-requested operation.
 *
 *   chunk
 *       An independently schedulable part of a workload.
 *
 *   resource request
 *       The predicted CPU, memory, storage, accelerator, network, display,
 *       or input capability needed by a chunk.
 *
 *   UI surface
 *       A display or input endpoint that the master may use optionally.
 *
 * TODO:
 *
 * - Keep workload state separate from chunk state.
 * - Keep worker observations separate from master decisions.
 * - Define which state is durable and which state is derived.
 * - Define ownership and lifetime of all byte buffers.
 * - Define error/status conventions shared by all modules.
 *
 * Comment-only future relationship:
 *
 *   // master owns vom_logical_device_state.
 *   // worker reports vom_worker_observation.
 *   // planner creates vom_chunk_plan nodes.
 *   // scheduler creates vom_chunk_lease records.
 *   // UI reads vom_cluster_view_model snapshots.
 */
