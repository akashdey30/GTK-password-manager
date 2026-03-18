#include <gtk/gtk.h>
#include "ui.h"
#include "master_auth.h"
#include "vault_storage.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create AppState and UI
    AppState *app = ui_create_app();

    // ma_prompt_master_password handles both first-run setup and existing-password
    // verification in one call. It populates app->session_master on success.
    if (!ma_prompt_master_password(GTK_WINDOW(app->window), app->session_master)) {
        g_print("Master password not set or incorrect. Exiting.\n");
        g_free(app);
        return 0;
    }

    // Refresh services panel and show main window
    ui_refresh_services(app);
    gtk_widget_show_all(app->window);
    gtk_main();

    g_free(app);
    return 0;
}
