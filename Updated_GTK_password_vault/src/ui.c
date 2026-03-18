#include "ui.h"
#include "dialogs.h"
#include "master_auth.h"
#include "service_manager.h"
#include "vault_storage.h"
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
    // Fixed: was dialog_change_master_password — correct name is ma_change_master_password
    g_signal_connect(btn_change_master, "clicked", G_CALLBACK(ma_change_master_password), app);
    gtk_box_pack_start(GTK_BOX(main_vbox), btn_change_master, FALSE, FALSE, 5);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 5);

    GtkWidget *btn_add = gtk_button_new_with_label("Add Credential");
    gtk_widget_set_size_request(btn_add, 120, 40);
    // Fixed: was dialog_add_credential — correct name is dl_add_credential_dialog
    g_signal_connect(btn_add, "clicked", G_CALLBACK(dl_add_credential_dialog), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_add, FALSE, FALSE, 0);

    GtkWidget *btn_search = gtk_button_new_with_label("Search");
    gtk_widget_set_size_request(btn_search, 120, 40);
    // Fixed: was dialog_search_credential — correct name is dl_search_credential_dialog
    g_signal_connect(btn_search, "clicked", G_CALLBACK(dl_search_credential_dialog), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_search, FALSE, FALSE, 0);

    GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
    gtk_widget_set_size_request(btn_delete, 120, 40);
    // Fixed: was dialog_delete_credential — correct name is dl_delete_credential_dialog
    g_signal_connect(btn_delete, "clicked", G_CALLBACK(dl_delete_credential_dialog), app);
    gtk_box_pack_start(GTK_BOX(button_box), btn_delete, FALSE, FALSE, 0);

    app->label_count = gtk_label_new("Services saved: 0");
    gtk_widget_set_halign(app->label_count, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), app->label_count, FALSE, FALSE, 5);

    GtkWidget *frame_services = gtk_frame_new(NULL);
    GtkWidget *frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), "<b>Saved Services</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame_services), frame_label);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame_services, TRUE, TRUE, 10);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
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

    // Collect unique service names
    GHashTable *services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (GList *l = all; l != NULL; l = l->next) {
        Credential *c = (Credential*)l->data;
        if (c->service[0] != '\0') g_hash_table_add(services, g_strdup(c->service));
    }

    guint count = g_hash_table_size(services);
    gchar count_text[64];
    snprintf(count_text, sizeof(count_text), "Services saved: %u", count);
    gtk_label_set_text(GTK_LABEL(app->label_count), count_text);

    // Create one button per unique service
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
        // Fixed: was dialog_show_service_credentials — correct name is dl_show_service_credentials
        g_signal_connect(btn, "clicked", G_CALLBACK(dl_show_service_credentials), app);
        gtk_box_pack_start(GTK_BOX(app->vbox_services), btn, FALSE, FALSE, 0);
    }

    g_hash_table_destroy(services);
    g_list_free_full(all, g_free);
    gtk_widget_show_all(app->vbox_services);
}
