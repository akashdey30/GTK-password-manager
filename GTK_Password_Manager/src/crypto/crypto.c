#include "crypto/crypto.h"
#include "crypto/aes_engine.h"
#include "utils/logger.h"
#include <glib.h>
#include <openssl/rand.h>
#include <string.h>

char *crypto_encrypt_str(const uint8_t *key, const char *plain)
{
    if (!plain) return NULL;
    uint8_t *blob = NULL;
    size_t   blob_len = 0;
    if (aes_encrypt(key, (const uint8_t*)plain, strlen(plain),
                    &blob, &blob_len) != APP_OK)
        return NULL;
    char *b64 = crypto_base64_encode(blob, blob_len);
    aes_wipe(blob, blob_len);
    g_free(blob);
    return b64;
}

char *crypto_decrypt_str(const uint8_t *key, const char *b64blob)
{
    if (!b64blob) return NULL;
    size_t   blob_len = 0;
    uint8_t *blob = crypto_base64_decode(b64blob, &blob_len);
    if (!blob) return NULL;
    uint8_t *plain = NULL;
    size_t   plain_len = 0;
    AppResult r = aes_decrypt(key, blob, blob_len, &plain, &plain_len);
    g_free(blob);
    if (r != APP_OK) return NULL;
    return (char*)plain;
}

char *crypto_base64_encode(const uint8_t *data, size_t len)
{
    return g_base64_encode(data, len);
}

uint8_t *crypto_base64_decode(const char *b64, size_t *out_len)
{
    gsize sz = 0;
    guchar *d = g_base64_decode(b64, &sz);
    *out_len = sz;
    return (uint8_t*)d;
}

bool crypto_random_bytes(uint8_t *buf, size_t len)
{
    return RAND_bytes(buf, (int)len) == 1;
}
