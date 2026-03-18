#ifndef APP_STATE_H
#define APP_STATE_H

#include <gtk/gtk.h>
#include <glib.h>
#include "credentials.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *vbox_services;
    GtkWidget *label_count;
    char session_master[MAX_LEN];
} AppState;

#endif // APP_STATE_H
