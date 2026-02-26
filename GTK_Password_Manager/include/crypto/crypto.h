#ifndef CRYPTO_H
#define CRYPTO_H

#include "global_types.h"
#include <stddef.h>

/* High-level helpers used by the rest of the app */

/* Encrypt a UTF-8 string; returns base64-encoded blob. Caller g_free(). */
char     *crypto_encrypt_str(const uint8_t *key, const char *plain);

/* Decrypt a base64-encoded blob; returns UTF-8 string. Caller g_free(). */
char     *crypto_decrypt_str(const uint8_t *key, const char *b64blob);

/* Base64 encode/decode helpers */
char     *crypto_base64_encode(const uint8_t *data, size_t len);
uint8_t  *crypto_base64_decode(const char *b64, size_t *out_len);

/* Cryptographically random bytes */
bool      crypto_random_bytes(uint8_t *buf, size_t len);

#endif
