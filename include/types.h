#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

/* ── Size constants ─────────────────────────────────────────────────────── */

/* SHA-256 produces 32 bytes → 64 hex chars + NUL */
#define HASH_SIZE        65

/* SHA-256(pubkey) as wallet address: 64 hex + NUL */
#define ADDR_SIZE        65

/* Generic identifier fields (member IDs, policy IDs, claim IDs …) */
#define ID_SIZE          48

/* Human-readable name fields */
#define NAME_SIZE        64

/* PEM-encoded P-256 key: public ~180 B, private ~250 B; 512 B is safe */
#define PEM_SIZE         512

/* DER-encoded ECDSA-P256 signature upper bound */
#define SIG_MAX_LEN      72

/* Longest coverage plan or diagnosis string */
#define DESC_SIZE        128

/* Max transactions per block */
#define MAX_TX_PER_BLOCK 512

/* ── Mining / chain constants ────────────────────────────────────────────── */

#define INITIAL_DIFFICULTY       2
#define INITIAL_BLOCK_REWARD     50.0   /* AHT */
#define BLOCKS_PER_RETARGET      10
#define TARGET_BLOCK_TIME_MIN    30     /* seconds: speed up → raise difficulty */
#define TARGET_BLOCK_TIME_MAX    90     /* seconds: slow  down → lower  difficulty */
#define MIN_DIFFICULTY           1

/* ── Insurance pool constants ────────────────────────────────────────────── */

#define REINSURANCE_RATE         0.05   /* 5 % of every premium auto-contributed */
#define SETTLEMENT_THRESHOLD     1000.0 /* AHT: above this the reinsurance pool co-pays */
#define MAX_CLAIMS_PER_24H       3      /* fraud: >3 claims in 24 h → SUSPICIOUS */
#define FRAUD_AMOUNT_MULTIPLIER  2.0    /* fraud: claim > 2× provider avg → SUSPICIOUS */

/* ── Well-known wallet addresses ─────────────────────────────────────────── */

#define ADDR_INSURANCE_POOL      "INSURANCE_POOL_WALLET"
#define ADDR_REINSURANCE_POOL    "REINSURANCE_POOL_WALLET"
#define ADDR_MINER_REWARD        "COINBASE"
#define ADDR_MINER_WALLET        "MINER_WALLET"    /* spec 3.ii: Miner Wallet account */
#define ADDR_GENESIS             "GENESIS"

/* ── Transaction types ───────────────────────────────────────────────────── */

typedef enum {
    TX_POLICY_ENROLL          = 0,
    TX_PREMIUM_PAYMENT        = 1,
    TX_REINSURANCE_CONTRIBUTION    = 2,  /* auto-generated with every premium */
    TX_SERVICE_REQUEST        = 3,
    TX_PREAUTH_REQUEST        = 4,
    TX_PREAUTH_APPROVE        = 5,
    TX_CLAIM_SUBMIT           = 6,
    TX_CLAIM_APPROVE          = 7,
    TX_CLAIM_REJECT           = 8,
    TX_CLAIM_SETTLE           = 9,
    TX_TOKEN_TRANSFER         = 10,
    TX_MINING_REWARD          = 11,
    TX_POOL_MEMBER            = 12   /* spec 5.v: pool membership recorded on-chain */
} TransactionType;

/* ── Policy lifecycle ────────────────────────────────────────────────────── */

typedef enum {
    POLICY_ACTIVE    = 0,
    POLICY_EXPIRED   = 1,
    POLICY_RENEWED   = 2,
    POLICY_SUSPENDED = 3
} PolicyStatus;

/* ── Claim lifecycle ─────────────────────────────────────────────────────── */

typedef enum {
    CLAIM_PENDING    = 0,
    CLAIM_APPROVED   = 1,
    CLAIM_REJECTED   = 2,
    CLAIM_SETTLED    = 3
} ClaimStatus;

/* Which pool paid during a settlement (for split settlements > threshold) */
typedef enum {
    SETTLE_INSURANCE    = 0,
    SETTLE_REINSURANCE  = 1,
    SETTLE_SPLIT        = 2
} SettleSource;

/* ── Pre-authorisation ───────────────────────────────────────────────────── */

typedef enum {
    PREAUTH_PENDING  = 0,
    PREAUTH_APPROVED = 1,
    PREAUTH_REJECTED = 2
} PreauthStatus;

/* ── Mempool entry status ────────────────────────────────────────────────── */

typedef enum {
    MEMPOOL_PENDING    = 0,
    MEMPOOL_CONFIRMED  = 1,
    MEMPOOL_SUSPICIOUS = 2
} MempoolStatus;

/* ── Utility macro ───────────────────────────────────────────────────────── */

/* Compile-time check that buf is large enough for str */
#define STATIC_ASSERT(expr) typedef char static_assert_##__LINE__[(expr) ? 1 : -1]

#endif /* TYPES_H */
