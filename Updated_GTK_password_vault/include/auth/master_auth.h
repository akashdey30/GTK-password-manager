#pragma once
// ============================================================
// auth/master_auth.h
// SRP: Authentication logic, brute-force lockout, session.
// DIP: Depends on IAuthDialog, IMasterStorage, ILogger.
// ============================================================
#ifndef MASTER_AUTH_H
#define MASTER_AUTH_H

#include "../app_state.h"
#include "i_auth_dialog.h"
#include "../db/i_storage.h"
#include "../utils/i_logger.h"

class MasterAuth {
public:
    // DIP: All dependencies injected — never self-created.
    MasterAuth(IAuthDialog&   dialog,
               IMasterStorage& storage,
               ILogger&        logger);

    // First-run setup or login prompt.
    // Returns AppResult::OK on success.
    // Returns AppResult::ErrAuthLocked if brute-force locked.
    AppResult promptMasterPassword(AppState& app);

    // Change master password (requires current password verification).
    AppResult changeMasterPassword(AppState& app);

    // Reset auto-lock countdown timer.
    void resetLockTimer(AppState& app);

    // Lock vault and force re-authentication.
    AppResult lockAndReauth(AppState& app);

    // Static GTK timeout callback for auto-lock.
    static gboolean onLockTimeout(gpointer userData);

private:
    IAuthDialog&    dialog_;
    IMasterStorage& storage_;
    ILogger&        logger_;

    // Brute-force lockout state
    int  failureCount_  = 0;
    bool lockedOut_     = false;

    // Backoff delay table (ms): 200, 400, 800, 1600, locked
    static constexpr int BACKOFF_MS[] = {200, 400, 800, 1600};

    // Attempt login with backoff; returns true on success.
    bool attemptLogin(const std::string& password, AppState& app);

    // Run the "Forgot Password?" recovery flow.
    AppResult runRecoveryFlow(AppState& app);
};

#endif // MASTER_AUTH_H
