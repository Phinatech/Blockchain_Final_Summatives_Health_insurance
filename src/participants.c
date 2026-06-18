#include "participants.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── participants.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/participants.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Member *member_register(Member **head, const char *member_id,
                         const char *name) {
    Member *m = calloc(1, sizeof(Member));
    if (!m) return NULL;
    strncpy(m->member_id, member_id, ID_SIZE   - 1);
    strncpy(m->name,      name,      NAME_SIZE - 1);
    if (crypto_generate_keypair(&m->keys) != 0) {
        fprintf(stderr, "[member_register] key generation failed for %s\n", member_id);
        /* Fall back to member_id as placeholder so system still runs */
        strncpy(m->keys.wallet_address, member_id, ADDR_SIZE - 1);
    }
    m->next = *head;
    *head   = m;
    return m;
}

Member *member_find(Member *head, const char *member_id) {
    for (Member *m = head; m; m = m->next)
        if (strncmp(m->member_id, member_id, ID_SIZE) == 0) return m;
    return NULL;
}

Member *member_find_by_address(Member *head, const char *address) {
    for (Member *m = head; m; m = m->next)
        if (strncmp(m->keys.wallet_address, address, ADDR_SIZE) == 0) return m;
    return NULL;
}

const char *member_pubkey(Member *head, const char *member_id) {
    Member *m = member_find(head, member_id);
    return m ? m->keys.public_key_pem : NULL;
}

void member_print(const Member *m) {
    if (!m) return;
    printf("  Member %-12s  name=%-20s  wallet=%.12s...  policy=%s\n",
           m->member_id, m->name, m->keys.wallet_address,
           m->active_policy[0] ? m->active_policy : "(none)");
}

void member_print_all(Member *head) {
    printf("=== Members ===\n");
    for (Member *m = head; m; m = m->next) member_print(m);
}

void member_free_all(Member **head) {
    Member *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}

Provider *provider_register(Provider **head, const char *provider_id,
                              const char *name, const char *specialty) {
    Provider *p = calloc(1, sizeof(Provider));
    if (!p) return NULL;
    strncpy(p->provider_id, provider_id, ID_SIZE   - 1);
    strncpy(p->name,        name,        NAME_SIZE - 1);
    strncpy(p->specialty,   specialty,   NAME_SIZE - 1);
    if (crypto_generate_keypair(&p->keys) != 0) {
        fprintf(stderr, "[provider_register] key generation failed for %s\n", provider_id);
        strncpy(p->keys.wallet_address, provider_id, ADDR_SIZE - 1);
    }
    p->next = *head;
    *head   = p;
    return p;
}

Provider *provider_find(Provider *head, const char *provider_id) {
    for (Provider *p = head; p; p = p->next)
        if (strncmp(p->provider_id, provider_id, ID_SIZE) == 0) return p;
    return NULL;
}

Provider *provider_find_by_address(Provider *head, const char *address) {
    for (Provider *p = head; p; p = p->next)
        if (strncmp(p->keys.wallet_address, address, ADDR_SIZE) == 0) return p;
    return NULL;
}

const char *provider_pubkey(Provider *head, const char *provider_id) {
    Provider *p = provider_find(head, provider_id);
    return p ? p->keys.public_key_pem : NULL;
}

void provider_print(const Provider *p) {
    if (!p) return;
    printf("  Provider %-12s  name=%-20s  specialty=%-16s  wallet=%.12s...\n",
           p->provider_id, p->name, p->specialty, p->keys.wallet_address);
}

void provider_print_all(Provider *head) {
    printf("=== Providers ===\n");
    for (Provider *p = head; p; p = p->next) provider_print(p);
}

void provider_free_all(Provider **head) {
    Provider *cur = *head, *nxt;
    while (cur) { nxt = cur->next; free(cur); cur = nxt; }
    *head = NULL;
}
