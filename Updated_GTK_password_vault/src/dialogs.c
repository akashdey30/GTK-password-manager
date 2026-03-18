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

    GtkWidget *entry_service  = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();
    GtkWidget *entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_password), FALSE);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service:"),  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service,              1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username,             1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Password:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_password,             1, 2, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service  = gtk_entry_get_text(GTK_ENTRY(entry_service));
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
            // Refresh the services panel in the main window
            extern void ui_refresh_services(AppState *app);
            ui_refresh_services(state);
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

    GtkWidget *entry_service  = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service (blank = any):"),  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service,                            1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username (blank = any):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username,                           1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service  = gtk_entry_get_text(GTK_ENTRY(entry_service));
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

    GtkWidget *entry_service  = gtk_entry_new();
    GtkWidget *entry_username = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Service (blank = any):"),  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_service,                            1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username (blank = any):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username,                           1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *service  = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));

        if (!sm_delete_credential(service, username)) {
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "No matching credential found.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        } else {
            extern void ui_refresh_services(AppState *app);
            ui_refresh_services(state);
        }
    }

    gtk_widget_destroy(dialog);
}

// ------------------- Show Service Credentials Dialog -------------------
// Triggered when a service button is clicked in the main window.
void dl_show_service_credentials(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;
    const char *service = (const char *)g_object_get_data(G_OBJECT(button), "service_name");
    if (!service) return;

    GList *results = sm_search_credentials(service, "");
    GString *msg = g_string_new("");

    for (GList *l = results; l != NULL; l = l->next) {
        Credential *c = (Credential *)l->data;
        g_string_append_printf(msg, "Username: %s\nPassword: %s\n\n",
                               c->username, c->password);
    }
    g_list_free_full(results, g_free);

    if (msg->len == 0) g_string_assign(msg, "No credentials found for this service.");

    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                          GTK_DIALOG_MODAL,
                                          GTK_MESSAGE_INFO,
                                          GTK_BUTTONS_OK,
                                          "%s", msg->str);
    gtk_window_set_title(GTK_WINDOW(d), service);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    g_string_free(msg, TRUE);
}
