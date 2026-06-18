/*
 * main.c — ALU Health Insurance Blockchain
 *
 * Entry point:
 *   1. chain_startup()  — load existing chain from disk or init fresh
 *   2. cli_run()        — REPL: parse and dispatch all 30+ commands
 *   3. chain_free()     — clean up on exit
 */

#include "chain.h"
#include "persistence.h"
#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_FILE "data/chain.dat"

int selftest(void);   /* forward declaration */

int main(int argc, char *argv[]) {
    /* Handle --selftest flag (used by: make check) */
    if (argc > 1 && strcmp(argv[1], "--selftest") == 0)
        return selftest();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║     ALU Health Insurance Blockchain  (AHT Token v1.0)       ║\n");
    printf("║     OpenSSL EVP  ·  ECDSA P-256  ·  SHA-256  ·  PoW        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* Load existing chain or start fresh */
    Blockchain *bc = chain_startup(SAVE_FILE);
    if (!bc) {
        fprintf(stderr, "[main] chain_startup() failed — aborting.\n");
        return EXIT_FAILURE;
    }

    /* Hand control to the REPL */
    cli_run(bc);

    /* Save and clean up */
    chain_free(bc);
    printf("[main] Goodbye.\n");
    return EXIT_SUCCESS;
}

/*
 * selftest — basic sanity checks run by "make check".
 * Tests: SHA-256, ECDSA key generation, blockchain init, and chain verify.
 * Returns 0 on pass, 1 on any failure.
 */
#include "crypto.h"
#include "merkle.h"
#include "transaction.h"
#include "block.h"
#include "chain.h"

int selftest(void) {
    int pass = 0, fail = 0;

    printf("=== ALU Blockchain Self-Test ===\n");

    /* Test 1: SHA-256 produces a 64-char hex string */
    {
        char out[65];
        uint8_t hash[32];
        const uint8_t data[] = "test";
        sha256_bytes(data, 4, hash);
        bytes_to_hex(hash, 32, out);
        if (strlen(out) == 64) {
            printf("  [PASS] SHA-256 produces 64-char hex\n"); pass++;
        } else {
            printf("  [FAIL] SHA-256 output length wrong: %zu\n", strlen(out)); fail++;
        }
    }

    /* Test 2: ECDSA key generation */
    {
        KeyPair kp;
        if (crypto_generate_keypair(&kp) == 0 && strlen(kp.wallet_address) == 64) {
            printf("  [PASS] ECDSA P-256 key generation and wallet address\n"); pass++;
        } else {
            printf("  [FAIL] ECDSA key generation failed\n"); fail++;
        }
    }

    /* Test 3: Blockchain initialises with genesis block */
    {
        Blockchain *bc = chain_init();
        if (bc && bc->state.height == 1 && bc->genesis) {
            printf("  [PASS] Chain init: genesis block created, height=1\n"); pass++;
        } else {
            printf("  [FAIL] Chain init failed\n"); fail++;
        }
        if (bc) chain_free(bc);
    }

    /* Test 4: blockchain_verify on fresh chain passes */
    {
        Blockchain *bc = chain_init();
        if (bc && chain_verify(bc)) {
            printf("  [PASS] blockchain_verify passes on fresh chain\n"); pass++;
        } else {
            printf("  [FAIL] blockchain_verify failed on fresh chain\n"); fail++;
        }
        if (bc) chain_free(bc);
    }

    /* Test 5: Merkle root is deterministic */
    {
        Transaction *tx = tx_create(TX_TOKEN_TRANSFER,
                                     "AAAA", "BBBB", 1.0);
        if (tx) {
            char root1[65], root2[65];
            merkle_root_hex(&tx, 1, root1);
            merkle_root_hex(&tx, 1, root2);
            if (strcmp(root1, root2) == 0 && strlen(root1) == 64) {
                printf("  [PASS] Merkle root is deterministic\n"); pass++;
            } else {
                printf("  [FAIL] Merkle root not deterministic\n"); fail++;
            }
            tx_free(tx);
        }
    }

    printf("\n=== Result: %d passed, %d failed ===\n", pass, fail);
    return (fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
