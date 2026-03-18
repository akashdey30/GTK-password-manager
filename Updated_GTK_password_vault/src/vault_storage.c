#include "vault_storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---------------- Load All Credentials ----------------
GList* vs_load_all_credentials(void) {
    GList *list = NULL;
    FILE *f = fopen(VAULT_FILE, "rb");
    if (!f) return list;

    Credential temp;
    while (fread(&temp, sizeof(Credential), 1, f) == 1) {
        xor_buffer((unsigned char*)&temp, sizeof(Credential));
        Credential *c = g_malloc(sizeof(Credential));
        memcpy(c, &temp, sizeof(Credential));
        list = g_list_append(list, c);
    }

    fclose(f);
    return list;
}

// ---------------- Append Credential ----------------
bool vs_append_credential(const Credential *c) {
    FILE *f = fopen(VAULT_FILE, "ab");
    if (!f) return false;

    Credential temp;
    memcpy(&temp, c, sizeof(Credential));
    xor_buffer((unsigned char*)&temp, sizeof(Credential));

    if (fwrite(&temp, sizeof(Credential), 1, f) != 1) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

// ---------------- Overwrite Vault ----------------
bool vs_overwrite_credentials(GList *list) {
    FILE *f = fopen(VAULT_FILE, "wb");
    if (!f) return false;

    for (GList *l = list; l != NULL; l = l->next) {
        Credential *c = (Credential*)l->data;
        Credential temp;
        memcpy(&temp, c, sizeof(Credential));
        xor_buffer((unsigned char*)&temp, sizeof(Credential));
        if (fwrite(&temp, sizeof(Credential), 1, f) != 1) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

// ---------------- Master Password ----------------
bool vs_save_master(const char *master) {
    FILE *f = fopen(MASTER_FILE, "wb");
    if (!f) return false;

    char buf[MAX_LEN] = {0};
    strncpy(buf, master, MAX_LEN-1);
    xor_buffer((unsigned char*)buf, sizeof(buf));

    if (fwrite(buf, 1, sizeof(buf), f) != sizeof(buf)) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

bool vs_verify_master(const char *input) {
    FILE *f = fopen(MASTER_FILE, "rb");
    if (!f) return false;

    char buf[MAX_LEN];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf)) {
        fclose(f);
        return false;
    }
    fclose(f);

    xor_buffer((unsigned char*)buf, sizeof(buf));
    buf[MAX_LEN-1] = '\0';
    return strcmp(buf, input) == 0;
}

bool vs_master_exists(void) {
    FILE *f = fopen(MASTER_FILE, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}
