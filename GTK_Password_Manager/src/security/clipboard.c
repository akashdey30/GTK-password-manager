#include "security/clipboard.h"
#include "global_types.h"
#include "utils/logger.h"
#include <glib.h>

static guint g_clear_timer = 0;

static gboolean do_clear(gpointer data)
{
    GdkDisplay *display = data;
    GdkClipboard *cb = gdk_display_get_clipboard(display);
    gdk_clipboard_set_text(cb, "");
    LOG_I("Clipboard cleared");
    g_clear_timer = 0;
    return G_SOURCE_REMOVE;
}

void clipboard_copy_and_clear(GdkDisplay *display, const char *text)
{
    GdkClipboard *cb = gdk_display_get_clipboard(display);
    gdk_clipboard_set_text(cb, text);
    LOG_I("Copied to clipboard, will clear in %d seconds", CLIPBOARD_CLEAR_SEC);

    clipboard_cancel_clear();
    g_clear_timer = g_timeout_add_seconds(CLIPBOARD_CLEAR_SEC, do_clear, display);
}

void clipboard_cancel_clear(void)
{
    if (g_clear_timer) {
        g_source_remove(g_clear_timer);
        g_clear_timer = 0;
    }
}
