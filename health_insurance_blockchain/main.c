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

#define SAVE_FILE "data/chain.dat"

int main(void) {
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
