#ifndef PASSWORD_GENERATOR_H
#define PASSWORD_GENERATOR_H

#include <stdbool.h>

typedef struct {
    int  length;          /* default 16 */
    bool use_upper;
    bool use_lower;
    bool use_digits;
    bool use_symbols;
    bool exclude_ambiguous; /* 0 O l 1 */
} PwGenOptions;

/* Returns newly allocated string; caller g_free() */
char *password_generate(const PwGenOptions *opts);

PwGenOptions password_gen_defaults(void);

#endif
