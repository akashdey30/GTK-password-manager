#include "auth/master_auth.h"
#include "crypto/aes_engine.h"
#include "crypto/crypto.h"
#include "database/db_init.h"
#include "utils/logger.h"
#include <sqlite3.h>
#include <string.h>
#include <glib.h>

/* We store a verifier: AES-encrypt a known constant with the derived key.
   On verify we decrypt and compare. */
#define VERIFIER_PLAINTEXT  "GTK_VAULT_OK"

AppResult master_auth_setup(const char *password,
                             uint8_t     key_out[AES_KEY_LEN])
{
    uint8_t salt[MASTER_SALT_LEN];
    if (!crypto_random_bytes(salt, MASTER_SALT_LEN)) return APP_ERR_CRYPTO;

    if (aes_derive_key(password, salt, key_out) != APP_OK)
        return APP_ERR_CRYPTO;

    return APP_OK;  /* caller stores salt + verifier via db_store_master */
}

AppResult master_auth_verify(const char *password,
                              const char *db_path,
                              uint8_t     key_out[AES_KEY_LEN])
{
    sqlite3 *db = NULL;
    if (db_open(db_path, &db) != APP_OK) return APP_ERR_DB;

    uint8_t salt[MASTER_SALT_LEN];
    char   *verifier_b64 = NULL;
    AppResult r = db_get_master(db, salt, &verifier_b64);
    db_close(db);
    if (r != APP_OK) return r;

    uint8_t key[AES_KEY_LEN];
    r = aes_derive_key(password, salt, key);
    if (r != APP_OK) { g_free(verifier_b64); return r; }

    char *decrypted = crypto_decrypt_str(key, verifier_b64);
    g_free(verifier_b64);

    if (!decrypted) { aes_wipe(key, AES_KEY_LEN); return APP_ERR_AUTH; }

    bool ok = (strcmp(decrypted, VERIFIER_PLAINTEXT) == 0);
    aes_wipe(decrypted, strlen(decrypted));
    g_free(decrypted);

    if (!ok) { aes_wipe(key, AES_KEY_LEN); return APP_ERR_AUTH; }

    memcpy(key_out, key, AES_KEY_LEN);
    aes_wipe(key, AES_KEY_LEN);
    return APP_OK;
}

bool master_auth_is_set(const char *db_path)
{
    sqlite3 *db = NULL;
    if (db_open(db_path, &db) != APP_OK) return false;
    bool set = db_is_initialized(db);
    db_close(db);
    return set;
}

AppResult master_auth_change(const char *old_password,
                              const char *new_password,
                              const char *db_path)
{
    uint8_t old_key[AES_KEY_LEN];
    AppResult r = master_auth_verify(old_password, db_path, old_key);
    if (r != APP_OK) return r;

    uint8_t new_key[AES_KEY_LEN];
    uint8_t salt[MASTER_SALT_LEN];
    if (!crypto_random_bytes(salt, MASTER_SALT_LEN)) return APP_ERR_CRYPTO;
    if (aes_derive_key(new_password, salt, new_key) != APP_OK) return APP_ERR_CRYPTO;

    char *verifier = crypto_encrypt_str(new_key, VERIFIER_PLAINTEXT);
    if (!verifier) return APP_ERR_CRYPTO;

    sqlite3 *db = NULL;
    r = db_open(db_path, &db);
    if (r == APP_OK) {
        r = db_store_master(db, salt, verifier);
        db_close(db);
    }
    g_free(verifier);
    /* TODO: re-encrypt all credentials with new_key */
    aes_wipe(old_key, AES_KEY_LEN);
    aes_wipe(new_key, AES_KEY_LEN);
    return r;
}
