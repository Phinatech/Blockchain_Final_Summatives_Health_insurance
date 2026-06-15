#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"
#include "transaction.h"
#include <time.h>
#include <stdint.h>

/* ── Block struct ────────────────────────────────────────────────────────── */

/*
 * Block fields (exactly as specified):
 *   block_id         — sequential index (genesis = 0)
 *   timestamp        — Unix time when the block was mined
 *   transaction_count— number of transactions in this block
 *   previous_hash    — SHA-256 hex of the preceding block's fields
 *   merkle_root      — Merkle root of all transactions in this block
 *   nonce            — PoW counter; incremented until hash meets difficulty
 *   miner_id         — wallet address of the miner who found this block
 *   difficulty       — difficulty that was active when this block was mined
 *
 * Additionally kept in memory (not part of the canonical hash input):
 *   block_hash       — SHA-256 of the block's header fields (cached)
 *   transactions     — heap-allocated array of Transaction pointers
 *   next             — linked-list pointer for the in-memory chain
 */
typedef struct Block {
    uint32_t      block_id;
    time_t        timestamp;
    uint32_t      transaction_count;
    char          previous_hash[HASH_SIZE];
    char          merkle_root[HASH_SIZE];
    uint64_t      nonce;
    char          miner_id[ADDR_SIZE];
    uint32_t      difficulty;

    /* in-memory fields – not hashed, not serialised as part of hash input */
    char          block_hash[HASH_SIZE];
    Transaction **transactions;          /* dynamic array; count = transaction_count */
    struct Block *next;
} Block;

/* ── Construction ────────────────────────────────────────────────────────── */

/* Allocate a new block; caller must set transactions before finalising */
Block *block_create(uint32_t id, const char *prev_hash,
                    uint32_t difficulty, const char *miner_id);

/* Create the genesis block (block_id=0, previous_hash all-zeros) */
Block *block_create_genesis(void);

/* Deep-free a block and all its transactions */
void block_free(Block *b);

/* ── Hashing ─────────────────────────────────────────────────────────────── */

/*
 * block_compute_hash — serialise the six header fields
 *   (block_id, timestamp, transaction_count, previous_hash, merkle_root, nonce)
 *   and compute SHA-256.  Writes result into b->block_hash.
 */
int block_compute_hash(Block *b);

/*
 * block_hash_meets_difficulty — return 1 if b->block_hash starts with
 * `difficulty` leading zero hex digits, 0 otherwise.
 */
int block_hash_meets_difficulty(const Block *b, uint32_t difficulty);

/* ── Verification ────────────────────────────────────────────────────────── */

/*
 * block_verify — full integrity check:
 *   1. Recompute block hash and compare against b->block_hash.
 *   2. Check that the hash meets b->difficulty.
 *   3. Recompute Merkle root from transactions and compare against b->merkle_root.
 *   4. Verify ECDSA signature on every transaction.
 * Returns 1 if valid, 0 if tampered, -1 on error.
 * Caller must pass the public-key lookup callback to resolve sender addresses.
 */
typedef int (*PubkeyLookupFn)(const char *address, char pubkey_pem[PEM_SIZE]);
int block_verify(const Block *b, PubkeyLookupFn lookup);

/* ── Utility ─────────────────────────────────────────────────────────────── */

/* Add a transaction to the block's transaction array (realloc-safe) */
int block_add_transaction(Block *b, Transaction *tx);

void block_print(const Block *b);
void block_print_summary(const Block *b);

#endif /* BLOCK_H */
