#include "ui/app_window.h"
#include "ui/login_view.h"
#include "ui/vault_view.h"
#include "ui/theme.h"
#include "core/session.h"
#include "database/db_init.h"
#include "auth/master_auth.h"
#include "security/auto_logout.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

struct _AppWindow {
    GtkApplicationWindow parent;

    GtkWidget *stack;
    GtkWidget *login_view;
    GtkWidget *vault_view;

    Session    session;
    char      *db_path;
};

G_DEFINE_TYPE(AppWindow, app_window, GTK_TYPE_APPLICATION_WINDOW)

/* ── forward declarations ───────────────────────────── */
static void on_login_success(const uint8_t key[AES_KEY_LEN], gpointer user_data);
static void on_vault_lock   (gpointer user_data);

/* ── class / instance init ──────────────────────────── */
static void app_window_dispose(GObject *obj)
{
    AppWindow *self = APP_WINDOW(obj);
    session_wipe(&self->session);
    g_free(self->db_path);
    G_OBJECT_CLASS(app_window_parent_class)->dispose(obj);
}

static void app_window_class_init(AppWindowClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = app_window_dispose;
}

static void app_window_init(AppWindow *self)
{
    session_init(&self->session);

    /* Determine db path */
    const char *data_dir = g_get_user_data_dir();
    char *vault_dir = g_build_filename(data_dir, "gtk-password-vault", NULL);
    g_mkdir_with_parents(vault_dir, 0700);
    self->db_path = g_build_filename(vault_dir, DB_FILENAME, NULL);
    g_free(vault_dir);

    /* Ensure schema exists */
    sqlite3 *db = NULL;
    if (db_open(self->db_path, &db) == APP_OK) {
        db_init_schema(db);
        db_close(db);
    }

    gtk_window_set_title(GTK_WINDOW(self), APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(self), 960, 640);
    gtk_widget_add_css_class(GTK_WIDGET(self), "vault-window");

    self->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(self->stack),
                                   GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(self->stack), 180);
    gtk_window_set_child(GTK_WINDOW(self), self->stack);

    /* Login view */
    self->login_view = login_view_new(self->db_path, on_login_success, self);
    gtk_stack_add_named(GTK_STACK(self->stack), self->login_view, "login");

    /* Vault view (built later after first login) */
    self->vault_view = NULL;
}

/* ── callbacks ──────────────────────────────────────── */
static void on_login_success(const uint8_t key[AES_KEY_LEN], gpointer user_data)
{
    AppWindow *self = APP_WINDOW(user_data);
    session_unlock(&self->session, key);

    if (!self->vault_view) {
        self->vault_view = vault_view_new(self->db_path, &self->session,
                                           on_vault_lock, self);
        gtk_stack_add_named(GTK_STACK(self->stack), self->vault_view, "vault");
    } else {
        vault_view_refresh(self->vault_view);
    }

    auto_logout_start(&self->session, on_vault_lock, self);
    app_window_show_view(self, VIEW_VAULT);
    LOG_I("Vault unlocked");
}

static void on_vault_lock(gpointer user_data)
{
    AppWindow *self = APP_WINDOW(user_data);
    session_lock(&self->session);
    auto_logout_stop();
    app_window_show_view(self, VIEW_LOGIN);
    LOG_I("Vault locked");
}

/* ── public API ─────────────────────────────────────── */
GtkWidget *app_window_new(GtkApplication *app)
{
    return g_object_new(APP_WINDOW_TYPE,
                        "application", app,
                        NULL);
}

void app_window_show_view(AppWindow *self, ViewId view)
{
    const char *name = (view == VIEW_LOGIN) ? "login" : "vault";
    gtk_stack_set_visible_child_name(GTK_STACK(self->stack), name);
}
