#include "chain.h"
#include "merkle.h"
#include "transaction.h"
#include "token.h"
#include "reinsurance.h"
#include "account.h"
#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File-scope pointer used by chain_verify to pass blockchain context
 * to block_verify's PubkeyLookupFn callback. */
static Blockchain *s_verify_bc = NULL;

static int chain_pubkey_lookup(const char *address, char pubkey_pem[PEM_SIZE]) {
    if (!s_verify_bc) return -1;
    return chain_lookup_pubkey(s_verify_bc, address, pubkey_pem);
}

/* ── chain.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/chain.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Blockchain *chain_init(void) {
    Blockchain *bc = calloc(1, sizeof(Blockchain));
    if (!bc) return NULL;

    /* Initialise ChainState */
    bc->state.height              = 0;
    bc->state.difficulty          = INITIAL_DIFFICULTY;
    bc->state.block_reward        = INITIAL_BLOCK_REWARD;
    bc->state.last_retarget_block = 0;
    strncpy(bc->state.save_file, "data/chain.dat", 255);

    /* Initialise AHT token */
    token_init(&bc->token);

    /* Initialise reinsurance pool */
    reinsurance_init(&bc->reinsurance);

    /* Create the five mandatory accounts (spec Section 3.ii) */
    account_create(&bc->accounts, ADDR_INSURANCE_POOL,   "Insurance Pool",   10000.0);
    account_create(&bc->accounts, ADDR_REINSURANCE_POOL, "Reinsurance Pool",     0.0);
    account_create(&bc->accounts, ADDR_MINER_WALLET,     "Miner Wallet",         0.0);
    account_create(&bc->accounts, ADDR_MINER_REWARD,     "Coinbase",             0.0);
    account_create(&bc->accounts, ADDR_GENESIS,          "Genesis",              0.0);

    /* Create and append the genesis block */
    Block *g = block_create_genesis();
    if (!g) { chain_free(bc); return NULL; }
    bc->genesis = g;
    bc->head    = g;
    bc->state.height = 1;

    printf("[chain] Initialised — genesis block #0 created.\n");
    return bc;
}

int chain_append_block(Blockchain *bc, Block *b) {
    if (!bc || !b) return -1;
    bc->head->next = b;
    bc->head       = b;
    bc->state.height++;

    /* Remove confirmed transactions from mempool */
    for (uint32_t i = 0; i < b->transaction_count; i++)
        mempool_remove_by_id(&bc->mempool, b->transactions[i]->transaction_id);

    chain_check_retarget(bc);
    return 0;
}

Block *chain_get_block(Blockchain *bc, uint32_t block_id) {
    for (Block *b = bc->genesis; b; b = b->next)
        if (b->block_id == block_id) return b;
    return NULL;
}

Block *chain_get_block_by_hash(Blockchain *bc, const char *hash) {
    for (Block *b = bc->genesis; b; b = b->next)
        if (strncmp(b->block_hash, hash, HASH_SIZE) == 0) return b;
    return NULL;
}

void chain_check_retarget(Blockchain *bc) {
    uint32_t height = bc->state.height;
    if (height < 2 || (height - 1) % BLOCKS_PER_RETARGET != 0) return;

    /*
     * Difficulty retarget — rolling window algorithm (spec Section 5.iii):
     *
     * We need the average block interval over the last BLOCKS_PER_RETARGET
     * (10) blocks. Instead of a two-pass scan, we use a circular buffer of
     * size (BLOCKS_PER_RETARGET + 1) to hold the most recent block pointers.
     *
     * How the circular buffer works:
     *   arr[n % SIZE] = current block  — overwrites the oldest slot.
     * After walking all n blocks, the buffer holds the last min(n, SIZE)
     * blocks in circular order, with the oldest at index (n % SIZE).
     *
     * We then reconstruct the chronological order using modular arithmetic
     * and sum the timestamp differences between consecutive blocks to get
     * the total elapsed time over the window.
     *
     * avg_time = total_time / (slots - 1) intervals
     * Fallback: if we have fewer than 2 blocks (< 1 interval), use 60s.
     */
    double total_time = 0.0; int count = 0;

    Block *arr[BLOCKS_PER_RETARGET + 1];
    int n = 0;
    /* Walk the chain, keeping only the last (BLOCKS_PER_RETARGET+1) pointers */
    for (Block *cur = bc->genesis; cur; cur = cur->next) {
        arr[n % (BLOCKS_PER_RETARGET + 1)] = cur;
        n++;
    }
    /* slots = how many valid entries are in the buffer */
    int slots  = (n < BLOCKS_PER_RETARGET + 1) ? n : BLOCKS_PER_RETARGET + 1;
    /* oldest = index of the chronologically first entry in the buffer */
    int oldest = (n < BLOCKS_PER_RETARGET + 1) ? 0 : n % (BLOCKS_PER_RETARGET + 1);
    /* Sum the intervals between consecutive blocks in the window */
    for (int i = 1; i < slots; i++) {
        int idx      = (oldest + i)     % (BLOCKS_PER_RETARGET + 1);
        int prev_idx = (oldest + i - 1) % (BLOCKS_PER_RETARGET + 1);
        total_time  += difftime(arr[idx]->timestamp, arr[prev_idx]->timestamp);
        count++;
    }
    double avg_time = (count > 0) ? total_time / count : 60.0;

    uint32_t old_diff = bc->state.difficulty;
    if (avg_time < TARGET_BLOCK_TIME_MIN)
        bc->state.difficulty++;
    else if (avg_time > TARGET_BLOCK_TIME_MAX && bc->state.difficulty > MIN_DIFFICULTY)
        bc->state.difficulty--;

    bc->state.last_retarget_block = height - 1;

    printf("[retarget] Block %u: avg_time=%.1fs  difficulty %u → %u\n",
           height - 1, avg_time, old_diff, bc->state.difficulty);
}

int chain_verify(Blockchain *bc) {
    printf("=== Blockchain Verification ===\n");

    /* Set file-scope context so the PubkeyLookupFn callback can reach bc */
    s_verify_bc = bc;

    Block *prev = NULL;
    int ok = 1;

    for (Block *b = bc->genesis; b; prev = b, b = b->next) {
        /* Per-block checks delegated to block_verify (hash, PoW, Merkle, ECDSA) */
        int bv = block_verify(b, chain_pubkey_lookup);

        /* Chain-level checks that need the previous block */
        int link_ok = (!prev) ? 1 :
            (strncmp(b->previous_hash, prev->block_hash, HASH_SIZE) == 0);
        int ts_ok = (!prev) ? 1 : (b->timestamp >  prev->timestamp); /* spec 7.ix: strictly greater */

        int block_ok = (bv == 1) && link_ok && ts_ok;

        printf("  Block #%-4u  %s  link=%s  ts=%s  -> %s\n",
               b->block_id,
               (bv == 1) ? "[hash OK][pow OK][merkle OK][sigs OK]" : "[INVALID]",
               link_ok   ? "OK"    : "FAIL",
               ts_ok     ? "OK"    : "FAIL",
               block_ok  ? "VALID" : "TAMPERED");

        if (!block_ok) ok = 0;
    }

    s_verify_bc = NULL;
    printf("=== Result: %s ===\n", ok ? "CHAIN VALID" : "CHAIN TAMPERED");
    return ok;
}

int chain_lookup_pubkey(Blockchain *bc, const char *address,
                         char pubkey_pem[PEM_SIZE]) {
    Member   *m = member_find_by_address(bc->members,   address);
    Provider *p = provider_find_by_address(bc->providers, address);
    if (m) { strncpy(pubkey_pem, m->keys.public_key_pem, PEM_SIZE - 1); return 0; }
    if (p) { strncpy(pubkey_pem, p->keys.public_key_pem, PEM_SIZE - 1); return 0; }
    return -1;
}

uint32_t chain_collect_tx_ids(Blockchain *bc,
                               char (*out_ids)[HASH_SIZE],
                               uint32_t max_ids) {
    uint32_t n = 0;
    for (Block *b = bc->genesis; b && n < max_ids; b = b->next)
        for (uint32_t i = 0; i < b->transaction_count && n < max_ids; i++) {
            strncpy(out_ids[n], b->transactions[i]->transaction_id, HASH_SIZE - 1);
            n++;
        }
    for (MempoolEntry *e = bc->mempool; e && n < max_ids; e = e->next) {
        strncpy(out_ids[n], e->transaction_id, HASH_SIZE - 1);
        n++;
    }
    return n;
}

void chain_print(Blockchain *bc) {
    printf("=== Blockchain  height=%u  difficulty=%u  reward=%.2f AHT ===\n",
           bc->state.height, bc->state.difficulty, bc->state.block_reward);
    for (Block *b = bc->genesis; b; b = b->next) block_print_summary(b);
    token_print(&bc->token);
    reinsurance_print(&bc->reinsurance);
}

void chain_free(Blockchain *bc) {
    if (!bc) return;
    for (Block *b = bc->genesis, *nxt; b; b = nxt) { nxt = b->next; block_free(b); }
    mempool_free_all(&bc->mempool);
    utxo_free_all(&bc->utxo_set);
    account_free_all(&bc->accounts);
    member_free_all(&bc->members);
    provider_free_all(&bc->providers);
    policy_free_all(&bc->policies);
    claim_free_all(&bc->claims);
    preauth_free_all(&bc->preauths);
    fraud_log_free_all(&bc->fraud_log);
    free(bc);
}
