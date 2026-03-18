#include "service_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ---------------- Add Credential ----------------
void sm_add_credential(const char *service, const char *username, const char *password) {
    Credential c;
    memset(&c, 0, sizeof(Credential));
    strncpy(c.service,  service,  MAX_LEN-1);
    strncpy(c.username, username, MAX_LEN-1);
    strncpy(c.password, password, MAX_LEN-1);

    if (!vs_append_credential(&c)) {
        g_warning("Failed to save credential!");
    }
}

// ---------------- Delete Credential ----------------
bool sm_delete_credential(const char *service, const char *username) {
    GList *list = vs_load_all_credentials();
    if (!list) return false;

    GList *new_list = NULL;
    bool found = false;

    for (GList *l = list; l != NULL; l = l->next) {
        Credential *c = (Credential*)l->data;
        if ((strlen(service) == 0 || strcmp(c->service, service) == 0) &&
            (strlen(username) == 0 || strcmp(c->username, username) == 0)) {
            found = true; // skip (delete) this credential
            g_free(c);
        } else {
            new_list = g_list_append(new_list, c);
        }
    }

    vs_overwrite_credentials(new_list);
    g_list_free(new_list);
    g_list_free(list);

    return found;
}

// ---------------- Search Credentials ----------------
GList* sm_search_credentials(const char *service, const char *username) {
    GList *all = vs_load_all_credentials();
    GList *results = NULL;

    for (GList *l = all; l != NULL; l = l->next) {
        Credential *c = (Credential*)l->data;
        if ((strlen(service) == 0 || strcmp(c->service, service) == 0) &&
            (strlen(username) == 0 || strcmp(c->username, username) == 0)) {
            Credential *match = g_malloc(sizeof(Credential));
            memcpy(match, c, sizeof(Credential));
            results = g_list_append(results, match);
        }
    }

    g_list_free_full(all, g_free);
    return results;
}
