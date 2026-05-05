#pragma once
// ============================================================
// mvc/view/include/vault_view.h
//
// MVC ROLE: VIEW
// ─────────────────────────────────────────────────────────────
// Wraps VaultUI (GTK window builder) and wires all GTK signal
// callbacks to the Controller.
//
// The View:
//   • Constructs all GTK widgets (via VaultUI)
//   • Connects GTK signals → Controller action handlers
//   • Has NO business logic and NO storage calls
//   • Receives a VaultController* to forward events
// ============================================================
#ifndef VAULT_VIEW_H
#define VAULT_VIEW_H

#include "../../../include/app_state.h"
#include "../../../include/ui/ui.h"
#include "../../../include/ui/service_manager.h"

// Forward declaration to avoid circular dependency
class VaultController;

class VaultView {
public:
    // Build the GTK window and wire signals.
    // controller must outlive VaultView.
    static AppState* buildAndWire(VaultController* controller,
                                  ServiceManager*  svcMgr);

private:
    // GTK static signal adapters — bridge GTK C callbacks to Controller
    static void onAddClicked      (GtkButton*,      gpointer ctrl);
    static void onExportClicked   (GtkButton*,      gpointer ctrl);
    static void onImportClicked   (GtkButton*,      gpointer ctrl);
    static void onLockClicked     (GtkButton*,      gpointer ctrl);
    static void onChangePwdClicked(GtkButton*,      gpointer ctrl);
};

#endif // VAULT_VIEW_H
