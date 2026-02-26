#ifndef CREDENTIAL_ROW_H
#define CREDENTIAL_ROW_H

#include <gtk/gtk.h>
#include "global_types.h"

typedef void (*CredRowCallback)(const Credential *c, gpointer user_data);

GtkWidget *credential_row_new(const Credential *c,
                               CredRowCallback   on_edit,
                               CredRowCallback   on_delete,
                               CredRowCallback   on_copy_password,
                               gpointer          user_data);

#endif
