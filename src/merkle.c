/*
 * merkle.c — Binary Merkle tree, built from scratch (no external libraries).
 *
 * Algorithm overview (spec Section 1.vi):
 *   1. Serialise each transaction and SHA-256 hash it → leaf hashes.
 *   2. Pair adjacent leaf hashes, concatenate, and SHA-256 the pair → next level.
 *   3. If the number of hashes at any level is odd, duplicate the last one
 *      before pairing so every node has two children.
 *   4. Repeat until exactly one hash remains: this is the Merkle root.
 *   5. Store the root in the block header ONLY. Leaf hashes are discarded.
 *
 * The root lets the miner prove, via block_verify(), that no transaction
 * was altered after the block was mined: any change to any tx changes its
 * leaf hash, which ripples up and changes the root.
 */

#include "merkle.h"
#include "crypto.h"
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal helper ─────────────────────────────────────────────────────── */

/*
 * hash_pair — concatenate two 32-byte hashes and SHA-256 the result.
 *
 * In Bitcoin's Merkle tree (which this follows), an inner node is:
 *   H(left || right)
 * where || is concatenation. Both inputs are 32 bytes so the combined
 * buffer is always exactly 64 bytes.
 */
static void hash_pair(const uint8_t *left32, const uint8_t *right32,
                       uint8_t out32[32]) {
    uint8_t combined[64];
    memcpy(combined,      left32,  32);   /* first half  */
    memcpy(combined + 32, right32, 32);   /* second half */
    sha256_bytes(combined, 64, out32);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

MerkleTree *merkle_build(Transaction **txs, uint32_t count) {
    if (!txs || count == 0) return NULL;

    /* ── Step 1: Leaf hashes ─────────────────────────────────────────────
     * Serialise every transaction to a stable byte representation, then
     * SHA-256 it to produce the leaf hash. The serialisation deliberately
     * excludes the digital_signature field so that the signed bytes and the
     * Merkle leaf bytes use the same canonical form.
     * The resulting array hashes[] holds count × 32 bytes on the heap.    */
    uint8_t (*hashes)[32] = calloc(count, 32);
    if (!hashes) return NULL;

    for (uint32_t i = 0; i < count; i++) {
        uint8_t buf[4096];
        uint8_t digest[32];
        int n = tx_serialise(txs[i], buf, sizeof(buf));
        if (n < 0)
            sha256_bytes(NULL, 0, digest);    /* fallback: hash of empty input */
        else
            sha256_bytes(buf, (size_t)n, digest);
        memcpy(hashes[i], digest, 32);
    }

    /* ── Steps 2–4: Reduce level by level until one root hash remains ────
     *
     * Each iteration halves the number of hashes:
     *   next_size = ceil(level_size / 2)
     *
     * For every pair (i=0,1), (i=2,3), …:
     *   li = left child index  = 2 * i
     *   ri = right child index = 2*i + 1 if it exists, else li (duplicate)
     *
     * Duplicating the last hash when count is odd is the standard rule
     * for Bitcoin-style Merkle trees and prevents structural ambiguity.
     *
     * hashes[] is freed each iteration and replaced with next[]; when the
     * loop exits, hashes[0] is the root.                                  */
    uint32_t level_size = count;
    while (level_size > 1) {
        uint32_t next_size = (level_size + 1) / 2;           /* ceil divide */
        uint8_t (*next)[32] = calloc(next_size, 32);
        if (!next) { free(hashes); return NULL; }

        for (uint32_t i = 0; i < next_size; i++) {
            uint32_t li = 2 * i;
            /* If level_size is odd, the last node has no right sibling:
             * duplicate it (li == ri) as per the Merkle tree standard.   */
            uint32_t ri = (li + 1 < level_size) ? li + 1 : li;
            hash_pair(hashes[li], hashes[ri], next[i]);
        }

        free(hashes);         /* discard the now-processed level */
        hashes    = next;
        level_size = next_size;
    }

    /* ── Wrap the single remaining hash in a MerkleTree struct ──────────── */
    MerkleTree *tree = calloc(1, sizeof(MerkleTree));
    if (!tree) { free(hashes); return NULL; }

    MerkleNode *root = calloc(1, sizeof(MerkleNode));
    if (!root)  { free(hashes); free(tree); return NULL; }

    memcpy(root->hash, hashes[0], 32);
    free(hashes);               /* leaf storage no longer needed */

    tree->root       = root;
    tree->leaf_count = count;
    return tree;
}

/*
 * merkle_root_hex — convenience wrapper.
 * Builds the tree, encodes the root as 64 lowercase hex chars + NUL,
 * then frees the tree. This is what the miner stores in block.merkle_root.
 */
int merkle_root_hex(Transaction **txs, uint32_t count,
                     char out_hex[HASH_SIZE]) {
    MerkleTree *tree = merkle_build(txs, count);
    if (!tree) return -1;
    bytes_to_hex(tree->root->hash, 32, out_hex);
    merkle_free(tree);
    return 0;
}

/*
 * merkle_verify — recompute the root from txs and compare against
 * the stored root_hex. Used by block_verify() during chain audit.
 * Returns 1 if the transactions match the recorded root (untampered),
 *         0 if the root doesn't match (a transaction was altered),
 *        -1 on error.
 */
int merkle_verify(Transaction **txs, uint32_t count,
                   const char root_hex[HASH_SIZE]) {
    char computed[HASH_SIZE];
    if (merkle_root_hex(txs, count, computed) < 0) return -1;
    return (strncmp(computed, root_hex, HASH_SIZE) == 0) ? 1 : 0;
}

void merkle_free(MerkleTree *tree) {
    if (!tree) return;
    /* This flat-array implementation allocates only the root MerkleNode.
     * A full tree-node implementation would require a recursive free here. */
    free(tree->root);
    free(tree);
}
