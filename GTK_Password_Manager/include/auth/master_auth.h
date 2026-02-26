#ifndef MASTER_AUTH_H
#define MASTER_AUTH_H

#include "global_types.h"

/* Returns APP_OK and fills key[AES_KEY_LEN] on success */
AppResult master_auth_setup(const char *password,
                            uint8_t     key_out[AES_KEY_LEN]);

AppResult master_auth_verify(const char *password,
                              const char *db_path,
                              uint8_t     key_out[AES_KEY_LEN]);

AppResult master_auth_change(const char *old_password,
                              const char *new_password,
                              const char *db_path);

bool      master_auth_is_set(const char *db_path);

#endif
