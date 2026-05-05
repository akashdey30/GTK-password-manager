#pragma once
// ============================================================
// auth/i_auth_dialog.h
// ISP: Auth dialog methods only — consumed by MasterAuth.
// ============================================================
#ifndef I_AUTH_DIALOG_H
#define I_AUTH_DIALOG_H

#include "../global_types.h"
#include "../db/i_storage.h"
#include <string>

// ── Result of master password setup ──────────────────────────
struct SetupResult {
    std::string  password;
    RecoveryData recovery;
};

class IAuthDialog {
public:
    virtual ~IAuthDialog() = default;

    // Show first-run setup: password + confirm + recovery data.
    // Returns true and fills result if user confirmed.
    virtual bool promptSetupMaster(SetupResult& result) = 0;

    // Show login prompt: password field only + Forgot Password button.
    // Returns true if user entered a password (may be wrong).
    // Sets forgotPassword=true if "Forgot Password?" was clicked.
    virtual bool promptVerifyMaster(std::string& outPass,
                                    bool& forgotPassword) = 0;

    // Show recovery window: phone + gmail + school answer.
    // Returns true and fills recovery if user confirmed.
    virtual bool promptRecovery(RecoveryData& recovery) = 0;

    // Show new password dialog for reset (after recovery).
    virtual bool promptNewPassword(std::string& outPass) = 0;

    // Show error dialog.
    virtual void showError(const std::string& message) = 0;

    // Show info dialog.
    virtual void showInfo(const std::string& message) = 0;
};

#endif // I_AUTH_DIALOG_H
