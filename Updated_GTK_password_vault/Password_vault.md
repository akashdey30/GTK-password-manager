================================================================================
  PROJECT B — Password_vault
  Complete Directory Structure + All File Contents (unmodified)
================================================================================

--------------------------------------------------------------------------------
DIRECTORY STRUCTURE
--------------------------------------------------------------------------------

Password_vault/
│
├── data/
│   └── vault.dat
│
├── include/
│   ├── app_state.h
│   ├── dialogs.h
│   ├── encryption.h
│   ├── master_auth.h
│   ├── service_manager.h
│   ├── ui.h
│   └── vault_storage.h
│
└── src/
    ├── dialogs.c
    ├── encryption.c
    ├── main.c
    ├── master_auth.c
    ├── service_manager.c
    ├── ui.c
    └── vault_storage.c

================================================================================
DATA FILES
================================================================================

--------------------------------------------------------------------------------
FILE: data/vault.dat
--------------------------------------------------------------------------------
(empty file — 0 bytes)

================================================================================
HEADER FILES  (include/)
================================================================================

--------------------------------------------------------------------------------
FILE: include/app_state.h
--------------------------------------------------------------------------------
#ifndef APP_STATE_H
#define APP_STATE_H

#include <gtk/gtk.h>
#include <glib.h>
#include "vault_storage.h"

#define MAX_LEN 64

typedef struct {
    GtkWidget *window;
    GtkWidget *vbox_services;
    GtkWidget *label_count;
    char session_master[MAX_LEN];
} AppState;

#endif // APP_STATE_H

--------------------------------------------------------------------------------
FILE: include/dialogs.h
--------------------------------------------------------------------------------
#ifndef DIALOGS_H
#define DIALOGS_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "app_state.h"

// ------------------- Master Password Dialogs -------------------

// Prompt user to enter a new master password (first-time setup or change)
// Returns true if user entered and confirmed password, false if cancelled
bool dl_prompt_new_master(GtkWindow *parent, char out_pass[]);

// Prompt user to verify master password
// Returns true if user entered a password (caller checks validity)
bool dl_prompt_verify_master(GtkWindow *parent, char out_pass[]);

// ------------------- Credential Dialogs -------------------

// Add Credential dialog
void dl_add_credential_dialog(GtkButton *button, gpointer data);

// Search Credential dialog
void dl_search_credential_dialog(GtkButton *button, gpointer data);

// Delete Credential dialog
void dl_delete_credential_dialog(GtkButton *button, gpointer data);

#endif // DIALOGS_H

--------------------------------------------------------------------------------
FILE: include/encryption.h
--------------------------------------------------------------------------------
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stddef.h>

void xor_buffer(unsigned char *buf, size_t len);

#endif // ENCRYPTION_H

--------------------------------------------------------------------------------
FILE: include/master_auth.h
--------------------------------------------------------------------------------
#ifndef MASTER_AUTH_H
#define MASTER_AUTH_H

#include "app_state.h"
#include <gtk/gtk.h>
#include <stdbool.h>

// Prompts the user for the master password at startup
bool ma_prompt_master_password(GtkWindow *parent, char session_master[]);

// Change the master password
void ma_change_master_password(GtkButton *button, gpointer data);

// Check if master password file exists
bool ma_master_exists(void);

#endif // MASTER_AUTH_H

--------------------------------------------------------------------------------
FILE: include/service_manager.h
--------------------------------------------------------------------------------
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

--------------------------------------------------------------------------------
FILE: include/ui.h
--------------------------------------------------------------------------------
#ifndef UI_H
#define UI_H

#include "app_state.h"

// Create the main application window and widgets
AppState* ui_create_app(void);

// Refresh service buttons in the main window
void ui_refresh_services(AppState *app_state);

#endif // UI_H

--------------------------------------------------------------------------------
FILE: include/vault_storage.h
--------------------------------------------------------------------------------
#ifndef VAULT_STORAGE_H
#define VAULT_STORAGE_H

#include "app_state.h"
#include "encryption.h"
#include <glib.h>
#include <stdbool.h>

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

================================================================================
SOURCE FILES  (src/)
================================================================================

--------------------------------------------------------------------------------
FILE: src/dialogs.c
--------------------------------------------------------------------------------
#include "dialogs.h"
#include "service_manager.h"
#include "app_state.h"
#include <string.h>
#include <stdlib.h>

// ------------------- Prompt New Master Password -------------------
bool dl_prompt_new_master(GtkWindow *parent, char out_pass[]) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter Master Password",
                                                    parent,
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry1 = gtk_entry_new();
    GtkWidget *entry2 = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry1), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(entry2), FALSE);

    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("Enter new master password:"));
    gtk_container_add(GTK_CONTAINER(content), entry1);
    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("Confirm master password:"));
    gtk_container_add(GTK_CONTAINER(content), entry2);
    gtk_widget_show_all(dialog);

    bool ok = false;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *a = gtk_entry_get_text(GTK_ENTRY(entry1));
        const char *b = gtk_entry_get_text(GTK_ENTRY(entry2));
        if (strlen(a) > 0 && strcmp(a, b) == 0) {
            strncpy(out_pass, a, MAX_LEN-1);
            ok = true;
        }
    }

    gtk_widget_destroy(dialog);
    return ok;
}

// ------------------- Prompt Verify Master Password -------------------
bool dl_prompt_verify_master(GtkWindow *parent, char out_pass[]) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter Master Password",
                                                    parent,
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("Master password:"));
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);

    bool ok = false;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *a = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strlen(a) > 0) {
            strncpy(out_pass, a, MAX_LEN-1);
            ok = true;
        }
    }

    gtk_widget_destroy(dialog);
    return ok;
}

// ------------------- Add Credential Dialog -------------------
void dl_add_credential_dialog(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Credential",
                                                    GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Add", GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

    GtkWidget *entry_service = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();
    GtkWidget *entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_password), FALSE);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Password:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_password, 1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
        const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));

        if (strlen(service) == 0 || strlen(username) == 0) {
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK,
                                                 "Service and Username cannot be empty.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        } else {
            sm_add_credential(service, username, password);
        }
    }

    gtk_widget_destroy(dialog);
}

// ------------------- Search Credential Dialog -------------------
void dl_search_credential_dialog(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Search Credential",
                                                    GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Search", GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

    GtkWidget *entry_service = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));

        GList *results = sm_search_credentials(service, username);
        GString *msg = g_string_new("");

        for (GList *l = results; l != NULL; l = l->next) {
            Credential *c = (Credential *)l->data;
            g_string_append_printf(msg, "Service: %s\nUsername: %s\nPassword: %s\n\n",
                                   c->service, c->username, c->password);
        }
        g_list_free_full(results, g_free);

        if (msg->len == 0) g_string_assign(msg, "No matching credentials found.");

        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s", msg->str);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_string_free(msg, TRUE);
    }

    gtk_widget_destroy(dialog);
}

// ------------------- Delete Credential Dialog -------------------
void dl_delete_credential_dialog(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Delete Credential",
                                                    GTK_WINDOW(state->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Delete", GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

    GtkWidget *entry_service = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));

        if (!sm_delete_credential(service, username)) {
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO,
                                                 GTK_BUTTONS_OK,
                                                 "No matching credential found.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        }
    }

    gtk_widget_destroy(dialog);
}

--------------------------------------------------------------------------------
FILE: src/encryption.c
--------------------------------------------------------------------------------
#include "encryption.h"

#define XOR_KEY 5

void xor_buffer(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] ^= XOR_KEY;
    }
}

--------------------------------------------------------------------------------
FILE: src/main.c
--------------------------------------------------------------------------------
#include <gtk/gtk.h>
#include "ui.h"
#include "dialogs.h"
#include "vault_storage.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create AppState and UI
    AppState *app = ui_create_app();

    // Check if master exists; prompt setup or verify
    if (!vs_master_exists()) {
        if (!dialog_prompt_new_master(GTK_WINDOW(app->window), app)) {
            g_print("No master password entered. Exiting.\n");
            return 0;
        }
        vs_save_master(app->session_master);
    } else {
        if (!dialog_prompt_verify_master(GTK_WINDOW(app->window), app)) {
            g_print("Master password verification failed. Exiting.\n");
            return 0;
        }
    }

    // Refresh services and show main window
    ui_refresh_services(app);
    gtk_widget_show_all(app->window);
    gtk_main();

    g_free(app);
    return 0;
}

--------------------------------------------------------------------------------
FILE: src/master_auth.c
--------------------------------------------------------------------------------
#include "master_auth.h"
#include "vault_storage.h"
#include "dialogs.h"    // for optional GTK dialogs
#include "encryption.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ------------------- Check if master exists -------------------
bool ma_master_exists(void) {
    FILE *f = fopen(MASTER_FILE, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

// ------------------- Prompt for master password -------------------
bool ma_prompt_master_password(GtkWindow *parent, char session_master[]) {
    if (!ma_master_exists()) {
        // No master password exists, prompt user to create one
        const char *msg1 = "No master password found. Please create a new one.";
        GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                   "%s", msg1);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        char new_pass[MAX_LEN] = {0};
        if (!dl_prompt_new_master(parent, new_pass)) {
            return false; // user cancelled
        }

        if (!vs_save_master(new_pass)) {
            GtkWidget *err = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Failed to save master password.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            return false;
        }

        strncpy(session_master, new_pass, MAX_LEN-1);
        return true;
    } else {
        // Master exists → prompt for verification
        char input_pass[MAX_LEN] = {0};
        if (!dl_prompt_verify_master(parent, input_pass)) {
            return false;
        }

        if (!vs_verify_master(input_pass)) {
            GtkWidget *err = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Incorrect master password.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            return false;
        }

        strncpy(session_master, input_pass, MAX_LEN-1);
        return true;
    }
}

// ------------------- Change master password -------------------
void ma_change_master_password(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;

    char new_pass[MAX_LEN] = {0};
    if (!dl_prompt_new_master(GTK_WINDOW(state->window), new_pass)) {
        return; // user cancelled
    }

    if (!vs_save_master(new_pass)) {
        GtkWidget *err = gtk_message_dialog_new(GTK_WINDOW(state->window), GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                "Failed to save new master password.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    // Update session
    strncpy(state->session_master, new_pass, MAX_LEN-1);

    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(state->window), GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                            "Master password changed successfully.");
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}

--------------------------------------------------------------------------------
FILE: src/service_manager.c
--------------------------------------------------------------------------------
#include "service_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ---------------- Add Credential ----------------
void sm_add_credential(const char *service, const char *username, const char *password) {
    Credential c;
    memset(&c, 0, sizeof(Credential));
    strncpy(c.service, service, MAX_LEN-1);
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
            found = true; // skip this credential
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

    // Free original list, but not the credentials in results
    g_list_free_full(all, g_free);
    return results;
}

--------------------------------------------------------------------------------
FILE: src/ui.c
--------------------------------------------------------------------------------
#include "ui.h"
#include "dialogs.h"
#include "service_manager.h"
#include <glib.h>
#include <stdlib.h>

AppState* ui_create_app(void) {
    AppState *app = g_new0(AppState, 1);

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "GTK Password Vault");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 500, 700);
    gtk_window_set_resizable(GTK_WINDOW(app->window), TRUE);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 15);
    gtk_container_add(GTK_CONTAINER(app->window), main_vbox);

    GtkWidget *label_header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_header), "<b>Password Vault</b>");
    gtk_widget_set_halign(label_header, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), label_header, FALSE, FALSE, 5);

    GtkWidget *btn_change_master = gtk_button_new_with_label("Change Master Password");
    gtk_widget_set_size_request(btn_change_master, 200, 50);
    gtk_widget_set_halign(btn_change_master, GTK_ALIGN_CENTER);
    g_signal_connect(btn_change_master, "clicked", G_CALLBACK(dialog_change_master_password), app);
    gtk_box_pack_start(GTK_BOX(main_vbox), btn_change_master, FALSE, FALSE, 5);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 5);

    GtkWidget *btn_add = gtk_button_new_with_label("Add Credential");
    gtk_widget_set_size_request(btn_add, 120, 40);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(dialog_add_credential), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_add, FALSE, FALSE, 0);

    GtkWidget *btn_search = gtk_button_new_with_label("Search");
    gtk_widget_set_size_request(btn_search, 120, 40);
    g_signal_connect(btn_search, "clicked", G_CALLBACK(dialog_search_credential), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_search, FALSE, FALSE, 0);

    GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
    gtk_widget_set_size_request(btn_delete, 120, 40);
    g_signal_connect(btn_delete, "clicked", G_CALLBACK(dialog_delete_credential), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_delete, FALSE, FALSE, 0);

    app->label_count = gtk_label_new("<b>Services saved: 0</b>");
    gtk_widget_set_halign(app->label_count, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), app->label_count, FALSE, FALSE, 5);

    GtkWidget *frame_services = gtk_frame_new(NULL);
    GtkWidget *frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), "<b>Saved Services</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame_services), frame_label);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame_services, TRUE, TRUE, 10);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame_services), scrolled);

    app->vbox_services = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(scrolled), app->vbox_services);

    return app;
}

void ui_refresh_services(AppState *app) {
    // Clear existing service buttons
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->vbox_services));
    for (GList *c = children; c != NULL; c = c->next)
        gtk_widget_destroy(GTK_WIDGET(c->data));
    g_list_free(children);

    GList *all = vs_load_all_credentials();
    if (!all) {
        gtk_label_set_text(GTK_LABEL(app->label_count), "Services saved: 0");
        gtk_widget_show_all(app->vbox_services);
        return;
    }

    // Unique service names
    GHashTable *services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (GList *l = all; l != NULL; l = l->next) {
        Credential *c = (Credential*)l->data;
        if (c->service[0] != '\0') g_hash_table_add(services, g_strdup(c->service));
    }

    guint count = g_hash_table_size(services);
    gchar count_text[64];
    snprintf(count_text, sizeof(count_text), "Services saved: %u", count);
    gtk_label_set_text(GTK_LABEL(app->label_count), count_text);

    // Create buttons
    GHashTableIter iter;
    gpointer key;
    g_hash_table_iter_init(&iter, services);
    while (g_hash_table_iter_next(&iter, &key, NULL)) {
        const char *svc = (const char*)key;
        GtkWidget *btn = gtk_button_new();
        GtkWidget *lbl = gtk_label_new(NULL);
        gchar *markup = g_strdup_printf("<b>%s</b>", svc);
        gtk_label_set_markup(GTK_LABEL(lbl), markup);
        g_free(markup);
        gtk_container_add(GTK_CONTAINER(btn), lbl);
        gtk_widget_set_size_request(btn, 180, 60);
        g_object_set_data_full(G_OBJECT(btn), "service_name", g_strdup(svc), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(dialog_show_service_credentials), app);
        gtk_box_pack_start(GTK_BOX(app->vbox_services), btn, FALSE, FALSE, 0);
    }

    g_hash_table_destroy(services);
    g_list_free_full(all, g_free);
    gtk_widget_show_all(app->vbox_services);
}

--------------------------------------------------------------------------------
FILE: src/vault_storage.c
--------------------------------------------------------------------------------
#include "vault_storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VAULT_FILE "data/vault.dat"
#define MASTER_FILE "data/master.dat"
#define MAX_LEN 64

// ---------------- XOR encryption wrapper ----------------
static void xor_buffer(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) buf[i] ^= XOR_KEY;
}

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

================================================================================
END OF PROJECT B
================================================================================
