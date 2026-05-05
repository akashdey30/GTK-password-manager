// ============================================================
// auth/master_auth.cpp
// ============================================================
#include "../../include/auth/master_auth.h"
#include <openssl/crypto.h>
#include <gtk/gtk.h>
#include <thread>
#include <chrono>

constexpr int MasterAuth::BACKOFF_MS[];

MasterAuth::MasterAuth(IAuthDialog&    dialog,
                       IMasterStorage& storage,
                       ILogger&        logger)
    : dialog_(dialog), storage_(storage), logger_(logger)
{}

// ── promptMasterPassword ──────────────────────────────────────
AppResult MasterAuth::promptMasterPassword(AppState& app) {
    if (lockedOut_) {
        dialog_.showError(
            "Too many failed attempts. Application is locked.\n"
            "Please restart the application.");
        logger_.log(LogLevel::WARNING,
                    "Login attempted while vault is locked out");
        return AppResult::ErrAuthLocked;
    }

    // ── First run: setup ──────────────────────────────────────
    if (!storage_.masterExists()) {
        logger_.log(LogLevel::INFO, "First run — showing master password setup");
        SetupResult result;
        if (!dialog_.promptSetupMaster(result)) {
            logger_.log(LogLevel::INFO, "User cancelled setup");
            return AppResult::ErrAuth;
        }
        AppResult r = storage_.saveMaster(result.password, result.recovery);
        if (!isOk(r)) {
            dialog_.showError("Failed to save master password. Check disk space.");
            logger_.log(LogLevel::ERROR_LEVEL, "Failed to save master password");
            return r;
        }
        // Derive session key
        app.sessionMaster = result.password;
        app.sessionKey    = storage_.deriveKey(result.password);
        // Cleanse plaintext password from result struct
        OPENSSL_cleanse(&result.password[0], result.password.size());
        app.isLocked      = false;
        resetLockTimer(app);
        logger_.log(LogLevel::INFO, "Master password set successfully on first run");
        return AppResult::OK;
    }

    // ── Subsequent runs: login ────────────────────────────────
    while (true) {
        if (lockedOut_) {
            dialog_.showError(
                "Too many failed attempts. Application is locked.\n"
                "Please restart.");
            logger_.log(LogLevel::WARNING, "Vault locked after max failures");
            return AppResult::ErrAuthLocked;
        }

        std::string input;
        bool        forgotPassword = false;

        if (!dialog_.promptVerifyMaster(input, forgotPassword)) {
            logger_.log(LogLevel::INFO, "User cancelled login");
            return AppResult::ErrAuth;
        }

        if (forgotPassword) {
            AppResult r = runRecoveryFlow(app);
            if (isOk(r)) return r;
            continue;
        }

        if (attemptLogin(input, app)) {
            // Cleanse plaintext
            OPENSSL_cleanse(&input[0], input.size());
            return AppResult::OK;
        }
        // attemptLogin already showed the error
        OPENSSL_cleanse(&input[0], input.size());
    }
}

// ── attemptLogin ──────────────────────────────────────────────
bool MasterAuth::attemptLogin(const std::string& password, AppState& app) {
    if (storage_.verifyMaster(password)) {
        app.sessionMaster = password;
        app.sessionKey    = storage_.deriveKey(password);
        app.isLocked      = false;
        failureCount_     = 0;
        resetLockTimer(app);
        logger_.log(LogLevel::INFO, "Successful login");
        return true;
    }

    failureCount_++;
    logger_.log(LogLevel::WARNING,
                "Failed login attempt " + std::to_string(failureCount_) +
                "/" + std::to_string(MAX_AUTH_FAILURES));

    if (failureCount_ >= MAX_AUTH_FAILURES) {
        lockedOut_ = true;
        app.isLockedOut = true;
        logger_.log(LogLevel::ERROR_LEVEL,
                    "Vault locked after " + std::to_string(MAX_AUTH_FAILURES) +
                    " failed attempts");
        dialog_.showError(
            "Maximum login attempts exceeded.\n"
            "Application is now locked. Please restart.");
        return false;
    }

    // Apply exponential backoff
    int idx = std::min(failureCount_ - 1,
                       static_cast<int>(sizeof(BACKOFF_MS)/sizeof(BACKOFF_MS[0])) - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(BACKOFF_MS[idx]));

    std::string msg = "Incorrect password. "
                    + std::to_string(MAX_AUTH_FAILURES - failureCount_)
                    + " attempt(s) remaining.";
    dialog_.showError(msg);
    return false;
}

// ── runRecoveryFlow ───────────────────────────────────────────
AppResult MasterAuth::runRecoveryFlow(AppState& app) {
    logger_.log(LogLevel::INFO, "Recovery flow started");

    RecoveryData recovery;
    if (!dialog_.promptRecovery(recovery)) {
        logger_.log(LogLevel::INFO, "User cancelled recovery");
        return AppResult::ErrAuth;
    }

    if (!storage_.verifyRecovery(recovery)) {
        dialog_.showError("Recovery information does not match our records.");
        logger_.log(LogLevel::WARNING, "Failed recovery verification attempt");
        return AppResult::ErrAuth;
    }

    // Recovery verified — prompt for new password
    std::string newPass;
    if (!dialog_.promptNewPassword(newPass)) {
        return AppResult::ErrAuth;
    }

    AppResult r = storage_.resetMaster(newPass);
    if (!isOk(r)) {
        dialog_.showError("Failed to reset master password.");
        logger_.log(LogLevel::ERROR_LEVEL, "Failed to reset master password");
        OPENSSL_cleanse(&newPass[0], newPass.size());
        return r;
    }

    app.sessionMaster = newPass;
    app.sessionKey    = storage_.deriveKey(newPass);
    OPENSSL_cleanse(&newPass[0], newPass.size());
    app.isLocked  = false;
    failureCount_ = 0;
    lockedOut_    = false;
    resetLockTimer(app);

    dialog_.showInfo("Master password reset successfully. Welcome back.");
    logger_.log(LogLevel::INFO, "Master password reset via recovery flow");
    return AppResult::OK;
}

// ── changeMasterPassword ──────────────────────────────────────
AppResult MasterAuth::changeMasterPassword(AppState& app) {
    std::string current;
    bool        forgot = false;

    if (!dialog_.promptVerifyMaster(current, forgot)) return AppResult::ErrAuth;

    if (!storage_.verifyMaster(current)) {
        dialog_.showError("Current password is incorrect.");
        logger_.log(LogLevel::WARNING, "Failed master password change — wrong current");
        OPENSSL_cleanse(&current[0], current.size());
        return AppResult::ErrAuth;
    }
    OPENSSL_cleanse(&current[0], current.size());

    std::string newPass;
    if (!dialog_.promptNewPassword(newPass)) return AppResult::ErrAuth;

    // Build minimal recovery (reuse existing — no change to recovery data)
    RecoveryData dummy; // resetMaster preserves existing recovery
    AppResult r = storage_.resetMaster(newPass);
    if (!isOk(r)) {
        dialog_.showError("Failed to update master password.");
        logger_.log(LogLevel::ERROR_LEVEL, "Failed to update master password");
        OPENSSL_cleanse(&newPass[0], newPass.size());
        return r;
    }

    // Update session key
    app.clearSession();
    app.sessionMaster = newPass;
    app.sessionKey    = storage_.deriveKey(newPass);
    OPENSSL_cleanse(&newPass[0], newPass.size());

    dialog_.showInfo("Master password changed successfully.");
    logger_.log(LogLevel::INFO, "Master password changed");
    return AppResult::OK;
}

// ── Auto-lock ─────────────────────────────────────────────────
void MasterAuth::resetLockTimer(AppState& app) {
    if (app.autoLockTimerId != 0) {
        g_source_remove(app.autoLockTimerId);
        app.autoLockTimerId = 0;
    }
    if (app.autoLockSeconds > 0) {
        app.autoLockTimerId = g_timeout_add_seconds(
            static_cast<guint>(app.autoLockSeconds),
            onLockTimeout,
            &app);
    }
}

AppResult MasterAuth::lockAndReauth(AppState& app) {
    if (app.autoLockTimerId != 0) {
        g_source_remove(app.autoLockTimerId);
        app.autoLockTimerId = 0;
    }
    app.isLocked = true;
    logger_.log(LogLevel::INFO, "Vault locked");

    // Disable UI while locked
    if (app.vboxServices)
        gtk_widget_set_sensitive(app.vboxServices, FALSE);
    if (app.addButton)
        gtk_widget_set_sensitive(app.addButton, FALSE);

    // Wipe session key while locked
    if (!app.sessionKey.empty()) {
        OPENSSL_cleanse(app.sessionKey.data(), app.sessionKey.size());
        app.sessionKey.clear();
    }

    std::string input;
    bool        forgot = false;
    if (!dialog_.promptVerifyMaster(input, forgot)) {
        gtk_main_quit();
        return AppResult::ErrAuth;
    }

    if (storage_.verifyMaster(input)) {
        app.sessionKey    = storage_.deriveKey(input);
        app.sessionMaster = input;
        OPENSSL_cleanse(&input[0], input.size());
        app.isLocked = false;
        if (app.vboxServices)
            gtk_widget_set_sensitive(app.vboxServices, TRUE);
        if (app.addButton)
            gtk_widget_set_sensitive(app.addButton, TRUE);
        resetLockTimer(app);
        logger_.log(LogLevel::INFO, "Vault unlocked successfully");
        return AppResult::OK;
    }

    OPENSSL_cleanse(&input[0], input.size());
    dialog_.showError("Incorrect password. Vault remains locked.");
    logger_.log(LogLevel::WARNING, "Failed unlock attempt");
    return AppResult::ErrAuth;
}

gboolean MasterAuth::onLockTimeout(gpointer userData) {
    AppState* app = static_cast<AppState*>(userData);
    app->autoLockTimerId = 0;

    GtkWidget* msg = gtk_message_dialog_new(
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK,
        "Vault locked due to inactivity.\nClick OK to re-authenticate.");
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);

    if (app->vboxServices)
        gtk_widget_set_sensitive(app->vboxServices, FALSE);
    if (app->addButton)
        gtk_widget_set_sensitive(app->addButton, FALSE);
    app->isLocked = true;

    return G_SOURCE_REMOVE;
}
