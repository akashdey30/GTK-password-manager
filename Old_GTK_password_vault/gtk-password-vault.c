// compile with:
// gcc pkg-config --cflags gtk+-3.0 vault.c -o vault pkg-config --libs gtk+-3.0

#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAULT_FILE "vault.dat"
#define MASTER_FILE "D:\\master.dat" /* consider changing to a path under user's HOME */
#define XOR_KEY 5
#define MAX_LEN 64

typedef struct {
    char service[MAX_LEN];
    char username[MAX_LEN];
    char password[MAX_LEN];
} Credential;

typedef struct {
    GtkWidget *window;
    GtkWidget *vbox_services;
    GtkWidget *label_count;
} AppWidgets;

int prompt_set_master(GtkWindow *parent);
int prompt_verify_master(GtkWindow *parent);
void refresh_service_buttons(AppWidgets *w);

void xor_buffer(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) buf[i] ^= XOR_KEY;
}

GList* load_all_credentials(void) {
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

int append_credential(const Credential *c) {
    FILE *f = fopen(VAULT_FILE, "ab");
    if (!f) return 0;
    Credential temp;
    memcpy(&temp, c, sizeof(Credential));
    xor_buffer((unsigned char*)&temp, sizeof(Credential));
    if (fwrite(&temp, sizeof(Credential), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

int overwrite_credentials_from_list(GList *list) {
    FILE *f = fopen(VAULT_FILE, "wb");
    if (!f) return 0;
    for (GList *l = list; l != NULL; l = l->next) {
        Credential *c = (Credential *)l->data;
        Credential temp;
        memcpy(&temp, c, sizeof(Credential));
        xor_buffer((unsigned char*)&temp, sizeof(Credential));
        if (fwrite(&temp, sizeof(Credential), 1, f) != 1) { fclose(f); return 0; }
    }
    fclose(f);
    return 1;
}

int save_master(const char *master) {
    FILE *f = fopen(MASTER_FILE, "wb");
    if (!f) return 0;
    char buf[MAX_LEN];
    memset(buf, 0, sizeof(buf));
    strncpy(buf, master, MAX_LEN - 1);
    xor_buffer((unsigned char*)buf, sizeof(buf));
    if (fwrite(buf, 1, sizeof(buf), f) != sizeof(buf)) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

int verify_master(const char *input) {
    FILE *f = fopen(MASTER_FILE, "rb");
    if (!f) return 0;
    char buf[MAX_LEN];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf)) { fclose(f); return 0; }
    fclose(f);
    xor_buffer((unsigned char*)buf, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    return strcmp(buf, input) == 0;
}

int master_exists(void) {
    FILE *f = fopen(MASTER_FILE, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

void show_service_credentials(GtkButton *button, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;
    const char *service_name = (const char *)g_object_get_data(G_OBJECT(button), "service_name");
    if (!service_name) return;

    GList *list = load_all_credentials();
    GString *msg = g_string_new("");
    for (GList *l = list; l != NULL; l = l->next) {
        Credential *c = (Credential *)l->data;
        if (g_strcmp0(c->service, service_name) == 0) {
            g_string_append_printf(msg, "Username: %s\nPassword: %s\n\n", c->username, c->password);
        }
    }
    g_list_free_full(list, g_free);

    if (msg->len == 0) g_string_assign(msg, "No credentials found for this service.");

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", msg->str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_string_free(msg, TRUE);
}

void refresh_service_buttons(AppWidgets *w) {
    /* Clear previous service widgets */
    GList *children = gtk_container_get_children(GTK_CONTAINER(w->vbox_services));
    for (GList *c = children; c != NULL; c = c->next) {
        gtk_widget_destroy(GTK_WIDGET(c->data));
    }
    g_list_free(children);

    GList *list = load_all_credentials();
    if (!list) {
        gtk_label_set_text(GTK_LABEL(w->label_count), "Services saved: 0");
        gtk_widget_show_all(w->vbox_services);
        return;
    }

    /* Build a set of unique service names - duplicate keys so they survive after freeing list */
    GHashTable *services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (GList *l = list; l != NULL; l = l->next) {
        Credential *c = (Credential *)l->data;
        if (c->service[0] != '\0') {
            g_hash_table_add(services, g_strdup(c->service));
        }
    }

    guint count = g_hash_table_size(services);
    gchar count_text[64];
    snprintf(count_text, sizeof(count_text), "Services saved: %u", count);
    gtk_label_set_text(GTK_LABEL(w->label_count), count_text);

    /* Create buttons for each unique service */
    GHashTableIter iter;
    gpointer key;
    g_hash_table_iter_init(&iter, services);
    while (g_hash_table_iter_next(&iter, &key, NULL)) {
        const char *svc = (const char *)key;
        GtkWidget *btn = gtk_button_new();
        GtkWidget *lbl = gtk_label_new(NULL);

        gchar *markup = g_strdup_printf("<b>%s</b>", svc);
        gtk_label_set_markup(GTK_LABEL(lbl), markup);
        g_free(markup);

        gtk_container_add(GTK_CONTAINER(btn), lbl);
        gtk_widget_set_size_request(btn, 180, 60);
        gtk_widget_set_margin_start(btn, 10);
        gtk_widget_set_margin_end(btn, 10);
        gtk_widget_set_margin_top(btn, 5);
        gtk_widget_set_margin_bottom(btn, 5);

        /* duplicate service string for storing as data on the button (g_hash_table key will be freed with table; here we attach another copy to the button) */
        g_object_set_data_full(G_OBJECT(btn), "service_name", g_strdup(svc), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(show_service_credentials), w);
        gtk_box_pack_start(GTK_BOX(w->vbox_services), btn, FALSE, FALSE, 0);
    }

    g_hash_table_destroy(services);
    g_list_free_full(list, g_free);
    gtk_widget_show_all(w->vbox_services);
}

void add_credential_dialog(GtkButton *button, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Credential",
                                                    GTK_WINDOW(w->window),
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

    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
        const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));

        if (strlen(service) == 0 || strlen(username) == 0) {
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK,
                                                 "Service and Username cannot be empty.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        } else {
            Credential c;
            memset(&c, 0, sizeof(Credential));
            strncpy(c.service, service, MAX_LEN-1);
            strncpy(c.username, username, MAX_LEN-1);
            strncpy(c.password, password, MAX_LEN-1);
            if (!append_credential(&c)) {
                GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "Error saving credential.");
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
            }
            refresh_service_buttons(w);
        }
    }
    gtk_widget_destroy(dialog);
}

void search_credential_dialog(GtkButton *button, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Search Credential",
                                                    GTK_WINDOW(w->window),
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

    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));

        GList *list = load_all_credentials();
        gboolean found = FALSE;
        GString *msg = g_string_new("");
        for (GList *l = list; l != NULL; l = l->next) {
            Credential *c = (Credential *)l->data;
            if ((strlen(service)==0 || g_strcmp0(c->service, service)==0) &&
                (strlen(username)==0 || g_strcmp0(c->username, username)==0)) {
                g_string_append_printf(msg, "Service: %s\nUsername: %s\nPassword: %s\n\n", c->service, c->username, c->password);
                found = TRUE;
            }
        }
        g_list_free_full(list, g_free);

        if (!found) g_string_assign(msg, "No matching credentials found.");

        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
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

void delete_credential_dialog(GtkButton *button, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Delete Credential",
                                                    GTK_WINDOW(w->window),
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

    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK) {
        const char *service = gtk_entry_get_text(GTK_ENTRY(entry_service));
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));

        GList *list = load_all_credentials();
        GList *new_list = NULL;
        gboolean found = FALSE;
        for (GList *l = list; l != NULL; l = l->next) {
            Credential *c = (Credential *)l->data;
            if (!((strlen(service)==0 || g_strcmp0(c->service, service)==0) &&
                  (strlen(username)==0 || g_strcmp0(c->username, username)==0))) {
                Credential *keep = g_malloc(sizeof(Credential));
                memcpy(keep, c, sizeof(Credential));
                new_list = g_list_append(new_list, keep);
            } else {
                found = TRUE;
            }
        }
        g_list_free_full(list, g_free);

        if (!found) {
            g_list_free_full(new_list, g_free);
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO,
                                                 GTK_BUTTONS_OK,
                                                 "No matching credential found.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        } else {
            overwrite_credentials_from_list(new_list);
            g_list_free_full(new_list, g_free);
            refresh_service_buttons(w);
        }
    }
    gtk_widget_destroy(dialog);
}

void change_master_password(GtkButton *button, gpointer data) {
    AppWidgets *w = (AppWidgets *)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Change Master Password",
                                                    GTK_WINDOW(w->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_OK", GTK_RESPONSE_OK,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *entry_new = gtk_entry_new();
    GtkWidget *entry_confirm = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_new), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(entry_confirm), FALSE);

    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("New Password:"));
    gtk_container_add(GTK_CONTAINER(content), entry_new);
    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("Confirm Password:"));
    gtk_container_add(GTK_CONTAINER(content), entry_confirm);

    gtk_widget_show_all(dialog);
    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK) {
        const char *n = gtk_entry_get_text(GTK_ENTRY(entry_new));
        const char *c = gtk_entry_get_text(GTK_ENTRY(entry_confirm));
        if (strlen(n) == 0 || strcmp(n, c) != 0) {
            GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK,
                                                 "Passwords empty or do not match.");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
        } else {
            if (!save_master(n)) {
                GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "Failed to save master password.");
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
            } else {
                GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(w->window),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_INFO,
                                                     GTK_BUTTONS_OK,
                                                     "Master password changed.");
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
            }
        }
    }
    gtk_widget_destroy(dialog);
}

AppWidgets* create_app(void) {
    AppWidgets *w = g_new0(AppWidgets, 1);
    w->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(w->window), "GTK Password Vault");
    gtk_window_set_default_size(GTK_WINDOW(w->window), 500, 700);
    gtk_window_set_resizable(GTK_WINDOW(w->window), TRUE);
    g_signal_connect(w->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 15);
    gtk_container_add(GTK_CONTAINER(w->window), main_vbox);

    GtkWidget *label_header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_header), "<b>Password Vault</b>");
    gtk_widget_set_halign(label_header, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), label_header, FALSE, FALSE, 5);

    GtkWidget *btn_change_master = gtk_button_new_with_label("Change Master Password");
    gtk_widget_set_size_request(btn_change_master, 200, 50);
    gtk_widget_set_halign(btn_change_master, GTK_ALIGN_CENTER);
    g_signal_connect(btn_change_master, "clicked", G_CALLBACK(change_master_password), w);
    gtk_box_pack_start(GTK_BOX(main_vbox), btn_change_master, FALSE, FALSE, 5);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 5);

    GtkWidget *btn_add = gtk_button_new_with_label("Add Credential");
    gtk_widget_set_size_request(btn_add, 120, 40);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(add_credential_dialog), w);
    gtk_box_pack_start(GTK_BOX(button_box), btn_add, FALSE, FALSE, 0);

    GtkWidget *btn_search = gtk_button_new_with_label("Search");
    gtk_widget_set_size_request(btn_search, 120, 40);
    g_signal_connect(btn_search, "clicked", G_CALLBACK(search_credential_dialog), w);
    gtk_box_pack_start(GTK_BOX(button_box), btn_search, FALSE, FALSE, 0);

    GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
    gtk_widget_set_size_request(btn_delete, 120, 40);
    g_signal_connect(btn_delete, "clicked", G_CALLBACK(delete_credential_dialog), w);
    gtk_box_pack_start(GTK_BOX(button_box), btn_delete, FALSE, FALSE, 0);

    w->label_count = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(w->label_count), "<b>Services saved: 0</b>");
    gtk_widget_set_halign(w->label_count, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), w->label_count, FALSE, FALSE, 5);

    GtkWidget *frame_services = gtk_frame_new(NULL);
    GtkWidget *frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), "<b>Saved Services</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame_services), frame_label);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame_services, TRUE, TRUE, 10);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame_services), scrolled);

    w->vbox_services = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(scrolled), w->vbox_services);

    return w;
}

static char session_master[MAX_LEN] = {0}; // store session password

int prompt_new_master(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter Master Password", parent, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, NULL);
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

    int ok = 0;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *a = gtk_entry_get_text(GTK_ENTRY(entry1));
        const char *b = gtk_entry_get_text(GTK_ENTRY(entry2));
        if (strlen(a) > 0 && strcmp(a, b) == 0) {
            strncpy(session_master, a, MAX_LEN-1);
            ok = 1;
        }
    }

    gtk_widget_destroy(dialog);
    return ok;
}

int prompt_verify_master(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter Master Password", parent, GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_container_add(GTK_CONTAINER(content), gtk_label_new("Master password:"));
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);

    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    int ok = 0;
    if (resp == GTK_RESPONSE_OK) {
        const char *a = gtk_entry_get_text(GTK_ENTRY(entry));
        if (verify_master(a)) ok = 1;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    AppWidgets *appw = create_app();

    if (!prompt_new_master(GTK_WINDOW(appw->window))) {
        g_print("No master password entered. Exiting.\n");
        return 0;
    }

    refresh_service_buttons(appw);
    gtk_widget_show_all(appw->window);
    gtk_main();
    g_free(appw);
    return 0;
}