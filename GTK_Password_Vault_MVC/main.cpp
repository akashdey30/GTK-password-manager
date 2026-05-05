// ============================================================
// main.cpp — MVC Composition Root (REFACTORED)
//
// ┌─────────────────────────────────────────────────────────┐
// │                  MVC ARCHITECTURE MAP                   │
// ├──────────────┬──────────────────────────────────────────┤
// │  MODEL       │ SqliteVaultStorage  — data persistence    │
// │              │ AesGcmEncryption    — encryption logic    │
// │              │ PasswordStrength    — strength analysis   │
// │              │ MasterAuth          — auth business logic  │
// │              │ ServiceManager      — credential CRUD      │
// │              │ FileLogger          — audit logging        │
// ├──────────────┼──────────────────────────────────────────┤
// │  VIEW        │ VaultUI             — GTK widget factory  │
// │              │ GtkDialogProvider   — all GTK dialogs     │
// ├──────────────┼──────────────────────────────────────────┤
// │  CONTROLLER  │ VaultController     — event dispatcher    │
// │              │   onAddCredential() │ onSearch()          │
// │              │   onExport()        │ onImport()          │
// │              │   onLock()          │ onChangePwd()       │
// └──────────────┴──────────────────────────────────────────┘
//
// MVC Data Flow:
//   GTK signal → (View adapter) → Controller method
//             → Model operation → (AppState update)
//             → View refresh (refreshServiceList / dialog)
// ============================================================

#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <filesystem>

// ── Shared types ──────────────────────────────────────────────
#include "include/global_types.h"
#include "include/app_state.h"

// ── MODEL layer ───────────────────────────────────────────────
#include "include/crypto/aes_gcm_encryption.h"
#include "include/security/password_strength_evaluator.h"
#include "include/utils/file_logger.h"
#include "include/db/sqlite_vault_storage.h"
#include "include/auth/master_auth.h"
#include "include/ui/service_manager.h"

// ── VIEW layer ────────────────────────────────────────────────
#include "include/ui/gtk_dialog_provider.h"
#include "include/ui/ui.h"

// ── CONTROLLER layer ──────────────────────────────────────────
#include "mvc/controller/include/vault_controller.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    std::filesystem::create_directories("data");

    // ══════════════════════════════════════════════════════════
    // STEP 1 — MODEL: Construct all concrete implementations
    // These are the ONLY lines naming concrete types.
    // All high-level classes see only their interface.
    // ══════════════════════════════════════════════════════════

    AesGcmEncryption          encryption;         // MODEL: IEncryption
    PasswordStrengthEvaluator strengthEval;        // MODEL: IPasswordStrengthEvaluator
    FileLogger                logger(LOG_FILE);    // MODEL: ILogger
    SqliteVaultStorage        storage(encryption); // MODEL: IMasterStorage + ICredentialStorage
    MasterAuth                masterAuth(
        // GtkDialogProvider satisfies IAuthDialog (VIEW-provided interface)
        // constructed below — forward-referenced via temp; fixed in step 3
        *reinterpret_cast<GtkDialogProvider*>(1), // placeholder, replaced below
        storage, logger);

    // ══════════════════════════════════════════════════════════
    // STEP 2 — VIEW: Build GTK window
    // VaultUI::createApp builds ALL GTK widgets. No logic here.
    // Placeholder callbacks — real ones wired in Step 4.
    // ══════════════════════════════════════════════════════════
    VaultUI::Callbacks placeholder;
    placeholder.onAdd       = []{};
    placeholder.onSearch    = [](const std::string&){};
    placeholder.onExport    = []{};
    placeholder.onImport    = []{};
    placeholder.onLock      = []{};
    placeholder.onChangePwd = []{};

    AppState* rawApp = VaultUI::createApp(placeholder);
    std::unique_ptr<AppState> app(rawApp);

    // ══════════════════════════════════════════════════════════
    // STEP 3 — VIEW: Build dialog provider with real parent window
    // GtkDialogProvider is VIEW layer — all GTK dialog rendering.
    // It satisfies IAuthDialog (used by MasterAuth/MODEL)
    // and ICredentialDialog (used by ServiceManager/MODEL).
    // ══════════════════════════════════════════════════════════
    GtkDialogProvider dialogs(GTK_WINDOW(app->window), strengthEval); // VIEW

    // Now rebuild Model objects with real dialog reference
    MasterAuth  realMasterAuth(dialogs, storage, logger);  // MODEL
    ServiceManager svcMgr(storage, dialogs, logger);        // MODEL

    // ══════════════════════════════════════════════════════════
    // STEP 4 — CONTROLLER: Construct with refs to Model + View
    // Controller is the ONLY class that holds references to both.
    // ══════════════════════════════════════════════════════════
    VaultController controller(*app, realMasterAuth, svcMgr, dialogs); // CONTROLLER

    // ══════════════════════════════════════════════════════════
    // STEP 5 — VIEW: Wire all GTK signals → CONTROLLER methods
    // View has NO logic — just forwards signals to Controller.
    // ══════════════════════════════════════════════════════════

    // Disconnect placeholder add-button signal from Step 2
    g_signal_handlers_disconnect_matched(
        app->addButton, G_SIGNAL_MATCH_DATA,
        0, 0, nullptr, nullptr, nullptr);

    // Add button → Controller.onAddCredential()
    g_signal_connect(app->addButton, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer ctrl){
            static_cast<VaultController*>(ctrl)->onAddCredential();
        }), &controller);

    // Export / Import / Lock / ChangePwd via VoidCallback object-data
    // (VaultUI's existing button handlers pick these up by key name)
    auto* exportCb    = new VoidCallback([&controller]{ controller.onExport(); });
    auto* importCb    = new VoidCallback([&controller]{ controller.onImport(); });
    auto* lockCb      = new VoidCallback([&controller]{ controller.onLock(); });
    auto* changePwdCb = new VoidCallback([&controller]{ controller.onChangeMasterPassword(); });

    g_object_set_data(G_OBJECT(app->window), "export-cb",    exportCb);
    g_object_set_data(G_OBJECT(app->window), "import-cb",    importCb);
    g_object_set_data(G_OBJECT(app->window), "lock-cb",      lockCb);
    g_object_set_data(G_OBJECT(app->window), "changepwd-cb", changePwdCb);

    // Search entry → ServiceManager GTK bridge (calls controller.onSearch internally)
    VaultUI::wireSearch(app.get(), &svcMgr);

    // ══════════════════════════════════════════════════════════
    // STEP 6 — CONTROLLER: Authenticate
    // Flow: Controller → Model (MasterAuth) → View (dialogs)
    // ══════════════════════════════════════════════════════════
    AppResult authResult = realMasterAuth.promptMasterPassword(*app);
    if (!isOk(authResult)) {
        logger.log(LogLevel::INFO, "Exiting — login cancelled or vault locked");
        app->clearSession();
        delete exportCb; delete importCb;
        delete lockCb;   delete changePwdCb;
        return (authResult == AppResult::ErrAuthLocked) ? 1 : 0;
    }

    // ══════════════════════════════════════════════════════════
    // STEP 7 — CONTROLLER: Load credentials → VIEW refresh
    // Flow: Controller → Model (storage.loadAll) → AppState
    //                 → View (addServiceButton per credential)
    // ══════════════════════════════════════════════════════════
    svcMgr.refreshServiceList(*app);

    // ══════════════════════════════════════════════════════════
    // STEP 8 — VIEW: Show window and run GTK event loop
    // From here all interactions follow MVC data flow:
    //   User action → GTK signal → View adapter
    //              → Controller method → Model operation
    //              → View update (dialog / list refresh)
    // ══════════════════════════════════════════════════════════
    gtk_widget_show_all(app->window);
    gtk_main();

    // ══════════════════════════════════════════════════════════
    // STEP 9 — Clean shutdown via Controller
    // ══════════════════════════════════════════════════════════
    controller.shutdown();
    delete exportCb; delete importCb;
    delete lockCb;   delete changePwdCb;
    logger.log(LogLevel::INFO, "Clean MVC shutdown");
    return 0;
}
