#ifndef INSURANCE_H
#define INSURANCE_H

#include "types.h"
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Policy
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct Policy {
    char         policy_id[ID_SIZE];
    char         member_id[ID_SIZE];
    char         coverage_plan[NAME_SIZE]; /* e.g. "Basic", "Premium", "Family" */
    time_t       enrollment_date;
    time_t       expiry_date;              /* enrollment_date + 365 days */
    PolicyStatus status;
    double       premium_amount;           /* AHT per payment */
    struct Policy *next;
} Policy;

/* Create a policy (status = ACTIVE, expiry = enroll + 365 days) */
Policy *policy_create(Policy **head, const char *policy_id,
                       const char *member_id, const char *coverage_plan,
                       double premium, time_t enroll_date);

Policy *policy_find(Policy *head, const char *policy_id);
Policy *policy_find_by_member(Policy *head, const char *member_id);

/*
 * policy_check_expiry — if current_time > p->expiry_date, set status to
 * POLICY_EXPIRED and return 1.  Returns 0 if still active, -1 if not found.
 * Must be called on every claim submission.
 */
int policy_check_expiry(Policy *head, const char *policy_id, time_t now);

/*
 * policy_renew — reset expiry_date to now + 365 days, set status RENEWED.
 * Returns 0 on success, -1 if not found.
 */
int policy_renew(Policy *head, const char *policy_id, time_t now);

/* Return 1 if policy exists and is ACTIVE or RENEWED, 0 otherwise */
int policy_is_claimable(Policy *head, const char *policy_id, time_t now);

const char *policy_status_str(PolicyStatus s);

void policy_print(const Policy *p);
void policy_print_all(Policy *head);
void policy_free_all(Policy **head);

/* ═══════════════════════════════════════════════════════════════════════════
 * Claim
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct Claim {
    char        claim_id[ID_SIZE];
    char        member_id[ID_SIZE];
    char        policy_id[ID_SIZE];
    char        provider_id[ID_SIZE];
    double      claimed_amount;
    double      approved_amount;    /* set during TX_CLAIM_APPROVE */
    ClaimStatus status;
    time_t      submitted_at;
    time_t      service_date;
    char        diagnosis[DESC_SIZE];
    char        rejection_reason[DESC_SIZE];
    struct Claim *next;
} Claim;

Claim *claim_create(Claim **head, const char *claim_id,
                     const char *member_id, const char *policy_id,
                     const char *provider_id, double amount,
                     time_t service_date, const char *diagnosis);

Claim *claim_find(Claim *head, const char *claim_id);

int claim_approve(Claim *head, const char *claim_id, double approved_amount);
int claim_reject (Claim *head, const char *claim_id, const char *reason);
int claim_settle (Claim *head, const char *claim_id);

/* Count claims by member_id within a rolling 24-hour window ending at `now` */
int claim_count_in_window(Claim *head, const char *member_id, time_t now);

/* Average claimed amount for a given provider (for fraud heuristic #2) */
double claim_provider_average(Claim *head, const char *provider_id);

const char *claim_status_str(ClaimStatus s);

void claim_print(const Claim *c);
void claim_print_all(Claim *head);
void claim_print_by_member(Claim *head, const char *member_id);
void claim_print_by_provider(Claim *head, const char *provider_id);
void claim_free_all(Claim **head);

/* ═══════════════════════════════════════════════════════════════════════════
 * Pre-authorisation
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct Preauth {
    char          preauth_id[ID_SIZE];
    char          member_id[ID_SIZE];
    char          provider_id[ID_SIZE];
    char          service_type[NAME_SIZE];
    double        estimated_cost;
    double        approved_amount;
    PreauthStatus status;
    time_t        requested_at;
    struct Preauth *next;
} Preauth;

Preauth *preauth_create(Preauth **head, const char *preauth_id,
                         const char *member_id, const char *provider_id,
                         const char *service_type, double estimated_cost);

Preauth *preauth_find(Preauth *head, const char *preauth_id);

int preauth_approve(Preauth *head, const char *preauth_id,
                    double approved_amount);
int preauth_reject (Preauth *head, const char *preauth_id);

const char *preauth_status_str(PreauthStatus s);

void preauth_print(const Preauth *p);
void preauth_print_all(Preauth *head);
void preauth_free_all(Preauth **head);

#endif /* INSURANCE_H */
