#ifndef MINING_H
#define MINING_H

#include "types.h"
#include "chain.h"
#include <stdint.h>

/* ── Pool mining: per-miner contribution ─────────────────────────────────── */

/*
 * Each miner in a pool session records how many hashes they attempted.
 * When the block is found, reward is split proportionally:
 *   miner_share = (miner.hashes_attempted / session.total_hashes) * reward
 */
typedef struct MinerContrib {
    char              miner_id[ID_SIZE];
    char              miner_address[ADDR_SIZE];
    uint64_t          hashes_attempted;
    double            reward_share;     /* computed after block is found */
    Transaction      *membership_tx;    /* spec 5.v: pool join recorded on-chain */
    struct MinerContrib *next;
} MinerContrib;

typedef struct {
    MinerContrib *miners;
    uint32_t      miner_count;
    uint64_t      total_hashes;
    double        total_reward;
} PoolSession;

/* ── Proof-of-Work ───────────────────────────────────────────────────────── */

/*
 * mining_pow — run the PoW loop on `block` until its hash meets `difficulty`.
 *
 * Each iteration:
 *   1. block->nonce++
 *   2. block_compute_hash(block)
 *   3. if hash starts with `difficulty` leading zero hex chars → found
 *
 * Returns the winning nonce and writes the final hash into block->block_hash.
 */
uint64_t mining_pow(Block *block, uint32_t difficulty);

/* ── Solo mining ─────────────────────────────────────────────────────────── */

/*
 * mine_solo — full solo-mining workflow:
 *   1. Pull top-priority PENDING transactions from the mempool.
 *   2. Compute Merkle root.
 *   3. Assemble block header.
 *   4. Run mining_pow until valid hash found.
 *   5. Create a TX_MINING_REWARD transaction crediting miner_address.
 *   6. Append block to chain; remove confirmed txs from mempool.
 *   7. Update account balances (Account model) and UTXO set.
 *   8. Trigger chain_check_retarget.
 *
 * Returns pointer to the newly appended block, or NULL on failure.
 */
Block *mine_solo(Blockchain *bc, const char *miner_id,
                 const char *miner_address);

/* ── Pool mining ─────────────────────────────────────────────────────────── */

/*
 * mine_pool_init  — allocate a PoolSession with zero miners.
 * mine_pool_add   — register a miner into the session.
 * mine_pool_run   — run PoW; each miner takes turns incrementing the nonce
 *                   and recording their attempt count until the block is found.
 *                   Distributes reward proportionally; records each payout as
 *                   a TX_MINING_REWARD transaction on the chain.
 * mine_pool_free  — deallocate the session.
 */
PoolSession *mine_pool_init(void);
int          mine_pool_add(PoolSession *s, const char *miner_id,
                            const char *miner_address);
Block       *mine_pool_run(Blockchain *bc, PoolSession *s);
void         mine_pool_free(PoolSession *s);

/* ── Utility ─────────────────────────────────────────────────────────────── */

void mining_print_pool_result(const PoolSession *s, const Block *b);

#endif /* MINING_H */
