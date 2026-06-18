#include "reinsurance.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

void reinsurance_init(ReinsurancePool *pool) {
    if (!pool) return;
    memset(pool, 0, sizeof(*pool));
}

void reinsurance_contribute(ReinsurancePool *pool, double amount) {
    if (!pool || amount <= 0.0) return;
    pool->balance        += amount;
    pool->total_received += amount;
    pool->contribution_count++;
}

double reinsurance_disburse(ReinsurancePool *pool, double requested) {
    if (!pool || requested <= 0.0) return 0.0;
    double actual = (pool->balance >= requested) ? requested : pool->balance;
    pool->balance        -= actual;
    pool->total_disbursed += actual;
    if (actual > 0.0) pool->disbursement_count++;
    return actual;
}

void reinsurance_compute_split(double total,
                                double *insurance_out,
                                double *reinsurance_out) {
    if (total <= SETTLEMENT_THRESHOLD) {
        *insurance_out   = total;
        *reinsurance_out = 0.0;
    } else {
        *insurance_out   = SETTLEMENT_THRESHOLD;
        *reinsurance_out = total - SETTLEMENT_THRESHOLD;
    }
}

void reinsurance_print(const ReinsurancePool *pool) {
    if (!pool) return;
    printf("Reinsurance Pool\n");
    printf("  Balance      : %.4f AHT\n", pool->balance);
    printf("  Total in     : %.4f AHT (%u contributions)\n",
           pool->total_received, pool->contribution_count);
    printf("  Total out    : %.4f AHT (%u disbursements)\n",
           pool->total_disbursed, pool->disbursement_count);
}
