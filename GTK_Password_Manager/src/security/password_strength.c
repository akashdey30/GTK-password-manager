#include "security/password_strength.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

PasswordStrengthResult password_check_strength(const char *password)
{
    PasswordStrengthResult res = {0};
    if (!password || !*password) {
        res.level = PW_STRENGTH_VERY_WEAK;
        res.score = 0;
        res.label = "Very Weak";
        res.color_hex = "#e53935";
        res.feedback = "Enter a password";
        return res;
    }

    int len     = (int)strlen(password);
    bool upper  = false, lower = false, digit = false, sym = false;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)password[i];
        if (isupper(c)) upper = true;
        else if (islower(c)) lower = true;
        else if (isdigit(c)) digit = true;
        else sym = true;
    }

    int score = 0;
    score += (len >= 8) ? 20 : (len * 2);
    score += (len >= 12) ? 10 : 0;
    score += (len >= 16) ? 10 : 0;
    score += upper  ? 10 : 0;
    score += lower  ? 10 : 0;
    score += digit  ? 10 : 0;
    score += sym    ? 20 : 0;
    if (score > 100) score = 100;

    if (score < 20) {
        res.level = PW_STRENGTH_VERY_WEAK;
        res.label = "Very Weak";
        res.color_hex = "#e53935";
        res.feedback = "Too short or simple";
    } else if (score < 40) {
        res.level = PW_STRENGTH_WEAK;
        res.label = "Weak";
        res.color_hex = "#fb8c00";
        res.feedback = "Add more character types";
    } else if (score < 60) {
        res.level = PW_STRENGTH_FAIR;
        res.label = "Fair";
        res.color_hex = "#fdd835";
        res.feedback = "Could be stronger";
    } else if (score < 80) {
        res.level = PW_STRENGTH_STRONG;
        res.label = "Strong";
        res.color_hex = "#43a047";
        res.feedback = "Good password";
    } else {
        res.level = PW_STRENGTH_VERY_STRONG;
        res.label = "Very Strong";
        res.color_hex = "#1e88e5";
        res.feedback = "Excellent password";
    }
    res.score = score;
    return res;
}
