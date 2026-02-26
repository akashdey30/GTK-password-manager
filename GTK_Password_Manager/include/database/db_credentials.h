#ifndef DB_CREDENTIALS_H
#define DB_CREDENTIALS_H

#include "global_types.h"
#include <sqlite3.h>

/* All string fields in Credential are encrypted before storage */

AppResult db_cred_insert(sqlite3          *db,
                          const uint8_t    *key,
                          const Credential *c);

AppResult db_cred_update(sqlite3          *db,
                          const uint8_t    *key,
                          const Credential *c);

AppResult db_cred_delete(sqlite3  *db,
                          int64_t  id);

/* Returns NULL-terminated array; caller frees with db_cred_list_free() */
Credential **db_cred_list_all(sqlite3       *db,
                               const uint8_t *key,
                               int           *count_out);

Credential  *db_cred_get(sqlite3       *db,
                          const uint8_t *key,
                          int64_t        id);

void         db_cred_list_free(Credential **list);

#endif
