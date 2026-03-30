// ============================================================
// crypto/aes_gcm_encryption.cpp
// AES-256-GCM authenticated encryption via OpenSSL EVP API.
// Format: [IV(12 bytes)][Tag(16 bytes)][Ciphertext(N bytes)]
// ============================================================
#include "../../include/crypto/aes_gcm_encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <cstring>

AesGcmEncryption::AesGcmEncryption() = default;
AesGcmEncryption::~AesGcmEncryption() = default;

AppResult AesGcmEncryption::encrypt(
    const std::vector<unsigned char>& key,
    const std::string&                plaintext,
    std::vector<unsigned char>&       ciphertext)
{
    if (key.size() != static_cast<size_t>(AES_KEY_LEN))
        return AppResult::ErrCrypto;

    // Generate fresh random IV
    std::vector<unsigned char> iv(AES_IV_LEN);
    if (RAND_bytes(iv.data(), AES_IV_LEN) != 1)
        return AppResult::ErrCrypto;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return AppResult::ErrCrypto;

    int len = 0;
    std::vector<unsigned char> tag(AES_TAG_LEN);
    std::vector<unsigned char> enc(plaintext.size() + EVP_MAX_BLOCK_LENGTH);

    bool ok = true;
    ok = ok && (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(),
                                   nullptr, nullptr, nullptr) == 1);
    ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                                    AES_IV_LEN, nullptr) == 1);
    ok = ok && (EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                                   key.data(), iv.data()) == 1);
    int enc_len = 0;
    if (ok) {
        ok = (EVP_EncryptUpdate(ctx, enc.data(), &len,
                reinterpret_cast<const unsigned char*>(plaintext.data()),
                static_cast<int>(plaintext.size())) == 1);
        enc_len = len;
    }
    ok = ok && (EVP_EncryptFinal_ex(ctx, enc.data() + enc_len, &len) == 1);
    enc_len += len;
    ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                                    AES_TAG_LEN, tag.data()) == 1);
    EVP_CIPHER_CTX_free(ctx);

    if (!ok) return AppResult::ErrCrypto;

    // Assemble: [IV][Tag][Ciphertext]
    ciphertext.clear();
    ciphertext.insert(ciphertext.end(), iv.begin(), iv.end());
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());
    ciphertext.insert(ciphertext.end(), enc.begin(), enc.begin() + enc_len);

    return AppResult::OK;
}

AppResult AesGcmEncryption::decrypt(
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& ciphertext,
    std::string&                      plaintext)
{
    const size_t minLen = static_cast<size_t>(AES_IV_LEN + AES_TAG_LEN);
    if (key.size() != static_cast<size_t>(AES_KEY_LEN) ||
        ciphertext.size() < minLen)
        return AppResult::ErrCrypto;

    const unsigned char* iv_ptr  = ciphertext.data();
    unsigned char tag_buf[AES_TAG_LEN];
    std::memcpy(tag_buf, ciphertext.data() + AES_IV_LEN, AES_TAG_LEN);
    const unsigned char* enc_ptr = ciphertext.data() + minLen;
    int                  enc_len = static_cast<int>(ciphertext.size() - minLen);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return AppResult::ErrCrypto;

    std::vector<unsigned char> dec(enc_len + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    bool ok = true;

    ok = ok && (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(),
                                   nullptr, nullptr, nullptr) == 1);
    ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                                    AES_IV_LEN, nullptr) == 1);
    ok = ok && (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                                   key.data(), iv_ptr) == 1);
    int dec_len = 0;
    if (ok) {
        ok = (EVP_DecryptUpdate(ctx, dec.data(), &len,
                                enc_ptr, enc_len) == 1);
        dec_len = len;
    }
    ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                                    AES_TAG_LEN, tag_buf) == 1);
    int final_ret = ok ? EVP_DecryptFinal_ex(ctx, dec.data() + dec_len, &len) : 0;
    dec_len += len;
    EVP_CIPHER_CTX_free(ctx);

    if (!ok || final_ret != 1) return AppResult::ErrCrypto; // Tag mismatch

    plaintext.assign(reinterpret_cast<char*>(dec.data()),
                     static_cast<size_t>(dec_len));
    return AppResult::OK;
}
