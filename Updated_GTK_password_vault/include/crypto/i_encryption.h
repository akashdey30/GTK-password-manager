#pragma once
// ============================================================
// crypto/i_encryption.h
// ISP: Single responsibility — encrypt/decrypt byte buffers.
// DIP: Storage layer depends on this abstraction.
// ============================================================
#ifndef I_ENCRYPTION_H
#define I_ENCRYPTION_H

#include "../global_types.h"
#include <vector>
#include <string>

class IEncryption {
public:
    virtual ~IEncryption() = default;

    // Encrypt plaintext using the provided key.
    // Returns AppResult::OK and fills ciphertext on success.
    // Output format: [IV(12)] [Tag(16)] [Ciphertext(N)]
    virtual AppResult encrypt(
        const std::vector<unsigned char>& key,
        const std::string&                plaintext,
        std::vector<unsigned char>&       ciphertext) = 0;

    // Decrypt ciphertext using the provided key.
    // Returns AppResult::OK and fills plaintext on success.
    // Returns AppResult::ErrCrypto on tag mismatch / tamper.
    virtual AppResult decrypt(
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& ciphertext,
        std::string&                      plaintext) = 0;
};

#endif // I_ENCRYPTION_H
