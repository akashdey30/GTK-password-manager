#ifndef RECOVERY_AUTH_H
#define RECOVERY_AUTH_H

#include "global_types.h"

/* Generate and store a recovery phrase (BIP-39 style 12 words).
   Returns newly allocated phrase string – caller must g_free(). */
char     *recovery_generate_phrase(void);

AppResult recovery_store_phrase(const char *phrase,
                                 const char *db_path,
                                 const uint8_t key[AES_KEY_LEN]);

AppResult recovery_verify_phrase(const char *phrase,
                                  const char *db_path,
                                  uint8_t     new_key[AES_KEY_LEN]);

#endif
