#ifndef AES_ENGINE_H
#define AES_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include "global_types.h"

/*
 * AES-256-GCM authenticated encryption.
 *
 * Encrypted blob layout:
 *   [IV 16 bytes][TAG 16 bytes][ciphertext]
 *
 * Returns APP_OK on success; out_len set to total blob length.
 * Caller must g_free() *out.
 */
AppResult aes_encrypt(const uint8_t *key,      /* AES_KEY_LEN */
                       const uint8_t *plain,
                       size_t         plain_len,
                       uint8_t      **out,
                       size_t        *out_len);

AppResult aes_decrypt(const uint8_t *key,
                       const uint8_t *blob,
                       size_t         blob_len,
                       uint8_t      **out,
                       size_t        *out_len);

/* Derive a 256-bit key from password + salt using PBKDF2-HMAC-SHA256 */
AppResult aes_derive_key(const char    *password,
                          const uint8_t *salt,   /* MASTER_SALT_LEN */
                          uint8_t        key_out[AES_KEY_LEN]);

/* Secure memory wipe */
void aes_wipe(void *buf, size_t len);

#endif
