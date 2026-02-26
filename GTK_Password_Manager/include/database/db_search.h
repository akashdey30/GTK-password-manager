#ifndef DB_SEARCH_H
#define DB_SEARCH_H

#include "global_types.h"
#include <sqlite3.h>

/*
 * Full-text search across title, username, url.
 * Returns NULL-terminated array; free with db_cred_list_free().
 */
Credential **db_search(sqlite3       *db,
                        const uint8_t *key,
                        const char    *query,
                        int           *count_out);

/* Filter by category */
Credential **db_filter_category(sqlite3       *db,
                                  const uint8_t *key,
                                  const char    *category,
                                  int           *count_out);

#endif
