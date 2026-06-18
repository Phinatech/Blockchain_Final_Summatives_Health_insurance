#include "fraud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FraudFlag fraud_check_transaction(const Transaction *tx,
                                   int claim_count_24h,
                                   double provider_avg,
                                   const char (*existing_ids)[HASH_SIZE],
                                   uint32_t id_count) {
    FraudFlag flags = FRAUD_CLEAN;
    if (fraud_check_high_frequency(claim_count_24h))
        flags |= FRAUD_HIGH_FREQ_CLAIMS;
    if (provider_avg > 0.0 && fraud_check_abnormal_amount(tx->amount, provider_avg))
        flags |= FRAUD_ABNORMAL_AMOUNT;
    if (fraud_check_duplicate_tx(tx->transaction_id, existing_ids, id_count))
        flags |= FRAUD_DUPLICATE_TX;
    return flags;
}

int fraud_check_high_frequency(int claim_count_24h) {
    return claim_count_24h > MAX_CLAIMS_PER_24H ? 1 : 0;
}

int fraud_check_abnormal_amount(double amount, double provider_avg) {
    return (provider_avg > 0.0 && amount > FRAUD_AMOUNT_MULTIPLIER * provider_avg) ? 1 : 0;
}

int fraud_check_duplicate_tx(const char *tx_id,
                              const char (*existing_ids)[HASH_SIZE],
                              uint32_t id_count) {
    for (uint32_t i = 0; i < id_count; i++)
        if (strncmp(existing_ids[i], tx_id, HASH_SIZE) == 0) return 1;
    return 0;
}

FraudLogEntry *fraud_log_add(FraudLogEntry **head, const Transaction *tx,
                              FraudFlag flags, const char *notes) {
    FraudLogEntry *e = calloc(1, sizeof(FraudLogEntry));
    if (!e) return NULL;
    strncpy(e->tx_id, tx->transaction_id, HASH_SIZE - 1);
    /* Extract member_id from claim_submit payload if applicable */
    if (tx->transaction_type == TX_CLAIM_SUBMIT)
        strncpy(e->member_id,   tx->payload.claim_submit.member_id,   ID_SIZE - 1);
    if (tx->transaction_type == TX_CLAIM_SUBMIT)
        strncpy(e->provider_id, tx->payload.claim_submit.provider_id, ID_SIZE - 1);
    e->flags      = flags;
    e->flagged_at = time(NULL);
    if (notes) strncpy(e->notes, notes, DESC_SIZE - 1);
    e->next = *head;
    *head   = e;
    return e;
}

FraudLogEntry *fraud_log_find(FraudLogEntry *head, const char *tx_id) {
    for (FraudLogEntry *e = head; e; e = e->next)
        if (strncmp(e->tx_id, tx_id, HASH_SIZE) == 0) return e;
    return NULL;
}

int fraud_log_remove(FraudLogEntry **head, const char *tx_id) {
    FraudLogEntry *prev = NULL, *cur = *head;
    while (cur) {
        if (strncmp(cur->tx_id, tx_id, HASH_SIZE) == 0) {
            if (prev) prev->next = cur->next; else *head = cur->next;
            free(cur);
            return 0;
        }
        prev = cur; cur = cur->next;
    }
    return -1;
}

void fraud_log_print(FraudLogEntry *head) {
    printf("=== Fraud Review Queue ===\n");
    int count = 0;
    for (FraudLogEntry *e = head; e; e = e->next, count++) {
        printf("  TX : %.16s...\n", e->tx_id);
        printf("  Member  : %s  Provider: %s\n", e->member_id, e->provider_id);
        printf("  Flags   : %s%s%s%s\n",
               (e->flags & FRAUD_HIGH_FREQ_CLAIMS)    ? "[HIGH-FREQ] "       : "",
               (e->flags & FRAUD_ABNORMAL_AMOUNT)      ? "[ABN-AMOUNT] "     : "",
               (e->flags & FRAUD_DUPLICATE_TX)         ? "[DUPLICATE] "      : "",
               (e->flags & FRAUD_PARTIAL_REINSURANCE)  ? "[PARTIAL-REINSR] " : "");
        printf("  Notes   : %s\n", e->notes);
        printf("  Flagged : %s", ctime(&e->flagged_at));
        printf("  ---\n");
    }
    if (count == 0) printf("  (no suspicious transactions)\n");
}

void fraud_log_free_all(FraudLogEntry **head) {
    FraudLogEntry *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}
