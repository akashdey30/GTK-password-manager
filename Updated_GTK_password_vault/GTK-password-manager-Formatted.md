# 🔐 Password Vault Application

**Code Architecture & Design Documentation**\
**Modular Design Pattern Implementation Guide**\
**Version 2.0 \| February 2026**\
*Advanced Programming Lab - CSE 2nd Year*

------------------------------------------------------------------------

## 📌 Executive Summary

This document provides a complete overview of the **Password Vault**
application, detailing:

-   Architecture design
-   Modular structure
-   Centralized state management
-   Encryption workflow

The application follows professional software engineering practices such
as:

-   ✅ Separation of concerns\
-   ✅ Modularization\
-   ✅ Centralized AppState management\
-   ✅ Secure encrypted storage

------------------------------------------------------------------------

## 🚀 1. Project Overview

### 1.1 Application Description

A GTK-based desktop password manager that supports:

-   Master password authentication
-   Recovery question system
-   Add / View / Manage credentials
-   Encrypted persistent storage

------------------------------------------------------------------------

### 1.2 Technology Stack

| Component \| Technology \|

\|-----------\|------------\| Language \| C \| \| GUI Framework \| GTK+
3.x \| \| Platform \| Windows (MSYS2 / UCRT64) \| \| Encryption \|
XOR-based custom crypto \| \| Storage \| Encrypted `vault.dat` file \|

------------------------------------------------------------------------

## 🏗 2. Architecture Evolution

### ❌ Before: Monolithic Design

Originally, the entire application existed in a single file (`main.c`)
which included:

-   GUI
-   Master password logic
-   Encryption
-   File handling

Problems:

-   Hard to maintain
-   No separation between logic & UI
-   Difficult to extend

------------------------------------------------------------------------

### ✅ After: Modular Architecture

  Module           Responsibility                 Approx LOC
  ---------------- ------------------------------ ------------
  `main.c`         Entry point & initialization   25
  `app.c/.h`       Central AppState management    60
  `auth.c/.h`      Master password & recovery     150
  `vault.c/.h`     Credential management          180
  `crypto.c/.h`    Encryption utilities           80
  `file_io.c/.h`   Encrypted storage handling     70
  `ui/*`           GUI windows                    \~400

------------------------------------------------------------------------

## 📂 3. Directory Structure

    password_vault/
    │
    ├── src/
    │   ├── main.c
    │   ├── app.c
    │   ├── auth.c
    │   ├── vault.c
    │   ├── crypto.c
    │   ├── file_io.c
    │   └── ui/
    │       ├── login_window.c
    │       ├── dashboard_window.c
    │       ├── add_credential_window.c
    │       └── view_credentials_window.c
    │
    ├── include/
    ├── data/vault.dat
    └── Makefile

------------------------------------------------------------------------

## 🧠 4. Naming Conventions

### File Naming

    <functionality>.c
    <functionality>.h

Example:

-   `auth.c`
-   `vault.c`
-   `crypto.c`

------------------------------------------------------------------------

### Function Naming

Pattern:

    verb_object()
    action_target()

Examples:

-   `ma_prompt_master_password()`
-   `vs_load_all_credentials()`
-   `ui_refresh_services()`

------------------------------------------------------------------------

### Variable Naming

  Type              Convention
  ----------------- ------------
  Local Variables   snake_case
  Struct Types      PascalCase

Example:

``` c
AppState *app;
char master_pass[MAX_LEN];
GList *credential_list;
```

------------------------------------------------------------------------

## 🔄 5. State Management

All runtime state is stored in a centralized structure:

``` c
typedef struct {
    char session_master[MAX_LEN];
    GList *credentials;
    GtkWidget *main_window;
    gboolean login_success;
} AppState;
```

Benefits:

-   No global variables
-   Easy state access in GTK callbacks
-   Clean architecture

------------------------------------------------------------------------

## 🔐 6. Encryption Flow

### Adding Credential

1.  User inputs password
2.  `crypto_encrypt()` is called
3.  Encrypted data saved to `vault.dat`

### Retrieving Credential

1.  Read encrypted vault file
2.  `crypto_decrypt()` applied
3.  Display in UI

⚠ Note: XOR is lightweight. For production, use AES / SHA-256.

------------------------------------------------------------------------

## 📖 7. Documentation Standard

Each function follows this structure:

``` c
/*
 * Brief description
 *
 * Parameters:
 *   param - description
 *
 * Returns:
 *   description
 */
```

Example:

``` c
void crypto_encrypt(const char *input, const char *key, char *output);
```

------------------------------------------------------------------------

## 🧩 8. Design Patterns Used

  Pattern           Implementation              Benefit
  ----------------- --------------------------- ---------------------
  Modular           `.c/.h` pairs               Encapsulation
  State Container   `AppState`                  Centralized state
  Callback          GTK signals                 UI/logic separation
  Strategy          Encrypt/Decrypt functions   Flexible encryption

------------------------------------------------------------------------

## 🎯 9. Conclusion

The modular refactor:

-   Eliminated global variables
-   Separated GUI from business logic
-   Added encryption workflow
-   Improved maintainability & scalability

This document serves as a blueprint for future extension and
improvement.

------------------------------------------------------------------------

### 📌 Advanced Programming Lab -- CSE Discipline
