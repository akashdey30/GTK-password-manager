// ============================================================
// ui/ui.cpp
// ============================================================
#include "../../include/ui/ui.h"
#include "../../include/ui/service_manager.h"
#include <gtk/gtk.h>

// ── createApp ─────────────────────────────────────────────────
AppState* VaultUI::createApp(const Callbacks& cbs) {
    AppState* app = new AppState();

    // ── Main window ───────────────────────────────────────────
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 480, 640);
    gtk_window_set_resizable(GTK_WINDOW(app->window), TRUE);
    g_signal_connect(app->window, "destroy",
                     G_CALLBACK(gtk_main_quit), nullptr);

    // ── Root vertical box ─────────────────────────────────────
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), root);

    // ── Header bar ────────────────────────────────────────────
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(header), 8);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    GtkWidget* appTitle = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(appTitle), "<b>Password Vault</b>");
    gtk_widget_set_halign(appTitle, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), appTitle, FALSE, FALSE, 0);

    GtkWidget* headerSpacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(header), headerSpacer, TRUE, TRUE, 0);

    // Change Password button
    GtkWidget* btnChangePwd = gtk_button_new_with_label("Change Password");
    gtk_widget_set_size_request(btnChangePwd, 140, 32);
    g_signal_connect(btnChangePwd, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer win){
            auto* cb = static_cast<VoidCallback*>(
                g_object_get_data(G_OBJECT(win), "changepwd-cb"));
            if (cb && *cb) (*cb)();
        }), app->window);
    gtk_box_pack_start(GTK_BOX(header), btnChangePwd, FALSE, FALSE, 0);

    // Lock button
    GtkWidget* btnLock = gtk_button_new_with_label("Lock");
    gtk_widget_set_size_request(btnLock, 60, 32);
    g_signal_connect(btnLock, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer win){
            auto* cb = static_cast<VoidCallback*>(
                g_object_get_data(G_OBJECT(win), "lock-cb"));
            if (cb && *cb) (*cb)();
        }), app->window);
    gtk_box_pack_start(GTK_BOX(header), btnLock, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    // ── Toolbar ───────────────────────────────────────────────
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 8);
    gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

    // Add button — signal wired in main.cpp after ServiceManager exists
    app->addButton = gtk_button_new_with_label("+ Add Credential");
    gtk_widget_set_size_request(app->addButton, 140, 32);
    auto* addData = new AddBtnData{cbs.onAdd};
    g_signal_connect(app->addButton, "clicked",
                     G_CALLBACK(onAddClicked), addData);
    g_object_set_data_full(G_OBJECT(app->addButton), "add-data", addData,
        [](gpointer p){ delete static_cast<AddBtnData*>(p); });
    gtk_box_pack_start(GTK_BOX(toolbar), app->addButton, FALSE, FALSE, 0);

    // Export button — retrieves callback from window object data
    GtkWidget* btnExport = gtk_button_new_with_label("Export CSV");
    gtk_widget_set_size_request(btnExport, 100, 32);
    g_signal_connect(btnExport, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer win){
            auto* cb = static_cast<VoidCallback*>(
                g_object_get_data(G_OBJECT(win), "export-cb"));
            if (cb && *cb) (*cb)();
        }), app->window);
    gtk_box_pack_start(GTK_BOX(toolbar), btnExport, FALSE, FALSE, 0);

    // Import button
    GtkWidget* btnImport = gtk_button_new_with_label("Import CSV");
    gtk_widget_set_size_request(btnImport, 100, 32);
    g_signal_connect(btnImport, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer win){
            auto* cb = static_cast<VoidCallback*>(
                g_object_get_data(G_OBJECT(win), "import-cb"));
            if (cb && *cb) (*cb)();
        }), app->window);
    gtk_box_pack_start(GTK_BOX(toolbar), btnImport, FALSE, FALSE, 0);

    // ── Search bar ────────────────────────────────────────────
    GtkWidget* searchBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(searchBox), 8);
    gtk_box_pack_start(GTK_BOX(root), searchBox, FALSE, FALSE, 0);

    GtkWidget* searchLbl = gtk_label_new("Search:");
    gtk_box_pack_start(GTK_BOX(searchBox), searchLbl, FALSE, FALSE, 0);

    app->searchEntry = GTK_WIDGET(gtk_search_entry_new());
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->searchEntry),
                                   "Filter by service or username...");
    gtk_widget_set_hexpand(app->searchEntry, TRUE);
    gtk_box_pack_start(GTK_BOX(searchBox), app->searchEntry, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(root),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    // ── Count label ───────────────────────────────────────────
    app->labelCount = gtk_label_new("Credentials: 0");
    gtk_label_set_xalign(GTK_LABEL(app->labelCount), 0.0f);
    GtkWidget* countBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(countBox), 6);
    gtk_box_pack_start(GTK_BOX(countBox), app->labelCount, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), countBox, FALSE, FALSE, 0);

    // ── Scrolled service list ─────────────────────────────────
    // Only this widget expands with the window
    app->scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(app->scrolled),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(app->scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(root), app->scrolled, TRUE, TRUE, 0);

    app->vboxServices = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_set_border_width(GTK_CONTAINER(app->vboxServices), 6);
    gtk_container_add(GTK_CONTAINER(app->scrolled), app->vboxServices);

    return app;
}

// ── wireSearch ────────────────────────────────────────────────
void VaultUI::wireSearch(AppState* app, ServiceManager* mgr) {
    g_object_set_data(G_OBJECT(app->searchEntry), "service-manager", mgr);
    g_signal_connect(app->searchEntry, "search-changed",
                     G_CALLBACK(ServiceManager::onSearchChanged), app);
}

// ── onAddClicked ──────────────────────────────────────────────
void VaultUI::onAddClicked(GtkButton*, gpointer data) {
    auto* d = static_cast<AddBtnData*>(data);
    if (d->cb) d->cb();
}
