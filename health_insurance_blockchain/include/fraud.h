#ifndef FRAUD_H
#define FRAUD_H

#include "types.h"
#include "transaction.h"
#include <time.h>

/* ── Fraud check result ──────────────────────────────────────────────────── */

typedef enum {
    FRAUD_CLEAN               = 0,
    FRAUD_HIGH_FREQ_CLAIMS    = 1,  /* heuristic 1: > 3 claims in 24 h */
    FRAUD_ABNORMAL_AMOUNT     = 2,  /* heuristic 2: > 2× provider avg  */
    FRAUD_DUPLICATE_TX        = 4,  /* heuristic 3: tx_id already known */
    FRAUD_PARTIAL_REINSURANCE = 8   /* spec 3.iii: reinsurance pool short, manual review needed */
} FraudFlag;

/* ── Per-entry audit log ─────────────────────────────────────────────────── */

/*
 * FraudLogEntry is created whenever a transaction is flagged SUSPICIOUS.
 * It persists until the operator runs approve_suspicious or reject_suspicious.
 */
typedef struct FraudLogEntry {
    char             tx_id[HASH_SIZE];
    char             member_id[ID_SIZE];
    char             provider_id[ID_SIZE];
    FraudFlag        flags;            /* bitfield: multiple flags OR'd together */
    time_t           flagged_at;
    char             notes[DESC_SIZE];
    struct FraudLogEntry *next;
} FraudLogEntry;

/* ── Core fraud-check function ───────────────────────────────────────────── */

/*
 * fraud_check_transaction — run all three heuristics against tx.
 *
 * Parameters:
 *   tx             — the candidate transaction
 *   claim_count_24h— pre-computed count of member's claims in last 24 h
 *   provider_avg   — pre-computed average claim amount for the provider
 *   existing_ids   — array of all known tx IDs (chain + mempool) for dup check
 *   id_count       — length of existing_ids
 *
 * Returns a bitfield of FraudFlag values (0 = clean).
 */
FraudFlag fraud_check_transaction(const Transaction *tx,
                                   int claim_count_24h,
                                   double provider_avg,
                                   const char (*existing_ids)[HASH_SIZE],
                                   uint32_t id_count);

/* ── Heuristics (can be called independently for testing) ────────────────── */

/* Returns 1 if member has > MAX_CLAIMS_PER_24H claims in the last 24 h */
int fraud_check_high_frequency(int claim_count_24h);

/* Returns 1 if amount > FRAUD_AMOUNT_MULTIPLIER * provider_avg */
int fraud_check_abnormal_amount(double amount, double provider_avg);

/* Returns 1 if tx_id already exists in the chain or mempool */
int fraud_check_duplicate_tx(const char *tx_id,
                              const char (*existing_ids)[HASH_SIZE],
                              uint32_t id_count);

/* ── Log management ──────────────────────────────────────────────────────── */

FraudLogEntry *fraud_log_add(FraudLogEntry **head, const Transaction *tx,
                              FraudFlag flags, const char *notes);

FraudLogEntry *fraud_log_find(FraudLogEntry *head, const char *tx_id);

int fraud_log_remove(FraudLogEntry **head, const char *tx_id);

void fraud_log_print(FraudLogEntry *head);

void fraud_log_free_all(FraudLogEntry **head);

#endif /* FRAUD_H */
