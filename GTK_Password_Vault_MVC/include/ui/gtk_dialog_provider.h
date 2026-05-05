#pragma once
// ============================================================
// ui/gtk_dialog_provider.h
// SRP: All GTK dialog rendering — auth and credential dialogs.
// LSP: Satisfies both IAuthDialog and ICredentialDialog.
// DIP: IPasswordStrengthEvaluator injected for live strength.
// ============================================================
#ifndef GTK_DIALOG_PROVIDER_H
#define GTK_DIALOG_PROVIDER_H

#include "../auth/i_auth_dialog.h"
#include "i_credential_dialog.h"
#include "../security/i_strength_evaluator.h"
#include <gtk/gtk.h>

class GtkDialogProvider : public IAuthDialog,
                           public ICredentialDialog {
public:
    // DIP: parent window + strength evaluator injected.
    GtkDialogProvider(GtkWindow*                  parent,
                      IPasswordStrengthEvaluator& strengthEval);

    // ── IAuthDialog ───────────────────────────────────────────
    bool promptSetupMaster(SetupResult& result)                   override;
    bool promptVerifyMaster(std::string& outPass,
                            bool& forgotPassword)                  override;
    bool promptRecovery(RecoveryData& recovery)                   override;
    bool promptNewPassword(std::string& outPass)                  override;
    void showError(const std::string& message)                    override;
    void showInfo(const std::string& message)                     override;

    // ── ICredentialDialog ─────────────────────────────────────
    bool promptAddCredential(Credential& out)                     override;
    bool promptEditCredential(Credential& cred)                   override;
    bool confirmDelete(const std::string& service)                override;
    void showCredentialDetail(const Credential& cred)             override;

    // Update parent window reference (after window recreation).
    void setParent(GtkWindow* parent) { parent_ = parent; }

private:
    GtkWindow*                  parent_;
    IPasswordStrengthEvaluator& strengthEval_;

    // Helper: create a masked password entry with Show/Hide toggle.
    // entryOut receives the GtkEntry pointer.
    GtkWidget* makePasswordRow(const char* placeholder,
                                GtkWidget** entryOut);

    // Helper: create a plain text entry.
    GtkWidget* makeEntry(const char* placeholder, bool secret = false);

    // Helper: create a label.
    GtkWidget* makeLabel(const char* text);

    // Helper: create a strength indicator bar + label.
    // Updates when passwordEntry emits "changed".
    void attachStrengthIndicator(GtkWidget* box, GtkWidget* passwordEntry);

    // Static GTK callback for strength update.
    struct StrengthData {
        GtkWidget*                  bar;
        GtkWidget*                  label;
        IPasswordStrengthEvaluator* eval;
    };
    static void onPasswordChanged(GtkEditable* editable, gpointer data);
    static void onToggleVisibility(GtkButton* btn, gpointer entryPtr);

    // Build the first-run setup dialog content.
    bool buildSetupDialog(GtkWidget* dialog, SetupResult& result);

    // Build the credential form fields into a dialog box.
    struct CredentialFields {
        GtkWidget* service;
        GtkWidget* username;
        GtkWidget* password;
        GtkWidget* url;
        GtkWidget* notes;
        GtkWidget* category;
    };
    CredentialFields buildCredentialFields(GtkWidget* box);
    Credential       readCredentialFields(const CredentialFields& f);
};

#endif // GTK_DIALOG_PROVIDER_H
