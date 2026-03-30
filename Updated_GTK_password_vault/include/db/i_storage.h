#pragma once
// ============================================================
// db/i_storage.h
// ISP: Two narrow interfaces so each caller depends only on
//      the methods it actually uses.
//      MasterAuth     → IMasterStorage  (auth operations)
//      ServiceManager → ICredentialStorage (CRUD operations)
// DIP: High-level classes depend on these abstractions.
// OCP: New backends (PostgreSQL, cloud) are new subclasses only.
// ============================================================
#ifndef I_STORAGE_H
#define I_STORAGE_H

#include "../global_types.h"
#include <string>
#include <vector>

// ── Recovery data bundle ──────────────────────────────────────
struct RecoveryData {
    std::string phone;
    std::string gmail;
    std::string schoolAnswer;  // answer to hardcoded security question
};

// ── Interface 1: master password + recovery operations ────────
// Consumed exclusively by MasterAuth.
class IMasterStorage {
public:
    virtual ~IMasterStorage() = default;

    virtual bool masterExists()                                    const = 0;

    // Save master password (hashed) and recovery data (stored encrypted).
    virtual AppResult saveMaster(const std::string& plainPassword,
                                 const RecoveryData& recovery)          = 0;

    // Verify plaintext against stored hash. Increments failure count.
    virtual bool verifyMaster(const std::string& plainPassword)   const = 0;

    // Verify all three recovery fields match stored values.
    virtual bool verifyRecovery(const RecoveryData& recovery)     const = 0;

    // Reset master password after successful recovery verification.
    virtual AppResult resetMaster(const std::string& newPassword)       = 0;

    // Derive AES encryption key from master password + stored salt.
    virtual std::vector<unsigned char> deriveKey(
        const std::string& plainPassword)                         const = 0;

    // Return stored salt hex for key derivation.
    virtual std::string saltHex()                                  const = 0;
};

// ── Interface 2: credential CRUD operations ───────────────────
// Consumed exclusively by ServiceManager.
class ICredentialStorage {
public:
    virtual ~ICredentialStorage() = default;

    // Load all credentials, decrypting with the provided key.
    virtual std::vector<Credential> loadAll(
        const std::vector<unsigned char>& key)                    const = 0;

    // Search by service name or username (case-insensitive).
    virtual std::vector<Credential> search(
        const std::string&                query,
        const std::vector<unsigned char>& key)                    const = 0;

    // Save a new credential (encrypts password before storage).
    virtual AppResult save(const Credential&                  cred,
                           const std::vector<unsigned char>&  key)      = 0;

    // Update an existing credential by id.
    virtual AppResult update(const Credential&                 cred,
                             const std::vector<unsigned char>& key)      = 0;

    // Delete a credential by id.
    virtual AppResult remove(const std::string& id)                      = 0;

    // Retrieve password history for a credential id.
    virtual std::vector<std::string> getHistory(
        const std::string&                id,
        const std::vector<unsigned char>& key)                    const = 0;

    // Export to CSV (passwords in plaintext for usability).
    virtual AppResult exportCSV(
        const std::string&                filepath,
        const std::vector<unsigned char>& key)                    const = 0;

    // Import from CSV and encrypt on ingestion.
    virtual AppResult importCSV(
        const std::string&                filepath,
        const std::vector<unsigned char>& key)                          = 0;
};

#endif // I_STORAGE_H
