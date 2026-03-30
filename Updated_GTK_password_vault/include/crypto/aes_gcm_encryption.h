#pragma once
// ============================================================
// crypto/aes_gcm_encryption.h
// SRP: AES-256-GCM authenticated encryption only.
// LSP: Fully satisfies IEncryption contract.
// ============================================================
#ifndef AES_GCM_ENCRYPTION_H
#define AES_GCM_ENCRYPTION_H

#include "i_encryption.h"

class AesGcmEncryption : public IEncryption {
public:
    AesGcmEncryption();
    ~AesGcmEncryption() override;

    AppResult encrypt(
        const std::vector<unsigned char>& key,
        const std::string&                plaintext,
        std::vector<unsigned char>&       ciphertext) override;

    AppResult decrypt(
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& ciphertext,
        std::string&                      plaintext) override;
};

#endif // AES_GCM_ENCRYPTION_H
