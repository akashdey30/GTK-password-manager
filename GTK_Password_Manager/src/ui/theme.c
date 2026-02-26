#include "ui/theme.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

static GtkCssProvider *g_css_provider = NULL;
static ThemeMode       g_mode         = THEME_LIGHT;

void theme_init(GtkApplication *app)
{
    (void)app;
    g_css_provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(g_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void load_css_from_resource(const char *css)
{
    gtk_css_provider_load_from_string(g_css_provider, css);
}

static const char *LIGHT_CSS =
    /* Base */
    "window { background-color: #f8f9fa; color: #202124; }"
    "window.vault-window headerbar { background-color: #ffffff; border-bottom: 1px solid #e0e0e0; }"
    /* Login card */
    ".login-card { background-color: #ffffff; border-radius: 12px; padding: 40px 48px;"
    "  box-shadow: 0 1px 3px rgba(0,0,0,0.12), 0 2px 8px rgba(0,0,0,0.08); }"
    ".login-title { font-size: 24px; font-weight: 700; color: #202124; }"
    ".login-subtitle { font-size: 13px; color: #5f6368; }"
    /* Buttons */
    "button.suggested-action { background-color: #1a73e8; color: #ffffff; border-radius: 6px;"
    "  font-weight: 600; padding: 10px 24px; border: none; }"
    "button.suggested-action:hover { background-color: #1765cc; }"
    "button.destructive-action { background-color: #d93025; color: #ffffff; border-radius: 6px; border: none; }"
    /* Sidebar */
    ".sidebar { background-color: #ffffff; border-right: 1px solid #e0e0e0; min-width: 220px; }"
    ".sidebar row { padding: 6px 12px; border-radius: 6px; margin: 1px 6px; }"
    ".sidebar row:selected { background-color: #e8f0fe; color: #1a73e8; }"
    /* Credential row */
    ".cred-row { background-color: #ffffff; border-radius: 8px; padding: 12px 16px;"
    "  margin: 4px 8px; border: 1px solid #e0e0e0; }"
    ".cred-row:hover { background-color: #f1f3f4; border-color: #c5c5c5; }"
    ".cred-avatar { background-color: #e8f0fe; border-radius: 50%; }"
    ".cred-title { font-weight: 600; font-size: 14px; color: #202124; }"
    ".cred-username { font-size: 12px; color: #5f6368; }"
    /* Search */
    ".search-entry { border-radius: 24px; padding: 6px 16px; background-color: #f1f3f4;"
    "  border: 1px solid transparent; }"
    ".search-entry:focus { border-color: #1a73e8; background-color: #ffffff; }"
    /* Strength bar */
    ".strength-bar trough { border-radius: 4px; min-height: 6px; }"
    ".strength-bar.very-weak progress { background-color: #e53935; }"
    ".strength-bar.weak progress { background-color: #fb8c00; }"
    ".strength-bar.fair progress { background-color: #fdd835; }"
    ".strength-bar.strong progress { background-color: #43a047; }"
    ".strength-bar.very-strong progress { background-color: #1e88e5; }"
    /* Forgot password link */
    "button.link { color: #1a73e8; background: none; border: none; padding: 0; }"
    "button.link:hover { color: #1765cc; text-decoration: underline; }";

static const char *DARK_CSS =
    "window { background-color: #1e1e2e; color: #cdd6f4; }"
    "window.vault-window headerbar { background-color: #181825; border-bottom: 1px solid #313244; }"
    ".login-card { background-color: #24273a; border-radius: 12px; padding: 40px 48px;"
    "  box-shadow: 0 2px 12px rgba(0,0,0,0.4); }"
    ".login-title { font-size: 24px; font-weight: 700; color: #cdd6f4; }"
    ".login-subtitle { font-size: 13px; color: #a6adc8; }"
    "button.suggested-action { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px;"
    "  font-weight: 600; padding: 10px 24px; border: none; }"
    "button.suggested-action:hover { background-color: #74c7ec; }"
    "button.destructive-action { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; border: none; }"
    ".sidebar { background-color: #181825; border-right: 1px solid #313244; min-width: 220px; }"
    ".sidebar row { padding: 6px 12px; border-radius: 6px; margin: 1px 6px; }"
    ".sidebar row:selected { background-color: #313244; color: #89b4fa; }"
    ".cred-row { background-color: #24273a; border-radius: 8px; padding: 12px 16px;"
    "  margin: 4px 8px; border: 1px solid #313244; }"
    ".cred-row:hover { background-color: #2a2d3d; border-color: #45475a; }"
    ".cred-avatar { background-color: #313244; border-radius: 50%; }"
    ".cred-title { font-weight: 600; font-size: 14px; color: #cdd6f4; }"
    ".cred-username { font-size: 12px; color: #a6adc8; }"
    ".search-entry { border-radius: 24px; padding: 6px 16px; background-color: #313244;"
    "  border: 1px solid transparent; color: #cdd6f4; }"
    ".search-entry:focus { border-color: #89b4fa; background-color: #24273a; }"
    "button.link { color: #89b4fa; background: none; border: none; padding: 0; }"
    "button.link:hover { color: #74c7ec; }";

void theme_apply(ThemeMode mode)
{
    g_mode = mode;
    load_css_from_resource(mode == THEME_DARK ? DARK_CSS : LIGHT_CSS);
    LOG_D("Theme applied: %s", mode == THEME_DARK ? "dark" : "light");
}

void theme_toggle(void)
{
    theme_apply(g_mode == THEME_LIGHT ? THEME_DARK : THEME_LIGHT);
}

ThemeMode theme_current(void)
{
    return g_mode;
}
