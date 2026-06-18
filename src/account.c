#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Account *account_create(Account **head, const char *address,
                         const char *label, double initial_balance) {
    Account *a = calloc(1, sizeof(Account));
    if (!a) return NULL;
    strncpy(a->address, address, ADDR_SIZE - 1);
    strncpy(a->label,   label,   NAME_SIZE - 1);
    a->balance = initial_balance;
    a->nonce   = 0;
    a->next    = *head;
    *head      = a;
    return a;
}

Account *account_find(Account *head, const char *address) {
    for (Account *a = head; a; a = a->next)
        if (strncmp(a->address, address, ADDR_SIZE) == 0) return a;
    return NULL;
}

Account *account_find_by_label(Account *head, const char *label) {
    for (Account *a = head; a; a = a->next)
        if (strncmp(a->label, label, NAME_SIZE) == 0) return a;
    return NULL;
}

int account_credit(Account *head, const char *address, double amount) {
    Account *a = account_find(head, address);
    if (!a || amount <= 0.0) return -1;
    a->balance += amount;
    return 0;
}

int account_debit(Account *head, const char *address, double amount) {
    Account *a = account_find(head, address);
    if (!a || amount <= 0.0 || a->balance < amount) return -1;
    a->balance -= amount;
    return 0;
}

int account_transfer(Account *head, const char *from,
                     const char *to, double amount) {
    if (account_debit(head, from, amount)  < 0) return -1;
    if (account_credit(head, to,  amount)  < 0) {
        account_credit(head, from, amount);     /* rollback */
        return -1;
    }
    return 0;
}

double account_get_balance(Account *head, const char *address) {
    Account *a = account_find(head, address);
    return a ? a->balance : -1.0;
}

uint64_t account_get_next_nonce(Account *head, const char *address) {
    Account *a = account_find(head, address);
    return a ? a->nonce + 1 : 0;
}

int account_validate_nonce(Account *head, const char *address,
                            uint64_t tx_nonce) {
    Account *a = account_find(head, address);
    if (!a) return -1;
    return (tx_nonce == a->nonce + 1) ? 1 : 0;
}

int account_confirm_nonce(Account *head, const char *address) {
    Account *a = account_find(head, address);
    if (!a) return -1;
    a->nonce++;
    return 0;
}

void account_print(const Account *a) {
    if (!a) return;
    printf("  [%s] %-20s  balance=%.4f AHT  nonce=%llu\n",
           a->address, a->label, a->balance,
           (unsigned long long)a->nonce);
}

void account_print_all(Account *head) {
    printf("=== Accounts ===\n");
    for (Account *a = head; a; a = a->next) account_print(a);
}

void account_free_all(Account **head) {
    Account *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}
