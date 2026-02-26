#include "core/session.h"
#include "crypto/aes_engine.h"
#include <time.h>
#include <string.h>

void session_init(Session *s)
{
    memset(s, 0, sizeof(*s));
    s->locked = true;
}

void session_unlock(Session *s, const uint8_t key[AES_KEY_LEN])
{
    memcpy(s->aes_key, key, AES_KEY_LEN);
    s->locked        = false;
    s->last_activity = (int64_t)time(NULL);
}

void session_lock(Session *s)
{
    aes_wipe(s->aes_key, AES_KEY_LEN);
    s->locked = true;
}

bool session_is_locked(const Session *s)
{
    return s->locked;
}

void session_touch(Session *s)
{
    s->last_activity = (int64_t)time(NULL);
}

bool session_is_expired(const Session *s)
{
    if (s->locked) return true;
    int64_t now = (int64_t)time(NULL);
    return (now - s->last_activity) >= AUTO_LOGOUT_SEC;
}

void session_wipe(Session *s)
{
    aes_wipe(s, sizeof(*s));
    s->locked = true;
}
