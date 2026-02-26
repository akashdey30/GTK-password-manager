#ifndef PASSWORD_STRENGTH_H
#define PASSWORD_STRENGTH_H

typedef enum {
    PW_STRENGTH_VERY_WEAK = 0,
    PW_STRENGTH_WEAK,
    PW_STRENGTH_FAIR,
    PW_STRENGTH_STRONG,
    PW_STRENGTH_VERY_STRONG,
} PasswordStrength;

typedef struct {
    PasswordStrength  level;
    int               score;        /* 0-100 */
    const char       *label;        /* "Very Weak" … */
    const char       *color_hex;    /* CSS color */
    const char       *feedback;
} PasswordStrengthResult;

PasswordStrengthResult password_check_strength(const char *password);

#endif
