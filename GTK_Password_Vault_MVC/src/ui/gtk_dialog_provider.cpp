// ============================================================
// ui/gtk_dialog_provider.cpp
// ============================================================
#include "../../include/ui/gtk_dialog_provider.h"
#include <cstring>

// ── Constructor ───────────────────────────────────────────────
GtkDialogProvider::GtkDialogProvider(GtkWindow*                  parent,
                                      IPasswordStrengthEvaluator& strengthEval)
    : parent_(parent), strengthEval_(strengthEval)
{}

// ── Helpers ───────────────────────────────────────────────────
GtkWidget* GtkDialogProvider::makeEntry(const char* placeholder, bool secret) {
    GtkWidget* e = gtk_entry_new();
    if (placeholder)
        gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    if (secret)
        gtk_entry_set_visibility(GTK_ENTRY(e), FALSE);
    gtk_widget_set_hexpand(e, FALSE);
    gtk_widget_set_size_request(e, 280, -1);
    return e;
}

GtkWidget* GtkDialogProvider::makeLabel(const char* text) {
    GtkWidget* lbl = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    return lbl;
}

GtkWidget* GtkDialogProvider::makePasswordRow(const char* placeholder,
                                               GtkWidget** entryOut) {
    GtkWidget* hbox  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget* entry = makeEntry(placeholder, true);
    GtkWidget* btn   = gtk_button_new_with_label("Show");
    gtk_widget_set_size_request(btn, 60, -1);

    g_signal_connect(btn, "clicked", G_CALLBACK(onToggleVisibility), entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn,   FALSE, FALSE, 0);

    if (entryOut) *entryOut = entry;
    return hbox;
}

void GtkDialogProvider::onToggleVisibility(GtkButton* btn, gpointer entryPtr) {
    GtkEntry* entry = GTK_ENTRY(entryPtr);
    gboolean vis = gtk_entry_get_visibility(entry);
    gtk_entry_set_visibility(entry, !vis);
    gtk_button_set_label(btn, vis ? "Show" : "Hide");
}

void GtkDialogProvider::attachStrengthIndicator(GtkWidget* box,
                                                 GtkWidget* passwordEntry) {
    GtkWidget* barBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* bar    = gtk_progress_bar_new();
    GtkWidget* lbl    = gtk_label_new("Strength: —");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), 0.0);
    gtk_widget_set_size_request(bar, 180, 10);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);

    gtk_box_pack_start(GTK_BOX(barBox), bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barBox), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), barBox, FALSE, FALSE, 2);

    auto* data   = new StrengthData{bar, lbl, &strengthEval_};
    g_signal_connect(passwordEntry, "changed",
                     G_CALLBACK(onPasswordChanged), data);
    g_object_set_data_full(G_OBJECT(passwordEntry), "strength-data", data,
        [](gpointer p){ delete static_cast<StrengthData*>(p); });
}

void GtkDialogProvider::onPasswordChanged(GtkEditable* editable,
                                           gpointer userData) {
    auto* d = static_cast<StrengthData*>(userData);
    const gchar* text = gtk_entry_get_text(GTK_ENTRY(editable));
    std::string  pwd  = text ? text : "";

    StrengthLevel level = d->eval->evaluate(pwd);
    double fraction = (static_cast<double>(level) + 1.0) / 5.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(d->bar), fraction);

    const char* colors[] = {
        "red", "orange", "goldenrod", "steelblue", "green"
    };
    std::string markup = std::string("<span foreground=\"")
                       + colors[static_cast<int>(level)] + "\">"
                       + "Strength: " + strengthLabel(level)
                       + "</span>";
    gtk_label_set_markup(GTK_LABEL(d->label), markup.c_str());
}

// ── IAuthDialog: promptSetupMaster ───────────────────────────
bool GtkDialogProvider::promptSetupMaster(SetupResult& result) {
    // Window sized to ~1/4 screen; fixed layout; resizable but elements don't stretch
    GdkDisplay*  display = gdk_display_get_default();
    GdkMonitor*  monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    int ww = geom.width  / 2;
    int wh = geom.height / 2;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Setup Master Password — GTK Password Vault",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create Vault", GTK_RESPONSE_OK,
        nullptr);

    // Resizable, sized ~1/4 screen
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), ww, wh);

    GtkWidget* outer = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(outer), 0);

    // Scroll container so content doesn't stretch on resize
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(outer), scroll, TRUE, TRUE, 0);

    // Fixed-layout inner box — elements will NOT stretch
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);   // center horizontally
    gtk_widget_set_valign(vbox, GTK_ALIGN_START);    // anchor to top
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    // Title
    GtkWidget* title = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(title),
        "<b><big>Create Your Master Password</big></b>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

    GtkWidget* sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 4);

    // ── Password fields ───────────────────────────────────────
    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Master Password:"),
                        FALSE, FALSE, 0);
    GtkWidget *ep1, *ep2;
    gtk_box_pack_start(GTK_BOX(vbox), makePasswordRow("Enter master password", &ep1),
                        FALSE, FALSE, 0);
    attachStrengthIndicator(vbox, ep1);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Confirm Password:"),
                        FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), makePasswordRow("Confirm master password", &ep2),
                        FALSE, FALSE, 0);

    // ── Recovery section ──────────────────────────────────────
    GtkWidget* sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 8);

    GtkWidget* recTitle = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(recTitle), "<b>Recovery Information</b>");
    gtk_box_pack_start(GTK_BOX(vbox), recTitle, FALSE, FALSE, 0);

    GtkWidget* recNote = gtk_label_new(
        "This information is used if you forget your master password.");
    gtk_label_set_line_wrap(GTK_LABEL(recNote), TRUE);
    gtk_widget_set_size_request(recNote, 280, -1);
    gtk_box_pack_start(GTK_BOX(vbox), recNote, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Recovery Phone Number:"),
                        FALSE, FALSE, 4);
    GtkWidget* ePhone = makeEntry("e.g. +1-555-0100");
    gtk_box_pack_start(GTK_BOX(vbox), ePhone, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Recovery Gmail:"),
                        FALSE, FALSE, 4);
    GtkWidget* eGmail = makeEntry("e.g. yourname@gmail.com");
    gtk_box_pack_start(GTK_BOX(vbox), eGmail, FALSE, FALSE, 0);

    std::string secQ = std::string("Security Question: ") + SECURITY_QUESTION;
    gtk_box_pack_start(GTK_BOX(vbox), makeLabel(secQ.c_str()),
                        FALSE, FALSE, 4);
    GtkWidget* eSchool = makeEntry("Your answer");
    gtk_box_pack_start(GTK_BOX(vbox), eSchool, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    bool ok = false;
    while (true) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK) break;

        std::string p1     = gtk_entry_get_text(GTK_ENTRY(ep1));
        std::string p2     = gtk_entry_get_text(GTK_ENTRY(ep2));
        std::string phone  = gtk_entry_get_text(GTK_ENTRY(ePhone));
        std::string gmail  = gtk_entry_get_text(GTK_ENTRY(eGmail));
        std::string school = gtk_entry_get_text(GTK_ENTRY(eSchool));

        if (p1.empty() || p1 != p2) {
            showError("Passwords do not match or are empty.");
            continue;
        }
        if (p1.size() < 8) {
            showError("Master password must be at least 8 characters.");
            continue;
        }
        if (phone.empty() || gmail.empty() || school.empty()) {
            showError("All recovery fields are required.");
            continue;
        }

        result.password          = p1;
        result.recovery.phone    = phone;
        result.recovery.gmail    = gmail;
        result.recovery.schoolAnswer = school;
        ok = true;
        break;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── IAuthDialog: promptVerifyMaster ──────────────────────────
bool GtkDialogProvider::promptVerifyMaster(std::string& outPass,
                                            bool& forgotPassword) {
    GdkDisplay*  display = gdk_display_get_default();
    GdkMonitor*  monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    int ww = geom.width  / 2;
    int wh = geom.height / 2;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "GTK Password Vault — Login",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Login",  GTK_RESPONSE_OK,
        nullptr);

    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), ww, wh);

    GtkWidget* outer = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(outer), scroll, TRUE, TRUE, 0);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    GtkWidget* titleLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLbl),
        "<b><big>GTK Password Vault</big></b>");
    gtk_box_pack_start(GTK_BOX(vbox), titleLbl, FALSE, FALSE, 0);

    GtkWidget* subLbl = gtk_label_new("Enter your master password to unlock.");
    gtk_box_pack_start(GTK_BOX(vbox), subLbl, FALSE, FALSE, 0);

    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Master Password:"),
                        FALSE, FALSE, 0);
    GtkWidget* pwdEntry;
    gtk_box_pack_start(GTK_BOX(vbox),
                        makePasswordRow("Enter master password", &pwdEntry),
                        FALSE, FALSE, 0);

    // "Forgot Password?" button — not a dialog button, inline
    GtkWidget* forgotBtn = gtk_button_new_with_label("Forgot Password?");
    gtk_widget_set_halign(forgotBtn, GTK_ALIGN_START);
    gtk_button_set_relief(GTK_BUTTON(forgotBtn), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), forgotBtn, FALSE, FALSE, 0);

    bool forgotClicked = false;
    g_signal_connect(forgotBtn, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer ud){
            *static_cast<bool*>(ud) = true;
        }), &forgotClicked);
    // Clicking "Forgot" also presses OK to exit the dialog
    g_signal_connect(forgotBtn, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer dlg){
            gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
        }), dialog);

    gtk_widget_show_all(dialog);

    // Allow Enter key to submit
    gtk_entry_set_activates_default(GTK_ENTRY(pwdEntry), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
    forgotPassword = forgotClicked;

    bool ok = false;
    if (resp == GTK_RESPONSE_OK) {
        outPass = gtk_entry_get_text(GTK_ENTRY(pwdEntry));
        ok = true;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── IAuthDialog: promptRecovery ───────────────────────────────
bool GtkDialogProvider::promptRecovery(RecoveryData& recovery) {
    GdkDisplay*  display = gdk_display_get_default();
    GdkMonitor*  monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    int ww = geom.width  / 2;
    int wh = geom.height / 2;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Password Recovery",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Verify", GTK_RESPONSE_OK,
        nullptr);

    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), ww, wh);

    GtkWidget* outer = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(outer), scroll, TRUE, TRUE, 0);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);

    GtkWidget* title = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Password Recovery</b>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

    GtkWidget* note = gtk_label_new(
        "Provide the recovery information you registered\n"
        "to verify your identity and reset your password.");
    gtk_label_set_justify(GTK_LABEL(note), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), note, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                        FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Recovery Phone Number:"),
                        FALSE, FALSE, 0);
    GtkWidget* ePhone = makeEntry("Your registered phone number");
    gtk_box_pack_start(GTK_BOX(vbox), ePhone, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Recovery Gmail:"),
                        FALSE, FALSE, 4);
    GtkWidget* eGmail = makeEntry("Your registered Gmail address");
    gtk_box_pack_start(GTK_BOX(vbox), eGmail, FALSE, FALSE, 0);

    std::string secQ = std::string("Security Question:\n") + SECURITY_QUESTION;
    gtk_box_pack_start(GTK_BOX(vbox), makeLabel(secQ.c_str()),
                        FALSE, FALSE, 4);
    GtkWidget* eSchool = makeEntry("Your answer");
    gtk_box_pack_start(GTK_BOX(vbox), eSchool, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    bool ok = false;
    while (true) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK) break;

        std::string phone  = gtk_entry_get_text(GTK_ENTRY(ePhone));
        std::string gmail  = gtk_entry_get_text(GTK_ENTRY(eGmail));
        std::string school = gtk_entry_get_text(GTK_ENTRY(eSchool));

        if (phone.empty() || gmail.empty() || school.empty()) {
            showError("All recovery fields are required.");
            continue;
        }
        recovery.phone        = phone;
        recovery.gmail        = gmail;
        recovery.schoolAnswer = school;
        ok = true;
        break;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── IAuthDialog: promptNewPassword ───────────────────────────
bool GtkDialogProvider::promptNewPassword(std::string& outPass) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Set New Master Password",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Set Password", GTK_RESPONSE_OK,
        nullptr);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, -1);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("New Master Password:"),
                        FALSE, FALSE, 0);
    GtkWidget *ep1, *ep2;
    gtk_box_pack_start(GTK_BOX(vbox), makePasswordRow("New password", &ep1),
                        FALSE, FALSE, 0);
    attachStrengthIndicator(vbox, ep1);

    gtk_box_pack_start(GTK_BOX(vbox), makeLabel("Confirm New Password:"),
                        FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), makePasswordRow("Confirm password", &ep2),
                        FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    bool ok = false;
    while (true) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK) break;

        std::string p1 = gtk_entry_get_text(GTK_ENTRY(ep1));
        std::string p2 = gtk_entry_get_text(GTK_ENTRY(ep2));
        if (p1.empty() || p1 != p2) {
            showError("Passwords do not match or are empty.");
            continue;
        }
        if (p1.size() < 8) {
            showError("Password must be at least 8 characters.");
            continue;
        }
        outPass = p1;
        ok = true;
        break;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── showError / showInfo ──────────────────────────────────────
void GtkDialogProvider::showError(const std::string& message) {
    GtkWidget* msg = gtk_message_dialog_new(
        parent_, GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "%s", message.c_str());
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}

void GtkDialogProvider::showInfo(const std::string& message) {
    GtkWidget* msg = gtk_message_dialog_new(
        parent_, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "%s", message.c_str());
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}

// ── Credential field helpers ──────────────────────────────────
GtkDialogProvider::CredentialFields
GtkDialogProvider::buildCredentialFields(GtkWidget* box) {
    CredentialFields f{};

    gtk_box_pack_start(GTK_BOX(box), makeLabel("Service / Website:"),
                        FALSE, FALSE, 0);
    f.service = makeEntry("e.g. GitHub");
    gtk_box_pack_start(GTK_BOX(box), f.service, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), makeLabel("Username / Email:"),
                        FALSE, FALSE, 4);
    f.username = makeEntry("e.g. john@example.com");
    gtk_box_pack_start(GTK_BOX(box), f.username, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), makeLabel("Password:"),
                        FALSE, FALSE, 4);
    GtkWidget* pwdRow = makePasswordRow("Enter password", &f.password);
    gtk_box_pack_start(GTK_BOX(box), pwdRow, FALSE, FALSE, 0);
    attachStrengthIndicator(box, f.password);

    gtk_box_pack_start(GTK_BOX(box), makeLabel("URL (optional):"),
                        FALSE, FALSE, 4);
    f.url = makeEntry("e.g. https://github.com");
    gtk_box_pack_start(GTK_BOX(box), f.url, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), makeLabel("Notes (optional):"),
                        FALSE, FALSE, 4);
    f.notes = makeEntry("Any extra info");
    gtk_box_pack_start(GTK_BOX(box), f.notes, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), makeLabel("Category:"),
                        FALSE, FALSE, 4);
    f.category = makeEntry("e.g. Work, Personal");
    gtk_entry_set_text(GTK_ENTRY(f.category), "General");
    gtk_box_pack_start(GTK_BOX(box), f.category, FALSE, FALSE, 0);

    return f;
}

Credential GtkDialogProvider::readCredentialFields(const CredentialFields& f) {
    Credential c;
    c.service  = gtk_entry_get_text(GTK_ENTRY(f.service));
    c.username = gtk_entry_get_text(GTK_ENTRY(f.username));
    c.password = gtk_entry_get_text(GTK_ENTRY(f.password));
    c.url      = gtk_entry_get_text(GTK_ENTRY(f.url));
    c.notes    = gtk_entry_get_text(GTK_ENTRY(f.notes));
    c.category = gtk_entry_get_text(GTK_ENTRY(f.category));
    if (c.category.empty()) c.category = "General";
    return c;
}

// ── ICredentialDialog: promptAddCredential ────────────────────
bool GtkDialogProvider::promptAddCredential(Credential& out) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Add Credential",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add", GTK_RESPONSE_OK,
        nullptr);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

    auto fields = buildCredentialFields(vbox);
    gtk_widget_show_all(dialog);

    bool ok = false;
    while (true) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK) break;
        out = readCredentialFields(fields);
        if (out.service.empty() || out.username.empty()) {
            showError("Service and username are required.");
            continue;
        }
        ok = true;
        break;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── ICredentialDialog: promptEditCredential ───────────────────
bool GtkDialogProvider::promptEditCredential(Credential& cred) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Edit Credential",
        parent_, GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_OK,
        nullptr);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

    auto fields = buildCredentialFields(vbox);

    // Pre-fill with existing values
    gtk_entry_set_text(GTK_ENTRY(fields.service),  cred.service.c_str());
    gtk_entry_set_text(GTK_ENTRY(fields.username), cred.username.c_str());
    gtk_entry_set_text(GTK_ENTRY(fields.password), cred.password.c_str());
    gtk_entry_set_text(GTK_ENTRY(fields.url),      cred.url.c_str());
    gtk_entry_set_text(GTK_ENTRY(fields.notes),    cred.notes.c_str());
    gtk_entry_set_text(GTK_ENTRY(fields.category), cred.category.c_str());

    gtk_widget_show_all(dialog);

    bool ok = false;
    while (true) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK) break;
        Credential updated = readCredentialFields(fields);
        if (updated.service.empty() || updated.username.empty()) {
            showError("Service and username are required.");
            continue;
        }
        updated.id = cred.id;  // preserve id
        cred = updated;
        ok = true;
        break;
    }
    gtk_widget_destroy(dialog);
    return ok;
}

// ── ICredentialDialog: confirmDelete ─────────────────────────
bool GtkDialogProvider::confirmDelete(const std::string& service) {
    GtkWidget* msg = gtk_message_dialog_new(
        parent_, GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Delete credential for \"%s\"?", service.c_str());
    gint resp = gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
    return resp == GTK_RESPONSE_YES;
}

// ── ICredentialDialog: showCredentialDetail ───────────────────
void GtkDialogProvider::showCredentialDetail(const Credential& cred) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        cred.service.c_str(),
        parent_, GTK_DIALOG_MODAL,
        "_Close",  GTK_RESPONSE_CLOSE,
        "_Edit",   GTK_RESPONSE_APPLY,
        "_Delete", GTK_RESPONSE_REJECT,
        nullptr);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, -1);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 16);
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

    auto addRow = [&](int row, const char* label, const std::string& value) {
        GtkWidget* lbl = gtk_label_new(nullptr);
        std::string markup = std::string("<b>") + label + "</b>";
        gtk_label_set_markup(GTK_LABEL(lbl), markup.c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);

        GtkWidget* val = gtk_label_new(value.empty() ? "—" : value.c_str());
        gtk_label_set_xalign(GTK_LABEL(val), 0.0f);
        gtk_label_set_selectable(GTK_LABEL(val), TRUE);
        gtk_grid_attach(GTK_GRID(grid), val, 1, row, 1, 1);
    };

    addRow(0, "Service:",   cred.service);
    addRow(1, "Username:",  cred.username);
    addRow(2, "Password:",  std::string(cred.password.size(), '*'));
    addRow(3, "URL:",       cred.url);
    addRow(4, "Category:",  cred.category);
    addRow(5, "Notes:",     cred.notes);

    // "Copy Password" button
    GtkWidget* copyBtn = gtk_button_new_with_label("Copy Password");
    gtk_grid_attach(GTK_GRID(grid), copyBtn, 1, 6, 1, 1);
    std::string pwd = cred.password;
    g_signal_connect(copyBtn, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer ud){
            auto* p = static_cast<std::string*>(ud);
            GtkClipboard* cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_clipboard_set_text(cb, p->c_str(), -1);
        }), &pwd);

    gtk_widget_show_all(dialog);
    gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Edit/Delete handled by caller (ServiceManager) after detail returns
    (void)resp;
}
