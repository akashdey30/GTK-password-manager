#ifndef DB_INIT_H
#define DB_INIT_H

#include "global_types.h"
#include <sqlite3.h>

AppResult db_open(const char *path, sqlite3 **db_out);
AppResult db_init_schema(sqlite3 *db);
void      db_close(sqlite3 *db);

/* Store / retrieve master salt + auth verifier */
AppResult db_store_master(sqlite3       *db,
                           const uint8_t *salt,
                           const char    *verifier_b64);

AppResult db_get_master(sqlite3   *db,
                         uint8_t   *salt_out,   /* MASTER_SALT_LEN */
                         char     **verifier_b64_out);

bool      db_is_initialized(sqlite3 *db);

#endif
