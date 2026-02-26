#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <gtk/gtk.h>

/* Copy text to clipboard and schedule a clear after CLIPBOARD_CLEAR_SEC */
void clipboard_copy_and_clear(GdkDisplay *display, const char *text);
void clipboard_cancel_clear(void);

#endif
