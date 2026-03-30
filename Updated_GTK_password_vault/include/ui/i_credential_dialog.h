#pragma once
// ============================================================
// ui/i_credential_dialog.h
// ISP: Credential dialog methods only — consumed by ServiceManager.
// ============================================================
#ifndef I_CREDENTIAL_DIALOG_H
#define I_CREDENTIAL_DIALOG_H

#include "../global_types.h"
#include <string>

class ICredentialDialog {
public:
    virtual ~ICredentialDialog() = default;

    // Prompt user to add a new credential. Returns true on confirm.
    virtual bool promptAddCredential(Credential& out) = 0;

    // Prompt user to edit an existing credential. Returns true on save.
    virtual bool promptEditCredential(Credential& cred) = 0;

    // Ask for delete confirmation. Returns true if user confirmed.
    virtual bool confirmDelete(const std::string& service) = 0;

    // Show credential details (read-only view).
    virtual void showCredentialDetail(const Credential& cred) = 0;
};

#endif // I_CREDENTIAL_DIALOG_H
