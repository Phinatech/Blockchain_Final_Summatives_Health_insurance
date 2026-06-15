#include "persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── persistence.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/persistence.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int chain_save(const Blockchain *bc) {
    return chain_save_to(bc, bc->state.save_file);
}

int chain_save_to(const Blockchain *bc, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("chain_save"); return -1; }

    fwrite(PERSIST_MAGIC, 1, PERSIST_MAGIC_LEN, f);
    fwrite(&bc->state,       sizeof(ChainState),    1, f);
    fwrite(&bc->token,       sizeof(Token),         1, f);
    fwrite(&bc->reinsurance, sizeof(ReinsurancePool),1, f);

    /* Accounts */
    uint32_t cnt = 0;
    for (Account *a = bc->accounts; a; a = a->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (Account *a = bc->accounts; a; a = a->next)
        fwrite(a, sizeof(Account) - sizeof(Account *), 1, f);

    /* Members */
    cnt = 0;
    for (Member *m = bc->members; m; m = m->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (Member *m = bc->members; m; m = m->next)
        fwrite(m, sizeof(Member) - sizeof(Member *), 1, f);

    /* Providers */
    cnt = 0;
    for (Provider *p = bc->providers; p; p = p->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (Provider *p = bc->providers; p; p = p->next)
        fwrite(p, sizeof(Provider) - sizeof(Provider *), 1, f);

    /* Policies */
    cnt = 0;
    for (Policy *p = bc->policies; p; p = p->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (Policy *p = bc->policies; p; p = p->next)
        fwrite(p, sizeof(Policy) - sizeof(Policy *), 1, f);

    /* Claims */
    cnt = 0;
    for (Claim *c = bc->claims; c; c = c->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (Claim *c = bc->claims; c; c = c->next)
        fwrite(c, sizeof(Claim) - sizeof(Claim *), 1, f);

    /* UTXOs */
    cnt = 0;
    for (UTXO *u = bc->utxo_set; u; u = u->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    for (UTXO *u = bc->utxo_set; u; u = u->next)
        fwrite(u, sizeof(UTXO) - sizeof(UTXO *), 1, f);

    /* Blocks */
    cnt = bc->state.height;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    /* Header size = block minus the two pointer fields at the end */
    size_t hdr_sz = sizeof(Block) - sizeof(Transaction **) - sizeof(Block *);
    for (Block *b = bc->genesis; b; b = b->next) {
        fwrite(b, hdr_sz, 1, f);
        fwrite(&b->transaction_count, sizeof(uint32_t), 1, f);
        for (uint32_t i = 0; i < b->transaction_count; i++)
            fwrite(b->transactions[i], sizeof(Transaction), 1, f);
    }

    /* Mempool */
    cnt = 0;
    for (MempoolEntry *e = bc->mempool; e; e = e->next) cnt++;
    fwrite(&cnt, sizeof(uint32_t), 1, f);
    size_t entry_sz = sizeof(MempoolEntry) - sizeof(Transaction *) - sizeof(MempoolEntry *);
    for (MempoolEntry *e = bc->mempool; e; e = e->next) {
        fwrite(e, entry_sz, 1, f);
        fwrite(e->tx, sizeof(Transaction), 1, f);
    }

    fclose(f);
    printf("[persist] Chain saved to %s\n", path);
    return 0;
}

Blockchain *chain_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    char magic[PERSIST_MAGIC_LEN];
    fread(magic, 1, PERSIST_MAGIC_LEN, f);
    if (memcmp(magic, PERSIST_MAGIC, PERSIST_MAGIC_LEN) != 0) {
        fprintf(stderr, "[persist] Bad magic in %s\n", path);
        fclose(f); return NULL;
    }

    Blockchain *bc = calloc(1, sizeof(Blockchain));
    if (!bc) { fclose(f); return NULL; }

    fread(&bc->state,       sizeof(ChainState),     1, f);
    fread(&bc->token,       sizeof(Token),          1, f);
    fread(&bc->reinsurance, sizeof(ReinsurancePool), 1, f);

    /* Accounts */
    uint32_t cnt;
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        Account *a = calloc(1, sizeof(Account));
        fread(a, sizeof(Account) - sizeof(Account *), 1, f);
        a->next = bc->accounts; bc->accounts = a;
    }

    /* Members */
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        Member *m = calloc(1, sizeof(Member));
        fread(m, sizeof(Member) - sizeof(Member *), 1, f);
        m->next = bc->members; bc->members = m;
    }

    /* Providers */
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        Provider *p = calloc(1, sizeof(Provider));
        fread(p, sizeof(Provider) - sizeof(Provider *), 1, f);
        p->next = bc->providers; bc->providers = p;
    }

    /* Policies */
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        Policy *p = calloc(1, sizeof(Policy));
        fread(p, sizeof(Policy) - sizeof(Policy *), 1, f);
        p->next = bc->policies; bc->policies = p;
    }

    /* Claims */
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        Claim *c = calloc(1, sizeof(Claim));
        fread(c, sizeof(Claim) - sizeof(Claim *), 1, f);
        c->next = bc->claims; bc->claims = c;
    }

    /* UTXOs */
    fread(&cnt, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < cnt; i++) {
        UTXO *u = calloc(1, sizeof(UTXO));
        fread(u, sizeof(UTXO) - sizeof(UTXO *), 1, f);
        u->next = bc->utxo_set; bc->utxo_set = u;
    }

    /* Blocks */
    fread(&cnt, sizeof(uint32_t), 1, f);
    size_t hdr_sz = sizeof(Block) - sizeof(Transaction **) - sizeof(Block *);
    Block *prev_blk = NULL;
    for (uint32_t i = 0; i < cnt; i++) {
        Block *b = calloc(1, sizeof(Block));
        fread(b, hdr_sz, 1, f);
        uint32_t tx_cnt;
        fread(&tx_cnt, sizeof(uint32_t), 1, f);
        b->transactions = calloc(tx_cnt, sizeof(Transaction *));
        b->transaction_count = tx_cnt;
        for (uint32_t j = 0; j < tx_cnt; j++) {
            Transaction *tx = malloc(sizeof(Transaction));
            fread(tx, sizeof(Transaction), 1, f);
            b->transactions[j] = tx;
        }
        if (!bc->genesis) bc->genesis = b;
        if (prev_blk) prev_blk->next = b;
        bc->head = b;
        prev_blk = b;
    }

    /* Mempool */
    fread(&cnt, sizeof(uint32_t), 1, f);
    size_t entry_sz = sizeof(MempoolEntry) - sizeof(Transaction *) - sizeof(MempoolEntry *);
    for (uint32_t i = 0; i < cnt; i++) {
        MempoolEntry *e = calloc(1, sizeof(MempoolEntry));
        fread(e, entry_sz, 1, f);
        Transaction *tx = malloc(sizeof(Transaction));
        fread(tx, sizeof(Transaction), 1, f);
        e->tx   = tx;
        e->next = bc->mempool;
        bc->mempool = e;
    }

    fclose(f);
    printf("[persist] Chain loaded from %s  height=%u\n", path, bc->state.height);
    return bc;
}

Blockchain *chain_startup(const char *path) {
    Blockchain *bc = chain_load(path);
    if (bc) {
        printf("[startup] Verifying loaded chain...\n");
        if (!chain_verify(bc)) {
            fprintf(stderr, "[startup] Verification FAILED. Aborting.\n");
            chain_free(bc);
            return NULL;
        }
        return bc;
    }
    printf("[startup] No save file found. Initialising fresh chain.\n");
    bc = chain_init();
    if (bc) strncpy(bc->state.save_file, path, 255);
    return bc;
}
