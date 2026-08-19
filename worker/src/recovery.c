#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define WORKER_MAX_ACTIVE_LEASES 16
#define IDENTITY_NAME_MAX        64

typedef enum {
    CHUNK_RUNNING,
    CHUNK_REVOKED,
    CHUNK_COMPLETED,
    CHUNK_RECONCILED
} WorkerChunkState;

typedef struct {
    uint64_t lease_id;
    uint64_t chunk_id;
    WorkerChunkState state;
    char output_checksum[IDENTITY_NAME_MAX];
    bool partial_output_preserved;
    bool policy_discard_on_cancel;
} worker_active_lease_t;

struct vom_worker_recovery_context {
    worker_active_lease_t active_leases[WORKER_MAX_ACTIVE_LEASES];
    int active_lease_count;
    bool is_draining;
    bool is_connected_to_master;
};

typedef struct vom_worker_recovery_context vom_worker_recovery_context_t;

vom_worker_recovery_context_t* vom_worker_recovery_init(void) {
    vom_worker_recovery_context_t* ctx = (vom_worker_recovery_context_t*)calloc(1, sizeof(vom_worker_recovery_context_t));
    if (!ctx) return NULL;
    ctx->is_draining = false;
    ctx->is_connected_to_master = true;
    ctx->active_lease_count = 0;
    return ctx;
}

void vom_worker_recovery_destroy(vom_worker_recovery_context_t* ctx) {
    if (!ctx) return;
    free(ctx);
}

void vom_worker_recovery_set_drain(vom_worker_recovery_context_t* ctx, bool drain_active) {
    if (!ctx) return;
    ctx->is_draining = drain_active;
}

bool vom_worker_recovery_accept_new_chunk(vom_worker_recovery_context_t* ctx, uint64_t lease_id, uint64_t chunk_id, bool discard_policy) {
    if (!ctx) return false;
    if (ctx->is_draining) return false;
    if (ctx->active_lease_count >= WORKER_MAX_ACTIVE_LEASES) return false;

    worker_active_lease_t* l = &ctx->active_leases[ctx->active_lease_count++];
    l->lease_id = lease_id;
    l->chunk_id = chunk_id;
    l->state = CHUNK_RUNNING;
    l->partial_output_preserved = false;
    l->policy_discard_on_cancel = discard_policy;
    memset(l->output_checksum, 0, IDENTITY_NAME_MAX);
    return true;
}

void vom_worker_recovery_handle_revocation(vom_worker_recovery_context_t* ctx, uint64_t lease_id) {
    if (!ctx) return;
    for (int i = 0; i < ctx->active_lease_count; i++) {
        worker_active_lease_t* l = &ctx->active_leases[i];
        if (l->lease_id == lease_id) {
            l->state = CHUNK_REVOKED;
            l->partial_output_preserved = !l->policy_discard_on_cancel;
            break;
        }
    }
}

void vom_worker_recovery_on_disconnect(vom_worker_recovery_context_t* ctx) {
    if (!ctx) return;
    ctx->is_connected_to_master = false;
}

void vom_worker_recovery_reconnect_reconcile(vom_worker_recovery_context_t* ctx, const uint64_t* authoritative_leases, int auth_count) {
    if (!ctx) return;
    ctx->is_connected_to_master = true;

    int i = 0;
    while (i < ctx->active_lease_count) {
        worker_active_lease_t* l = &ctx->active_leases[i];
        bool matched = false;

        for (int j = 0; j < auth_count; j++) {
            if (authoritative_leases[j] == l->lease_id) {
                matched = true;
                break;
            }
        }

        if (!matched) {
            if (l->state == CHUNK_RUNNING) {
                l->state = CHUNK_REVOKED;
                l->partial_output_preserved = !l->policy_discard_on_cancel;
            }
            if (l->state != CHUNK_REVOKED) {
                if (i < ctx->active_lease_count - 1) {
                    memmove(&ctx->active_leases[i], &ctx->active_leases[i + 1], sizeof(worker_active_lease_t) * (ctx->active_lease_count - i - 1));
                }
                ctx->active_lease_count--;
                continue;
            }
        } else {
            if (l->state == CHUNK_COMPLETED) {
                l->state = CHUNK_RECONCILED;
            }
        }
        i++;
    }
}

bool vom_worker_recovery_claim_result(vom_worker_recovery_context_t* ctx, uint64_t lease_id, char* out_checksum) {
    if (!ctx || !out_checksum) return false;
    for (int i = 0; i < ctx->active_lease_count; i++) {
        worker_active_lease_t* l = &ctx->active_leases[i];
        if (l->lease_id == lease_id) {
            if (l->state != CHUNK_RECONCILED && l->state != CHUNK_COMPLETED) return false;
            strncpy(out_checksum, l->output_checksum, IDENTITY_NAME_MAX - 1);
            out_checksum[IDENTITY_NAME_MAX - 1] = '\0';
            return true;
        }
    }
    return false;
}

void vom_worker_recovery_complete_chunk(vom_worker_recovery_context_t* ctx, uint64_t lease_id, const char* checksum) {
    if (!ctx || !checksum) return;
    for (int i = 0; i < ctx->active_lease_count; i++) {
        worker_active_lease_t* l = &ctx->active_leases[i];
        if (l->lease_id == lease_id && l->state == CHUNK_RUNNING) {
            l->state = CHUNK_COMPLETED;
            strncpy(l->output_checksum, checksum, IDENTITY_NAME_MAX - 1);
            l->output_checksum[IDENTITY_NAME_MAX - 1] = '\0';
            break;
        }
    }
}

void execute_worker_recovery_test_suite(void) {
    vom_worker_recovery_context_t* rec = vom_worker_recovery_init();

    bool r1 = vom_worker_recovery_accept_new_chunk(rec, 9001, 101, true);
    printf("Test Normal Accept: %s\n", r1 ? "PASS" : "FAIL");

    vom_worker_recovery_set_drain(rec, true);
    bool r2 = vom_worker_recovery_accept_new_chunk(rec, 9002, 102, true);
    printf("Test Reject During Drain: %s\n", !r2 ? "PASS" : "FAIL");

    vom_worker_recovery_set_drain(rec, false);
    vom_worker_recovery_accept_new_chunk(rec, 9003, 103, false);
    vom_worker_recovery_handle_revocation(rec, 9003);
    printf("Test Lease Revocation Applied State\n");

    vom_worker_recovery_accept_new_chunk(rec, 9004, 104, true);
    vom_worker_recovery_complete_chunk(rec, 9004, "sha256-verified-output");
    
    uint64_t authoritative_leases[1] = { 9004 };
    vom_worker_recovery_reconnect_reconcile(rec, authoritative_leases, 1);
    
    char claimed_hash[IDENTITY_NAME_MAX];
    bool claimed = vom_worker_recovery_claim_result(rec, 9004, claimed_hash);
    printf("Test Reconciled Lease Claim: %s | Hash: %s\n", claimed ? "PASS" : "FAIL", claimed_hash);

    bool stray_claimed = vom_worker_recovery_claim_result(rec, 9001, claimed_hash);
    printf("Test Stray Completed Lease Rejection: %s\n", !stray_claimed ? "PASS" : "FAIL");

    vom_worker_recovery_destroy(rec);
}

int main(void) {
    execute_worker_recovery_test_suite();
    return 0;
}
