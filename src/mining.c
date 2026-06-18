#include "mining.h"
#include "merkle.h"
#include "persistence.h"
#include "transaction.h"
#include "mempool.h"
#include "block.h"
#include "account.h"
#include "utxo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── mining.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/mining.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Apply confirmed transactions to ledger state ───────────────────────── */
/*
 * apply_confirmed_tx — called once per tx after the block containing it is
 * appended.  Updates Account balances, UTXO set, and reinsurance pool based
 * on the transaction type.
 */
static void apply_confirmed_tx(Blockchain *bc, const Transaction *tx) {
    switch (tx->transaction_type) {
        case TX_PREMIUM_PAYMENT:
            account_debit (bc->accounts, tx->sender_address,   tx->amount);
            account_credit(bc->accounts, tx->receiver_address, tx->amount);
            /* UTXO: create output for insurance pool */
            utxo_create(&bc->utxo_set, tx->receiver_address, tx->amount,
                        tx->transaction_id, 0);
            break;
        case TX_REINSURANCE_CONTRIBUTION:
            account_debit (bc->accounts, tx->sender_address,   tx->amount);
            reinsurance_contribute(&bc->reinsurance, tx->amount);
            account_credit(bc->accounts, ADDR_REINSURANCE_POOL, tx->amount);
            utxo_create(&bc->utxo_set, ADDR_REINSURANCE_POOL, tx->amount,
                        tx->transaction_id, 0);
            break;
        case TX_TOKEN_TRANSFER:
            account_debit (bc->accounts, tx->sender_address,   tx->amount);
            account_credit(bc->accounts, tx->receiver_address, tx->amount);
            /* UTXO: consume sender output (best-effort: find any unspent) */
            { UTXO *u = bc->utxo_set;
              double remaining = tx->amount;
              uint32_t out_idx = 0;
              while (u && remaining > 0.0) {
                  if (!u->spent &&
                      strncmp(u->owner_address, tx->sender_address, ADDR_SIZE) == 0) {
                      utxo_spend(bc->utxo_set, u->utxo_id, tx->transaction_id);
                      remaining -= u->amount;
                  }
                  u = u->next;
              }
              /* Create change output for sender if over-spent */
              if (remaining < 0.0)
                  utxo_create(&bc->utxo_set, tx->sender_address, -remaining,
                              tx->transaction_id, out_idx++);
              /* Create output for receiver */
              utxo_create(&bc->utxo_set, tx->receiver_address, tx->amount,
                          tx->transaction_id, out_idx);
            }
            break;
        case TX_CLAIM_SETTLE:
            /*
             * UTXO model for claim settlement (spec Section 3.i):
             * 1. Consume unspent outputs from the paying pool wallet until
             *    we have covered the settlement amount.
             * 2. Create a new unspent output for the provider (receiver).
             * 3. If UTXOs consumed exceed the settlement amount, create a
             *    change output back to the pool wallet.
             *
             * The Account model credit is also applied for the balance ledger.
             */
            account_credit(bc->accounts, tx->receiver_address, tx->amount);
            {
                double remaining = tx->amount;
                double consumed  = 0.0;
                uint32_t out_idx = 0;

                /* Consume pool UTXOs as inputs until settlement is covered */
                for (UTXO *u = bc->utxo_set; u && remaining > 0.0; u = u->next) {
                    if (!u->spent &&
                        strncmp(u->owner_address, tx->sender_address,
                                ADDR_SIZE) == 0) {
                        utxo_spend(bc->utxo_set, u->utxo_id,
                                   tx->transaction_id);
                        consumed  += u->amount;
                        remaining -= u->amount;
                    }
                }

                /* Output to provider for the settlement amount */
                utxo_create(&bc->utxo_set, tx->receiver_address,
                            tx->amount, tx->transaction_id, out_idx++);

                /* Change output back to pool wallet if we over-consumed */
                if (consumed > tx->amount) {
                    double change = consumed - tx->amount;
                    utxo_create(&bc->utxo_set, tx->sender_address,
                                change, tx->transaction_id, out_idx);
                }
            }
            break;
        case TX_MINING_REWARD:
            /* Miner credit already applied before PoW; create UTXO output */
            utxo_create(&bc->utxo_set, tx->receiver_address, tx->amount,
                        tx->transaction_id, 0);
            break;
        default:
            break;
    }
    /* Nonce confirmation for sender (skip system-generated txs) */
    if (tx->transaction_type != TX_MINING_REWARD &&
        tx->transaction_type != TX_REINSURANCE_CONTRIBUTION)
        account_confirm_nonce(bc->accounts, tx->sender_address);
}

uint64_t mining_pow(Block *block, uint32_t difficulty) {
    printf("[mining] PoW started  block=#%u  difficulty=%u\n",
           block->block_id, difficulty);
    while (1) {
        block->nonce++;
        block_compute_hash(block);
        if (block_hash_meets_difficulty(block, difficulty)) break;
    }
    printf("[mining] PoW solved   nonce=%llu  hash=%.16s...\n",
           (unsigned long long)block->nonce, block->block_hash);
    return block->nonce;
}

Block *mine_solo(Blockchain *bc, const char *miner_id,
                  const char *miner_address) {
    if (!bc || !miner_id || !miner_address) return NULL;

    /* 1. Sort mempool so highest-fee txs are selected first, then select */
    bc->mempool = mempool_sort(bc->mempool);
    MempoolEntry *selected[MAX_TX_PER_BLOCK];
    uint32_t tx_count = mempool_select_top(bc->mempool, selected, MAX_TX_PER_BLOCK);

    /* 2. Build block */
    Block *b = block_create(bc->state.height,
                             bc->head->block_hash,
                             bc->state.difficulty,
                             miner_address);
    if (!b) return NULL;

    /* spec 7.iii: validate sender_nonce before including each transaction.
     * System wallet txs (no account or unsigned) are exempt.
     * Invalid-nonce txs are logged and left in the mempool.           */
    for (uint32_t i = 0; i < tx_count; i++) {
        Transaction *tx = selected[i]->tx;
        /* Skip nonce check for system-generated transactions */
        int is_system = (strncmp(tx->sender_address, ADDR_INSURANCE_POOL,   ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_REINSURANCE_POOL, ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_MINER_REWARD,     ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_GENESIS,          ADDR_SIZE) == 0 ||
                         tx->sig_len == 0);
        if (!is_system) {
            int valid = account_validate_nonce(bc->accounts,
                                               tx->sender_address,
                                               tx->sender_nonce);
            if (valid != 1) {
                printf("[mine_solo] skipping tx %.12s...: invalid nonce "
                       "(expected %llu, got %llu)\n",
                       tx->transaction_id,
                       (unsigned long long)account_get_next_nonce(
                           bc->accounts, tx->sender_address),
                       (unsigned long long)tx->sender_nonce);
                continue;
            }
        }
        block_add_transaction(b, tx_clone(tx));
    }

    /* 3. Add mining reward (coinbase) BEFORE PoW so it's part of the hash */
    Transaction *reward = tx_create(TX_MINING_REWARD,
                                     ADDR_MINER_REWARD, miner_address,
                                     bc->state.block_reward);
    tx_generate_id(reward);
    block_add_transaction(b, reward);

    /* 4. Compute Merkle root over final transaction set */
    merkle_root_hex(b->transactions, b->transaction_count, b->merkle_root);

    /* 5. Run PoW — block_hash is set to the valid solution hash */
    b->timestamp = time(NULL);
    /* spec 7.ix: block timestamp must be strictly greater than previous block */
    if (bc->head && b->timestamp <= bc->head->timestamp)
        b->timestamp = bc->head->timestamp + 1;
    mining_pow(b, bc->state.difficulty);

    /* 6. Credit miner in account model */
    account_credit(bc->accounts, miner_address, bc->state.block_reward);
    bc->token.total_supply += bc->state.block_reward;

    /* 7. Process confirmed transactions — update ledger state */
    for (uint32_t i = 0; i < b->transaction_count; i++)
        apply_confirmed_tx(bc, b->transactions[i]);

    /* 8. Append */
    chain_append_block(bc, b);

    printf("[mine_solo] Block #%u mined by %s  reward=%.2f AHT  txs=%u\n",
           b->block_id, miner_id, bc->state.block_reward, b->transaction_count);
    chain_save(bc);  /* spec 6.i: save after every mined block */
    return b;
}

PoolSession *mine_pool_init(void) {
    PoolSession *s = calloc(1, sizeof(PoolSession));
    return s;
}

int mine_pool_add(PoolSession *s, const char *miner_id,
                   const char *miner_address) {
    if (!s) return -1;
    MinerContrib *mc = calloc(1, sizeof(MinerContrib));
    if (!mc) return -1;
    strncpy(mc->miner_id,      miner_id,      ID_SIZE   - 1);
    strncpy(mc->miner_address, miner_address, ADDR_SIZE - 1);
    mc->hashes_attempted = 0;
    mc->next = s->miners;
    s->miners = mc;
    s->miner_count++;

    /* spec 5.v: pool membership must be recorded as a separate blockchain
     * transaction. Store the tx pointer in the session for later inclusion
     * in the mined block.                                                 */
    Transaction *mem_tx = tx_create(TX_POOL_MEMBER,
                                     miner_address,
                                     ADDR_MINER_WALLET,
                                     0.0);
    if (mem_tx) {
        strncpy(mem_tx->payload.generic.note, miner_id, DESC_SIZE - 1);
        tx_generate_id(mem_tx);
        mc->membership_tx = mem_tx;   /* owned by MinerContrib */
    }
    return 0;
}

Block *mine_pool_run(Blockchain *bc, PoolSession *s) {
    if (!bc || !s || s->miner_count == 0) return NULL;

    bc->mempool = mempool_sort(bc->mempool);
    MempoolEntry *selected[MAX_TX_PER_BLOCK];
    uint32_t tx_count = mempool_select_top(bc->mempool, selected, MAX_TX_PER_BLOCK);

    Block *b = block_create(bc->state.height, bc->head->block_hash,
                             bc->state.difficulty, "POOL");
    if (!b) return NULL;
    /* spec 7.iii: validate sender_nonce before including each transaction */
    for (uint32_t i = 0; i < tx_count; i++) {
        Transaction *tx = selected[i]->tx;
        int is_system = (strncmp(tx->sender_address, ADDR_INSURANCE_POOL,   ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_REINSURANCE_POOL, ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_MINER_REWARD,     ADDR_SIZE) == 0 ||
                         strncmp(tx->sender_address, ADDR_GENESIS,          ADDR_SIZE) == 0 ||
                         tx->sig_len == 0);
        if (!is_system) {
            int valid = account_validate_nonce(bc->accounts,
                                               tx->sender_address,
                                               tx->sender_nonce);
            if (valid != 1) {
                printf("[mine_pool] skipping tx %.12s...: invalid nonce\n",
                       tx->transaction_id);
                continue;
            }
        }
        block_add_transaction(b, tx_clone(tx));
    }

    snprintf(b->miner_id, ADDR_SIZE, "POOL_%u_miners", s->miner_count);
    s->total_reward = bc->state.block_reward;

    /* spec 5.v: include each miner's pool membership TX in the block */
    for (MinerContrib *mc = s->miners; mc; mc = mc->next) {
        if (mc->membership_tx) {
            block_add_transaction(b, tx_clone(mc->membership_tx));
        }
    }

    /* ── Phase 1: PoW over mempool transactions only ──────────────────────
     * We run PoW first so we can collect each miner's exact hash count.
     * Reward TXs are NOT included yet because their amounts depend on the
     * hash counts that are only known after the loop ends.               */
    merkle_root_hex(b->transactions, b->transaction_count, b->merkle_root);
    b->timestamp = time(NULL);
    if (bc->head && b->timestamp <= bc->head->timestamp)
        b->timestamp = bc->head->timestamp + 1;

    MinerContrib *mc = s->miners;
    while (1) {
        b->nonce++;
        if (mc) mc->hashes_attempted++;
        block_compute_hash(b);
        if (block_hash_meets_difficulty(b, bc->state.difficulty)) break;
        if (mc) mc = mc->next;
        if (!mc) mc = s->miners;
    }

    /* ── Tally hash counts and calculate proportional reward shares ───── */
    for (MinerContrib *c = s->miners; c; c = c->next)
        s->total_hashes += c->hashes_attempted;

    for (MinerContrib *c = s->miners; c; c = c->next) {
        c->reward_share = (s->total_hashes > 0)
            ? (double)c->hashes_attempted / s->total_hashes * s->total_reward
            : s->total_reward / s->miner_count;
    }

    /* ── Phase 2: add proportional reward TXs, recompute Merkle, re-PoW ─
     * Now that proportional amounts are known, create correctly-valued
     * TX_MINING_REWARD transactions and find a valid hash for the
     * complete block (mempool txs + proportional reward txs).           */
    for (MinerContrib *c = s->miners; c; c = c->next) {
        Transaction *reward = tx_create(TX_MINING_REWARD,
                                         ADDR_MINER_REWARD, c->miner_address,
                                         c->reward_share);
        tx_generate_id(reward);
        block_add_transaction(b, reward);
        account_credit(bc->accounts, c->miner_address, c->reward_share);
    }
    bc->token.total_supply += s->total_reward;

    /* Recompute Merkle with complete transaction set */
    merkle_root_hex(b->transactions, b->transaction_count, b->merkle_root);

    /* Brief second PoW pass — finds a valid hash for the final Merkle root.
     * Continues from current nonce so the extra work is minimal.         */
    while (1) {
        b->nonce++;
        block_compute_hash(b);
        if (block_hash_meets_difficulty(b, bc->state.difficulty)) break;
    }

    for (uint32_t i = 0; i < b->transaction_count; i++)
        apply_confirmed_tx(bc, b->transactions[i]);
    chain_append_block(bc, b);
    mining_print_pool_result(s, b);
    chain_save(bc);
    return b;
}

void mine_pool_free(PoolSession *s) {
    if (!s) return;
    MinerContrib *cur = s->miners, *nxt;
    while (cur) {
        nxt = cur->next;
        tx_free(cur->membership_tx);  /* free the pool membership tx */
        free(cur);
        cur = nxt;
    }
    free(s);
}

void mining_print_pool_result(const PoolSession *s, const Block *b) {
    printf("[mine_pool] Block #%u  total_hashes=%llu  reward=%.4f AHT\n",
           b->block_id, (unsigned long long)s->total_hashes, s->total_reward);
    for (MinerContrib *c = s->miners; c; c = c->next)
        printf("  Miner %-16s  hashes=%llu  share=%.4f AHT\n",
               c->miner_id,
               (unsigned long long)c->hashes_attempted,
               c->reward_share);
}
