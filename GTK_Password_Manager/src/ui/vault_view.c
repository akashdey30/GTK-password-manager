#include "ui/vault_view.h"
#include "ui/credential_row.h"
#include "ui/dialogs.h"
#include "core/session.h"
#include "ui/theme.h"
#include "database/db_init.h"
#include "database/db_credentials.h"
#include "database/db_search.h"
#include "security/clipboard.h"
#include "security/auto_logout.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

typedef struct {
    char         *db_path;
    Session      *session;
    LockCallback  on_lock;
    gpointer      user_data;

    GtkWidget    *root;
    GtkWidget    *cred_list;      /* GtkBox inside scroll */
    GtkWidget    *search_entry;
    GtkWidget    *category_list;  /* sidebar GtkListBox */
    GtkWidget    *count_label;

    char         *active_category; /* NULL = All */
} VaultViewData;

static void vvd_free(VaultViewData *d)
{
    g_free(d->db_path);
    g_free(d->active_category);
    g_free(d);
}

/* ── Helpers ──────────────────────────────────────── */
static void clear_list(GtkWidget *list_box)
{
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(list_box)))
        gtk_box_remove(GTK_BOX(list_box), child);
}

static void refresh_categories(VaultViewData *d)
{
    sqlite3 *db = NULL;
    if (db_open(d->db_path, &db) != APP_OK) return;
    int count = 0;
    Credential **all = db_cred_list_all(db, d->session->aes_key, &count);
    db_close(db);

    /* Build unique sorted set */
    GHashTable *seen = g_hash_table_new(g_str_hash, g_str_equal);
    GPtrArray *cats = g_ptr_array_new_with_free_func(g_free);
    for (int i = 0; i < count; i++) {
        if (all[i]->category && *all[i]->category &&
            !g_hash_table_contains(seen, all[i]->category)) {
            g_hash_table_insert(seen, all[i]->category, NULL);
            g_ptr_array_add(cats, g_strdup(all[i]->category));
        }
    }
    db_cred_list_free(all);
    g_hash_table_destroy(seen);

    /* Rebuild sidebar list */
    GtkWidget *lb = d->category_list;
    GtkWidget *row;
    while ((row = gtk_widget_get_first_child(lb)))
        gtk_list_box_remove(GTK_LIST_BOX(lb), row);

    /* "All" row */
    GtkWidget *all_row = gtk_label_new("All Credentials");
    gtk_widget_set_halign(all_row, GTK_ALIGN_START);
    gtk_list_box_append(GTK_LIST_BOX(lb), all_row);

    for (guint i = 0; i < cats->len; i++) {
        GtkWidget *cat_label = gtk_label_new(cats->pdata[i]);
        gtk_widget_set_halign(cat_label, GTK_ALIGN_START);
        gtk_list_box_append(GTK_LIST_BOX(lb), cat_label);
    }
    g_ptr_array_free(cats, TRUE);
}

static void populate_list(VaultViewData *d,
                            Credential **creds, int count);

static void do_refresh(VaultViewData *d)
{
    auto_logout_reset();

    sqlite3 *db = NULL;
    if (db_open(d->db_path, &db) != APP_OK) return;

    const char *query = gtk_editable_get_text(
        GTK_EDITABLE(d->search_entry));

    int count = 0;
    Credential **creds;

    if (d->active_category && *d->active_category) {
        creds = db_filter_category(db, d->session->aes_key,
                                    d->active_category, &count);
    } else if (query && *query) {
        creds = db_search(db, d->session->aes_key, query, &count);
    } else {
        creds = db_cred_list_all(db, d->session->aes_key, &count);
    }
    db_close(db);

    populate_list(d, creds, count);
    db_cred_list_free(creds);
    refresh_categories(d);

    char *cnt_txt = g_strdup_printf("%d item%s", count, count == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(d->count_label), cnt_txt);
    g_free(cnt_txt);
}

/* ── Credential row callbacks ───────────────────────── */
static void on_copy_password(const Credential *c, gpointer user_data)
{
    (void)user_data;
    GdkDisplay *disp = gdk_display_get_default();
    clipboard_copy_and_clear(disp, c->password);
    LOG_I("Password copied for: %s", c->title);
    auto_logout_reset();
}

static void save_edit_cb(const Credential *updated, gpointer ud2)
{
    VaultViewData *dd = ud2;
    sqlite3 *db2 = NULL;
    if (db_open(dd->db_path, &db2) == APP_OK) {
        db_cred_update(db2, dd->session->aes_key, updated);
        db_close(db2);
    }
    do_refresh(dd);
}

static void on_edit_cred(const Credential *c, gpointer user_data)
{
    VaultViewData *d = user_data;
    auto_logout_reset();
    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    dialog_edit_credential(win, c, save_edit_cb, d);
}

static void on_delete_cred(const Credential *c, gpointer user_data)
{
    VaultViewData *d = user_data;
    auto_logout_reset();

    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    if (!dialog_confirm(win, "Delete Credential",
                         "Are you sure you want to delete this credential?"))
        return;

    sqlite3 *db2 = NULL;
    if (db_open(d->db_path, &db2) == APP_OK) {
        db_cred_delete(db2, c->id);
        db_close(db2);
    }
    do_refresh(d);
}

static void populate_list(VaultViewData *d, Credential **creds, int count)
{
    clear_list(d->cred_list);
    for (int i = 0; i < count; i++) {
        GtkWidget *row = credential_row_new(creds[i],
                                             on_edit_cred,
                                             on_delete_cred,
                                             on_copy_password,
                                             d);
        gtk_box_append(GTK_BOX(d->cred_list), row);
    }
    if (count == 0) {
        GtkWidget *empty = gtk_label_new("No credentials found.");
        gtk_widget_set_margin_top(empty, 48);
        gtk_widget_add_css_class(empty, "login-subtitle");
        gtk_box_append(GTK_BOX(d->cred_list), empty);
    }
}


static void save_add_cb(const Credential *c, gpointer ud2)
{
    VaultViewData *dd = ud2;
    sqlite3 *db2 = NULL;
    if (db_open(dd->db_path, &db2) == APP_OK) {
        db_cred_insert(db2, dd->session->aes_key, c);
        db_close(db2);
    }
    do_refresh(dd);
}

/* ── Toolbar callbacks ──────────────────────────────── */
static void on_settings_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VaultViewData *d = user_data;
    auto_logout_reset();
    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    dialog_settings(win, d->db_path, d->session);
    /* Refresh list in case credentials were edited/deleted in settings */
    do_refresh(d);
}

static void on_add_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VaultViewData *d = user_data;
    auto_logout_reset();

    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    dialog_add_credential(win, save_add_cb, d);
}

static void on_lock_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VaultViewData *d = user_data;
    if (d->on_lock) d->on_lock(d->user_data);
}

static void on_theme_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn; (void)user_data;
    theme_toggle();
}

static void on_search_changed(GtkEditable *e, gpointer user_data)
{
    (void)e;
    VaultViewData *d = user_data;
    d->active_category = (g_free(d->active_category), NULL);
    do_refresh(d);
}

static void on_category_selected(GtkListBox *lb, GtkListBoxRow *row, gpointer user_data)
{
    (void)lb;
    VaultViewData *d = user_data;
    if (!row) return;
    GtkWidget *label = gtk_list_box_row_get_child(row);
    const char *text = gtk_label_get_text(GTK_LABEL(label));
    g_free(d->active_category);
    d->active_category = g_strcmp0(text, "All Credentials") == 0
        ? NULL : g_strdup(text);
    do_refresh(d);
}

static void on_generator_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VaultViewData *d = user_data;
    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    dialog_generator(win);
}

static void on_backup_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VaultViewData *d = user_data;
    GtkWindow *win = GTK_WINDOW(gtk_widget_get_root(d->root));
    dialog_backup(win, d->db_path);
}

/* ── Builder ────────────────────────────────────────── */
GtkWidget *vault_view_new(const char   *db_path,
                           Session       *session,
                           LockCallback   on_lock,
                           gpointer       user_data)
{
    VaultViewData *d = g_new0(VaultViewData, 1);
    d->db_path   = g_strdup(db_path);
    d->session   = session;
    d->on_lock   = on_lock;
    d->user_data = user_data;

    /* ── Root: vertical box (headerbar + content) ─── */
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    d->root = root;
    g_object_set_data_full(G_OBJECT(root), "vault-data", d,
                            (GDestroyNotify)vvd_free);

    /* ── Header bar ─── */
    GtkWidget *hbar = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(hbar), TRUE);

    /* Search */
    d->search_entry = gtk_search_entry_new();
    gtk_widget_add_css_class(d->search_entry, "search-entry");
    gtk_widget_set_size_request(d->search_entry, 260, -1);
    g_signal_connect(d->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), d);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(hbar), d->search_entry);

    /* Left buttons */
    GtkWidget *add_btn = gtk_button_new_from_icon_name("list-add-symbolic");
    gtk_widget_set_tooltip_text(add_btn, "Add credential");
    gtk_widget_add_css_class(add_btn, "suggested-action");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), d);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hbar), add_btn);

    GtkWidget *gen_btn = gtk_button_new_from_icon_name("system-run-symbolic");
    gtk_widget_set_tooltip_text(gen_btn, "Password generator");
    gtk_widget_add_css_class(gen_btn, "flat");
    g_signal_connect(gen_btn, "clicked", G_CALLBACK(on_generator_clicked), d);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hbar), gen_btn);

    /* Right buttons */
    GtkWidget *theme_btn = gtk_button_new_from_icon_name("display-brightness-symbolic");
    gtk_widget_set_tooltip_text(theme_btn, "Toggle theme");
    gtk_widget_add_css_class(theme_btn, "flat");
    g_signal_connect(theme_btn, "clicked", G_CALLBACK(on_theme_clicked), d);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hbar), theme_btn);

    GtkWidget *lock_btn = gtk_button_new_from_icon_name("system-lock-screen-symbolic");
    gtk_widget_set_tooltip_text(lock_btn, "Lock vault");
    gtk_widget_add_css_class(lock_btn, "flat");
    g_signal_connect(lock_btn, "clicked", G_CALLBACK(on_lock_clicked), d);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hbar), lock_btn);

    GtkWidget *backup_btn = gtk_button_new_from_icon_name("document-save-symbolic");
    gtk_widget_set_tooltip_text(backup_btn, "Backup vault");
    gtk_widget_add_css_class(backup_btn, "flat");
    g_signal_connect(backup_btn, "clicked", G_CALLBACK(on_backup_clicked), d);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hbar), backup_btn);

    GtkWidget *settings_btn = gtk_button_new_from_icon_name("preferences-system-symbolic");
    gtk_widget_set_tooltip_text(settings_btn, "Settings");
    gtk_widget_add_css_class(settings_btn, "flat");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), d);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hbar), settings_btn);

    gtk_box_append(GTK_BOX(root), hbar);

    /* ── Content pane: sidebar + credential list ─── */
    GtkWidget *content = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_append(GTK_BOX(root), content);

    /* Sidebar */
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sidebar_scroll, 220, -1);

    d->category_list = gtk_list_box_new();
    gtk_widget_add_css_class(d->category_list, "sidebar");
    g_signal_connect(d->category_list, "row-selected",
                     G_CALLBACK(on_category_selected), d);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   d->category_list);
    gtk_paned_set_start_child(GTK_PANED(content), sidebar_scroll);
    gtk_paned_set_shrink_start_child(GTK_PANED(content), FALSE);

    /* Credential panel */
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_set_end_child(GTK_PANED(content), right_box);

    /* Status bar */
    GtkWidget *status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(status_bar, 12);
    gtk_widget_set_margin_end(status_bar, 12);
    gtk_widget_set_margin_top(status_bar, 6);
    gtk_widget_set_margin_bottom(status_bar, 6);
    d->count_label = gtk_label_new("0 items");
    gtk_widget_add_css_class(d->count_label, "cred-username");
    gtk_box_append(GTK_BOX(status_bar), d->count_label);
    gtk_box_append(GTK_BOX(right_box), status_bar);
    gtk_box_append(GTK_BOX(right_box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Scrollable credential list */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    d->cred_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), d->cred_list);
    gtk_box_append(GTK_BOX(right_box), scroll);

    /* Initial populate */
    do_refresh(d);

    return root;
}

void vault_view_refresh(GtkWidget *vault_view)
{
    VaultViewData *d = g_object_get_data(G_OBJECT(vault_view), "vault-data");
    if (d) do_refresh(d);
}
