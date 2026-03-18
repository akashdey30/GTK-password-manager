#ifndef VAULT_STORAGE_H
#define VAULT_STORAGE_H

#include "credentials.h"
#include "encryption.h"
#include <glib.h>
#include <stdbool.h>

#define VAULT_FILE  "data/vault.dat"
#define MASTER_FILE "data/master.dat"

// ---------------- Credential File Operations ----------------

// Load all credentials from vault file, returns a GList of Credential*
GList* vs_load_all_credentials(void);

// Append a new credential to the vault
bool vs_append_credential(const Credential *c);

// Overwrite the vault with a list of credentials
bool vs_overwrite_credentials(GList *list);

// Master password operations
bool vs_save_master(const char *master);
bool vs_verify_master(const char *input);
bool vs_master_exists(void);

#endif // VAULT_STORAGE_H
