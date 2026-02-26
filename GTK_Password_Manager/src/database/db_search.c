#include "database/db_search.h"
#include "database/db_credentials.h"
#include "crypto/crypto.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

/*
 * Because fields are encrypted at rest we do a full table scan,
 * decrypt each row, then filter in memory.
 * For large vaults a local FTS index over decrypted data could be added.
 */

static bool field_match(const char *field, const char *ql)
{
    if (!field) return false;
    char *fl = g_utf8_strdown(field, -1);
    bool m = strstr(fl, ql) != NULL;
    g_free(fl);
    return m;
}

static bool cred_matches(const Credential *c, const char *q)
{
    if (!q || !*q) return true;
    char *ql = g_utf8_strdown(q, -1);
    bool match = field_match(c->title, ql) ||
                 field_match(c->username, ql) ||
                 field_match(c->url, ql) ||
                 field_match(c->notes, ql);
    g_free(ql);
    return match;
}

Credential **db_search(sqlite3 *db, const uint8_t *key,
                        const char *query, int *count_out)
{
    int total = 0;
    Credential **all = db_cred_list_all(db, key, &total);

    GPtrArray *matches = g_ptr_array_new();
    for (int i = 0; i < total; i++) {
        if (cred_matches(all[i], query)) {
            g_ptr_array_add(matches, all[i]);
            all[i] = NULL;  /* transferred */
        }
    }
    /* free non-matched rows */
    for (int i = 0; i < total; i++) {
        if (all[i]) credential_free(all[i]);
    }
    g_free(all);

    g_ptr_array_add(matches, NULL);
    *count_out = (int)matches->len - 1;
    return (Credential**)g_ptr_array_free(matches, FALSE);
}

Credential **db_filter_category(sqlite3 *db, const uint8_t *key,
                                  const char *category, int *count_out)
{
    int total = 0;
    Credential **all = db_cred_list_all(db, key, &total);

    GPtrArray *matches = g_ptr_array_new();
    for (int i = 0; i < total; i++) {
        bool hit = !category || !*category ||
                   g_strcmp0(all[i]->category, category) == 0;
        if (hit) {
            g_ptr_array_add(matches, all[i]);
            all[i] = NULL;
        }
    }
    for (int i = 0; i < total; i++)
        if (all[i]) credential_free(all[i]);
    g_free(all);

    g_ptr_array_add(matches, NULL);
    *count_out = (int)matches->len - 1;
    return (Credential**)g_ptr_array_free(matches, FALSE);
}
