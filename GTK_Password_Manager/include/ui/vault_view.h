#ifndef VAULT_VIEW_H
#define VAULT_VIEW_H

#include <gtk/gtk.h>
#include "global_types.h"

typedef void (*LockCallback)(gpointer user_data);

GtkWidget *vault_view_new(const char   *db_path,
                           Session       *session,
                           LockCallback   on_lock,
                           gpointer       user_data);

void        vault_view_refresh(GtkWidget *vault_view);

#endif
