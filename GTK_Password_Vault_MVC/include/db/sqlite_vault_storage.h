#pragma once
// ============================================================
// db/sqlite_vault_storage.h
// SRP: SQLite persistence of master + credential data only.
// LSP: Fully satisfies both IMasterStorage and ICredentialStorage.
// DIP: IEncryption injected via constructor — never self-created.
// ============================================================
#ifndef SQLITE_VAULT_STORAGE_H
#define SQLITE_VAULT_STORAGE_H

#include "i_storage.h"
#include "../crypto/i_encryption.h"

class SqliteVaultStorage : public IMasterStorage,
                            public ICredentialStorage {
public:
    // DIP: IEncryption injected — storage never knows concrete cipher.
    explicit SqliteVaultStorage(IEncryption& encryption);
    ~SqliteVaultStorage() override;

    // ── IMasterStorage ────────────────────────────────────────
    bool      masterExists()                                  const override;
    AppResult saveMaster(const std::string&  plainPassword,
                         const RecoveryData& recovery)              override;
    bool      verifyMaster(const std::string& plainPassword)  const override;
    bool      verifyRecovery(const RecoveryData& recovery)    const override;
    AppResult resetMaster(const std::string& newPassword)           override;
    std::vector<unsigned char> deriveKey(
                         const std::string& plainPassword)    const override;
    std::string saltHex()                                      const override;

    // ── ICredentialStorage ────────────────────────────────────
    std::vector<Credential> loadAll(
        const std::vector<unsigned char>& key)                const override;
    std::vector<Credential> search(
        const std::string&                query,
        const std::vector<unsigned char>& key)                const override;
    AppResult save(const Credential&                 cred,
                   const std::vector<unsigned char>& key)           override;
    AppResult update(const Credential&               cred,
                     const std::vector<unsigned char>& key)         override;
    AppResult remove(const std::string& id)                         override;
    std::vector<std::string> getHistory(
        const std::string&                id,
        const std::vector<unsigned char>& key)                const override;
    AppResult exportCSV(
        const std::string&                filepath,
        const std::vector<unsigned char>& key)                const override;
    AppResult importCSV(
        const std::string&                filepath,
        const std::vector<unsigned char>& key)                      override;

private:
    struct Impl;
    Impl*       pImpl_;
    IEncryption& encryption_;

    void initSchema();
    std::string encryptField(const std::string& plaintext,
                              const std::vector<unsigned char>& key) const;
    std::string decryptField(const std::string& stored,
                              const std::vector<unsigned char>& key) const;
    Credential  rowToCredential(void* stmt,
                                 const std::vector<unsigned char>& key) const;
};

#endif // SQLITE_VAULT_STORAGE_H
