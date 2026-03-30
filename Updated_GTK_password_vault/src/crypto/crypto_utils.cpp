// ============================================================
// crypto/crypto_utils.cpp
// ============================================================
#include "../../include/crypto/crypto_utils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

std::string CryptoUtils::hashPassword(const std::string& plaintext) {
    // Generate random salt
    std::vector<unsigned char> salt(SALT_LEN);
    if (RAND_bytes(salt.data(), SALT_LEN) != 1)
        throw std::runtime_error("RAND_bytes failed");

    std::vector<unsigned char> hash(32);
    if (PKCS5_PBKDF2_HMAC(
            plaintext.c_str(),
            static_cast<int>(plaintext.size()),
            salt.data(), SALT_LEN,
            PBKDF2_ITERATIONS,
            EVP_sha256(),
            32, hash.data()) != 1)
        throw std::runtime_error("PBKDF2 failed");

    // Wipe plaintext from stack-local copy via cleanse
    std::string result = toHex(salt) + ":" + toHex(hash);

    // Cleanse intermediate hash buffer
    OPENSSL_cleanse(hash.data(), hash.size());
    return result;
}

bool CryptoUtils::verifyPassword(const std::string& plaintext,
                                   const std::string& stored) {
    auto colon = stored.find(':');
    if (colon == std::string::npos) return false;

    std::string saltHex = stored.substr(0, colon);
    std::string hashHex = stored.substr(colon + 1);

    auto salt = fromHex(saltHex);
    auto expected = fromHex(hashHex);

    std::vector<unsigned char> computed(32);
    bool ok = PKCS5_PBKDF2_HMAC(
        plaintext.c_str(),
        static_cast<int>(plaintext.size()),
        salt.data(), static_cast<int>(salt.size()),
        PBKDF2_ITERATIONS,
        EVP_sha256(),
        32, computed.data()) == 1;

    // Constant-time comparison
    bool match = ok && (CRYPTO_memcmp(computed.data(),
                                       expected.data(), 32) == 0);

    // Wipe
    OPENSSL_cleanse(computed.data(), computed.size());
    return match;
}

std::vector<unsigned char> CryptoUtils::deriveKey(
    const std::string& masterPassword,
    const std::string& saltHex)
{
    auto salt = fromHex(saltHex);
    std::vector<unsigned char> key(AES_KEY_LEN);

    if (PKCS5_PBKDF2_HMAC(
            masterPassword.c_str(),
            static_cast<int>(masterPassword.size()),
            salt.data(), static_cast<int>(salt.size()),
            PBKDF2_ITERATIONS,
            EVP_sha256(),
            AES_KEY_LEN, key.data()) != 1) {
        OPENSSL_cleanse(key.data(), key.size());
        throw std::runtime_error("Key derivation failed");
    }
    return key;
}

std::string CryptoUtils::extractSaltHex(const std::string& stored) {
    auto colon = stored.find(':');
    if (colon == std::string::npos) return {};
    return stored.substr(0, colon);
}

std::string CryptoUtils::toHex(const std::vector<unsigned char>& bytes) {
    std::ostringstream oss;
    for (auto b : bytes)
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(b);
    return oss.str();
}

std::vector<unsigned char> CryptoUtils::fromHex(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte;
        std::istringstream(hex.substr(i, 2)) >> std::hex >> byte;
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

std::string CryptoUtils::toBase64(const std::vector<unsigned char>& bytes) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, bytes.data(), static_cast<int>(bytes.size()));
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

std::vector<unsigned char> CryptoUtils::fromBase64(const std::string& b64) {
    BIO* bio = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.size()));
    BIO* b64f = BIO_new(BIO_f_base64());
    bio = BIO_push(b64f, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    std::vector<unsigned char> result(b64.size());
    int len = BIO_read(bio, result.data(), static_cast<int>(result.size()));
    BIO_free_all(bio);
    if (len > 0) result.resize(static_cast<size_t>(len));
    else result.clear();
    return result;
}
