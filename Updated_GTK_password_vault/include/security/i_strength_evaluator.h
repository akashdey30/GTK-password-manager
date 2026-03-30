#pragma once
// ============================================================
// security/i_strength_evaluator.h
// ISP: Single method — evaluate password strength only.
// DIP: GtkDialogProvider depends on this abstraction.
// ============================================================
#ifndef I_STRENGTH_EVALUATOR_H
#define I_STRENGTH_EVALUATOR_H

#include "../global_types.h"
#include <string>

class IPasswordStrengthEvaluator {
public:
    virtual ~IPasswordStrengthEvaluator() = default;

    // Evaluate the strength of a plaintext password.
    virtual StrengthLevel evaluate(const std::string& password) const = 0;
};

#endif // I_STRENGTH_EVALUATOR_H
