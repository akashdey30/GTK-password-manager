#ifndef THEME_H
#define THEME_H

#include <gtk/gtk.h>

typedef enum { THEME_LIGHT, THEME_DARK } ThemeMode;

void theme_init(GtkApplication *app);
void theme_apply(ThemeMode mode);
void theme_toggle(void);
ThemeMode theme_current(void);

#endif
