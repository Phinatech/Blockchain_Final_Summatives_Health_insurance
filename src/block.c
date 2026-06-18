#include "block.h"
#include "merkle.h"
#include "transaction.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── block.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/block.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Block *block_create(uint32_t id, const char *prev_hash,
                     uint32_t difficulty, const char *miner_id) {
    Block *b = calloc(1, sizeof(Block));
    if (!b) return NULL;
    b->block_id    = id;
    b->timestamp   = time(NULL);
    b->difficulty  = difficulty;
    b->nonce       = 0;
    if (prev_hash) strncpy(b->previous_hash, prev_hash, HASH_SIZE - 1);
    if (miner_id)  strncpy(b->miner_id,      miner_id,  ADDR_SIZE - 1);
    /* merkle_root filled after transactions added */
    return b;
}

Block *block_create_genesis(void) {
    Block *g = block_create(0, "0000000000000000000000000000000000000000000000000000000000000000",
                             INITIAL_DIFFICULTY, ADDR_GENESIS);
    if (!g) return NULL;
    memset(g->merkle_root, '0', 64); g->merkle_root[64] = '\0';
    block_compute_hash(g);
    return g;
}

void block_free(Block *b) {
    if (!b) return;
    for (uint32_t i = 0; i < b->transaction_count; i++)
        tx_free(b->transactions[i]);
    free(b->transactions);
    free(b);
}

int block_compute_hash(Block *b) {
    /* Hash all eight canonical header fields (spec Section 1.i).
     * Including miner_id and difficulty ensures tampering with either
     * is detected by blockchain_verify.                              */
    uint8_t buf[1024]; uint8_t hash[32];
    int n = snprintf((char *)buf, sizeof(buf),
        "%u|%lld|%u|%s|%s|%llu|%s|%u",
        b->block_id, (long long)b->timestamp, b->transaction_count,
        b->previous_hash, b->merkle_root, (unsigned long long)b->nonce,
        b->miner_id, b->difficulty);
    if (n <= 0) return -1;
    sha256_bytes(buf, (size_t)n, hash);
    bytes_to_hex(hash, 32, b->block_hash);
    return 0;
}

int block_hash_meets_difficulty(const Block *b, uint32_t difficulty) {
    for (uint32_t i = 0; i < difficulty; i++)
        if (b->block_hash[i] != '0') return 0;
    return 1;
}

/* Addresses that belong to the system and are never signed by participants */
static int is_system_address(const char *addr) {
    if (!addr) return 1;
    return (strncmp(addr, ADDR_INSURANCE_POOL,   ADDR_SIZE) == 0 ||
            strncmp(addr, ADDR_REINSURANCE_POOL, ADDR_SIZE) == 0 ||
            strncmp(addr, ADDR_MINER_REWARD,     ADDR_SIZE) == 0 ||
            strncmp(addr, ADDR_GENESIS,          ADDR_SIZE) == 0);
}

int block_verify(const Block *b, PubkeyLookupFn lookup) {
    if (!b) return -1;

    /* ── 1. Recompute block hash ──────────────────────────────────────────── */
    char saved_hash[HASH_SIZE];
    strncpy(saved_hash, b->block_hash, HASH_SIZE);
    block_compute_hash((Block *)b);          /* cast away const: writes b->block_hash */
    int hash_ok = (strncmp(saved_hash, b->block_hash, HASH_SIZE) == 0);

    /* ── 2. Proof-of-Work (genesis is exempt) ─────────────────────────────── */
    int pow_ok = (b->block_id == 0) ? 1
               : block_hash_meets_difficulty(b, b->difficulty);

    /* ── 3. Merkle root ───────────────────────────────────────────────────── */
    int merkle_ok;
    if (b->transaction_count == 0) {
        /* Empty block (genesis): canonical root is the all-zero sentinel */
        char zeros[HASH_SIZE];
        memset(zeros, '0', 64); zeros[64] = '\0';
        merkle_ok = (strncmp(b->merkle_root, zeros, HASH_SIZE) == 0);
    } else {
        char computed_root[HASH_SIZE];
        merkle_root_hex(b->transactions, b->transaction_count, computed_root);
        merkle_ok = (strncmp(computed_root, b->merkle_root, HASH_SIZE) == 0);
    }

    /* ── 4. ECDSA signature on every participant-signed transaction ────────── */
    int sigs_ok = 1;
    if (lookup) {
        for (uint32_t i = 0; i < b->transaction_count; i++) {
            const Transaction *tx = b->transactions[i];

            /* Skip system-generated transactions (no private key to sign with) */
            if (tx->transaction_type == TX_MINING_REWARD     ||
                tx->transaction_type == TX_REINSURANCE_CONTRIBUTION ||
                tx->transaction_type == TX_CLAIM_SETTLE       ||
                is_system_address(tx->sender_address) ||
                tx->sig_len == 0)
                continue;

            /* Look up the sender's public key */
            char pubkey_pem[PEM_SIZE];
            if (lookup(tx->sender_address, pubkey_pem) != 0) {
                /* Unknown sender — treat as invalid */
                sigs_ok = 0;
                printf("[block_verify] block #%u tx %u: pubkey not found for %.12s...\n",
                       b->block_id, i, tx->sender_address);
                break;
            }

            int v = tx_verify(tx, pubkey_pem);
            if (v != 1) {
                sigs_ok = 0;
                printf("[block_verify] block #%u tx %u [%s]: INVALID signature\n",
                       b->block_id, i, tx_type_str(tx->transaction_type));
                break;
            }
        }
    }

    return (hash_ok && pow_ok && merkle_ok && sigs_ok) ? 1 : 0;
}

int block_add_transaction(Block *b, Transaction *tx) {
    Transaction **tmp = realloc(b->transactions,
                                (b->transaction_count + 1) * sizeof(Transaction *));
    if (!tmp) return -1;
    b->transactions = tmp;
    b->transactions[b->transaction_count++] = tx;
    return 0;
}

void block_print(const Block *b) {
    if (!b) return;
    printf("Block #%u\n", b->block_id);
    printf("  hash     : %.16s...\n", b->block_hash);
    printf("  prev     : %.16s...\n", b->previous_hash);
    printf("  merkle   : %.16s...\n", b->merkle_root);
    printf("  nonce    : %llu  difficulty: %u  miner: %s\n",
           (unsigned long long)b->nonce, b->difficulty, b->miner_id);
    printf("  txs      : %u\n", b->transaction_count);
    for (uint32_t i = 0; i < b->transaction_count; i++)
        tx_print(b->transactions[i]);
}

void block_print_summary(const Block *b) {
    if (!b) return;
    printf("Block #%-4u  hash=%.12s...  txs=%-3u  diff=%u\n",
           b->block_id, b->block_hash, b->transaction_count, b->difficulty);
}
