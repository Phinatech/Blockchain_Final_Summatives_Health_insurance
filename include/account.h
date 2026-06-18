#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "types.h"
#include <stdint.h>

/* ── Account struct ──────────────────────────────────────────────────────── */

/*
 * Every participant on the chain has an Account.
 *
 *   address     — wallet address (SHA-256 of public key, hex)
 *   label       — human-readable name ("Alice", "Insurance Pool", …)
 *   balance     — current AHT balance
 *   nonce       — starts at 0; increments by 1 each time a transaction
 *                 sent by this account is confirmed in a mined block.
 *
 * Replay-protection rule (Section 7.iii):
 *   When a transaction is created, sender copies account.nonce into
 *   tx.sender_nonce.  The network accepts the tx only if:
 *       tx.sender_nonce == account.nonce + 1
 *   The account.nonce increments only on block confirmation, not on
 *   mempool insertion.
 */
typedef struct Account {
    char          address[ADDR_SIZE];
    char          label[NAME_SIZE];
    double        balance;
    uint64_t      nonce;
    struct Account *next;
} Account;

/* ── Required accounts (created at chain initialisation) ─────────────────── */

/*
 * Five wallets must exist from genesis:
 *   ADDR_INSURANCE_POOL, ADDR_REINSURANCE_POOL, ADDR_MINER_REWARD,
 *   plus a generic member wallet and a provider wallet created per-user.
 */

/* ── CRUD ────────────────────────────────────────────────────────────────── */

/*
 * account_create — allocate and prepend a new account to *head.
 * Returns the new Account pointer, or NULL on OOM.
 */
Account *account_create(Account **head, const char *address,
                         const char *label, double initial_balance);

/* ── Lookup ──────────────────────────────────────────────────────────────── */

Account *account_find(Account *head, const char *address);
Account *account_find_by_label(Account *head, const char *label);

/* ── Balance operations ──────────────────────────────────────────────────── */

/*
 * account_credit  — add amount to address's balance.
 * account_debit   — subtract amount; returns -1 if insufficient funds.
 * account_transfer— atomic debit from + credit to; returns -1 on failure.
 */
int account_credit  (Account *head, const char *address, double amount);
int account_debit   (Account *head, const char *address, double amount);
int account_transfer(Account *head, const char *from, const char *to,
                     double amount);

double account_get_balance(Account *head, const char *address);

/* ── Nonce management ────────────────────────────────────────────────────── */

/*
 * account_get_next_nonce — returns account.nonce + 1 (value to put in
 * a new outgoing transaction's sender_nonce field before signing).
 */
uint64_t account_get_next_nonce(Account *head, const char *address);

/*
 * account_validate_nonce — returns 1 if tx_nonce == account.nonce + 1,
 * 0 if mismatch (reject tx), -1 if account not found.
 */
int account_validate_nonce(Account *head, const char *address,
                            uint64_t tx_nonce);

/*
 * account_confirm_nonce — called after a tx is confirmed in a block;
 * increments account.nonce by 1.
 */
int account_confirm_nonce(Account *head, const char *address);

/* ── Display ─────────────────────────────────────────────────────────────── */

void account_print(const Account *a);
void account_print_all(Account *head);

/* ── Cleanup ─────────────────────────────────────────────────────────────── */

void account_free_all(Account **head);

#endif /* ACCOUNT_H */
