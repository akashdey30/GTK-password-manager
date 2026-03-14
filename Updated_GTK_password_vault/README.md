# 🔐 PASSWORD VAULT

> **A GTK-Based Desktop Password Manager — Built in C**
>
> **Course:** CSE 2100 Advanced Programming Laboratory &nbsp;|&nbsp; **Year:** CSE 2nd Year &nbsp;|&nbsp; **Date:** February 2026

---

## 📑 Table of Contents

1. [Project Overview](#-project-overview)
2. [Version Comparison at a Glance](#-version-comparison-at-a-glance)
3. [What Changed: ver1 → ver2 (Full Code Analysis)](#-what-changed-ver1--ver2-full-code-analysis)
   - [File Structure](#1-file-structure)
   - [Global Variables Eliminated](#2-global-variables-eliminated--appstate-introduced)
   - [Naming Conventions](#3-naming-conventions)
   - [Function Naming & Module Prefixing](#4-function-naming--module-prefixing)
   - [Encryption Workflow Encapsulation](#5-encryption-workflow-encapsulation)
   - [Cleanup Safety](#6-cleanup-null-safety)
   - [Static Scope — Information Hiding](#7-static-scope--information-hiding)
4. [Architecture](#-architecture)
5. [Module Breakdown](#-module-breakdown)
6. [File Structure](#-file-structure)
7. [Encryption Workflow](#-encryption-workflow)
8. [Documentation Standard](#-documentation-standard)
9. [AI Refactoring Prompts: ver1 → ver2](#-ai-refactoring-prompts-ver1--ver2)
10. [Design Principles Applied](#-design-principles-applied)
11. [How to Build](#️-how-to-build)

---

## 🎯 Project Overview

**PASSWORD VAULT** is a GTK-based desktop password manager written in C. It allows users to securely store, retrieve, and manage credentials protected by a master password and a recovery question system. All credential data is encrypted before being written to disk.

The project was developed in two distinct phases:

| Version | Description |
|---|---|
| **ver1** | Single monolithic `main.c` — all GUI, logic, encryption, and file I/O in one file with global variables |
| **ver2** | Modular architecture — 9 source files, 9 headers, 0 global variables, full Doxygen documentation |

---

## 📊 Version Comparison at a Glance

| Metric | ver1 (`main.c`) | ver2 (Modular) | Change |
|---|---|---|---|
| **Source files (.c)** | 1 | 9 | Full modular separation |
| **Header files (.h)** | 0 | 9 | Clean public interfaces |
| **Global variables** | **Multiple** (`main_window`, `credential_list`, `master_pass`, `login_success`, etc.) | **0** | Complete elimination |
| **Naming convention — functions** | Mixed (`LoadCredentials`, `crypto_encrypt`, `refresh_ui`) | Uniform module-prefixed (`ma_*`, `vs_*`, `fio_*`, `ui_*`) | Consistent throughout |
| **Struct fields** | Mixed `snake_case` and abbreviated names | Uniform `snake_case` with descriptive names | Consistent throughout |
| **Struct type names** | Inconsistent casing | All `PascalCase` (`AppState`, `Credential`) | Standard enforced |
| **Encryption calls** | Scattered raw XOR inline across multiple functions | Encapsulated behind `crypto_encrypt()` / `crypto_decrypt()` | Single responsibility |
| **File I/O** | Inline in multiple places | Isolated to `file_io.c` | Encapsulated |
| **Cleanup safety** | Unconditional free — crash risk | All handles null-checked before free | Crash-safe |
| **Documentation** | Zero comments | Full Doxygen `@brief/@param/@return` on every function | Fully documented |
| **Context passing** | Functions read globals directly | `AppState*` passed explicitly | Dependency Injection |

---

## 🔄 What Changed: ver1 → ver2 (Full Code Analysis)

### 1. File Structure

**ver1 — Everything in one flat file:**
```
password_vault/
├── main.c          ← All logic: GUI, auth, encryption, file I/O, main loop
├── vault.dat       ← Encrypted credentials (written directly from main.c)
└── Makefile
```

**ver2 — Organized into clean layers:**
```
password_vault/
├── src/
│   ├── main.c                        ← ~25 lines: init, launch, cleanup only
│   ├── app.c                         ← AppState initialization & cleanup
│   ├── auth.c                        ← Master password & recovery logic
│   ├── vault.c                       ← Credential list management
│   ├── crypto.c                      ← Encryption / decryption utilities
│   ├── file_io.c                     ← Encrypted disk read/write
│   └── ui/
│       ├── login_window.c            ← GTK login screen
│       ├── dashboard_window.c        ← Main dashboard & service list
│       ├── add_credential_window.c   ← Add credential form
│       └── view_credentials_window.c ← Credential viewer
├── include/
│   ├── app.h
│   ├── auth.h
│   ├── vault.h
│   ├── crypto.h
│   ├── file_io.h
│   └── ui/
│       ├── login_window.h
│       ├── dashboard_window.h
│       ├── add_credential_window.h
│       └── view_credentials_window.h
├── data/
│   └── vault.dat
└── Makefile
```

---

### 2. Global Variables Eliminated → `AppState` Introduced

**ver1 — Globals accessible and modifiable by any function:**
```c
// Anyone can read or corrupt these at any time
GtkWidget  *main_window      = NULL;
GList      *credential_list  = NULL;
char        master_pass[256] = {0};
gboolean    login_success    = FALSE;
```

**ver2 — Zero globals. All state bundled into `AppState` in `app.h`:**
```c
// app.h — inject exactly what each function needs, nothing more
typedef struct {
    char        session_master[MAX_LEN];
    GList      *credentials;
    GtkWidget  *main_window;
    gboolean    login_success;
} AppState;
```

Functions now receive exactly what they need via `AppState*` — no hidden dependencies:

```c
// ver1 — silently reads globals, impossible to test in isolation
void refresh_ui() {
    gtk_list_store_clear(/* where did this widget come from? */);
    if (login_success) { /* reads global */ }
}

// ver2 — every dependency is explicit at the call site
void ui_refresh_services(AppState *app) {
    gtk_list_store_clear(/* passed through app */);
    if (app->login_success) { /* clear ownership */ }
}
```

---

### 3. Naming Conventions

All identifiers were renamed to follow a strict, uniform convention across the entire codebase:

| Category | ver1 | ver2 | Rule |
|---|---|---|---|
| Local variables | Mixed | `snake_case` | Always |
| Parameters | Mixed | `snake_case` | Always |
| Struct type names | Mixed casing | `PascalCase` | Always |
| Struct members | Mixed | `snake_case` | Always |
| Functions | Mixed | `module_verb_object()` | Always |
| Header guards | Absent | `UPPER_SNAKE_CASE_H` | Always |

**Struct field renames (sample):**

| ver1 Field | ver2 Field | Note |
|---|---|---|
| `masterPass` | `session_master` | Descriptive, snake_case |
| `credList` | `credentials` | Full word |
| `loginOk` | `login_success` | Clear boolean name |
| `win` | `main_window` | Unambiguous |

---

### 4. Function Naming & Module Prefixing

Every function was renamed from inconsistent flat naming to a **module-prefixed** format: `module_verb_object()`. Internal helpers were also marked `static`.

| ver1 (flat/mixed) | ver2 (module-prefixed) | Module | Visibility |
|---|---|---|---|
| `LoadCredentials()` | `vs_load_all_credentials()` | `vault.c` | public |
| `SaveVault()` | `fio_save_vault()` | `file_io.c` | public |
| `LoadVault()` | `fio_load_vault()` | `file_io.c` | public |
| `crypto_encrypt()` | `crypto_encrypt()` | `crypto.c` | public |
| `crypto_decrypt()` | `crypto_decrypt()` | `crypto.c` | public |
| `verify_master()` | `ma_verify_master()` | `auth.c` | public |
| `handle_recovery()` | `ma_handle_recovery()` | `auth.c` | public |
| `prompt_master()` | `ma_prompt_master_password()` | `auth.c` | public |
| `add_cred()` | `vs_add_credential()` | `vault.c` | public |
| `delete_cred()` | `vs_delete_credential()` | `vault.c` | public |
| `refresh_ui()` | `ui_refresh_services()` | `dashboard_window.c` | public |
| `show_login()` | `ui_show_login_window()` | `login_window.c` | public |
| `show_dash()` | `ui_show_dashboard()` | `dashboard_window.c` | public |
| `show_add()` | `ui_show_add_credential()` | `add_credential_window.c` | public |
| `show_creds()` | `ui_show_credentials()` | `view_credentials_window.c` | public |
| `find_duplicate()` | `vs_find_duplicate()` | `vault.c` | `static` |
| `build_login_widgets()` | `build_login_widgets()` | `login_window.c` | `static` |

**Module prefix reference:**

| Prefix | Module | Owns |
|---|---|---|
| `ma_` | `auth.c` | Authentication & recovery |
| `vs_` | `vault.c` | Credential management |
| `fio_` | `file_io.c` | Disk read/write |
| `crypto_` | `crypto.c` | Encryption utilities |
| `ui_` | `ui/*.c` | All GTK windows |

---

### 5. Encryption Workflow Encapsulation

**ver1 — XOR logic duplicated inline in multiple functions:**
```c
// Inside SaveVault() — mixes I/O with crypto
for (int i = 0; i < strlen(password); i++) {
    encrypted[i] = password[i] ^ master_pass[i % strlen(master_pass)];
}
fwrite(encrypted, 1, len, file);

// Inside LoadVault() — same XOR written again
for (int i = 0; i < len; i++) {
    decrypted[i] = raw[i] ^ master_pass[i % strlen(master_pass)];
}
```

**ver2 — Single responsibility. `crypto.c` is the only place encryption logic lives:**
```c
// crypto.c — owns the algorithm entirely
void crypto_encrypt(const char *input, const char *key, char *output) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i < strlen(input); i++) {
        output[i] = input[i] ^ key[i % key_len];
    }
}

void crypto_decrypt(const char *input, const char *key, char *output) {
    crypto_encrypt(input, key, output);  // XOR is symmetric
}

// file_io.c — delegates entirely, knows nothing about XOR
void fio_save_vault(AppState *app) {
    crypto_encrypt(raw_password, app->session_master, encrypted_buf);
    fwrite(encrypted_buf, 1, len, file);
}
```

Swapping XOR for AES now requires changing **only `crypto.c`** — zero other files touched.

---

### 6. Cleanup Null Safety

**ver1 — Frees all resources unconditionally (crash risk if init partially failed):**
```c
void cleanup() {
    g_list_free(credential_list);    // ← crash if list was never initialized
    gtk_widget_destroy(main_window); // ← crash if window creation failed
}
```

**ver2 — Every resource null-checked before freeing:**
```c
void app_cleanup(AppState *app) {
    if (app->credentials)  g_list_free_full(app->credentials, g_free); // ✓ safe
    if (app->main_window)  gtk_widget_destroy(app->main_window);       // ✓ safe
    g_free(app);
}
```

---

### 7. Static Scope — Information Hiding

ver2 explicitly marks internal functions `static`, preventing accidental linkage from other translation units:

| Symbol | ver1 scope | ver2 scope |
|---|---|---|
| `vs_find_duplicate()` | global linkage | `static` in `vault.c` |
| `build_login_widgets()` | global linkage | `static` in `login_window.c` |
| `build_dashboard_widgets()` | global linkage | `static` in `dashboard_window.c` |
| `on_login_clicked()` (GTK callback) | global linkage | `static` in `login_window.c` |
| `on_add_confirm_clicked()` (GTK callback) | global linkage | `static` in `add_credential_window.c` |
| `on_delete_clicked()` (GTK callback) | global linkage | `static` in `dashboard_window.c` |

Only functions declared in a `.h` header have external linkage. Everything else is module-private.

---

## 🏛️ Architecture

```
╔══════════════════════════════════════════════════════════════════╗
║                      UI Layer  (GTK Windows)                     ║
║                                                                  ║
║  ┌──────────────┐  ┌──────────────────┐  ┌────────────────────┐ ║
║  │login_window.c│  │dashboard_window.c│  │add_credential_     │ ║
║  │              │  │                  │  │window.c            │ ║
║  │ui_show_login │  │ui_show_dashboard │  │ui_show_add()       │ ║
║  │              │  │ui_refresh_service│  │                    │ ║
║  └──────┬───────┘  └───────┬──────────┘  └────────┬───────────┘ ║
╚═════════╪══════════════════╪═════════════════════╪══════════════╝
          │                  │                     │
          ▼                  ▼                     ▼
╔══════════════════════════════════════════════════════════════════╗
║                    Business Logic Layer                          ║
║                                                                  ║
║  ┌─────────────────────┐      ┌──────────────────────────────┐  ║
║  │       auth.c        │      │           vault.c            │  ║
║  │                     │      │                              │  ║
║  │ ma_verify_master()  │      │ vs_load_all_credentials()    │  ║
║  │ ma_handle_recovery()│      │ vs_add_credential()          │  ║
║  │ ma_prompt_master()  │      │ vs_delete_credential()       │  ║
║  └─────────────────────┘      │ [static] vs_find_duplicate() │  ║
║                               └──────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════╝
                                        │
                                        ▼
╔══════════════════════════════════════════════════════════════════╗
║                    Infrastructure Layer                          ║
║                                                                  ║
║  ┌──────────────────────────┐    ┌──────────────────────────┐   ║
║  │        file_io.c         │───▶│        crypto.c          │   ║
║  │                          │◀───│                          │   ║
║  │ fio_save_vault()         │    │ crypto_encrypt()         │   ║
║  │ fio_load_vault()         │    │ crypto_decrypt()         │   ║
║  └──────────────────────────┘    └──────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════╝
                                        │
                                        ▼
╔══════════════════════════════════════════════════════════════════╗
║                      Foundation Layer                            ║
║                                                                  ║
║  ┌──────────────────────────────────────────────────────────┐   ║
║  │                         app.h                            │   ║
║  │                                                          │   ║
║  │  typedef AppState  { session_master, credentials,        │   ║
║  │                      main_window, login_success }         │   ║
║  │  typedef Credential { service, username, password }      │   ║
║  │  #define MAX_LEN, MAX_CREDENTIALS                        │   ║
║  └──────────────────────────────────────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════╝
```

**Dependency rule:** UI → Business Logic → Infrastructure → Foundation. No upward dependencies. No circular includes.

---

## 🧩 Module Breakdown

### `src/main.c` — ~25 lines
Minimal entry point. Calls `gtk_init()`, allocates `AppState` with `g_new0()`, launches `ui_show_login_window()`, runs `gtk_main()`, then frees state. Nothing else lives here.

### `src/app.c` — ~60 lines
Owns the `AppState` struct lifecycle. Provides `app_init()` to zero-initialize state and `app_cleanup()` to safely free all resources — every handle is null-checked before being freed.

### `src/auth.c` — ~150 lines
All authentication logic. `ma_verify_master()` compares input against the stored master hash. `ma_handle_recovery()` validates the recovery answer and resets the master password. `ma_prompt_master_password()` drives the first-run setup flow. No GTK widget construction happens here.

### `src/vault.c` — ~180 lines
Credential list management. `vs_add_credential()` appends to `app->credentials` and triggers a save. `vs_delete_credential()` removes by index. `vs_load_all_credentials()` populates the list from the decrypted vault. The `static` helper `vs_find_duplicate()` prevents storing the same service name twice.

### `src/crypto.c` — ~80 lines
Pure encryption utility — no GTK, no `AppState`, no file I/O. `crypto_encrypt()` and `crypto_decrypt()` operate only on char buffers and a key string. Because XOR is symmetric, `crypto_decrypt()` delegates directly to `crypto_encrypt()`. Swapping the algorithm requires touching only this file.

### `src/file_io.c` — ~70 lines
Encrypted disk persistence. `fio_save_vault()` serializes `app->credentials`, calls `crypto_encrypt()` on each password field, and writes to `vault.dat`. `fio_load_vault()` reads, decrypts, and populates `app->credentials`. No encryption logic lives here — fully delegated to `crypto.c`.

### `src/ui/login_window.c` — ~100 lines
GTK login screen. Builds the master password entry widget, wires the login button callback, and on success sets `app->login_success = TRUE` and transitions to `ui_show_dashboard()`. The GTK signal callback `on_login_clicked()` is `static` — invisible outside this file.

### `src/ui/dashboard_window.c` — ~150 lines
Main application window. Displays the credential service list via a `GtkListStore`. `ui_refresh_services()` clears and repopulates the list view from `app->credentials`. Buttons navigate to add/view/delete flows.

### `src/ui/add_credential_window.c` — ~80 lines
Credential input form. Collects service name, username, and password. On confirm, calls `vs_add_credential()` then `ui_refresh_services()`. The confirm callback is `static`.

---

## 📂 File Structure

```
password_vault/
│
├── src/
│   ├── main.c                           ← Entry point (~25 lines)
│   ├── app.c                            ← AppState lifecycle
│   ├── auth.c                           ← Master password & recovery
│   ├── vault.c                          ← Credential management
│   ├── crypto.c                         ← XOR encrypt/decrypt
│   ├── file_io.c                        ← vault.dat read/write
│   └── ui/
│       ├── login_window.c               ← GTK login screen
│       ├── dashboard_window.c           ← Main dashboard
│       ├── add_credential_window.c      ← Add credential form
│       └── view_credentials_window.c   ← Credential viewer
│
├── include/
│   ├── app.h                            ← AppState, Credential structs + constants
│   ├── auth.h                           ← ma_* declarations
│   ├── vault.h                          ← vs_* declarations
│   ├── crypto.h                         ← crypto_encrypt / crypto_decrypt
│   ├── file_io.h                        ← fio_save_vault / fio_load_vault
│   └── ui/
│       ├── login_window.h
│       ├── dashboard_window.h
│       ├── add_credential_window.h
│       └── view_credentials_window.h
│
├── data/
│   └── vault.dat                        ← Encrypted credential storage
│
└── Makefile
```

---

## 🔐 Encryption Workflow

### Adding a Credential
```
User Input (plaintext password)
        │
        ▼
vs_add_credential(AppState *app, ...)
        │
        ▼
crypto_encrypt(password, app->session_master, encrypted_buf)
        │
        ▼
Appended to app->credentials (stored encrypted)
        │
        ▼
fio_save_vault(AppState *app)  ──▶  vault.dat
```

### Retrieving a Credential
```
vault.dat
        │
        ▼
fio_load_vault(AppState *app)
        │
        ▼
crypto_decrypt(encrypted_buf, app->session_master, plaintext)
        │
        ▼
Loaded into app->credentials
        │
        ▼
ui_refresh_services(AppState *app)  ──▶  GTK list view
```

> ⚠️ **Note:** The current implementation uses XOR-based encryption, which is suitable for a lab context. For production use, AES-256 with PBKDF2 key derivation is strongly recommended.

---

## 📖 Documentation Standard

Every function in every header file follows this Doxygen block convention:

```c
/**
 * @brief One-sentence description of what the function does.
 *
 * @param param_name  Description of the parameter and expected format.
 * @return            Description of return value and error conditions.
 */
```

**Example — `vault.h`:**
```c
/**
 * @brief Adds a new credential entry to the vault and saves to disk.
 *
 * @param app       Pointer to the central application state.
 * @param service   Null-terminated string for the service name.
 * @param username  Null-terminated string for the username.
 * @param password  Null-terminated plaintext password to encrypt and store.
 * @return void
 */
void vs_add_credential(AppState *app, const char *service,
                        const char *username, const char *password);
```

**Example — `crypto.h`:**
```c
/**
 * @brief Encrypts a plaintext buffer using XOR with the provided key.
 *
 * @param input   Null-terminated plaintext string to encrypt.
 * @param key     Null-terminated key derived from the master password.
 * @param output  Caller-allocated buffer to receive the encrypted result.
 * @return void
 */
void crypto_encrypt(const char *input, const char *key, char *output);
```

---

## 🤖 AI Refactoring Prompts: ver1 → ver2

These are the exact prompts used to drive each refactoring pass from the monolithic ver1 to the modular ver2.

---

### Prompt 1 — Split monolith into modules

```
I have a single monolithic C file called main.c that contains GTK window
construction, master password authentication, credential management, XOR
encryption, and file I/O all in one place.

Refactor it into the following modular file structure without changing any logic:

  include/app.h       — AppState struct, Credential struct, and all #define constants
  include/auth.h      — declarations for authentication functions
  include/vault.h     — declarations for credential management functions
  include/crypto.h    — declarations for encryption/decryption functions
  include/file_io.h   — declarations for vault save/load functions
  include/ui/login_window.h
  include/ui/dashboard_window.h
  include/ui/add_credential_window.h
  include/ui/view_credentials_window.h

  src/main.c          — gtk_init, AppState allocation, ui_show_login_window, gtk_main only
  src/app.c           — AppState init and cleanup
  src/auth.c          — master password verification and recovery
  src/vault.c         — credential list add, delete, load
  src/crypto.c        — XOR encrypt and decrypt
  src/file_io.c       — vault.dat read and write
  src/ui/*.c          — one file per GTK window

Rules:
- Do not change any logic, algorithms, or values
- Each .c file must #include only the headers it actually needs
- No global variables — all shared state must go into AppState defined in app.h
- Header files must use include guards (#ifndef / #define / #endif)
- GTK signal callbacks must be declared static in their .c file
```

---

### Prompt 2 — Eliminate global variables with `AppState`

```
The following C file uses global variables for all application state:
  GtkWidget *main_window, GList *credential_list, char master_pass[256],
  gboolean login_success.

Refactor by:
1. Creating a typedef struct called AppState in include/app.h that holds
   all runtime state as named fields:
     session_master[MAX_LEN], credentials, main_window, login_success

2. Removing all global variable declarations

3. Declaring AppState *app in main() using g_new0(AppState, 1) for
   zero-initialization

4. Updating every function that previously read from globals to instead accept
   AppState* as its first parameter and access state through that pointer

5. GTK signal callbacks that need AppState must receive it through the
   gpointer user_data parameter and cast it:
     AppState *app = (AppState*)user_data;

Do not change any logic. Show the updated function signatures and the AppState struct.
```

---

### Prompt 3 — Apply naming conventions throughout

```
Apply the following naming conventions consistently to every identifier
in the entire codebase:

  Local variables     → snake_case
  Parameters          → snake_case
  Struct type names   → PascalCase   (AppState, Credential)
  Struct members      → snake_case   (session_master, login_success)
  Functions           → module_verb_object() style (see prefix table below)
  Header guards       → UPPER_SNAKE_CASE_H

Module prefix table:
  auth.c functions    → ma_   (e.g. ma_verify_master, ma_handle_recovery)
  vault.c functions   → vs_   (e.g. vs_add_credential, vs_load_all_credentials)
  file_io.c functions → fio_  (e.g. fio_save_vault, fio_load_vault)
  crypto.c functions  → crypto_ (e.g. crypto_encrypt, crypto_decrypt)
  ui/*.c functions    → ui_   (e.g. ui_show_login_window, ui_show_dashboard)

Apply every rename at both the definition site and all call sites.
Do not change any logic or struct field values.
```

---

### Prompt 4 — Encapsulate encryption behind `crypto.c`

```
Currently, XOR encryption logic is written inline in multiple places —
once inside the save function and once inside the load function:

  // Scattered inline — duplicated logic
  for (int i = 0; i < strlen(password); i++)
      encrypted[i] = password[i] ^ master_pass[i % strlen(master_pass)];

Refactor by:
1. Creating crypto_encrypt() and crypto_decrypt() in src/crypto.c:
   - Both accept: const char *input, const char *key, char *output
   - Since XOR is symmetric, crypto_decrypt() may call crypto_encrypt() directly
   - These functions must have NO dependency on AppState or GTK

2. Removing all inline XOR logic from file_io.c, vault.c, and anywhere else

3. Replacing every inline encryption site with a call to crypto_encrypt()
   or crypto_decrypt() using app->session_master as the key

After this change, swapping the encryption algorithm must require editing
only crypto.c — no other file should need modification.
```

---

### Prompt 5 — Add null safety to cleanup

```
The current cleanup code frees resources unconditionally, which will crash
if initialization partially failed:

  g_list_free(credential_list);    // unsafe if NULL
  gtk_widget_destroy(main_window); // unsafe if NULL

Rewrite app_cleanup(AppState *app) so every resource is null-checked
before being freed:

  if (handle) free_function(handle);

Required cleanup order:
  1. if (app->credentials)  g_list_free_full(app->credentials, g_free)
  2. if (app->main_window)  gtk_widget_destroy(app->main_window)
  3. g_free(app)

Do not change anything else.
```

---

### Prompt 6 — Mark internal functions as `static`

```
In each .c module, identify every function that is only used within that
single file and is NOT declared in the corresponding .h header.
Add the static keyword to each such function to enforce information hiding.

Functions to mark static:

  vault.c:
    — vs_find_duplicate()

  login_window.c:
    — build_login_widgets()
    — on_login_clicked()           (GTK callback)

  dashboard_window.c:
    — build_dashboard_widgets()
    — on_delete_clicked()          (GTK callback)

  add_credential_window.c:
    — build_add_form_widgets()
    — on_add_confirm_clicked()     (GTK callback)

  view_credentials_window.c:
    — build_view_widgets()

Do not change any logic. Only add the static keyword to the listed functions.
```

---

### Prompt 7 — Add Doxygen documentation to all function declarations

```
Add a Doxygen comment block to every function declaration in every header file.
Use this exact format:

/**
 * @brief One sentence describing what the function does.
 *
 * @param param_name  Description of the parameter.
 * @return            Description of the return value, or void.
 */

Rules:
- @brief must fit on one line
- Every parameter must have its own @param line
- If the function returns void, write: @return void
- GTK callbacks that have unused parameters should note them as (unused)
- Do not change any function signatures or logic
- Apply to every function in: app.h, auth.h, vault.h, crypto.h, file_io.h,
  and all ui/*.h headers
```

---

### Prompt 8 — Full end-to-end refactor (single master prompt)

> Use this to perform the entire ver1 → ver2 transformation in one pass.

```
I have a monolithic GTK password manager written in C as a single main.c file.
Refactor it completely into a professional modular architecture following all
of these rules simultaneously:

FILE STRUCTURE
- Split into: src/main.c, src/app.c, src/auth.c, src/vault.c, src/crypto.c,
  src/file_io.c, src/ui/login_window.c, src/ui/dashboard_window.c,
  src/ui/add_credential_window.c, src/ui/view_credentials_window.c
- Create matching headers in include/ and include/ui/
- Use #ifndef include guards on all headers

GLOBAL VARIABLES
- Remove all global variables
- Bundle all runtime state into typedef struct AppState in include/app.h:
    session_master[MAX_LEN], credentials (GList*), main_window, login_success
- Allocate with g_new0(AppState, 1) in main()
- Pass AppState* as first parameter to every function that needs state
- GTK callbacks receive it via gpointer user_data, cast to AppState*

NAMING CONVENTIONS
- Local variables and parameters: snake_case
- Struct type names: PascalCase (AppState, Credential)
- Struct members: snake_case
- Functions: module_verb_object() with prefixes:
    ma_ for auth.c, vs_ for vault.c, fio_ for file_io.c,
    crypto_ for crypto.c, ui_ for ui/*.c
- Header guards: UPPER_SNAKE_CASE_H

ENCRYPTION
- All XOR logic moves exclusively to crypto.c (crypto_encrypt, crypto_decrypt)
- file_io.c delegates entirely — no inline XOR anywhere else
- crypto.c must have zero dependency on AppState or GTK

CLEANUP SAFETY
- Null-check every handle before calling g_list_free_full, gtk_widget_destroy, g_free

STATIC SCOPE
- Mark all internal-only functions static in their .c files
- GTK signal callbacks must always be static
- Only functions declared in .h headers have external linkage

DOCUMENTATION
- Add @brief/@param/@return Doxygen block to every function declaration in every header

OUTPUT: Provide all files as separate clearly labeled code blocks.
Do not change any game logic, values, or algorithms beyond what is listed above.
```

---

## 🧠 Design Principles Applied

| Principle | ver1 Status | ver2 Implementation |
|---|---|---|
| **Separation of Concerns** | ❌ All logic in one file | ✅ Auth, vault, crypto, file I/O, and UI each in isolated modules |
| **Dependency Injection** | ❌ Functions silently read globals | ✅ `AppState*` passed explicitly to every function that needs state |
| **Single Responsibility** | ❌ `main.c` does everything | ✅ Each `.c` file has exactly one clear purpose |
| **Data Encapsulation** | ❌ All state globally mutable | ✅ State owned by `AppState`; internal helpers marked `static` |
| **Module Namespacing** | ❌ Flat inconsistent naming | ✅ `ma_*`, `vs_*`, `fio_*`, `crypto_*`, `ui_*` prefixes throughout |
| **DRY** | ❌ XOR logic duplicated in 2+ places | ✅ Encryption lives only in `crypto.c` |
| **Information Hiding** | ❌ All functions globally linked | ✅ Internal functions and GTK callbacks marked `static` |
| **Null Safety** | ❌ Unconditional free in cleanup | ✅ Every resource null-checked before free |
| **Layered Architecture** | ❌ No layers | ✅ UI → Logic → Infrastructure → Foundation; zero circular dependencies |
| **Extensibility** | ❌ Changing crypto touches multiple files | ✅ Swap encryption algorithm by editing only `crypto.c` |

---

## ⚙️ How to Build

### Prerequisites

| Requirement | Detail |
|---|---|
| **Compiler** | GCC (MinGW-w64 on Windows / GCC on Linux) |
| **GTK+** | GTK 3.x development libraries |
| **Platform** | Windows (MSYS2 / UCRT64) or Linux |

### Via Makefile
```bash
make
./password_vault
```

### Manual (Windows MSYS2)
```powershell
gcc src/*.c src/ui/*.c -o password_vault.exe \
    -Iinclude \
    $(pkg-config --cflags --libs gtk+-3.0)
```

### Manual (Linux)
```bash
gcc src/*.c src/ui/*.c -o password_vault \
    -Iinclude \
    $(pkg-config --cflags --libs gtk+-3.0)
```

---

*Password Vault — Advanced Programming Laboratory · CSE Discipline · February 2026*
