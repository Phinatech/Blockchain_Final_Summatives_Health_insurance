#ifndef REINSURANCE_H
#define REINSURANCE_H

#include "types.h"

/* ── Reinsurance pool ────────────────────────────────────────────────────── */

/*
 * The reinsurance pool is a second-tier buffer funded automatically from
 * every premium payment (5 % of the premium amount).
 *
 * For any claim settlement exceeding SETTLEMENT_THRESHOLD AHT:
 *   - The main insurance pool pays the first SETTLEMENT_THRESHOLD AHT.
 *   - The reinsurance pool pays the remainder.
 *   - If the reinsurance pool cannot cover the remainder, the settlement
 *     is partially approved and flagged for manual review.
 */
typedef struct {
    double   balance;           /* current AHT balance */
    double   total_received;    /* cumulative contributions */
    double   total_disbursed;   /* cumulative payouts */
    uint32_t contribution_count;
    uint32_t disbursement_count;
} ReinsurancePool;

/* ── Operations ──────────────────────────────────────────────────────────── */

void reinsurance_init(ReinsurancePool *pool);

/*
 * reinsurance_contribute — deposit `amount` into the pool.
 * Called automatically when processing TX_REINSURANCE_CONTRIBUTION.
 */
void reinsurance_contribute(ReinsurancePool *pool, double amount);

/*
 * reinsurance_disburse — withdraw up to `requested` from the pool.
 *
 * Returns the actual amount disbursed:
 *   - If pool.balance >= requested: returns requested (full payout).
 *   - If pool.balance < requested but > 0: returns pool.balance (partial).
 *   - If pool.balance == 0: returns 0.0 (flag for manual review).
 *
 * Partial payouts are expected to trigger a "flagged for manual review"
 * log entry by the caller.
 */
double reinsurance_disburse(ReinsurancePool *pool, double requested);

/*
 * reinsurance_compute_split — given a total settlement amount, compute
 * how much the insurance pool pays (insurance_out) and how much to request
 * from reinsurance (reinsuarance_out).
 *
 * If total <= SETTLEMENT_THRESHOLD: insurance covers all, reinsurance = 0.
 * Otherwise: insurance = SETTLEMENT_THRESHOLD, reinsurance = total - threshold.
 */
void reinsurance_compute_split(double total,
                                double *insurance_out,
                                double *reinsurance_out);

/* ── Display ─────────────────────────────────────────────────────────────── */

void reinsurance_print(const ReinsurancePool *pool);

#endif /* REINSURANCE_H */
