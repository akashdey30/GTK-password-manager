#include "auth/recovery_auth.h"
#include "crypto/crypto.h"
#include "crypto/aes_engine.h"
#include "database/db_init.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>
#include <sqlite3.h>

/* Simple wordlist (first 64 words – production use BIP-39 2048-word list) */
static const char *WORDLIST[] = {
    "apple","brave","cloud","dance","eagle","flame","grace","heart",
    "ivory","jewel","knife","lemon","maple","night","ocean","peace",
    "queen","river","storm","tiger","ultra","vapor","white","xenon",
    "yacht","zebra","amber","blaze","coral","delta","ember","frost",
    "globe","hazel","inlet","joker","karma","lunar","mossy","noble",
    "olive","prism","quest","ridge","spire","trove","unity","vivid",
    "water","xylem","yield","zonal","aloft","brisk","cedar","drift",
    "elder","flint","gavel","hyper","inert","joust","kneel","lusty",
    NULL
};
#define WORDLIST_SIZE 64
#define PHRASE_WORDS  12

char *recovery_generate_phrase(void)
{
    uint8_t rnd[PHRASE_WORDS];
    if (!crypto_random_bytes(rnd, PHRASE_WORDS)) return NULL;

    GString *gs = g_string_new(NULL);
    for (int i = 0; i < PHRASE_WORDS; i++) {
        if (i > 0) g_string_append_c(gs, ' ');
        g_string_append(gs, WORDLIST[rnd[i] % WORDLIST_SIZE]);
    }
    return g_string_free(gs, FALSE);
}

AppResult recovery_store_phrase(const char    *phrase,
                                  const char    *db_path,
                                  const uint8_t  key[AES_KEY_LEN])
{
    char *encrypted = crypto_encrypt_str(key, phrase);
    if (!encrypted) return APP_ERR_CRYPTO;

    sqlite3 *db = NULL;
    AppResult r = db_open(db_path, &db);
    if (r != APP_OK) { g_free(encrypted); return r; }

    const char *sql = "INSERT OR REPLACE INTO meta(key, value) VALUES('recovery_phrase', ?);";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, encrypted, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_close(db);
    g_free(encrypted);
    return APP_OK;
}

AppResult recovery_verify_phrase(const char *phrase,
                                   const char *db_path,
                                   uint8_t     new_key[AES_KEY_LEN])
{
    sqlite3 *db = NULL;
    AppResult r = db_open(db_path, &db);
    if (r != APP_OK) return r;

    const char *sql = "SELECT value FROM meta WHERE key='recovery_phrase';";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    char *stored = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        stored = g_strdup((const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    db_close(db);

    if (!stored) return APP_ERR_AUTH;

    /* The phrase IS the key derivation input */
    uint8_t salt[MASTER_SALT_LEN] = {0};  /* deterministic from phrase */
    uint8_t candidate_key[AES_KEY_LEN];
    r = aes_derive_key(phrase, salt, candidate_key);
    if (r != APP_OK) { g_free(stored); return r; }

    /* Decrypt stored phrase with candidate key to verify */
    char *decrypted = crypto_decrypt_str(candidate_key, stored);
    g_free(stored);

    bool ok = decrypted && (strcmp(decrypted, phrase) == 0);
    if (decrypted) {
        aes_wipe(decrypted, strlen(decrypted));
        g_free(decrypted);
    }

    if (!ok) { aes_wipe(candidate_key, AES_KEY_LEN); return APP_ERR_AUTH; }

    memcpy(new_key, candidate_key, AES_KEY_LEN);
    aes_wipe(candidate_key, AES_KEY_LEN);
    return APP_OK;
}
