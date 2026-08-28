 #ifndef VOM_COMMON_RESULT_H
 #define VOM_COMMON_RESULT_H

 #include <stdint.h>
 #include <stdbool.h>

 #ifdef __cplusplus
 extern "C" {
 #endif

 #define VOM_RESULT_DETAIL_MAX  128

 //core status categories

 typedef enum {
     VOM_CAT_SUCCESS = 0,
    VOM_CAT_INVALID_INPUT,        /* API syntax violations */
    VOM_CAT_TRANSPORT,            /* Socket/IPC/Wire network faults */
    VOM_CAT_PROTOCOL,             /* Bad framing or unexpected version signatures */
    VOM_CAT_SCHEDULER_REJECT,     /* Cluster-side constraint mismatches or resource bounds */
    VOM_CAT_WORKER_EXECUTION,     /* Sandboxed task runtime errors */
    VOM_CAT_TIMEOUT,              /* Temporal deadline constraints reached */
    VOM_CAT_CANCELLATION,         /* Active workload cancel signals tripped */
    VOM_CAT_AUTHENTICATION,       /* Key exchange or CURVE token faults */
    VOM_CAT_INTERNAL              /* System panicked or engine storage crashed */
} VomErrorCategory;

//retry /recovery action codes
typedef enum {
    VOM_RETRY_NONE = 0,           /* Permanent failure; terminate workload immediately */
    VOM_RETRY_IMMEDIATE,          /* Safe to retry on alternative worker immediately */
    VOM_RETRY_BACKOFF,            /* Retry on same worker with an exponential sleep delay */
    VOM_RETRY_REPLAN              /* Re-submit chunk to the planning engine to break loop */
} VomRetryAction;

//unified constract record
typedef struct {
    VomErrorCategory category;
    uint32_t stable_code;         /* Architecture-invariant error status token */
    VomRetryAction retry_policy;  /* Explicit instruction for scheduling loops */
    
    char human_detail[VOM_RESULT_DETAIL_MAX];
 } vom_result_t;

 //core utility inline shields

 static inline vom_result_t vom_result_success(void) {
    vom_result_t res = {
        .category = VOM_CAT_SUCCESS,
        .stable_code = 0,
        .retry_policy = VOM_RETRY_NONE,
        .human_detail = "Operation completed successfully"
    };
    return res;
 }

 static inline bool vom_result_is_success(vom_result_t res) {
    return (res.category == VOM_CAT_SUCCESS && res.retry_policy == VOM_RETRY_NONE);
 }

 #ifdef __cplusplus
}
#endif

#endif /*VOM_RESULT_H*/