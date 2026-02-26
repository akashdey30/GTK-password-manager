#ifndef LOGIN_VIEW_H
#define LOGIN_VIEW_H

#include <gtk/gtk.h>
#include "global_types.h"

typedef void (*LoginSuccessCallback)(const uint8_t key[AES_KEY_LEN],
                                      gpointer       user_data);

GtkWidget *login_view_new(const char         *db_path,
                           LoginSuccessCallback on_success,
                           gpointer             user_data);

#endif
