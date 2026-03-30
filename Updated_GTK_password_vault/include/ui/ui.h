#pragma once
// ============================================================
// ui/ui.h
// SRP: GTK main window construction and layout only.
// Note: Class named VaultUI to avoid clash with OpenSSL's UI typedef.
// ============================================================
#ifndef UI_H
#define UI_H

#include "../app_state.h"
#include <functional>
#include <string>

// Forward declaration to break circular dependency
class ServiceManager;

class VaultUI {
public:
    struct Callbacks {
        VoidCallback   onAdd;
        StringCallback onSearch;
        VoidCallback   onExport;
        VoidCallback   onImport;
        VoidCallback   onLock;
        VoidCallback   onChangePwd;
    };

    // Build and return an initialised AppState with the GTK window.
    static AppState* createApp(const Callbacks& cbs);

    // Wire search entry to search-changed with real ServiceManager callback.
    static void wireSearch(AppState* app, ServiceManager* mgr);

private:
    struct AddBtnData {
        VoidCallback cb;
    };

    static void onAddClicked(GtkButton* btn, gpointer data);
};

#endif // UI_H
