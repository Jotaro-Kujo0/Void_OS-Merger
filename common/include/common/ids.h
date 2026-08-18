//ok so thia file translates the identity contract into public C deinitions
//ıt gives unique representation styles accross the cluster while
//banning unstable vraibles

#ifndef VOM_COMMON_IDS_H
#define VOM_COMMON_IDS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//max len ident len contract
//gives a portable 64-byte text footprint 
//including null terminator

#define VOM_ID_MAX_LEN  64
typedef struct {
    char text[VOM_ID_MAX_LEN];
} vom_id_t;

//core domain object def
//identify the unified, cluster machine presented to the user
typedef vom_id_t vom_logical_device_id_t;

//identify the auth master node
typedef vom_id_t vom_master_id_t;

//identify worker node. Must survive reconnects and reboots
//generated from physical hardware markers
typedef vom_id_t vom_worker_id_t;

//identify user-requested trans
typedef vom_id_t vom_workload_id_t;

//identify independent sched segment of workload
typedef vom_id_t vom_chunk_id_t;

//identify identify active connection lifecycle tracking session
typedef vom_id_t vom_session_id_t;

//identify individual API / worker message frame
typedef vom_id_t vom_message_id_t;

//identify resource reservation lease by sched
typedef vom_id_t vom_lease_id_t;

//validation utility and comparison engine
/**
 * Validates whether a raw string conforms to prefix requirements,
 * checks character bounds, and verifies max-length length limitations.
 */
bool vom_id_validate(const char *text, const char *expected_prefix);

/**
 * Performs a deterministic constant-time comparison to protect identity checks.
 * Returns true if IDs are identical, false otherwise.
/** */
bool vom_id_equal(const vom_id_t *a, const vom_id_t *b);


// Copies source ID contents into target tracking structs safely.
 
void vom_id_copy(vom_id_t *dst, const vom_id_t *src);


// Sets an ID structure back to an empty, zero-initialized null state.

void vom_id_clear(vom_id_t *id);

#ifdef __cplusplus
}
#endif

#endif /* VOM_COMMON_IDS_H */
