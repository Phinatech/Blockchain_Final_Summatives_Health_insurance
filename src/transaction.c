#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── transaction.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/transaction.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TX_TYPE_NAMES[] = {
    "POLICY_ENROLL", "PREMIUM_PAYMENT", "REINSURANCE_CONTRIBUTION",
    "SERVICE_REQUEST", "PREAUTH_REQUEST", "PREAUTH_APPROVE",
    "CLAIM_SUBMIT", "CLAIM_APPROVE", "CLAIM_REJECT", "CLAIM_SETTLE",
    "TOKEN_TRANSFER", "MINING_REWARD", "POOL_MEMBER"
};

Transaction *tx_create(TransactionType type, const char *sender,
                        const char *receiver, double amount) {
    Transaction *tx = calloc(1, sizeof(Transaction));
    if (!tx) return NULL;
    tx->transaction_type      = type;
    tx->amount    = amount;
    tx->timestamp = time(NULL);
    if (sender)   strncpy(tx->sender_address,   sender,   ADDR_SIZE - 1);
    if (receiver) strncpy(tx->receiver_address, receiver, ADDR_SIZE - 1);
    return tx;
}

Transaction *tx_clone(const Transaction *src) {
    if (!src) return NULL;
    Transaction *dst = malloc(sizeof(Transaction));
    if (!dst) return NULL;
    memcpy(dst, src, sizeof(Transaction));
    // (no list pointer in Transaction struct)
    return dst;
}

void tx_free(Transaction *tx) { free(tx); }

int tx_serialise(const Transaction *tx, uint8_t *buf, size_t buf_max) {
    /* Stable serialisation excludes sig fields so sign/verify use same bytes */
    int n = snprintf((char *)buf, buf_max,
        "%s|%s|%s|%.8f|%d|%lld|%llu",
        tx->transaction_id,
        tx->sender_address, tx->receiver_address,
        tx->amount, (int)tx->transaction_type,
        (long long)tx->timestamp,
        (unsigned long long)tx->sender_nonce);
    return (n > 0 && (size_t)n < buf_max) ? n : -1;
}

int tx_sign(Transaction *tx, const KeyPair *kp) {
    uint8_t buf[4096]; uint8_t digest[32];
    int n = tx_serialise(tx, buf, sizeof(buf));
    if (n < 0) return -1;
    if (sha256_bytes(buf, (size_t)n, digest) < 0) return -1;
    return crypto_sign(kp, digest, 32, tx->digital_signature, &tx->sig_len);
}

int tx_verify(const Transaction *tx, const char *sender_pubkey_pem) {
    uint8_t buf[4096]; uint8_t digest[32];
    int n = tx_serialise(tx, buf, sizeof(buf));
    if (n < 0) return -1;
    if (sha256_bytes(buf, (size_t)n, digest) < 0) return -1;
    return crypto_verify(sender_pubkey_pem, digest, 32,
                         tx->digital_signature, tx->sig_len);
}

void tx_generate_id(Transaction *tx) {
    uint8_t buf[4096]; uint8_t hash[32];
    int n = tx_serialise(tx, buf, sizeof(buf));
    if (n > 0) {
        sha256_bytes(buf, (size_t)n, hash);
        bytes_to_hex(hash, 32, tx->transaction_id);
    }
}

const char *tx_type_str(TransactionType t) {
    if (t >= 0 && t <= TX_POOL_MEMBER) return TX_TYPE_NAMES[t];
    return "UNKNOWN";
}

void tx_print(const Transaction *tx) {
    if (!tx) return;
    printf("  TX [%.16s...]  type=%-22s  amount=%.4f AHT\n"
           "      from=%.16s...  to=%.16s...  nonce=%llu\n",
           tx->transaction_id, tx_type_str(tx->transaction_type), tx->amount,
           tx->sender_address, tx->receiver_address,
           (unsigned long long)tx->sender_nonce);
}
