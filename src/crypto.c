/*
 * crypto.c — OpenSSL 3.0 EVP-only implementation
 *
 * All functions use the EVP abstraction layer exclusively.
 * No deprecated EC_KEY, RSA, or low-level digest calls.
 */

#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Encoding helpers ────────────────────────────────────────────────────── */

void bytes_to_hex(const uint8_t *bytes, size_t len, char *out) {
    static const char HEX[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[2 * i]     = HEX[(bytes[i] >> 4) & 0xF];
        out[2 * i + 1] = HEX[bytes[i] & 0xF];
    }
    out[2 * len] = '\0';
}

int hex_to_bytes(const char *hex, uint8_t *out, size_t out_max) {
    size_t hex_len = strlen(hex);
    size_t byte_len = hex_len / 2;
    if (byte_len > out_max) byte_len = out_max;
    for (size_t i = 0; i < byte_len; i++) {
        unsigned int b;
        if (sscanf(hex + 2 * i, "%02x", &b) != 1) return -1;
        out[i] = (uint8_t)b;
    }
    return (int)byte_len;
}

/* ── SHA-256 ─────────────────────────────────────────────────────────────── */

int sha256_bytes(const uint8_t *data, size_t len, uint8_t out32[32]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    unsigned int digest_len = 32;
    int ok = (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1) &&
             (data == NULL || len == 0 ||
              EVP_DigestUpdate(ctx, data, len) == 1) &&
             (EVP_DigestFinal_ex(ctx, out32, &digest_len) == 1);
    EVP_MD_CTX_free(ctx);
    return ok ? 0 : -1;
}

int sha256_hex(const uint8_t *data, size_t len, char out_hex[HASH_SIZE]) {
    uint8_t hash[32];
    if (sha256_bytes(data, len, hash) != 0) return -1;
    bytes_to_hex(hash, 32, out_hex);
    return 0;
}

/* ── Key generation ──────────────────────────────────────────────────────── */

int crypto_generate_keypair(KeyPair *kp) {
    if (!kp) return -1;
    memset(kp, 0, sizeof(*kp));

    /*
     * Generate an ECDSA key pair on the P-256 (secp256r1) curve.
     *
     * OpenSSL 3.0 uses the EVP_PKEY abstraction exclusively — the old
     * EC_KEY API is deprecated. The steps are:
     *   1. EVP_PKEY_CTX_new_from_name — create a context for EC key ops.
     *   2. EVP_PKEY_keygen_init       — prepare for key generation.
     *   3. EVP_PKEY_CTX_set_params    — specify the curve ("P-256").
     *   4. EVP_PKEY_generate          — produce the key pair.
     *
     * P-256 gives 128-bit security, which is the current NIST standard
     * for digital signatures in financial applications.
     */
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
    if (!pctx) return -1;

    EVP_PKEY *pkey = NULL;
    OSSL_PARAM params[2];
    /* Specify P-256 as a UTF-8 string parameter — required by the EVP API */
    params[0] = OSSL_PARAM_construct_utf8_string("group", "P-256", 0);
    params[1] = OSSL_PARAM_construct_end();

    if (EVP_PKEY_keygen_init(pctx) != 1 ||
        EVP_PKEY_CTX_set_params(pctx, params) != 1 ||
        EVP_PKEY_generate(pctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(pctx);
        return -1;
    }
    EVP_PKEY_CTX_free(pctx);

    /* 2. Export private key as unencrypted PKCS#8 PEM */
    BIO *priv_bio = BIO_new(BIO_s_mem());
    if (!priv_bio || PEM_write_bio_PrivateKey(priv_bio, pkey, NULL,
                                               NULL, 0, NULL, NULL) != 1) {
        EVP_PKEY_free(pkey);
        BIO_free(priv_bio);
        return -1;
    }
    BUF_MEM *priv_buf = NULL;
    BIO_get_mem_ptr(priv_bio, &priv_buf);
    size_t priv_len = (priv_buf->length < (size_t)(PEM_SIZE - 1))
                      ? priv_buf->length : (size_t)(PEM_SIZE - 1);
    memcpy(kp->private_key_pem, priv_buf->data, priv_len);
    kp->private_key_pem[priv_len] = '\0';
    BIO_free(priv_bio);

    /* 3. Export public key as SubjectPublicKeyInfo PEM */
    BIO *pub_bio = BIO_new(BIO_s_mem());
    if (!pub_bio || PEM_write_bio_PUBKEY(pub_bio, pkey) != 1) {
        EVP_PKEY_free(pkey);
        BIO_free(pub_bio);
        return -1;
    }
    BUF_MEM *pub_buf = NULL;
    BIO_get_mem_ptr(pub_bio, &pub_buf);
    size_t pub_len = (pub_buf->length < (size_t)(PEM_SIZE - 1))
                     ? pub_buf->length : (size_t)(PEM_SIZE - 1);
    memcpy(kp->public_key_pem, pub_buf->data, pub_len);
    kp->public_key_pem[pub_len] = '\0';
    BIO_free(pub_bio);

    EVP_PKEY_free(pkey);

    /* 4. Derive wallet address = hex(SHA-256(DER-encoded public key)) */
    if (crypto_pubkey_to_address(kp->public_key_pem, kp->wallet_address) != 0)
        return -1;

    return 0;
}

int crypto_pubkey_to_address(const char *public_key_pem,
                               char out_addr[ADDR_SIZE]) {
    if (!public_key_pem || !out_addr) return -1;

    /* Load the public key from PEM */
    BIO *bio = BIO_new_mem_buf(public_key_pem, -1);
    if (!bio) return -1;
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!pkey) return -1;

    /* Export as DER */
    unsigned char *der = NULL;
    int der_len = i2d_PUBKEY(pkey, &der);
    EVP_PKEY_free(pkey);
    if (der_len <= 0) return -1;

    /* SHA-256(DER) → hex */
    uint8_t hash[32];
    int rc = sha256_bytes(der, (size_t)der_len, hash);
    OPENSSL_free(der);
    if (rc != 0) return -1;

    bytes_to_hex(hash, 32, out_addr);
    return 0;
}

/* ── ECDSA sign / verify ─────────────────────────────────────────────────── */

int crypto_sign(const KeyPair *kp,
                const uint8_t *data, size_t data_len,
                uint8_t sig_buf[SIG_MAX_LEN], size_t *sig_len) {
    if (!kp || !data || !sig_buf || !sig_len) return -1;

    /* Load the PKCS#8 PEM private key into an EVP_PKEY object */
    BIO *bio = BIO_new_mem_buf(kp->private_key_pem, -1);
    if (!bio) return -1;
    EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!pkey) return -1;

    /*
     * ECDSA signing via EVP_DigestSign* (OpenSSL 3.0 one-shot API):
     *
     *   EVP_DigestSignInit   — bind the private key and hash algorithm
     *                          (SHA-256) to the signing context.
     *   EVP_DigestSignUpdate — feed the data to be signed; can be called
     *                          multiple times for streaming data.
     *   EVP_DigestSignFinal  — compute SHA-256(data), sign the digest
     *                          with ECDSA, and write the DER-encoded
     *                          signature to sig_buf.
     *
     * The DER-encoded P-256 signature is at most 72 bytes (SIG_MAX_LEN).
     * The actual length is written to *sig_len.
     */
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }

    size_t sig_sz = SIG_MAX_LEN;
    int ok = (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey) == 1) &&
             (EVP_DigestSignUpdate(ctx, data, data_len) == 1) &&
             (EVP_DigestSignFinal(ctx, sig_buf, &sig_sz) == 1);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (ok) *sig_len = sig_sz;
    return ok ? 0 : -1;
}

int crypto_verify(const char    *public_key_pem,
                  const uint8_t *data,     size_t data_len,
                  const uint8_t *sig_buf,  size_t sig_len) {
    if (!public_key_pem || !data || !sig_buf) return -1;

    BIO *bio = BIO_new_mem_buf(public_key_pem, -1);
    if (!bio) return -1;
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!pkey) return -1;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }

    /*
     * ECDSA verification via EVP_DigestVerify*:
     *   EVP_DigestVerifyInit   — bind the public key + SHA-256.
     *   EVP_DigestVerifyUpdate — feed the same data that was signed.
     *   EVP_DigestVerifyFinal  — recompute SHA-256(data), then verify
     *                            that the ECDSA signature matches.
     *
     * Return values from EVP_DigestVerifyFinal:
     *   1  = signature is valid (sender owned the private key)
     *   0  = signature is invalid (data or key mismatch)
     *  -1  = error during verification
     */
    int result = (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey) == 1) &&
                 (EVP_DigestVerifyUpdate(ctx, data, data_len) == 1)
                 ? EVP_DigestVerifyFinal(ctx, sig_buf, sig_len)
                 : -1;

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return result;
}

/* ── Key file persistence ────────────────────────────────────────────────── */

int crypto_save_keypair(const KeyPair *kp,
                         const char *priv_path, const char *pub_path) {
    if (!kp) return -1;
    FILE *f;

    f = fopen(priv_path, "w");
    if (!f) { perror(priv_path); return -1; }
    fputs(kp->private_key_pem, f);
    fclose(f);

    f = fopen(pub_path, "w");
    if (!f) { perror(pub_path); return -1; }
    fputs(kp->public_key_pem, f);
    fclose(f);

    return 0;
}

int crypto_load_keypair(KeyPair *kp,
                         const char *priv_path, const char *pub_path) {
    if (!kp) return -1;

    /* Read private key PEM file */
    FILE *f = fopen(priv_path, "r");
    if (!f) { perror(priv_path); return -1; }
    size_t n = fread(kp->private_key_pem, 1, PEM_SIZE - 1, f);
    kp->private_key_pem[n] = '\0';
    fclose(f);

    /* Read public key PEM file */
    f = fopen(pub_path, "r");
    if (!f) { perror(pub_path); return -1; }
    n = fread(kp->public_key_pem, 1, PEM_SIZE - 1, f);
    kp->public_key_pem[n] = '\0';
    fclose(f);

    /* Re-derive wallet address from public key */
    return crypto_pubkey_to_address(kp->public_key_pem, kp->wallet_address);
}
