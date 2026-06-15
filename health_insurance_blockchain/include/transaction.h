#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "types.h"
#include "crypto.h"
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Type-specific payload structs
 *
 * Each payload carries only the domain fields that are NOT already in the
 * base Transaction struct (sender, receiver, amount, timestamp).
 * ═══════════════════════════════════════════════════════════════════════════ */

/* TX_POLICY_ENROLL */
typedef struct {
    char         member_id[ID_SIZE];
    char         policy_id[ID_SIZE];
    char         coverage_plan[NAME_SIZE];
    time_t       enrollment_date;
    time_t       expiry_date;          /* enrollment_date + 365 days */
    PolicyStatus status;               /* spec: field is "status", POLICY_ACTIVE on creation */
} PolicyEnrollPayload;

/* TX_PREMIUM_PAYMENT — base fields (amount) + policy reference */
typedef struct {
    char policy_id[ID_SIZE];
    char member_id[ID_SIZE];
} PremiumPayload;

/* TX_REINSURANCE_CONTRIBUTION — auto-generated alongside every premium */
typedef struct {
    char source_tx_id[HASH_SIZE];   /* references the premium tx that spawned this */
    char policy_id[ID_SIZE];
} ReinsuranceContributionPayload;

/* TX_SERVICE_REQUEST */
typedef struct {
    char member_id[ID_SIZE];
    char provider_id[ID_SIZE];
    char service_type[NAME_SIZE];
} ServicePayload;

/* TX_PREAUTH_REQUEST */
typedef struct {
    char   preauth_id[ID_SIZE];
    char   member_id[ID_SIZE];
    char   provider_id[ID_SIZE];
    char   service_type[NAME_SIZE];
    double estimated_cost;
} PreauthRequestPayload;

/* TX_PREAUTH_APPROVE */
typedef struct {
    char          preauth_id[ID_SIZE];
    double        approved_amount;
    PreauthStatus status;
} PreauthApprovePayload;

/* TX_CLAIM_SUBMIT */
typedef struct {
    char      claim_id[ID_SIZE];
    char      member_id[ID_SIZE];
    char      policy_id[ID_SIZE];
    char      provider_id[ID_SIZE];
    time_t    service_date;
    char      diagnosis[DESC_SIZE];
    ClaimStatus claim_status;        /* CLAIM_PENDING on submit */
} ClaimSubmitPayload;

/* TX_CLAIM_APPROVE / TX_CLAIM_REJECT */
typedef struct {
    char        claim_id[ID_SIZE];
    double      approved_amount;
    ClaimStatus decision;
    char        reason[DESC_SIZE];
} ClaimDecisionPayload;

/* TX_CLAIM_SETTLE */
typedef struct {
    char         claim_id[ID_SIZE];
    double       insurance_pays;     /* from main insurance pool */
    double       reinsurance_pays;   /* from reinsurance pool (> threshold) */
    SettleSource source;
} ClaimSettlePayload;

/* TX_TOKEN_TRANSFER / TX_MINING_REWARD — no extra payload needed */
typedef struct {
    char note[DESC_SIZE];            /* optional memo */
} GenericPayload;

/* ── Union of all payload types ─────────────────────────────────────────── */
typedef union {
    PolicyEnrollPayload       policy_enroll;
    PremiumPayload            premium;
    ReinsuranceContributionPayload reinsurance_contribution;
    ServicePayload            service;
    PreauthRequestPayload     preauth_request;
    PreauthApprovePayload     preauth_approve;
    ClaimSubmitPayload        claim_submit;
    ClaimDecisionPayload      claim_decision;
    ClaimSettlePayload        claim_settle;
    GenericPayload            generic;
} TxPayload;

/* ═══════════════════════════════════════════════════════════════════════════
 * Transaction struct
 *
 * NOTE: This struct never stores its own hash.
 *   • Signing: SHA-256 of serialised fields is signed with ECDSA; the
 *     resulting bytes go into digital_signature; the digest is discarded.
 *   • Merkle: the miner recomputes SHA-256 of the fields as a leaf hash;
 *     those hashes are never stored back here.
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct Transaction {
    char            transaction_id[HASH_SIZE];  /* unique, generated at creation */
    char            sender_address[ADDR_SIZE];
    char            receiver_address[ADDR_SIZE];
    double          amount;
    TransactionType transaction_type;  /* spec 1.ii: field name */
    time_t          timestamp;

    /*
     * sender_nonce: snapshot of the sender's account nonce at signing time.
     * Copied from Account.nonce; locked once signed.
     * Network rejects tx if sender_nonce != account.nonce + 1.
     */
    uint64_t        sender_nonce;

    /* DER-encoded ECDSA P-256 signature of (sha256 of serialised tx fields) */
    uint8_t         digital_signature[SIG_MAX_LEN];
    size_t          sig_len;

    /* Type-specific domain data */
    TxPayload       payload;
} Transaction;

/* ── Transaction factory functions ──────────────────────────────────────── */

/* Allocate and zero-initialise a new Transaction; returns NULL on OOM */
Transaction *tx_create(TransactionType type,
                        const char *sender, const char *receiver,
                        double amount);

/* Deep-copy a transaction (used when moving from mempool to block) */
Transaction *tx_clone(const Transaction *src);

void tx_free(Transaction *tx);

/* ── Serialisation (for signing and Merkle hashing) ─────────────────────── */

/*
 * tx_serialise — write a stable byte representation of tx fields into buf.
 * The digital_signature and sig_len fields are intentionally excluded so
 * that sign and verify operate on the same canonical form.
 * Returns number of bytes written, or -1 on error.
 */
int tx_serialise(const Transaction *tx, uint8_t *buf, size_t buf_max);

/* ── Signing / verification ─────────────────────────────────────────────── */

/*
 * tx_sign   — serialise tx, sign the SHA-256 digest with kp's private key,
 *             store the DER signature in tx->digital_signature.
 * tx_verify — serialise tx, recompute SHA-256, verify against sender's pubkey.
 *             Returns 1 valid, 0 invalid, -1 error.
 */
int tx_sign  (Transaction *tx, const KeyPair *kp);
int tx_verify(const Transaction *tx, const char *sender_public_key_pem);

/* ── Utility ─────────────────────────────────────────────────────────────── */

/* Generate a unique transaction ID (SHA-256 of fields at creation time) */
void tx_generate_id(Transaction *tx);

const char *tx_type_str(TransactionType t);

void tx_print(const Transaction *tx);

#endif /* TRANSACTION_H */
