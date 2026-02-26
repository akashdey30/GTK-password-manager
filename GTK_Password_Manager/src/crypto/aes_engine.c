#include "crypto/aes_engine.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <glib.h>
#include <string.h>

/* ── helpers ─────────────────────────────────────────── */
void aes_wipe(void *buf, size_t len)
{
    OPENSSL_cleanse(buf, len);
}

AppResult aes_derive_key(const char    *password,
                          const uint8_t *salt,
                          uint8_t        key_out[AES_KEY_LEN])
{
    if (!PKCS5_PBKDF2_HMAC(password, (int)strlen(password),
                             salt, MASTER_SALT_LEN,
                             PBKDF2_ITER, EVP_sha256(),
                             AES_KEY_LEN, key_out)) {
        LOG_E("PBKDF2 failed");
        return APP_ERR_CRYPTO;
    }
    return APP_OK;
}

/* ── AES-256-GCM encrypt ─────────────────────────────── */
AppResult aes_encrypt(const uint8_t *key,
                       const uint8_t *plain,
                       size_t         plain_len,
                       uint8_t      **out,
                       size_t        *out_len)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return APP_ERR_CRYPTO;

    uint8_t iv[AES_IV_LEN];
    if (RAND_bytes(iv, AES_IV_LEN) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return APP_ERR_CRYPTO;
    }

    *out_len = AES_IV_LEN + AES_TAG_LEN + plain_len;
    *out     = g_malloc(*out_len);
    uint8_t *iv_ptr  = *out;
    uint8_t *tag_ptr = *out + AES_IV_LEN;
    uint8_t *ct_ptr  = *out + AES_IV_LEN + AES_TAG_LEN;

    memcpy(iv_ptr, iv, AES_IV_LEN);

    int len = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_LEN, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    EVP_EncryptUpdate(ctx, ct_ptr, &len, plain, (int)plain_len);
    int final_len = 0;
    EVP_EncryptFinal_ex(ctx, ct_ptr + len, &final_len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, tag_ptr);
    EVP_CIPHER_CTX_free(ctx);

    return APP_OK;
}

/* ── AES-256-GCM decrypt ─────────────────────────────── */
AppResult aes_decrypt(const uint8_t *key,
                       const uint8_t *blob,
                       size_t         blob_len,
                       uint8_t      **out,
                       size_t        *out_len)
{
    if (blob_len < AES_IV_LEN + AES_TAG_LEN) return APP_ERR_CRYPTO;

    const uint8_t *iv  = blob;
    const uint8_t *tag = blob + AES_IV_LEN;
    const uint8_t *ct  = blob + AES_IV_LEN + AES_TAG_LEN;
    size_t ct_len      = blob_len - AES_IV_LEN - AES_TAG_LEN;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return APP_ERR_CRYPTO;

    *out     = g_malloc(ct_len + 1);
    *out_len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_LEN, NULL);
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);

    int len = 0;
    EVP_DecryptUpdate(ctx, *out, &len, ct, (int)ct_len);
    *out_len = (size_t)len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, (void*)tag);
    int ret = EVP_DecryptFinal_ex(ctx, (uint8_t*)*out + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        aes_wipe(*out, ct_len);
        g_free(*out);
        *out = NULL;
        return APP_ERR_CRYPTO;
    }
    *out_len += (size_t)len;
    (*out)[*out_len] = '\0';
    return APP_OK;
}
