#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>
#include "global_types.h"

typedef struct _VaultApp VaultApp;

VaultApp   *vault_app_new(void);
void        vault_app_run(VaultApp *app, int argc, char **argv);
void        vault_app_quit(VaultApp *app);

/* Accessors used by UI modules */
GtkWindow  *vault_app_get_window(VaultApp *app);
Session    *vault_app_get_session(VaultApp *app);
const char *vault_app_get_db_path(VaultApp *app);

#endif
