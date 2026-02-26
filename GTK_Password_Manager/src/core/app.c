#include "core/app.h"
#include "core/session.h"
#include "database/db_init.h"
#include "ui/app_window.h"
#include "ui/theme.h"
#include "security/auto_logout.h"
#include "utils/logger.h"
#include <glib.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

struct _VaultApp {
    GtkApplication *gtk_app;
    AppWindow      *window;
    Session         session;
    sqlite3        *db;
    char           *db_path;
};

static VaultApp *g_app_instance = NULL;

static void on_activate(GtkApplication *gtk_app, gpointer user_data)
{
    VaultApp *app = user_data;

    theme_init(gtk_app);
    theme_apply(THEME_LIGHT);

    app->window = APP_WINDOW(app_window_new(gtk_app));
    app_window_show_view(app->window, VIEW_LOGIN);
    gtk_window_present(GTK_WINDOW(app->window));

    LOG_I("Application activated");
}

VaultApp *vault_app_new(void)
{
    VaultApp *app = g_new0(VaultApp, 1);
    session_init(&app->session);

    /* Determine db path in user data dir */
    const char *data_dir = g_get_user_data_dir();
    char *vault_dir = g_build_filename(data_dir, "gtk-password-vault", NULL);
    g_mkdir_with_parents(vault_dir, 0700);
    app->db_path = g_build_filename(vault_dir, DB_FILENAME, NULL);
    g_free(vault_dir);

    app->gtk_app = gtk_application_new(APP_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app->gtk_app, "activate", G_CALLBACK(on_activate), app);

    g_app_instance = app;
    return app;
}

void vault_app_run(VaultApp *app, int argc, char **argv)
{
    g_application_run(G_APPLICATION(app->gtk_app), argc, argv);

    /* cleanup */
    auto_logout_stop();
    session_wipe(&app->session);
    if (app->db) db_close(app->db);
    g_free(app->db_path);
    g_object_unref(app->gtk_app);
    g_free(app);
    g_app_instance = NULL;
}

void vault_app_quit(VaultApp *app)
{
    g_application_quit(G_APPLICATION(app->gtk_app));
}

GtkWindow *vault_app_get_window(VaultApp *app)
{
    return GTK_WINDOW(app->window);
}

Session *vault_app_get_session(VaultApp *app)
{
    return &app->session;
}

const char *vault_app_get_db_path(VaultApp *app)
{
    return app->db_path;
}
