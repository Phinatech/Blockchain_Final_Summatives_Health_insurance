#ifndef UTXO_H
#define UTXO_H

#include "types.h"
#include <stdint.h>

/* ── UTXO struct ─────────────────────────────────────────────────────────── */

/*
 * A UTXO represents an unspent transaction output.
 *
 *   utxo_id       — unique key: "<source_tx_id>:<output_index>"
 *   owner_address — current owner's wallet address
 *   amount        — value in AHT
 *   source_tx_id  — the transaction that created this output
 *   output_index  — output slot index within the source transaction
 *   spent         — 0 = unspent, 1 = spent (marked; never deleted from set)
 *   spent_by_tx   — tx_id of the transaction that consumed this UTXO
 */
typedef struct UTXO {
    char         utxo_id[HASH_SIZE + 12];  /* tx_id:index */
    char         owner_address[ADDR_SIZE];
    double       amount;
    char         source_tx_id[HASH_SIZE];
    uint32_t     output_index;
    int          spent;
    char         spent_by_tx[HASH_SIZE];
    struct UTXO *next;
} UTXO;

/* ── CRUD ────────────────────────────────────────────────────────────────── */

/*
 * utxo_create — allocate and register a new unspent output.
 * Returns the new UTXO pointer (prepended to *head), or NULL on OOM.
 */
UTXO *utxo_create(UTXO **head, const char *owner, double amount,
                   const char *source_tx_id, uint32_t output_index);

/*
 * utxo_spend — mark the UTXO as spent by spending_tx_id.
 * Returns 0 on success, -1 if not found, -2 if already spent.
 */
int utxo_spend(UTXO *head, const char *utxo_id,
               const char *spending_tx_id);

/* ── Lookup ──────────────────────────────────────────────────────────────── */

UTXO *utxo_find(UTXO *head, const char *utxo_id);

/* utxo_get_balance — sum all unspent UTXOs owned by `address` */
double utxo_get_balance(UTXO *head, const char *address);

/* ── Validation ──────────────────────────────────────────────────────────── */

/*
 * utxo_validate_spend — check that utxo_id exists, is unspent,
 * and is owned by `spender_address`.
 * Returns 1 valid, 0 invalid, -1 error.
 */
int utxo_validate_spend(UTXO *head, const char *utxo_id,
                         const char *spender_address);

/*
 * utxo_check_double_spend — return 1 if utxo_id is already marked spent,
 * 0 if still unspent, -1 if not found.
 */
int utxo_check_double_spend(UTXO *head, const char *utxo_id);

/* ── Display ─────────────────────────────────────────────────────────────── */

void utxo_print_all(UTXO *head);
void utxo_print_by_owner(UTXO *head, const char *address);

/* ── Cleanup ─────────────────────────────────────────────────────────────── */

void utxo_free_all(UTXO **head);

#endif /* UTXO_H */
