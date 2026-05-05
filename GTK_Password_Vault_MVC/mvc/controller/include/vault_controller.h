#pragma once
// ============================================================
// mvc/controller/include/vault_controller.h
//
// MVC ROLE: CONTROLLER
// ─────────────────────────────────────────────────────────────
// Bridges Model and View:
//   • Receives UI events from the View layer
//   • Calls Model operations (storage, auth, crypto)
//   • Instructs View to update/refresh display
//
// The Controller owns no UI widgets and no raw data —
// it only orchestrates calls between the two layers.
// ============================================================
#ifndef VAULT_CONTROLLER_H
#define VAULT_CONTROLLER_H

#include "../../../include/app_state.h"
#include "../../../include/auth/master_auth.h"
#include "../../../include/ui/service_manager.h"
#include "../../../include/ui/gtk_dialog_provider.h"
#include <string>
#include <memory>

// Forward declaration
class VaultView;

class VaultController {
public:
    // ── Construction ─────────────────────────────────────────
    // Controller takes ownership of all Model and View objects.
    // All dependencies are injected — Controller never names concretes.
    VaultController(AppState&           app,
                    MasterAuth&         auth,
                    ServiceManager&     svcMgr,
                    GtkDialogProvider&  dialogs);

    // ── Lifecycle ─────────────────────────────────────────────

    // Called at startup: authenticate → load → show window.
    // Returns false if user cancels login or vault is locked out.
    bool startup();

    // Called on clean shutdown: wipe session data.
    void shutdown();

    // ── Action handlers (called by View signals) ──────────────

    // User clicked "+ Add Credential"
    void onAddCredential();

    // User typed in search box
    void onSearch(const std::string& query);

    // User clicked "Export CSV"
    void onExport();

    // User clicked "Import CSV"
    void onImport();

    // User clicked "Lock"
    void onLock();

    // User clicked "Change Password"
    void onChangeMasterPassword();

    // ── File picker utility (needed by export/import) ─────────
    std::string pickFile(bool save, const char* title);

private:
    // References — Controller does NOT own these (owned by main)
    AppState&           app_;
    MasterAuth&         auth_;
    ServiceManager&     svcMgr_;
    GtkDialogProvider&  dialogs_;
};

#endif // VAULT_CONTROLLER_H
