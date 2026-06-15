#ifndef TOKEN_H
#define TOKEN_H

#include "types.h"

/* ── AHT token ───────────────────────────────────────────────────────────── */

/*
 * The ALU Health Token (AHT) is the native currency of the chain.
 * total_supply grows with each mining reward; circulating_supply tracks
 * tokens currently held in non-pool wallets.
 */
typedef struct {
    char   token_name[NAME_SIZE];     /* "ALU Health Token" */
    char   token_symbol[8];           /* "AHT" */
    double total_supply;             /* all tokens ever minted */
    double circulating_supply;       /* tokens outside pool wallets */
} Token;

/* Initialise token fields to defaults */
void token_init(Token *t);

/* Print a one-line token summary to stdout */
void token_print(const Token *t);

#endif /* TOKEN_H */
