// ============================================================
// main.cpp — Composition Root
//
// SOLID compliance:
//   SRP — Each class has one reason to change.
//   OCP — New backends/ciphers require only new subclasses + one line here.
//   LSP — All interface implementations fully satisfy their contracts.
//   ISP — MasterAuth sees only IMasterStorage; ServiceManager sees only
//          ICredentialStorage. No class depends on methods it doesn't use.
//   DIP — Every dependency is injected here. No high-level class
//          instantiates a concrete dependency internally.
//
// AppState is constructed ONCE. All objects share the same instance.
// ============================================================
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <filesystem>

#include "include/global_types.h"
#include "include/app_state.h"
#include "include/crypto/aes_gcm_encryption.h"
#include "include/security/password_strength_evaluator.h"
#include "include/utils/file_logger.h"
#include "include/db/sqlite_vault_storage.h"
#include "include/auth/master_auth.h"
#include "include/ui/gtk_dialog_provider.h"
#include "include/ui/service_manager.h"
#include "include/ui/ui.h"

// ── File picker utility ───────────────────────────────────────
static std::string pickFile(GtkWindow* parent, bool save, const char* title) {
    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        title, parent,
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

// ── main ──────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    std::filesystem::create_directories("data");

    // ── 1. Construct all concrete implementations ─────────────
    // These are the ONLY lines in the project that name concrete types.

    AesGcmEncryption          encryption;         // satisfies IEncryption
    PasswordStrengthEvaluator strengthEval;        // satisfies IPasswordStrengthEvaluator
    FileLogger                logger(LOG_FILE);    // satisfies ILogger

    // DIP: AES injected into storage constructor — storage never names the cipher
    SqliteVaultStorage storage(encryption);        // satisfies IMasterStorage + ICredentialStorage

    // ── 2. Build AppState with placeholder callbacks ──────────
    // We build the GTK window before the high-level objects because
    // GtkDialogProvider needs a parent GtkWindow* to pass to dialog calls.
    // Placeholders are replaced in step 4 after all objects exist.
    VaultUI::Callbacks placeholder;
    placeholder.onAdd      = []{};
    placeholder.onSearch   = [](const std::string&){};
    placeholder.onExport   = []{};
    placeholder.onImport   = []{};
    placeholder.onLock     = []{};
    placeholder.onChangePwd= []{};

    // Single AppState construction — never deleted or recreated
    AppState* rawApp = VaultUI::createApp(placeholder);
    std::unique_ptr<AppState> app(rawApp);

    // ── 3. Build dialog providers (need GtkWindow*) ───────────
    // DIP: strengthEval injected — GtkDialogProvider never constructs it.
    GtkDialogProvider dialogs(GTK_WINDOW(app->window), strengthEval);

    // ── 4. Build high-level classes — inject all abstractions ──
    MasterAuth     masterAuth(dialogs, storage, logger);
    ServiceManager svcMgr(storage, dialogs, logger);

    // ── 5. Wire real callbacks now that all objects exist ─────
    // Disconnect the placeholder add-button signal and connect the real one.
    g_signal_handlers_disconnect_matched(
        app->addButton,
        G_SIGNAL_MATCH_DATA,
        0, 0, nullptr, nullptr, nullptr);

    // Real add callback
    auto addCb = new VoidCallback([&]() {
        AppResult r = svcMgr.openAddDialog(*app);
        if (isOk(r)) masterAuth.resetLockTimer(*app);
    });
    g_signal_connect(app->addButton, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer ud){
            (*static_cast<VoidCallback*>(ud))();
        }), addCb);

    // Wire search entry
    VaultUI::wireSearch(app.get(), &svcMgr);

    // Wire export button callback via object data
    auto exportCb = new VoidCallback([&]() {
        std::string path = pickFile(GTK_WINDOW(app->window), true,
                                    "Export Credentials");
        if (!path.empty()) {
            AppResult r = svcMgr.exportCSV(*app, path);
            if (isOk(r)) dialogs.showInfo("Exported to:\n" + path);
            else          dialogs.showError("Export failed.");
        }
    });
    auto importCb = new VoidCallback([&]() {
        std::string path = pickFile(GTK_WINDOW(app->window), false,
                                    "Import Credentials");
        if (!path.empty()) {
            AppResult r = svcMgr.importCSV(*app, path);
            if (isOk(r)) dialogs.showInfo("Import complete.");
            else          dialogs.showError("Import failed. Check CSV format.");
        }
    });
    auto lockCb = new VoidCallback([&]() {
        masterAuth.lockAndReauth(*app);
    });
    auto changePwdCb = new VoidCallback([&]() {
        masterAuth.changeMasterPassword(*app);
    });

    // Store real callbacks in window object data.
    // Button signal handlers retrieve them via g_object_get_data.
    g_object_set_data(G_OBJECT(app->window), "export-cb", exportCb);
    g_object_set_data(G_OBJECT(app->window), "import-cb", importCb);
    g_object_set_data(G_OBJECT(app->window), "lock-cb",   lockCb);
    g_object_set_data(G_OBJECT(app->window), "changepwd-cb", changePwdCb);

    // ── 6. Authenticate ───────────────────────────────────────
    AppResult authResult = masterAuth.promptMasterPassword(*app);
    if (!isOk(authResult)) {
        logger.log(LogLevel::INFO,
                   "Exiting — login cancelled or vault locked");
        app->clearSession();
        delete addCb; delete exportCb;
        delete importCb; delete lockCb; delete changePwdCb;
        return (authResult == AppResult::ErrAuthLocked) ? 1 : 0;
    }

    // ── 7. Load credentials into UI ───────────────────────────
    svcMgr.refreshServiceList(*app);

    // ── 8. Show window and run event loop ─────────────────────
    gtk_widget_show_all(app->window);
    gtk_main();

    // ── 9. Clean shutdown — wipe all sensitive data ───────────
    app->clearSession();
    delete addCb; delete exportCb;
    delete importCb; delete lockCb; delete changePwdCb;
    logger.log(LogLevel::INFO, "Clean shutdown");
    return 0;
}
