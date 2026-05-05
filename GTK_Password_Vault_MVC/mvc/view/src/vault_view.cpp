// ============================================================
// mvc/view/src/vault_view.cpp
//
// MVC ROLE: VIEW
// ─────────────────────────────────────────────────────────────
// buildAndWire() is the single entry-point:
//   1. Calls VaultUI::createApp() to build all GTK widgets
//   2. Disconnects placeholder signals from VaultUI
//   3. Connects real GTK signals → Controller methods
//   4. Wires search entry via ServiceManager callback
//
// This file contains ZERO business logic. All signal handlers
// are one-liners that delegate to VaultController.
// ============================================================
#include "../include/vault_view.h"
#include "../../../mvc/controller/include/vault_controller.h"
#include <gtk/gtk.h>

// ── buildAndWire ──────────────────────────────────────────────
AppState* VaultView::buildAndWire(VaultController* controller,
                                   ServiceManager*  svcMgr) {
    // ── Step 1: Build GTK window (View responsibility) ────────
    // VaultUI::createApp() is the GTK widget factory — pure View.
    // We pass empty placeholder callbacks; real ones wired below.
    VaultUI::Callbacks placeholder;
    placeholder.onAdd       = []{};
    placeholder.onSearch    = [](const std::string&){};
    placeholder.onExport    = []{};
    placeholder.onImport    = []{};
    placeholder.onLock      = []{};
    placeholder.onChangePwd = []{};

    AppState* app = VaultUI::createApp(placeholder);

    // ── Step 2: Disconnect placeholder add-button signal ──────
    g_signal_handlers_disconnect_matched(
        app->addButton,
        G_SIGNAL_MATCH_DATA,
        0, 0, nullptr, nullptr, nullptr);

    // ── Step 3: Wire all button signals → Controller ──────────
    // Each GTK signal points to a static C-style adapter that
    // casts gpointer back to VaultController* and calls the
    // appropriate Controller action handler.

    g_signal_connect(app->addButton, "clicked",
                     G_CALLBACK(onAddClicked), controller);

    // Export / Import / Lock / Change Password buttons retrieve
    // controller pointer from window object data (set here).
    g_object_set_data(G_OBJECT(app->window), "mvc-controller", controller);

    // Connect toolbar buttons via window object data pattern
    // (same pattern used by VaultUI for the window-scoped buttons)
    GList* children = gtk_container_get_children(
        GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(app->window))));
    // Walk the widget tree to reconnect — simpler to store via object data
    // and let the existing handler stubs retrieve ctrl from window data.
    // The buttons in VaultUI already retrieve callbacks from window object data;
    // we store lambda-wrapped VoidCallback objects keyed the same way.
    g_list_free(children);

    auto* exportCb    = new VoidCallback([controller]{ controller->onExport(); });
    auto* importCb    = new VoidCallback([controller]{ controller->onImport(); });
    auto* lockCb      = new VoidCallback([controller]{ controller->onLock(); });
    auto* changePwdCb = new VoidCallback([controller]{ controller->onChangeMasterPassword(); });

    g_object_set_data(G_OBJECT(app->window), "export-cb",    exportCb);
    g_object_set_data(G_OBJECT(app->window), "import-cb",    importCb);
    g_object_set_data(G_OBJECT(app->window), "lock-cb",      lockCb);
    g_object_set_data(G_OBJECT(app->window), "changepwd-cb", changePwdCb);

    // ── Step 4: Wire search entry → ServiceManager ────────────
    // ServiceManager.onSearchChanged is the GTK-C callback bridge;
    // it calls svcMgr->applyFilter internally — still Controller territory
    // but routed through ServiceManager for GTK compatibility.
    VaultUI::wireSearch(app, svcMgr);

    return app;
}

// ── GTK static signal adapters ────────────────────────────────
// These are pure bridge functions: GTK C API → C++ Controller.

void VaultView::onAddClicked(GtkButton*, gpointer ctrl) {
    static_cast<VaultController*>(ctrl)->onAddCredential();
}

void VaultView::onExportClicked(GtkButton*, gpointer ctrl) {
    static_cast<VaultController*>(ctrl)->onExport();
}

void VaultView::onImportClicked(GtkButton*, gpointer ctrl) {
    static_cast<VaultController*>(ctrl)->onImport();
}

void VaultView::onLockClicked(GtkButton*, gpointer ctrl) {
    static_cast<VaultController*>(ctrl)->onLock();
}

void VaultView::onChangePwdClicked(GtkButton*, gpointer ctrl) {
    static_cast<VaultController*>(ctrl)->onChangeMasterPassword();
}
