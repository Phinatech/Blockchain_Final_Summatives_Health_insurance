#include "token.h"
#include <stdio.h>
#include <string.h>

void token_init(Token *t) {
    if (!t) return;
    strncpy(t->token_name,   "ALU Health Token", NAME_SIZE - 1);
    strncpy(t->token_symbol, "AHT", 7);
    t->total_supply       = 0.0;
    t->circulating_supply = 0.0;
}

void token_print(const Token *t) {
    if (!t) return;
    printf("Token : %s (%s)\n", t->token_name, t->token_symbol);
    printf("  Total supply       : %.4f AHT\n", t->total_supply);
    printf("  Circulating supply : %.4f AHT\n", t->circulating_supply);
}
