#ifndef AUTO_LOGOUT_H
#define AUTO_LOGOUT_H

#include <gtk/gtk.h>
#include "global_types.h"
#include "core/session.h"

typedef void (*LogoutCallback)(gpointer user_data);

void auto_logout_start(Session        *session,
                        LogoutCallback  cb,
                        gpointer        user_data);

void auto_logout_stop(void);
void auto_logout_reset(void);

#endif
