#pragma once
// ============================================================
// app_state.h
// SRP: Shared runtime state container only — no logic.
// ============================================================
#ifndef APP_STATE_H
#define APP_STATE_H

#include "global_types.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <openssl/crypto.h>

struct AppState {
    // ── GTK widgets ───────────────────────────────────────────
    GtkWidget* window        = nullptr;
    GtkWidget* vboxServices  = nullptr;
    GtkWidget* labelCount    = nullptr;
    GtkWidget* addButton     = nullptr;
    GtkWidget* searchEntry   = nullptr;
    GtkWidget* scrolled      = nullptr;

    // ── Session data (sensitive — wiped on exit) ──────────────
    std::string              sessionMaster;   // plaintext — wiped on exit
    std::vector<unsigned char> sessionKey;   // AES key   — wiped on exit
    std::string              recoveryAnswer;

    // ── Credential cache ──────────────────────────────────────
    std::vector<Credential> credentials;

    // ── Lock state ────────────────────────────────────────────
    bool     isLocked        = false;
    int      failureCount    = 0;
    bool     isLockedOut     = false;
    guint    autoLockTimerId = 0;
    int      autoLockSeconds = AUTO_LOCK_SECONDS;
    guint    clipboardTimerId = 0;

    // ── Callbacks ─────────────────────────────────────────────
    std::function<void(int)> onDeleteCallback;

    AppState() = default;
    ~AppState() { clearSession(); }

    // Wipe all sensitive data from memory
    void clearSession() {
        if (!sessionMaster.empty()) {
            OPENSSL_cleanse(&sessionMaster[0], sessionMaster.size());
            sessionMaster.clear();
        }
        if (!sessionKey.empty()) {
            OPENSSL_cleanse(sessionKey.data(), sessionKey.size());
            sessionKey.clear();
        }
        if (!recoveryAnswer.empty()) {
            OPENSSL_cleanse(&recoveryAnswer[0], recoveryAnswer.size());
            recoveryAnswer.clear();
        }
    }
};

#endif // APP_STATE_H
