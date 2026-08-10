/*
 * common/result.h — shared result/error planning notes.
 *
 * All future public APIs should agree on success, retryable failure,
 * permanent failure, invalid input, timeout, and cancellation semantics.
 *
 * TODO:
 *
 * - Define a project-wide result convention for C APIs.
 * - Distinguish transport errors from protocol errors.
 * - Distinguish worker execution failures from scheduler rejection.
 * - Include an error category, stable numeric code, and human-readable detail.
 * - Never expose an errno value as the complete distributed error contract.
 * - Define which errors cause retry and which terminate a chunk.
 * - Define whether error details may contain sensitive workload data.
 *
 * Comment-only example:
 *
 *   // enum vom_error_category {
 *   //     VOM_ERROR_NONE,
 *   //     VOM_ERROR_INVALID_INPUT,
 *   //     VOM_ERROR_UNSUPPORTED,
 *   //     VOM_ERROR_RESOURCE_UNAVAILABLE,
 *   //     VOM_ERROR_TRANSPORT,
 *   //     VOM_ERROR_TIMEOUT,
 *   //     VOM_ERROR_EXECUTION,
 *   //     VOM_ERROR_AUTHENTICATION
 *   // };
 */
