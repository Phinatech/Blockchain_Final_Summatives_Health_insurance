#include "insurance.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Policy ──────────────────────────────────────────────────────────────── */

Policy *policy_create(Policy **head, const char *policy_id,
                       const char *member_id, const char *coverage_plan,
                       double premium, time_t enroll_date) {
    Policy *p = calloc(1, sizeof(Policy));
    if (!p) return NULL;
    strncpy(p->policy_id,      policy_id,      ID_SIZE   - 1);
    strncpy(p->member_id,      member_id,      ID_SIZE   - 1);
    strncpy(p->coverage_plan,  coverage_plan,  NAME_SIZE - 1);
    p->enrollment_date = enroll_date;
    p->expiry_date     = enroll_date + (365 * 24 * 60 * 60);
    p->status          = POLICY_ACTIVE;
    p->premium_amount  = premium;
    p->next = *head;
    *head   = p;
    return p;
}

Policy *policy_find(Policy *head, const char *policy_id) {
    for (Policy *p = head; p; p = p->next)
        if (strncmp(p->policy_id, policy_id, ID_SIZE) == 0) return p;
    return NULL;
}

Policy *policy_find_by_member(Policy *head, const char *member_id) {
    for (Policy *p = head; p; p = p->next)
        if (strncmp(p->member_id, member_id, ID_SIZE) == 0 &&
            p->status != POLICY_EXPIRED) return p;
    return NULL;
}

int policy_check_expiry(Policy *head, const char *policy_id, time_t now) {
    Policy *p = policy_find(head, policy_id);
    if (!p) return -1;
    if (now > p->expiry_date && p->status != POLICY_EXPIRED) {
        p->status = POLICY_EXPIRED;
        return 1;
    }
    return 0;
}

int policy_renew(Policy *head, const char *policy_id, time_t now) {
    Policy *p = policy_find(head, policy_id);
    if (!p) return -1;
    p->expiry_date = now + (365 * 24 * 60 * 60);
    p->status = POLICY_RENEWED;
    return 0;
}

int policy_is_claimable(Policy *head, const char *policy_id, time_t now) {
    Policy *p = policy_find(head, policy_id);
    if (!p) return 0;
    if (policy_check_expiry(head, policy_id, now) == 1) return 0;
    return (p->status == POLICY_ACTIVE || p->status == POLICY_RENEWED) ? 1 : 0;
}

const char *policy_status_str(PolicyStatus s) {
    switch (s) {
        case POLICY_ACTIVE:    return "ACTIVE";
        case POLICY_EXPIRED:   return "EXPIRED";
        case POLICY_RENEWED:   return "RENEWED";
        case POLICY_SUSPENDED: return "SUSPENDED";
        default:               return "UNKNOWN";
    }
}

void policy_print(const Policy *p) {
    if (!p) return;
    char exp_buf[32], enr_buf[32];
    strftime(enr_buf, sizeof(enr_buf), "%Y-%m-%d", localtime(&p->enrollment_date));
    strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d", localtime(&p->expiry_date));
    printf("  Policy %-16s  member=%-12s  plan=%-12s  "
           "enrolled=%s  expires=%s  status=%s  premium=%.2f AHT\n",
           p->policy_id, p->member_id, p->coverage_plan,
           enr_buf, exp_buf, policy_status_str(p->status), p->premium_amount);
}

void policy_print_all(Policy *head) {
    printf("=== Policies ===\n");
    for (Policy *p = head; p; p = p->next) policy_print(p);
}

void policy_free_all(Policy **head) {
    Policy *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}

/* ── Claim ───────────────────────────────────────────────────────────────── */

Claim *claim_create(Claim **head, const char *claim_id,
                     const char *member_id, const char *policy_id,
                     const char *provider_id, double amount,
                     time_t service_date, const char *diagnosis) {
    Claim *c = calloc(1, sizeof(Claim));
    if (!c) return NULL;
    strncpy(c->claim_id,    claim_id,    ID_SIZE   - 1);
    strncpy(c->member_id,   member_id,   ID_SIZE   - 1);
    strncpy(c->policy_id,   policy_id,   ID_SIZE   - 1);
    strncpy(c->provider_id, provider_id, ID_SIZE   - 1);
    strncpy(c->diagnosis,   diagnosis,   DESC_SIZE - 1);
    c->claimed_amount = amount;
    c->status         = CLAIM_PENDING;
    c->submitted_at   = time(NULL);
    c->service_date   = service_date;
    c->next = *head;
    *head   = c;
    return c;
}

Claim *claim_find(Claim *head, const char *claim_id) {
    for (Claim *c = head; c; c = c->next)
        if (strncmp(c->claim_id, claim_id, ID_SIZE) == 0) return c;
    return NULL;
}

int claim_approve(Claim *head, const char *claim_id, double approved_amount) {
    Claim *c = claim_find(head, claim_id);
    if (!c || c->status != CLAIM_PENDING) return -1;
    c->approved_amount = approved_amount;
    c->status          = CLAIM_APPROVED;
    return 0;
}

int claim_reject(Claim *head, const char *claim_id, const char *reason) {
    Claim *c = claim_find(head, claim_id);
    if (!c || c->status != CLAIM_PENDING) return -1;
    c->status = CLAIM_REJECTED;
    strncpy(c->rejection_reason, reason ? reason : "no reason given", DESC_SIZE - 1);
    return 0;
}

int claim_settle(Claim *head, const char *claim_id) {
    Claim *c = claim_find(head, claim_id);
    if (!c || c->status != CLAIM_APPROVED) return -1;
    c->status = CLAIM_SETTLED;
    return 0;
}

int claim_count_in_window(Claim *head, const char *member_id, time_t now) {
    int count = 0;
    time_t window = now - (24 * 60 * 60);
    for (Claim *c = head; c; c = c->next)
        if (strncmp(c->member_id, member_id, ID_SIZE) == 0 &&
            c->submitted_at >= window) count++;
    return count;
}

double claim_provider_average(Claim *head, const char *provider_id) {
    double total = 0.0; int count = 0;
    for (Claim *c = head; c; c = c->next)
        if (strncmp(c->provider_id, provider_id, ID_SIZE) == 0) {
            total += c->claimed_amount; count++;
        }
    return (count > 0) ? total / count : 0.0;
}

const char *claim_status_str(ClaimStatus s) {
    switch (s) {
        case CLAIM_PENDING:  return "PENDING";
        case CLAIM_APPROVED: return "APPROVED";
        case CLAIM_REJECTED: return "REJECTED";
        case CLAIM_SETTLED:  return "SETTLED";
        default:             return "UNKNOWN";
    }
}

void claim_print(const Claim *c) {
    if (!c) return;
    printf("  Claim %-12s  member=%-10s  provider=%-10s  "
           "amount=%.2f AHT  status=%s\n",
           c->claim_id, c->member_id, c->provider_id,
           c->claimed_amount, claim_status_str(c->status));
}

void claim_print_all(Claim *head) {
    printf("=== Claims ===\n");
    for (Claim *c = head; c; c = c->next) claim_print(c);
}

void claim_print_by_member(Claim *head, const char *member_id) {
    printf("=== Claims for member %s ===\n", member_id);
    for (Claim *c = head; c; c = c->next)
        if (strncmp(c->member_id, member_id, ID_SIZE) == 0) claim_print(c);
}

void claim_print_by_provider(Claim *head, const char *provider_id) {
    printf("=== Claims for provider %s ===\n", provider_id);
    for (Claim *c = head; c; c = c->next)
        if (strncmp(c->provider_id, provider_id, ID_SIZE) == 0) claim_print(c);
}

void claim_free_all(Claim **head) {
    Claim *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}

/* ── Preauth ─────────────────────────────────────────────────────────────── */

Preauth *preauth_create(Preauth **head, const char *preauth_id,
                         const char *member_id, const char *provider_id,
                         const char *service_type, double estimated_cost) {
    Preauth *p = calloc(1, sizeof(Preauth));
    if (!p) return NULL;
    strncpy(p->preauth_id,   preauth_id,    ID_SIZE   - 1);
    strncpy(p->member_id,    member_id,     ID_SIZE   - 1);
    strncpy(p->provider_id,  provider_id,   ID_SIZE   - 1);
    strncpy(p->service_type, service_type,  NAME_SIZE - 1);
    p->estimated_cost = estimated_cost;
    p->status         = PREAUTH_PENDING;
    p->requested_at   = time(NULL);
    p->next = *head;
    *head   = p;
    return p;
}

Preauth *preauth_find(Preauth *head, const char *preauth_id) {
    for (Preauth *p = head; p; p = p->next)
        if (strncmp(p->preauth_id, preauth_id, ID_SIZE) == 0) return p;
    return NULL;
}

int preauth_approve(Preauth *head, const char *preauth_id,
                    double approved_amount) {
    Preauth *p = preauth_find(head, preauth_id);
    if (!p || p->status != PREAUTH_PENDING) return -1;
    p->approved_amount = approved_amount;
    p->status          = PREAUTH_APPROVED;
    return 0;
}

int preauth_reject(Preauth *head, const char *preauth_id) {
    Preauth *p = preauth_find(head, preauth_id);
    if (!p || p->status != PREAUTH_PENDING) return -1;
    p->status = PREAUTH_REJECTED;
    return 0;
}

const char *preauth_status_str(PreauthStatus s) {
    switch (s) {
        case PREAUTH_PENDING:  return "PENDING";
        case PREAUTH_APPROVED: return "APPROVED";
        case PREAUTH_REJECTED: return "REJECTED";
        default:               return "UNKNOWN";
    }
}

void preauth_print(const Preauth *p) {
    if (!p) return;
    printf("  Preauth %-12s  member=%-10s  provider=%-10s  "
           "service=%-16s  est=%.2f  status=%s\n",
           p->preauth_id, p->member_id, p->provider_id,
           p->service_type, p->estimated_cost,
           preauth_status_str(p->status));
}

void preauth_print_all(Preauth *head) {
    printf("=== Pre-authorisations ===\n");
    for (Preauth *p = head; p; p = p->next) preauth_print(p);
}

void preauth_free_all(Preauth **head) {
    Preauth *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}
