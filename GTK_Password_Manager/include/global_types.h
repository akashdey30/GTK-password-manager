#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdbool.h>

/* ── App-wide constants ─────────────────────────────── */
#define APP_NAME          "GTK Password Vault"
#define APP_VERSION       "1.0.0"
#define APP_ID            "com.example.gtk_password_vault"

#define DB_FILENAME       "vault.db"
#define LOG_FILENAME      "vault.log"
#define BACKUP_EXT        ".vaultbak"

#define MASTER_SALT_LEN   32
#define AES_KEY_LEN       32   /* 256-bit */
#define AES_IV_LEN        16
#define AES_TAG_LEN       16
#define PBKDF2_ITER       600000

#define AUTO_LOGOUT_SEC   300  /* 5 minutes */
#define CLIPBOARD_CLEAR_SEC 30

/* ── Credential record ──────────────────────────────── */
typedef struct {
    int64_t  id;
    char    *title;
    char    *username;
    char    *password;   /* plaintext – only in memory, never stored raw */
    char    *url;
    char    *notes;
    char    *category;
    int64_t  created_at;
    int64_t  updated_at;
} Credential;

/* ── Session ────────────────────────────────────────── */
typedef struct {
    uint8_t  aes_key[AES_KEY_LEN];
    bool     locked;
    int64_t  last_activity;
} Session;

/* ── App result codes ───────────────────────────────── */
typedef enum {
    APP_OK = 0,
    APP_ERR_GENERIC,
    APP_ERR_AUTH,
    APP_ERR_CRYPTO,
    APP_ERR_DB,
    APP_ERR_IO,
} AppResult;

void credential_free(Credential *c);

#endif /* GLOBAL_TYPES_H */
