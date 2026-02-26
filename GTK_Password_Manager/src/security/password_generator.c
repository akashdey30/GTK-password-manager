#include "security/password_generator.h"
#include "crypto/crypto.h"
#include <glib.h>
#include <string.h>

#define UPPER   "ABCDEFGHJKLMNPQRSTUVWXYZ"
#define UPPER_A "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define LOWER   "abcdefghjkmnpqrstuvwxyz"
#define LOWER_A "abcdefghijklmnopqrstuvwxyz"
#define DIGITS  "23456789"
#define DIGITS_A "0123456789"
#define SYMBOLS "!@#$%^&*()-_=+[]{}|;:,.<>?"

PwGenOptions password_gen_defaults(void)
{
    return (PwGenOptions){
        .length            = 16,
        .use_upper         = true,
        .use_lower         = true,
        .use_digits        = true,
        .use_symbols       = true,
        .exclude_ambiguous = true,
    };
}

char *password_generate(const PwGenOptions *opts)
{
    GString *charset = g_string_new(NULL);

    if (opts->use_upper)
        g_string_append(charset, opts->exclude_ambiguous ? UPPER : UPPER_A);
    if (opts->use_lower)
        g_string_append(charset, opts->exclude_ambiguous ? LOWER : LOWER_A);
    if (opts->use_digits)
        g_string_append(charset, opts->exclude_ambiguous ? DIGITS : DIGITS_A);
    if (opts->use_symbols)
        g_string_append(charset, SYMBOLS);

    if (!charset->len) {
        g_string_free(charset, TRUE);
        return g_strdup("");
    }

    char *result = g_malloc(opts->length + 1);
    uint8_t *rnd = g_malloc(opts->length * 4);
    crypto_random_bytes(rnd, opts->length * 4);

    for (int i = 0; i < opts->length; i++) {
        uint32_t idx = 0;
        memcpy(&idx, rnd + i * 4, 4);
        result[i] = charset->str[idx % charset->len];
    }
    result[opts->length] = '\0';

    g_free(rnd);
    g_string_free(charset, TRUE);
    return result;
}
