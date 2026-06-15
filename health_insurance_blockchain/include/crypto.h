#ifndef CRYPTO_H
#define CRYPTO_H

#include "types.h"
#include <stdint.h>
#include <stddef.h>

/* ── Key material ────────────────────────────────────────────────────────── */

/*
 * KeyPair holds a participant's P-256 key material in PEM form.
 * The wallet_address is SHA-256(DER-encoded public key), hex-encoded.
 * Keys are persisted to separate PEM files; the paths are stored here.
 */
typedef struct {
    char private_key_pem[PEM_SIZE];   /* unencrypted PKCS#8 PEM */
    char public_key_pem[PEM_SIZE];    /* SubjectPublicKeyInfo PEM */
    char wallet_address[ADDR_SIZE];   /* hex(SHA-256(pubkey DER)) */
} KeyPair;

/* ── Core hash functions ─────────────────────────────────────────────────── */

/*
 * sha256_hex  — hash `data` of `len` bytes; write 64-char hex + NUL into
 *               `out_hex` (caller must provide HASH_SIZE bytes).
 * Returns 0 on success, -1 on OpenSSL error.
 */
int sha256_hex(const uint8_t *data, size_t len, char out_hex[HASH_SIZE]);

/*
 * sha256_bytes — same but writes raw 32-byte digest into `out32`.
 */
int sha256_bytes(const uint8_t *data, size_t len, uint8_t out32[32]);

/* ── Key generation ──────────────────────────────────────────────────────── */

/*
 * crypto_generate_keypair — generate a fresh P-256 key pair.
 * Populates *kp; returns 0 on success, -1 on failure.
 */
int crypto_generate_keypair(KeyPair *kp);

/*
 * crypto_pubkey_to_address — derive the wallet address from a PEM public key.
 * Writes hex(SHA-256(DER)) into out_addr[ADDR_SIZE].
 * Returns 0 on success, -1 on failure.
 */
int crypto_pubkey_to_address(const char *public_key_pem,
                              char out_addr[ADDR_SIZE]);

/* ── Signing and verification ────────────────────────────────────────────── */

/*
 * crypto_sign — sign `data` with the private key in `kp`.
 * Writes DER-encoded signature into sig_buf (must be SIG_MAX_LEN bytes).
 * Sets *sig_len to actual signature length.
 * Returns 0 on success, -1 on failure.
 */
int crypto_sign(const KeyPair *kp,
                const uint8_t *data, size_t data_len,
                uint8_t sig_buf[SIG_MAX_LEN], size_t *sig_len);

/*
 * crypto_verify — verify a DER signature against a PEM public key.
 * Returns 1 if valid, 0 if invalid, -1 on error.
 */
int crypto_verify(const char    *public_key_pem,
                  const uint8_t *data,     size_t data_len,
                  const uint8_t *sig_buf,  size_t sig_len);

/* ── Encoding helpers ────────────────────────────────────────────────────── */

/* bytes_to_hex — encode `len` bytes as lowercase hex; out must hold 2*len+1 */
void bytes_to_hex(const uint8_t *bytes, size_t len, char *out);

/* hex_to_bytes — decode hex string into bytes; returns number of bytes written */
int  hex_to_bytes(const char *hex, uint8_t *out, size_t out_max);

/* ── Key persistence ─────────────────────────────────────────────────────── */

/* Save/load PEM keys to/from files (used during chain_save / chain_load) */
int crypto_save_keypair(const KeyPair *kp,
                        const char *priv_path, const char *pub_path);
int crypto_load_keypair(KeyPair *kp,
                        const char *priv_path, const char *pub_path);

#endif /* CRYPTO_H */
