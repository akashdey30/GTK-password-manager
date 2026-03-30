#pragma once
// ============================================================
// ui/service_manager.h
// SRP: Credential CRUD, filtering, and UI list management.
// DIP: Depends on ICredentialStorage, ICredentialDialog, ILogger.
// ============================================================
#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "../app_state.h"
#include "i_credential_dialog.h"
#include "../db/i_storage.h"
#include "../utils/i_logger.h"
#include <gtk/gtk.h>
#include <string>

class ServiceManager {
public:
    ServiceManager(ICredentialStorage& storage,
                   ICredentialDialog&  dialog,
                   ILogger&            logger);

    // Rebuild the full GTK service-button list from app.credentials.
    void refreshServiceList(AppState& app);

    // Open "Add Credential" dialog and persist on confirm.
    AppResult openAddDialog(AppState& app);

    // Filter the displayed list by query string.
    void applyFilter(AppState& app, const std::string& query);

    // Export credentials to CSV.
    AppResult exportCSV(AppState& app, const std::string& filepath);

    // Import credentials from CSV.
    AppResult importCSV(AppState& app, const std::string& filepath);

    // GTK static signal callback for search-changed.
    static void onSearchChanged(GtkSearchEntry* entry, gpointer userData);

    // GTK static signal callback for service-button clicked.
    static void onServiceClicked(GtkWidget* btn, gpointer userData);

private:
    ICredentialStorage& storage_;
    ICredentialDialog&  dialog_;
    ILogger&            logger_;

    // Data passed to onServiceClicked
    struct ServiceClickData {
        AppState*       app;
        ServiceManager* mgr;
        std::string     service;
    };

    void updateCountLabel(AppState& app);
    void addServiceButton(AppState& app, const std::string& service);
    void showServiceCredentials(AppState& app, const std::string& service);
};

#endif // SERVICE_MANAGER_H
