#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "types.h"
#include "transaction.h"

/* ── Mempool entry ───────────────────────────────────────────────────────── */

/*
 * MempoolEntry mirrors the five fields required by the spec
 * (transaction_id, sender, receiver, amount, type, fee, status)
 * plus a pointer to the full Transaction and a timestamp for ordering.
 *
 * Ordering rules (applied before miner selection):
 *   1. fee  DESC  — highest fee picked first
 *   2. submitted_at ASC — ties broken by older submission
 */
typedef struct MempoolEntry {
    char             transaction_id[HASH_SIZE];
    char             sender[ADDR_SIZE];
    char             receiver[ADDR_SIZE];
    double           amount;
    TransactionType  transaction_type;  /* spec 1.iii: field name */
    double           fee;               /* priority value; set by caller */
    MempoolStatus    status;
    Transaction     *tx;                /* owned pointer */
    time_t           submitted_at;
    struct MempoolEntry *next;
} MempoolEntry;

/* ── CRUD ────────────────────────────────────────────────────────────────── */

/*
 * mempool_add — validate and insert tx with given fee.
 * Checks: non-duplicate tx_id, non-negative amount, sender exists.
 * Sets entry status to MEMPOOL_PENDING.
 * Returns 0 on success, -1 on validation failure.
 */
int mempool_add(MempoolEntry **head, Transaction *tx, double fee);

/*
 * mempool_remove_by_id — remove and free the entry with the given tx_id.
 * Returns 0 if found and removed, -1 if not found.
 */
int mempool_remove_by_id(MempoolEntry **head, const char *tx_id);

/*
 * mempool_remove_confirmed — after a block is mined, remove all entries
 * whose tx_id appears in the confirmed_ids array.
 */
void mempool_remove_confirmed(MempoolEntry **head,
                               const char (*confirmed_ids)[HASH_SIZE],
                               uint32_t count);

/* ── Status updates ─────────────────────────────────────────────────────── */

int mempool_mark_suspicious(MempoolEntry *head, const char *tx_id);
int mempool_approve_suspicious(MempoolEntry *head, const char *tx_id);
int mempool_reject_suspicious(MempoolEntry **head, const char *tx_id);

/* ── Selection ───────────────────────────────────────────────────────────── */

/*
 * mempool_select_top — collect up to `max_count` PENDING entries ordered
 * by fee DESC, timestamp ASC.  Writes pointers into out_entries[].
 * Returns actual count selected.
 *
 * NOTE: Does NOT include SUSPICIOUS entries.
 */
uint32_t mempool_select_top(MempoolEntry *head,
                             MempoolEntry **out_entries,
                             uint32_t max_count);

/* ── Sorting ─────────────────────────────────────────────────────────────── */

/*
 * mempool_sort — reorder the linked list by fee DESC, timestamp ASC.
 * (mergesort on a linked list; O(n log n))
 */
MempoolEntry *mempool_sort(MempoolEntry *head);

/* ── Query ───────────────────────────────────────────────────────────────── */

MempoolEntry *mempool_find(MempoolEntry *head, const char *tx_id);
uint32_t      mempool_count(MempoolEntry *head, MempoolStatus filter);

/* ── Display ─────────────────────────────────────────────────────────────── */

void mempool_print(MempoolEntry *head);
void mempool_print_suspicious(MempoolEntry *head);

/* ── Cleanup ─────────────────────────────────────────────────────────────── */

void mempool_free_all(MempoolEntry **head);

#endif /* MEMPOOL_H */
