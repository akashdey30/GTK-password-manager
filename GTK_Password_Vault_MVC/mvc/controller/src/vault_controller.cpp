// ============================================================
// mvc/controller/src/vault_controller.cpp
//
// MVC ROLE: CONTROLLER
// ─────────────────────────────────────────────────────────────
// All action handlers live here. Each handler:
//   1. Calls MODEL (MasterAuth / ServiceManager / Storage)
//   2. Calls VIEW (GtkDialogProvider) to show feedback
//   3. Triggers VIEW refresh (refreshServiceList)
//
// No GTK widget construction happens here — that belongs to View.
// No raw SQLite/crypto calls happen here — that belongs to Model.
// ============================================================
#include "../include/vault_controller.h"
#include <gtk/gtk.h>
#include <string>

// ── Constructor ───────────────────────────────────────────────
VaultController::VaultController(AppState&          app,
                                  MasterAuth&        auth,
                                  ServiceManager&    svcMgr,
                                  GtkDialogProvider& dialogs)
    : app_(app), auth_(auth), svcMgr_(svcMgr), dialogs_(dialogs)
{}

// ── startup ───────────────────────────────────────────────────
// MVC flow:
//   Controller → Model (auth.promptMasterPassword)
//   Controller → Model (svcMgr.refreshServiceList)
//   Controller → View  (gtk_widget_show_all)
bool VaultController::startup() {
    // Step 1: Authenticate via Model layer
    AppResult authResult = auth_.promptMasterPassword(app_);
    if (!isOk(authResult)) {
        return false;
    }

    // Step 2: Load credentials from Model into AppState cache
    svcMgr_.refreshServiceList(app_);

    // Step 3: Tell View to show the main window
    gtk_widget_show_all(app_.window);

    return true;
}

// ── shutdown ──────────────────────────────────────────────────
void VaultController::shutdown() {
    // Wipe all sensitive data held in AppState (Model layer)
    app_.clearSession();
}

// ── onAddCredential ───────────────────────────────────────────
// MVC flow:
//   View fires signal → Controller.onAddCredential()
//   Controller → Model (svcMgr.openAddDialog wraps storage.save)
//   Controller → Model (auth.resetLockTimer)
//   Model notifies Controller (via AppResult)
//   Controller → View (refreshServiceList already done inside svcMgr)
void VaultController::onAddCredential() {
    AppResult r = svcMgr_.openAddDialog(app_);
    if (isOk(r)) {
        // Reset auto-lock timer in Model since user is active
        auth_.resetLockTimer(app_);
    }
}

// ── onSearch ──────────────────────────────────────────────────
// MVC flow:
//   View fires search-changed → Controller.onSearch(query)
//   Controller → Model (svcMgr.applyFilter reads from storage)
//   View list is rebuilt inside applyFilter
void VaultController::onSearch(const std::string& query) {
    svcMgr_.applyFilter(app_, query);
}

// ── onExport ──────────────────────────────────────────────────
// MVC flow:
//   View fires export button → Controller.onExport()
//   Controller → View (pickFile dialog to get path)
//   Controller → Model (svcMgr.exportCSV)
//   Controller → View (dialogs.showInfo / showError)
void VaultController::onExport() {
    std::string path = pickFile(true, "Export Credentials");
    if (path.empty()) return;

    AppResult r = svcMgr_.exportCSV(app_, path);
    if (isOk(r))
        dialogs_.showInfo("Exported to:\n" + path);
    else
        dialogs_.showError("Export failed.");
}

// ── onImport ──────────────────────────────────────────────────
// MVC flow:
//   View fires import button → Controller.onImport()
//   Controller → View (pickFile dialog)
//   Controller → Model (svcMgr.importCSV → storage.importCSV)
//   Controller → View (dialogs.showInfo / showError)
void VaultController::onImport() {
    std::string path = pickFile(false, "Import Credentials");
    if (path.empty()) return;

    AppResult r = svcMgr_.importCSV(app_, path);
    if (isOk(r))
        dialogs_.showInfo("Import complete.");
    else
        dialogs_.showError("Import failed. Check CSV format.");
}

// ── onLock ────────────────────────────────────────────────────
// MVC flow:
//   View fires lock button → Controller.onLock()
//   Controller → Model (auth.lockAndReauth)
//   Model triggers View re-auth dialog internally (via IAuthDialog)
void VaultController::onLock() {
    auth_.lockAndReauth(app_);
}

// ── onChangeMasterPassword ────────────────────────────────────
// MVC flow:
//   View fires change-pwd button → Controller.onChangeMasterPassword()
//   Controller → Model (auth.changeMasterPassword)
//   Model uses IAuthDialog (View interface) to gather input
void VaultController::onChangeMasterPassword() {
    auth_.changeMasterPassword(app_);
}

// ── pickFile ──────────────────────────────────────────────────
// This is VIEW-adjacent (shows a GTK dialog) but belongs in
// Controller because it acts on user intent before calling Model.
// The GTK file chooser is UI infrastructure, not business logic.
std::string VaultController::pickFile(bool save, const char* title) {
    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        title,
        GTK_WINDOW(app_.window),
        save ? GTK_FILE_CHOOSER_ACTION_SAVE
             : GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        save ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
        nullptr);

    GtkFileFilter* f = gtk_file_filter_new();
    gtk_file_filter_add_pattern(f, "*.csv");
    gtk_file_filter_set_name(f, "CSV files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), f);

    if (save)
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg),
                                          "vault_export.csv");

    std::string path;
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        gchar* p = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        if (p) { path = p; g_free(p); }
    }
    gtk_widget_destroy(dlg);
    return path;
}
