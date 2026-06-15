#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "chain.h"

/* ── File format ─────────────────────────────────────────────────────────── */

/*
 * Persistence uses a binary flat-file format (.dat).
 *
 * Layout:
 *   [FILE_MAGIC]        4 bytes  — "AHT\x01"
 *   [ChainState]        sizeof(ChainState)
 *   [Token]             sizeof(Token)
 *   [ReinsurancePool]   sizeof(ReinsurancePool)
 *   [account_count]     uint32_t
 *   [Account * N]       account_count × sizeof(Account)
 *   [member_count]      uint32_t
 *   [Member * N]        member_count × sizeof(Member)
 *   [provider_count]    uint32_t
 *   [Provider * N]      provider_count × sizeof(Provider)
 *   [policy_count]      uint32_t
 *   [Policy * N]        policy_count × sizeof(Policy)
 *   [claim_count]       uint32_t
 *   [Claim * N]         claim_count × sizeof(Claim)
 *   [preauth_count]     uint32_t
 *   [Preauth * N]       preauth_count × sizeof(Preauth)
 *   [utxo_count]        uint32_t
 *   [UTXO * N]          utxo_count × sizeof(UTXO)
 *   [block_count]       uint32_t
 *   for each block:
 *     [Block header]    sizeof(Block) - (sizeof(Transaction**) + sizeof(Block*))
 *     [tx_count]        uint32_t
 *     for each tx:
 *       [Transaction]   sizeof(Transaction)
 *   [mempool_count]     uint32_t
 *   for each entry:
 *     [MempoolEntry]    sizeof(MempoolEntry) - sizeof(Transaction*) - sizeof(MempoolEntry*)
 *     [Transaction]     sizeof(Transaction)
 *
 * Note: pointers (next, tx, transactions[]) are NOT written; they are
 * reconstructed by the loader.
 */

#define PERSIST_MAGIC  "AHT\x01"
#define PERSIST_MAGIC_LEN 4

/* ── Save ────────────────────────────────────────────────────────────────── */

/*
 * chain_save — write full state to bc->state.save_file.
 * Returns 0 on success, -1 on I/O error.
 */
int chain_save(const Blockchain *bc);

/* chain_save_to — override path; useful for manual chain_save command */
int chain_save_to(const Blockchain *bc, const char *path);

/* ── Load ────────────────────────────────────────────────────────────────── */

/*
 * chain_load — read state from `path` into a freshly allocated Blockchain.
 * If path does not exist, returns NULL (caller should call chain_init instead).
 * Returns the loaded Blockchain on success, NULL on error.
 */
Blockchain *chain_load(const char *path);

/* ── Startup sequence ────────────────────────────────────────────────────── */

/*
 * chain_startup — open-or-create logic:
 *   1. Try chain_load(path).
 *   2. If successful, run chain_verify() on the loaded chain.
 *      If verification fails, print an error and return NULL.
 *   3. If file does not exist, call chain_init() and return fresh chain.
 *
 * This is the only entry point that should be used in main().
 */
Blockchain *chain_startup(const char *path);

#endif /* PERSISTENCE_H */
