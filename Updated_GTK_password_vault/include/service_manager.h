#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "vault_storage.h"
#include "app_state.h"
#include <glib.h>
#include <stdbool.h>

// ---------------- Credential Operations ----------------
void sm_add_credential(const char *service, const char *username, const char *password);
bool sm_delete_credential(const char *service, const char *username);
GList* sm_search_credentials(const char *service, const char *username);

#endif // SERVICE_MANAGER_H
