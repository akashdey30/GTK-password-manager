#pragma once
// ============================================================
// global_types.h
// SRP: Central definitions for shared types and constants.
//      No logic, no I/O, no UI.
// ============================================================
#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include <string>
#include <cstdint>
#include <functional>

// ── Application-wide constants ────────────────────────────────
constexpr const char* APP_NAME          = "GTK Password Vault";
constexpr const char* APP_VERSION       = "2.0.0";
constexpr const char* DB_FILE           = "data/vault.db";
constexpr const char* LOG_FILE          = "data/vault.log";
constexpr int         PBKDF2_ITERATIONS = 600000;   // OWASP 2023
constexpr int         AES_KEY_LEN       = 32;        // 256-bit
constexpr int         AES_IV_LEN        = 12;        // GCM standard
constexpr int         AES_TAG_LEN       = 16;
constexpr int         SALT_LEN          = 32;
constexpr int         MAX_AUTH_FAILURES = 5;
constexpr int         AUTO_LOCK_SECONDS = 300;       // 5 minutes
constexpr int         CLIPBOARD_CLEAR_SECONDS = 30;

// Security question - hardcoded as per requirement
constexpr const char* SECURITY_QUESTION = "What's your school/college name?";

// ── Typed error codes ─────────────────────────────────────────
// OCP: New error types added here without modifying callers.
enum class AppResult {
    OK = 0,
    ErrGeneric,
    ErrAuth,
    ErrAuthLocked,
    ErrCrypto,
    ErrDatabase,
    ErrIO,
    ErrValidation,
    ErrNotFound,
    ErrAlreadyExists
};

inline bool isOk(AppResult r) { return r == AppResult::OK; }

// ── Credential data model ─────────────────────────────────────
// SRP: Pure data structure - no logic.
struct Credential {
    std::string id;
    std::string service;
    std::string username;
    std::string password;   // plaintext in memory only
    std::string url;
    std::string notes;
    std::string category;
    int64_t     created_at = 0;
    int64_t     updated_at = 0;

    Credential() = default;
    Credential(const std::string& svc,
               const std::string& user,
               const std::string& pwd,
               const std::string& u   = "",
               const std::string& n   = "",
               const std::string& cat = "General")
        : service(svc), username(user), password(pwd),
          url(u), notes(n), category(cat) {}
};

// ── Password strength level ───────────────────────────────────
enum class StrengthLevel {
    VeryWeak = 0,
    Weak,
    Moderate,
    Strong,
    VeryStrong
};

inline const char* strengthLabel(StrengthLevel s) {
    switch (s) {
        case StrengthLevel::VeryWeak:   return "Very Weak";
        case StrengthLevel::Weak:       return "Weak";
        case StrengthLevel::Moderate:   return "Moderate";
        case StrengthLevel::Strong:     return "Strong";
        case StrengthLevel::VeryStrong: return "Very Strong";
    }
    return "Unknown";
}

// ── Log levels ────────────────────────────────────────────────
enum class LogLevel {
    INFO,
    WARNING,
    ERROR_LEVEL   // ERROR conflicts with Windows macro
};

// ── Callback type aliases ─────────────────────────────────────
using VoidCallback   = std::function<void()>;
using StringCallback = std::function<void(const std::string&)>;

#endif // GLOBAL_TYPES_H
