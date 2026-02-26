#ifndef SESSION_H
#define SESSION_H

#include "global_types.h"

void    session_init(Session *s);
void    session_unlock(Session *s, const uint8_t key[AES_KEY_LEN]);
void    session_lock(Session *s);
bool    session_is_locked(const Session *s);
void    session_touch(Session *s);
bool    session_is_expired(const Session *s);
void    session_wipe(Session *s);

#endif
