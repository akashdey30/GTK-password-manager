#include "global_types.h"
#include <glib.h>
#include "crypto/aes_engine.h"
#include <string.h>

void credential_free(Credential *c)
{
    if (!c) return;
    /* Wipe sensitive fields before freeing */
    if (c->password) { aes_wipe(c->password, strlen(c->password)); g_free(c->password); }
    g_free(c->title);
    g_free(c->username);
    g_free(c->url);
    g_free(c->notes);
    g_free(c->category);
    g_free(c);
}
