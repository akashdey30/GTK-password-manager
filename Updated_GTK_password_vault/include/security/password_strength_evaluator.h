#pragma once
// ============================================================
// security/password_strength_evaluator.h
// SRP: Password strength evaluation only.
// LSP: Fully satisfies IPasswordStrengthEvaluator.
// ============================================================
#ifndef PASSWORD_STRENGTH_EVALUATOR_H
#define PASSWORD_STRENGTH_EVALUATOR_H

#include "i_strength_evaluator.h"

class PasswordStrengthEvaluator : public IPasswordStrengthEvaluator {
public:
    StrengthLevel evaluate(const std::string& password) const override;

private:
    static int scoreLength(const std::string& password);
    static int scoreCharacterClasses(const std::string& password);
    static bool hasCommonPatterns(const std::string& password);
};

#endif // PASSWORD_STRENGTH_EVALUATOR_H
