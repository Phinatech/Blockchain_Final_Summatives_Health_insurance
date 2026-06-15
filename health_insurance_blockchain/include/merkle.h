#ifndef MERKLE_H
#define MERKLE_H

#include "types.h"
#include "transaction.h"
#include <stdint.h>
#include <stddef.h>

/* ── Internal tree node ─────────────────────────────────────────────────── */

typedef struct MerkleNode {
    uint8_t            hash[32];       /* raw SHA-256 bytes */
    struct MerkleNode *left;
    struct MerkleNode *right;
} MerkleNode;

/* ── Tree handle ─────────────────────────────────────────────────────────── */

typedef struct {
    MerkleNode *root;
    uint32_t    leaf_count;
} MerkleTree;

/* ── Building ────────────────────────────────────────────────────────────── */

/*
 * merkle_build — compute a Merkle tree from an array of transactions.
 *
 * Algorithm:
 *   1. For each tx, compute SHA-256(serialised tx fields) → leaf hash.
 *   2. Pair adjacent leaves; if odd count, duplicate the last leaf.
 *   3. Hash each pair together to produce the next level.
 *   4. Repeat until one hash remains: the root.
 *
 * Returns a heap-allocated MerkleTree (caller owns it), or NULL on error.
 */
MerkleTree *merkle_build(Transaction **txs, uint32_t count);

/*
 * merkle_root_hex — convenience wrapper: build the tree and copy the
 * root hash as a 64-char hex string + NUL into out_hex[HASH_SIZE].
 * Returns 0 on success, -1 on error.
 */
int merkle_root_hex(Transaction **txs, uint32_t count,
                    char out_hex[HASH_SIZE]);

/* ── Verification ────────────────────────────────────────────────────────── */

/*
 * merkle_verify — recompute the root from txs and compare against
 * the stored root_hex.  Returns 1 if they match, 0 if tampered, -1 on error.
 */
int merkle_verify(Transaction **txs, uint32_t count,
                  const char root_hex[HASH_SIZE]);

/* ── Cleanup ─────────────────────────────────────────────────────────────── */

void merkle_free(MerkleTree *tree);

#endif /* MERKLE_H */
