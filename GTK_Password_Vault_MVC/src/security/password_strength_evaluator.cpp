// ============================================================
// security/password_strength_evaluator.cpp
// ============================================================
#include "../../include/security/password_strength_evaluator.h"
#include <algorithm>
#include <cctype>

StrengthLevel PasswordStrengthEvaluator::evaluate(
    const std::string& password) const
{
    if (password.empty()) return StrengthLevel::VeryWeak;

    int score = 0;
    score += scoreLength(password);
    score += scoreCharacterClasses(password);
    if (hasCommonPatterns(password)) score -= 2;

    if (score <= 1)  return StrengthLevel::VeryWeak;
    if (score <= 3)  return StrengthLevel::Weak;
    if (score <= 5)  return StrengthLevel::Moderate;
    if (score <= 7)  return StrengthLevel::Strong;
    return StrengthLevel::VeryStrong;
}

int PasswordStrengthEvaluator::scoreLength(const std::string& p) {
    int len = static_cast<int>(p.size());
    if (len < 6)  return 0;
    if (len < 8)  return 1;
    if (len < 12) return 2;
    if (len < 16) return 3;
    return 4;
}

int PasswordStrengthEvaluator::scoreCharacterClasses(const std::string& p) {
    bool hasLower  = std::any_of(p.begin(), p.end(), ::islower);
    bool hasUpper  = std::any_of(p.begin(), p.end(), ::isupper);
    bool hasDigit  = std::any_of(p.begin(), p.end(), ::isdigit);
    bool hasSymbol = std::any_of(p.begin(), p.end(),
                                  [](char c){ return !::isalnum(c); });
    return static_cast<int>(hasLower) + static_cast<int>(hasUpper)
         + static_cast<int>(hasDigit) + static_cast<int>(hasSymbol);
}

bool PasswordStrengthEvaluator::hasCommonPatterns(const std::string& p) {
    static const char* patterns[] = {
        "password", "123456", "qwerty", "abc123", "letmein",
        "welcome", "monkey", "master", "login", "pass"
    };
    std::string lower = p;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const char* pat : patterns) {
        if (lower.find(pat) != std::string::npos) return true;
    }
    return false;
}
