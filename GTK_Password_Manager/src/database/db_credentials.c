#include "database/db_credentials.h"
#include "crypto/crypto.h"
#include "utils/logger.h"
#include <glib.h>
#include <time.h>
#include <string.h>

/* Helper: encrypt field or return empty string */
static char *enc(const uint8_t *key, const char *s)
{
    if (!s || !*s) return g_strdup("");
    char *r = crypto_encrypt_str(key, s);
    return r ? r : g_strdup("");
}

static char *dec(const uint8_t *key, const char *b64)
{
    if (!b64 || !*b64) return g_strdup("");
    char *r = crypto_decrypt_str(key, b64);
    return r ? r : g_strdup("");
}

AppResult db_cred_insert(sqlite3 *db, const uint8_t *key, const Credential *c)
{
    char *e_title    = enc(key, c->title);
    char *e_username = enc(key, c->username);
    char *e_password = enc(key, c->password);
    char *e_url      = enc(key, c->url);
    char *e_notes    = enc(key, c->notes);
    char *e_category = enc(key, c->category);

    int64_t now = (int64_t)time(NULL);

    const char *sql =
        "INSERT INTO credentials(title,username,password,url,notes,category,"
        "created_at,updated_at) VALUES(?,?,?,?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, e_title,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, e_username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, e_password, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, e_url,      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, e_notes,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, e_category, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, now);
    sqlite3_bind_int64(stmt, 8, now);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    g_free(e_title); g_free(e_username); g_free(e_password);
    g_free(e_url); g_free(e_notes); g_free(e_category);

    return (rc == SQLITE_DONE) ? APP_OK : APP_ERR_DB;
}

AppResult db_cred_update(sqlite3 *db, const uint8_t *key, const Credential *c)
{
    char *e_title    = enc(key, c->title);
    char *e_username = enc(key, c->username);
    char *e_password = enc(key, c->password);
    char *e_url      = enc(key, c->url);
    char *e_notes    = enc(key, c->notes);
    char *e_category = enc(key, c->category);
    int64_t now      = (int64_t)time(NULL);

    const char *sql =
        "UPDATE credentials SET title=?,username=?,password=?,url=?,notes=?,"
        "category=?,updated_at=? WHERE id=?;";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, e_title,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, e_username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, e_password, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, e_url,      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, e_notes,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, e_category, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, now);
    sqlite3_bind_int64(stmt, 8, c->id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    g_free(e_title); g_free(e_username); g_free(e_password);
    g_free(e_url); g_free(e_notes); g_free(e_category);

    return (rc == SQLITE_DONE) ? APP_OK : APP_ERR_DB;
}

AppResult db_cred_delete(sqlite3 *db, int64_t id)
{
    const char *sql = "DELETE FROM credentials WHERE id=?;";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? APP_OK : APP_ERR_DB;
}

static Credential *row_to_cred(sqlite3_stmt *stmt, const uint8_t *key)
{
    Credential *c = g_new0(Credential, 1);
    c->id       = sqlite3_column_int64(stmt, 0);
    c->title    = dec(key, (const char*)sqlite3_column_text(stmt, 1));
    c->username = dec(key, (const char*)sqlite3_column_text(stmt, 2));
    c->password = dec(key, (const char*)sqlite3_column_text(stmt, 3));
    c->url      = dec(key, (const char*)sqlite3_column_text(stmt, 4));
    c->notes    = dec(key, (const char*)sqlite3_column_text(stmt, 5));
    c->category = dec(key, (const char*)sqlite3_column_text(stmt, 6));
    c->created_at = sqlite3_column_int64(stmt, 7);
    c->updated_at = sqlite3_column_int64(stmt, 8);
    return c;
}

Credential **db_cred_list_all(sqlite3 *db, const uint8_t *key, int *count_out)
{
    const char *sql =
        "SELECT id,title,username,password,url,notes,category,"
        "created_at,updated_at FROM credentials ORDER BY updated_at DESC;";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    GPtrArray *arr = g_ptr_array_new();
    while (sqlite3_step(stmt) == SQLITE_ROW)
        g_ptr_array_add(arr, row_to_cred(stmt, key));
    sqlite3_finalize(stmt);

    g_ptr_array_add(arr, NULL);
    *count_out = (int)arr->len - 1;
    return (Credential**)g_ptr_array_free(arr, FALSE);
}

Credential *db_cred_get(sqlite3 *db, const uint8_t *key, int64_t id)
{
    const char *sql =
        "SELECT id,title,username,password,url,notes,category,"
        "created_at,updated_at FROM credentials WHERE id=?;";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, id);
    Credential *c = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        c = row_to_cred(stmt, key);
    sqlite3_finalize(stmt);
    return c;
}

void db_cred_list_free(Credential **list)
{
    if (!list) return;
    for (int i = 0; list[i]; i++) credential_free(list[i]);
    g_free(list);
}
