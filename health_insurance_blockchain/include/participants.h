#ifndef PARTICIPANTS_H
#define PARTICIPANTS_H

#include "types.h"
#include "crypto.h"

/* ── Member ──────────────────────────────────────────────────────────────── */

/*
 * Member represents an ALU student or staff policyholder.
 *
 *   member_id     — unique identifier (e.g. "MEM001")
 *   name          — display name
 *   keys          — P-256 key pair; wallet_address is the on-chain address
 *   active_policy — ID of the member's currently active policy, or ""
 */
typedef struct Member {
    char         member_id[ID_SIZE];
    char         name[NAME_SIZE];
    KeyPair      keys;               /* includes wallet_address */
    char         active_policy[ID_SIZE];
    struct Member *next;
} Member;

/* ── Provider ────────────────────────────────────────────────────────────── */

/*
 * Provider represents a healthcare facility registered with the scheme.
 *
 *   provider_id   — unique identifier (e.g. "PROV001")
 *   name          — clinic or hospital name
 *   specialty     — e.g. "General Practice", "Dental", "Radiology"
 *   keys          — P-256 key pair for signing claim transactions
 */
typedef struct Provider {
    char          provider_id[ID_SIZE];
    char          name[NAME_SIZE];
    char          specialty[NAME_SIZE];
    KeyPair       keys;
    struct Provider *next;
} Provider;

/* ── Member operations ───────────────────────────────────────────────────── */

/*
 * member_register — generate a fresh key pair, allocate a Member, prepend
 *                   to *head, and create a matching Account in the account
 *                   ledger.  Returns the new Member or NULL on failure.
 */
Member *member_register(Member **head, const char *member_id,
                         const char *name);

Member *member_find(Member *head, const char *member_id);
Member *member_find_by_address(Member *head, const char *address);

const char *member_pubkey(Member *head, const char *member_id);

void member_print(const Member *m);
void member_print_all(Member *head);
void member_free_all(Member **head);

/* ── Provider operations ─────────────────────────────────────────────────── */

Provider *provider_register(Provider **head, const char *provider_id,
                              const char *name, const char *specialty);

Provider *provider_find(Provider *head, const char *provider_id);
Provider *provider_find_by_address(Provider *head, const char *address);

const char *provider_pubkey(Provider *head, const char *provider_id);

void provider_print(const Provider *p);
void provider_print_all(Provider *head);
void provider_free_all(Provider **head);

#endif /* PARTICIPANTS_H */
