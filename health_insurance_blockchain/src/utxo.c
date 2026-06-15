#include "utxo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Build the composite utxo_id: "<tx_id>:<output_index>" */
static void make_utxo_id(char *buf, size_t buf_sz,
                          const char *tx_id, uint32_t idx) {
    snprintf(buf, buf_sz, "%s:%u", tx_id, idx);
}

UTXO *utxo_create(UTXO **head, const char *owner, double amount,
                   const char *source_tx_id, uint32_t output_index) {
    UTXO *u = calloc(1, sizeof(UTXO));
    if (!u) return NULL;
    make_utxo_id(u->utxo_id, sizeof(u->utxo_id), source_tx_id, output_index);
    strncpy(u->owner_address, owner, ADDR_SIZE - 1);
    u->amount        = amount;
    strncpy(u->source_tx_id, source_tx_id, HASH_SIZE - 1);
    u->output_index  = output_index;
    u->spent         = 0;
    u->next          = *head;
    *head            = u;
    return u;
}

int utxo_spend(UTXO *head, const char *utxo_id, const char *spending_tx_id) {
    UTXO *u = utxo_find(head, utxo_id);
    if (!u)        return -1;
    if (u->spent)  return -2;
    u->spent = 1;
    strncpy(u->spent_by_tx, spending_tx_id, HASH_SIZE - 1);
    return 0;
}

UTXO *utxo_find(UTXO *head, const char *utxo_id) {
    for (UTXO *u = head; u; u = u->next)
        if (strncmp(u->utxo_id, utxo_id, sizeof(u->utxo_id)) == 0) return u;
    return NULL;
}

double utxo_get_balance(UTXO *head, const char *address) {
    double total = 0.0;
    for (UTXO *u = head; u; u = u->next)
        if (!u->spent && strncmp(u->owner_address, address, ADDR_SIZE) == 0)
            total += u->amount;
    return total;
}

int utxo_validate_spend(UTXO *head, const char *utxo_id,
                         const char *spender_address) {
    UTXO *u = utxo_find(head, utxo_id);
    if (!u) return -1;
    if (u->spent) return 0;
    return (strncmp(u->owner_address, spender_address, ADDR_SIZE) == 0) ? 1 : 0;
}

int utxo_check_double_spend(UTXO *head, const char *utxo_id) {
    UTXO *u = utxo_find(head, utxo_id);
    if (!u) return -1;
    return u->spent ? 1 : 0;
}

void utxo_print_all(UTXO *head) {
    printf("=== UTXO Set ===\n");
    for (UTXO *u = head; u; u = u->next) {
        printf("  [%s] %s] owner=%.12s... amount=%.4f %s\n",
               u->utxo_id, u->spent ? "SPENT" : "UNSPENT",
               u->owner_address, u->amount, u->spent ? "(consumed)" : "");
    }
}

void utxo_print_by_owner(UTXO *head, const char *address) {
    printf("=== UTXOs for %.12s... ===\n", address);
    for (UTXO *u = head; u; u = u->next)
        if (!u->spent && strncmp(u->owner_address, address, ADDR_SIZE) == 0)
            printf("  %s  amount=%.4f AHT\n", u->utxo_id, u->amount);
}

void utxo_free_all(UTXO **head) {
    UTXO *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}
