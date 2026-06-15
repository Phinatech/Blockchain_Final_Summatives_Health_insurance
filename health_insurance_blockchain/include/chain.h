#ifndef CHAIN_H
#define CHAIN_H

#include "types.h"
#include "token.h"
#include "block.h"
#include "mempool.h"
#include "utxo.h"
#include "account.h"
#include "participants.h"
#include "insurance.h"
#include "reinsurance.h"
#include "fraud.h"
#include <stdint.h>

/* ── ChainState ──────────────────────────────────────────────────────────── */

/*
 * ChainState holds all mutable global chain parameters.
 * Stored persistently alongside the block data.
 */
typedef struct {
    uint32_t height;              /* number of blocks (genesis = block 0) */
    uint32_t difficulty;          /* current PoW difficulty */
    double   block_reward;        /* AHT reward per mined block */
    uint32_t last_retarget_block; /* block_id when difficulty was last changed */
    char     save_file[256];      /* path to the persistence file */
} ChainState;

/* ── Blockchain ──────────────────────────────────────────────────────────── */

/*
 * Blockchain is the single source of truth for the entire system.
 * It is initialised once in main() and passed to every subsystem.
 */
typedef struct {
    Block           *genesis;       /* head of the linked list (block 0) */
    Block           *head;          /* tail of the linked list (latest block) */
    ChainState       state;

    /* Pending transaction pool */
    MempoolEntry    *mempool;

    /* Ledger models */
    UTXO            *utxo_set;
    Account         *accounts;

    /* Domain data */
    Member          *members;
    Provider        *providers;
    Policy          *policies;
    Claim           *claims;
    Preauth         *preauths;

    /* Finance */
    ReinsurancePool  reinsurance;
    Token            token;

    /* Fraud audit log */
    FraudLogEntry   *fraud_log;
} Blockchain;

/* ── Initialisation ──────────────────────────────────────────────────────── */

/*
 * chain_init — allocate and return a fresh Blockchain.
 *   - Creates the genesis block.
 *   - Initialises five required accounts (insurance pool, reinsurance pool,
 *     miner reward account, and two placeholder wallets).
 *   - Sets default difficulty and block reward from types.h constants.
 * Returns NULL on allocation failure.
 */
Blockchain *chain_init(void);

/* ── Block operations ────────────────────────────────────────────────────── */

/*
 * chain_append_block — append a fully-mined block to the chain.
 * Updates head, increments height, and removes confirmed mempool entries.
 * Returns 0 on success, -1 on failure.
 */
int chain_append_block(Blockchain *bc, Block *b);

Block *chain_get_block(Blockchain *bc, uint32_t block_id);
Block *chain_get_block_by_hash(Blockchain *bc, const char *hash);

/* ── Difficulty retargeting ──────────────────────────────────────────────── */

/*
 * chain_check_retarget — called after every block is appended.
 *
 * Every BLOCKS_PER_RETARGET (10) blocks:
 *   avg_time = average seconds between the last 10 block timestamps
 *   if avg_time < TARGET_BLOCK_TIME_MIN: difficulty += 1
 *   if avg_time > TARGET_BLOCK_TIME_MAX: difficulty = max(1, difficulty - 1)
 *   otherwise: no change
 *
 * Prints the retarget event (old difficulty, new difficulty, avg time)
 * and stores the result in bc->state.
 */
void chain_check_retarget(Blockchain *bc);

/* ── Verification ────────────────────────────────────────────────────────── */

/*
 * chain_verify — full chain audit:
 *   For every block (starting from genesis):
 *     1. Recompute block hash; compare against stored value.
 *     2. Check previous_hash matches previous block's hash.
 *     3. Check PoW difficulty is met.
 *     4. Recompute Merkle root; compare against stored value.
 *     5. Verify ECDSA signature on every transaction.
 *     6. Check timestamp strictly greater than previous block.
 *   Prints a pass/fail summary for each block.
 *   Returns 1 if all blocks are valid, 0 if any block fails.
 */
int chain_verify(Blockchain *bc);

/* ── Pubkey lookup (used by block_verify and chain_verify) ───────────────── */

/*
 * chain_lookup_pubkey — find the PEM public key for a given wallet address.
 * Searches members, providers, and well-known pool addresses.
 * Returns 0 if found (copies PEM into pubkey_pem), -1 if not found.
 */
int chain_lookup_pubkey(Blockchain *bc, const char *address,
                         char pubkey_pem[PEM_SIZE]);

/* ── Utility ─────────────────────────────────────────────────────────────── */

/*
 * chain_collect_tx_ids — fill out_ids with all transaction IDs from
 * the chain and the mempool.  Used by fraud detection for duplicate check.
 * Returns total count written.
 */
uint32_t chain_collect_tx_ids(Blockchain *bc,
                               char (*out_ids)[HASH_SIZE],
                               uint32_t max_ids);

void chain_print(Blockchain *bc);

/* ── Cleanup ─────────────────────────────────────────────────────────────── */

void chain_free(Blockchain *bc);

#endif /* CHAIN_H */
