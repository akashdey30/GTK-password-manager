#include "database/db_init.h"
#include "crypto/crypto.h"
#include "utils/logger.h"
#include <string.h>
#include <glib.h>

AppResult db_open(const char *path, sqlite3 **db_out)
{
    if (sqlite3_open(path, db_out) != SQLITE_OK) {
        LOG_E("Cannot open DB: %s", sqlite3_errmsg(*db_out));
        return APP_ERR_DB;
    }
    sqlite3_exec(*db_out, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(*db_out, "PRAGMA foreign_keys=ON;",  NULL, NULL, NULL);
    return APP_OK;
}

AppResult db_init_schema(sqlite3 *db)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS meta ("
        "  key   TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS credentials ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title       TEXT NOT NULL,"
        "  username    TEXT,"
        "  password    TEXT NOT NULL,"
        "  url         TEXT,"
        "  notes       TEXT,"
        "  category    TEXT,"
        "  created_at  INTEGER NOT NULL,"
        "  updated_at  INTEGER NOT NULL"
        ");";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        LOG_E("Schema init failed: %s", err);
        sqlite3_free(err);
        return APP_ERR_DB;
    }
    return APP_OK;
}

void db_close(sqlite3 *db)
{
    if (db) sqlite3_close(db);
}

AppResult db_store_master(sqlite3       *db,
                           const uint8_t *salt,
                           const char    *verifier_b64)
{
    char *salt_b64 = crypto_base64_encode(salt, MASTER_SALT_LEN);

    const char *sql_salt =
        "INSERT OR REPLACE INTO meta(key, value) VALUES('master_salt', ?);";
    const char *sql_ver =
        "INSERT OR REPLACE INTO meta(key, value) VALUES('master_verifier', ?);";

    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql_salt, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, salt_b64, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, sql_ver, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, verifier_b64, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    g_free(salt_b64);
    return APP_OK;
}

AppResult db_get_master(sqlite3   *db,
                         uint8_t   *salt_out,
                         char     **verifier_b64_out)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT value FROM meta WHERE key=?;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, "master_salt", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); return APP_ERR_DB; }

    size_t salt_len = 0;
    uint8_t *salt = crypto_base64_decode(
        (const char*)sqlite3_column_text(stmt, 0), &salt_len);
    sqlite3_finalize(stmt);
    if (!salt || salt_len != MASTER_SALT_LEN) { g_free(salt); return APP_ERR_DB; }
    memcpy(salt_out, salt, MASTER_SALT_LEN);
    g_free(salt);

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, "master_verifier", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); return APP_ERR_DB; }
    *verifier_b64_out = g_strdup((const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return APP_OK;
}

bool db_is_initialized(sqlite3 *db)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT 1 FROM meta WHERE key='master_salt' LIMIT 1;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}
