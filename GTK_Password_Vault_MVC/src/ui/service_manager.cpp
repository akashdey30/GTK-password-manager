// ============================================================
// ui/service_manager.cpp
// ============================================================
#include "../../include/ui/service_manager.h"
#include <algorithm>
#include <set>

ServiceManager::ServiceManager(ICredentialStorage& storage,
                                ICredentialDialog&  dialog,
                                ILogger&            logger)
    : storage_(storage), dialog_(dialog), logger_(logger)
{}

// ── Refresh full list ─────────────────────────────────────────
void ServiceManager::refreshServiceList(AppState& app) {
    // Clear existing buttons
    GList* children = gtk_container_get_children(
                          GTK_CONTAINER(app.vboxServices));
    for (GList* l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    // Reload from storage
    app.credentials = storage_.loadAll(app.sessionKey);

    // Collect unique services in sorted order
    std::set<std::string> seen;
    for (const auto& c : app.credentials) {
        if (seen.insert(c.service).second)
            addServiceButton(app, c.service);
    }
    updateCountLabel(app);
    gtk_widget_show_all(app.vboxServices);
}

// ── addServiceButton ──────────────────────────────────────────
void ServiceManager::addServiceButton(AppState& app,
                                       const std::string& service) {
    GtkWidget* btn = gtk_button_new_with_label(service.c_str());
    gtk_widget_set_halign(btn, GTK_ALIGN_FILL);

    auto* data     = new ServiceClickData{&app, this, service};
    g_signal_connect(btn, "clicked",
                     G_CALLBACK(onServiceClicked), data);
    // Free data when button is destroyed
    g_object_set_data_full(G_OBJECT(btn), "click-data", data,
        [](gpointer p){ delete static_cast<ServiceClickData*>(p); });

    gtk_box_pack_start(GTK_BOX(app.vboxServices), btn, FALSE, FALSE, 2);
}

// ── Open add dialog ───────────────────────────────────────────
AppResult ServiceManager::openAddDialog(AppState& app) {
    Credential cred;
    if (!dialog_.promptAddCredential(cred)) return AppResult::ErrAuth;

    AppResult r = storage_.save(cred, app.sessionKey);
    if (!isOk(r)) {
        logger_.log(LogLevel::ERROR_LEVEL,
                    "Failed to save credential for: " + cred.service);
        return r;
    }
    logger_.log(LogLevel::INFO, "Credential added: " + cred.service);
    refreshServiceList(app);
    return AppResult::OK;
}

// ── Filter by query ───────────────────────────────────────────
void ServiceManager::applyFilter(AppState& app, const std::string& query) {
    GList* children = gtk_container_get_children(
                          GTK_CONTAINER(app.vboxServices));
    for (GList* l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    if (query.empty()) {
        refreshServiceList(app);
        return;
    }

    auto results = storage_.search(query, app.sessionKey);
    std::set<std::string> seen;
    for (const auto& c : results) {
        if (seen.insert(c.service).second)
            addServiceButton(app, c.service);
    }
    gtk_widget_show_all(app.vboxServices);
}

// ── Export / Import ───────────────────────────────────────────
AppResult ServiceManager::exportCSV(AppState& app,
                                     const std::string& filepath) {
    AppResult r = storage_.exportCSV(filepath, app.sessionKey);
    if (isOk(r))
        logger_.log(LogLevel::INFO, "Exported credentials to " + filepath);
    else
        logger_.log(LogLevel::ERROR_LEVEL, "Export failed: " + filepath);
    return r;
}

AppResult ServiceManager::importCSV(AppState& app,
                                     const std::string& filepath) {
    AppResult r = storage_.importCSV(filepath, app.sessionKey);
    if (isOk(r)) {
        logger_.log(LogLevel::INFO, "Imported credentials from " + filepath);
        refreshServiceList(app);
    } else {
        logger_.log(LogLevel::ERROR_LEVEL, "Import failed: " + filepath);
    }
    return r;
}

// ── updateCountLabel ──────────────────────────────────────────
void ServiceManager::updateCountLabel(AppState& app) {
    if (!app.labelCount) return;
    std::string text = "Credentials: "
                     + std::to_string(app.credentials.size());
    gtk_label_set_text(GTK_LABEL(app.labelCount), text.c_str());
}

// ── GTK static callbacks ──────────────────────────────────────
void ServiceManager::onSearchChanged(GtkSearchEntry* entry,
                                      gpointer userData) {
    AppState* app = static_cast<AppState*>(userData);
    // userData carries AppState; ServiceManager pointer stored in object data
    auto* mgr = static_cast<ServiceManager*>(
        g_object_get_data(G_OBJECT(entry), "service-manager"));
    if (!mgr || !app) return;
    const gchar* text = gtk_entry_get_text(GTK_ENTRY(entry));
    mgr->applyFilter(*app, text ? std::string(text) : "");
}

void ServiceManager::onServiceClicked(GtkWidget*, gpointer userData) {
    auto* data = static_cast<ServiceClickData*>(userData);
    data->mgr->showServiceCredentials(*data->app, data->service);
}

// ── Show service credentials popup ───────────────────────────
// Data bundle for multi-credential popup button
struct CredBtnData {
    Credential          cred;
    AppState*           app;
    ICredentialDialog*  dlg;
    ServiceManager*     mgr;
};

void ServiceManager::showServiceCredentials(AppState& app,
                                             const std::string& service) {
    std::vector<Credential> matching;
    for (const auto& c : app.credentials)
        if (c.service == service) matching.push_back(c);

    if (matching.empty()) return;

    // Single credential — show detail directly
    if (matching.size() == 1) {
        dialog_.showCredentialDetail(matching[0]);
        refreshServiceList(app);
        return;
    }

    // Multiple credentials — show a selection list
    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        service.c_str(),
        GTK_WINDOW(app.window),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 360, -1);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);

    for (const auto& c : matching) {
        std::string label = c.username
            + (c.url.empty() ? "" : "  [" + c.url + "]");
        GtkWidget* btn = gtk_button_new_with_label(label.c_str());

        auto* data = new CredBtnData{c, &app, &dialog_, this};
        g_signal_connect(btn, "clicked",
            G_CALLBACK(+[](GtkButton*, gpointer ud){
                auto* d = static_cast<CredBtnData*>(ud);
                d->dlg->showCredentialDetail(d->cred);
                d->mgr->refreshServiceList(*d->app);
            }), data);
        g_object_set_data_full(G_OBJECT(btn), "cred-data", data,
            [](gpointer p){ delete static_cast<CredBtnData*>(p); });

        gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 2);
    }
    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    refreshServiceList(app);
}
