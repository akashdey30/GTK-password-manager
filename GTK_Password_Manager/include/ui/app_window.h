#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include <gtk/gtk.h>

typedef enum {
    VIEW_LOGIN,
    VIEW_VAULT,
} ViewId;

#define APP_WINDOW_TYPE (app_window_get_type())
G_DECLARE_FINAL_TYPE(AppWindow, app_window, APP, WINDOW, GtkApplicationWindow)

GtkWidget *app_window_new(GtkApplication *app);
void        app_window_show_view(AppWindow *self, ViewId view);

#endif
