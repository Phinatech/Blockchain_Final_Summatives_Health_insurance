#include "mempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── mempool.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/mempool.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int mempool_add(MempoolEntry **head, Transaction *tx, double fee) {
    if (!head || !tx) return -1;

    /* Section 7.v: amount must be positive and non-zero */
    if (tx->amount <= 0.0 &&
        tx->transaction_type != TX_SERVICE_REQUEST &&
        tx->transaction_type != TX_PREAUTH_REQUEST &&
        tx->transaction_type != TX_PREAUTH_APPROVE &&
        tx->transaction_type != TX_CLAIM_APPROVE &&
        tx->transaction_type != TX_CLAIM_REJECT) {
        fprintf(stderr, "[mempool] rejected: amount %.4f is not positive\n", tx->amount);
        return -1;
    }

    /* Duplicate check */
    if (mempool_find(*head, tx->transaction_id)) {
        fprintf(stderr, "[mempool] rejected: duplicate tx_id %.16s...\n", tx->transaction_id);
        return -1;
    }
    MempoolEntry *e = calloc(1, sizeof(MempoolEntry));
    if (!e) return -1;
    strncpy(e->transaction_id, tx->transaction_id, HASH_SIZE - 1);
    strncpy(e->sender,   tx->sender_address,   ADDR_SIZE - 1);
    strncpy(e->receiver, tx->receiver_address, ADDR_SIZE - 1);
    e->amount       = tx->amount;
    e->transaction_type = tx->transaction_type;
    e->fee          = fee;
    e->status       = MEMPOOL_PENDING;
    e->tx           = tx;
    e->submitted_at = time(NULL);
    e->next = *head;
    *head   = e;
    return 0;
}

int mempool_remove_by_id(MempoolEntry **head, const char *tx_id) {
    MempoolEntry *prev = NULL, *cur = *head;
    while (cur) {
        if (strncmp(cur->transaction_id, tx_id, HASH_SIZE) == 0) {
            if (prev) prev->next = cur->next; else *head = cur->next;
            /* tx owned by block after confirmation; just free the entry */
            free(cur);
            return 0;
        }
        prev = cur; cur = cur->next;
    }
    return -1;
}

void mempool_remove_confirmed(MempoolEntry **head,
                               const char (*confirmed_ids)[HASH_SIZE],
                               uint32_t count) {
    for (uint32_t i = 0; i < count; i++)
        mempool_remove_by_id(head, confirmed_ids[i]);
}

int mempool_mark_suspicious(MempoolEntry *head, const char *tx_id) {
    MempoolEntry *e = mempool_find(head, tx_id);
    if (!e) return -1;
    e->status = MEMPOOL_SUSPICIOUS;
    return 0;
}

int mempool_approve_suspicious(MempoolEntry *head, const char *tx_id) {
    MempoolEntry *e = mempool_find(head, tx_id);
    if (!e || e->status != MEMPOOL_SUSPICIOUS) return -1;
    e->status = MEMPOOL_PENDING;
    return 0;
}

int mempool_reject_suspicious(MempoolEntry **head, const char *tx_id) {
    MempoolEntry *e = mempool_find(*head, tx_id);
    if (!e || e->status != MEMPOOL_SUSPICIOUS) return -1;
    return mempool_remove_by_id(head, tx_id);
}

/*
 * merge_sorted — merge two already-sorted halves into one sorted list.
 *
 * Ordering rule (spec Section 4): fee DESC, then submitted_at ASC.
 *   - Higher fee wins: miners maximise revenue by picking the most
 *     profitable transactions first.
 *   - Older timestamp wins on equal fee: FIFO fairness so no transaction
 *     starves indefinitely while its fee-equal peers jump ahead.
 *
 * This is a recursive merge (O(n) per call). It works by choosing the
 * smaller head each time and prepending it to the merged remainder.
 * The recursion depth is at most n, which is fine for mempool sizes.
 */
static MempoolEntry *merge_sorted(MempoolEntry *a, MempoolEntry *b) {
    if (!a) return b;   /* base cases: one half is exhausted */
    if (!b) return a;
    /* a wins if: higher fee, OR same fee but older (smaller timestamp) */
    if (a->fee > b->fee || (a->fee == b->fee && a->submitted_at <= b->submitted_at)) {
        a->next = merge_sorted(a->next, b);   /* a is the next head; recurse on tail */
        return a;
    }
    b->next = merge_sorted(a, b->next);       /* b wins this round */
    return b;
}

/*
 * split_list — divide a linked list into two halves.
 *
 * Uses the slow/fast pointer (tortoise-and-hare) algorithm:
 *   - `slow` advances one node per iteration
 *   - `fast` advances two nodes per iteration
 * When `fast` reaches the end, `slow` is at the midpoint.
 * Cutting the list at `slow` gives two roughly equal halves,
 * which is the O(n log n) guarantee of mergesort.
 *
 * Example: A→B→C→D→E
 *   After loop: slow=C, fast=E  →  *a=A→B→C, *b=D→E
 */
static void split_list(MempoolEntry *src, MempoolEntry **a, MempoolEntry **b) {
    MempoolEntry *slow = src;
    MempoolEntry *fast = src->next;
    /* Advance fast by 2 and slow by 1 until fast hits the end */
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    *a = src;             /* first half: from src to slow (inclusive) */
    *b = slow->next;      /* second half: from slow->next to end      */
    slow->next = NULL;    /* sever the link so the two halves are independent */
}

/*
 * mempool_sort — sort the entire mempool linked list using mergesort.
 *
 * Mergesort is chosen because:
 *   1. It is O(n log n) worst-case — unlike insertion sort O(n²).
 *   2. It works naturally on linked lists without needing random access.
 *   3. It is stable — equal-fee entries maintain their relative order.
 *
 * The algorithm:
 *   1. Base case: 0 or 1 nodes are already sorted.
 *   2. Split the list into two halves.
 *   3. Recursively sort each half.
 *   4. Merge the two sorted halves.
 */
MempoolEntry *mempool_sort(MempoolEntry *head) {
    if (!head || !head->next) return head;   /* 0 or 1 nodes: trivially sorted */
    MempoolEntry *a, *b;
    split_list(head, &a, &b);               /* divide */
    a = mempool_sort(a);                    /* conquer left  */
    b = mempool_sort(b);                    /* conquer right */
    return merge_sorted(a, b);             /* combine */
}

uint32_t mempool_select_top(MempoolEntry *head,
                             MempoolEntry **out, uint32_t max) {
    MempoolEntry *sorted = mempool_sort(head); /* non-destructive if we rebuild */
    uint32_t n = 0;
    for (MempoolEntry *e = sorted; e && n < max; e = e->next)
        if (e->status == MEMPOOL_PENDING) out[n++] = e;
    return n;
}

MempoolEntry *mempool_find(MempoolEntry *head, const char *tx_id) {
    for (MempoolEntry *e = head; e; e = e->next)
        if (strncmp(e->transaction_id, tx_id, HASH_SIZE) == 0) return e;
    return NULL;
}

uint32_t mempool_count(MempoolEntry *head, MempoolStatus filter) {
    uint32_t n = 0;
    for (MempoolEntry *e = head; e; e = e->next)
        if (e->status == filter) n++;
    return n;
}

static const char *mstatus_str(MempoolStatus s) {
    switch (s) {
        case MEMPOOL_PENDING:    return "PENDING";
        case MEMPOOL_CONFIRMED:  return "CONFIRMED";
        case MEMPOOL_SUSPICIOUS: return "SUSPICIOUS";
        default:                 return "UNKNOWN";
    }
}

void mempool_print(MempoolEntry *head) {
    printf("=== Mempool ===\n");
    uint32_t n = 0;
    for (MempoolEntry *e = head; e; e = e->next, n++) {
        printf("  [%-10s] %-22s  amount=%.4f  fee=%.4f  from=%.12s...\n",
               mstatus_str(e->status), tx_type_str(e->transaction_type),
               e->amount, e->fee, e->sender);
    }
    if (n == 0) printf("  (empty)\n");
    else printf("  Total: %u  Pending: %u  Suspicious: %u\n", n,
                mempool_count(head, MEMPOOL_PENDING),
                mempool_count(head, MEMPOOL_SUSPICIOUS));
}

void mempool_print_suspicious(MempoolEntry *head) {
    printf("=== Suspicious mempool entries ===\n");
    for (MempoolEntry *e = head; e; e = e->next)
        if (e->status == MEMPOOL_SUSPICIOUS)
            printf("  TX %.16s...  type=%s  amount=%.4f\n",
                   e->transaction_id, tx_type_str(e->transaction_type), e->amount);
}

void mempool_free_all(MempoolEntry **head) {
    MempoolEntry *cur = *head, *nxt;
    while (cur) { nxt = cur->next; tx_free(cur->tx); free(cur); cur = nxt; }
    *head = NULL;
}
