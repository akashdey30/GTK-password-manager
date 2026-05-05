#pragma once
// ============================================================
// crypto/crypto_utils.h
// SRP: Password hashing and key derivation utilities only.
// ============================================================
#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include "../global_types.h"
#include <string>
#include <vector>

class CryptoUtils {
public:
    // Hash a plaintext password with PBKDF2-HMAC-SHA256.
    // Returns "salt_hex:hash_hex" string for storage.
    static std::string hashPassword(const std::string& plaintext);

    // Verify plaintext against a stored "salt_hex:hash_hex".
    static bool verifyPassword(const std::string& plaintext,
                                const std::string& stored);

    // Derive a 32-byte AES key from master password + stored salt.
    // salt_hex is the first component of the hashPassword output.
    static std::vector<unsigned char> deriveKey(
        const std::string& masterPassword,
        const std::string& saltHex);

    // Extract salt hex from a "salt_hex:hash_hex" stored value.
    static std::string extractSaltHex(const std::string& stored);

    // Encode bytes to hex string.
    static std::string toHex(const std::vector<unsigned char>& bytes);

    // Decode hex string to bytes.
    static std::vector<unsigned char> fromHex(const std::string& hex);

    // Encode bytes to base64 string.
    static std::string toBase64(const std::vector<unsigned char>& bytes);

    // Decode base64 string to bytes.
    static std::vector<unsigned char> fromBase64(const std::string& b64);
};

#endif // CRYPTO_UTILS_H
