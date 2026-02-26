#include "security/auto_logout.h"
#include "utils/logger.h"
#include <glib.h>

static guint           g_timer_id   = 0;
static Session        *g_session    = NULL;
static LogoutCallback  g_cb         = NULL;
static gpointer        g_user_data  = NULL;

static gboolean on_tick(gpointer user_data)
{
    (void)user_data;
    if (!g_session) return G_SOURCE_REMOVE;

    session_touch(g_session);  /* tick still counts activity */

    if (session_is_expired(g_session)) {
        LOG_I("Session expired – triggering auto logout");
        session_lock(g_session);
        if (g_cb) g_cb(g_user_data);
        g_timer_id = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

void auto_logout_start(Session *session, LogoutCallback cb, gpointer user_data)
{
    auto_logout_stop();
    g_session   = session;
    g_cb        = cb;
    g_user_data = user_data;
    /* Check every 10 seconds */
    g_timer_id = g_timeout_add_seconds(10, on_tick, NULL);
}

void auto_logout_stop(void)
{
    if (g_timer_id) {
        g_source_remove(g_timer_id);
        g_timer_id = 0;
    }
}

void auto_logout_reset(void)
{
    if (g_session) session_touch(g_session);
}
