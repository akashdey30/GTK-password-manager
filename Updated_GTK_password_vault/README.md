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

**PASSWORD VAULT** is a GTK-based desktop password manager written in C. It allows users to securely store, retrieve, and manage credentials protected by a master password and a recovery question system. All credential data is encrypted before being written to disk across three separate data files.

The project was developed in two distinct phases:

| Version | Description |
|---|---|
| **ver1** | Single monolithic `main.c` — all GUI, logic, encryption, and file I/O in one file with global variables |
| **ver2** | Modular architecture — 7 source files, 8 headers, 3 data files, 0 global variables, full Doxygen documentation |

---

## 📊 Version Comparison at a Glance

| Metric | ver1 (`main.c`) | ver2 (Modular) | Change |
|---|---|---|---|
| **Source files (.c)** | 1 | 7 | Full modular separation |
| **Header files (.h)** | 0 | 8 | Clean public interfaces |
| **Data files** | 1 (`vault.dat`) | 3 (`vault.dat`, `vault_creds.dat`, `vault_master.dat`) | Separated by concern |
| **Global variables** | **Multiple** (`main_window`, `credential_list`, `master_pass`, `login_success`, etc.) | **0** | Complete elimination |
| **Naming convention — functions** | Mixed (`LoadCredentials`, `crypto_encrypt`, `refresh_ui`) | Uniform descriptive style per module | Consistent throughout |
| **Struct fields** | Mixed `snake_case` and abbreviated names | Uniform `snake_case` with descriptive names | Consistent throughout |
| **Struct type names** | Inconsistent casing | All `PascalCase` (`AppState`, `Credential`) | Standard enforced |
| **Encryption calls** | Scattered raw XOR inline across multiple functions | Encapsulated behind `encryption.h` | Single responsibility |
| **File I/O** | Inline in multiple places | Isolated to `vault_storage.c` | Encapsulated |
| **Dialog management** | Inline GTK dialog code mixed with logic | Isolated to `dialogs.c` | Encapsulated |
| **Cleanup safety** | Unconditional free — crash risk | All handles null-checked before free | Crash-safe |
| **Documentation** | Zero comments | Full Doxygen `@brief/@param/@return` on every function | Fully documented |
| **Context passing** | Functions read globals directly | `AppState*` passed explicitly | Dependency Injection |

---

## 🔄 What Changed: ver1 → ver2 (Full Code Analysis)

### 1. File Structure

**ver1 — Everything in one flat file:**
```
password_vault/
├── main.c          ← All logic: GUI, auth, encryption, file I/O, dialogs, main loop
├── vault.dat       ← Single data file for everything
└── Makefile
```

**ver2 — Organized into clean, purposeful layers:**
```
Updated_GTK_password_vault/
│
├── data/
│   ├── vault.dat           ← General vault metadata
│   ├── vault_creds.dat     ← Encrypted credential entries
│   └── vault_master.dat    ← Encrypted master password record
│
├── include/
│   ├── app_state.h         ← AppState struct + all constants
│   ├── credential.h        ← Credential struct definition
│   ├── dialogs.h           ← GTK dialog function declarations
│   ├── encryption.h        ← Encrypt/decrypt function declarations
│   ├── master_auth.h       ← Master password auth declarations
│   ├── service_manager.h   ← Credential CRUD declarations
│   ├── ui.h                ← UI construction declarations
│   └── vault_storage.h     ← File I/O declarations
│
└── src/
    ├── main.c              ← Entry point only (~25 lines)
    ├── dialogs.c           ← All GTK dialog construction
    ├── encryption.c        ← XOR encrypt/decrypt logic
    ├── master_auth.c       ← Master password & recovery
    ├── service_manager.c   ← Credential add/delete/list
    ├── ui.c                ← GTK window construction & wiring
    └── vault_storage.c     ← vault_creds.dat & vault_master.dat I/O
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

**ver2 — Zero globals. All state bundled into `AppState` in `app_state.h`:**
```c
// app_state.h — inject exactly what each function needs, nothing more
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
| Functions | Mixed flat naming | Descriptive `module_verb_object()` | Always |
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

Every function was renamed from inconsistent flat naming to a clear **module-descriptive** format. Internal helpers were also marked `static`.

| ver1 (flat/mixed) | ver2 (module-prefixed) | Module | Visibility |
|---|---|---|---|
| `LoadCredentials()` | `service_manager_load_all()` | `service_manager.c` | public |
| `SaveVault()` | `vault_storage_save_creds()` | `vault_storage.c` | public |
| `LoadVault()` | `vault_storage_load_creds()` | `vault_storage.c` | public |
| `SaveMaster()` | `vault_storage_save_master()` | `vault_storage.c` | public |
| `LoadMaster()` | `vault_storage_load_master()` | `vault_storage.c` | public |
| `crypto_encrypt()` | `encryption_encrypt()` | `encryption.c` | public |
| `crypto_decrypt()` | `encryption_decrypt()` | `encryption.c` | public |
| `verify_master()` | `master_auth_verify()` | `master_auth.c` | public |
| `handle_recovery()` | `master_auth_recover()` | `master_auth.c` | public |
| `setup_master()` | `master_auth_setup()` | `master_auth.c` | public |
| `add_cred()` | `service_manager_add()` | `service_manager.c` | public |
| `delete_cred()` | `service_manager_delete()` | `service_manager.c` | public |
| `show_error_dialog()` | `dialogs_show_error()` | `dialogs.c` | public |
| `show_input_dialog()` | `dialogs_show_input()` | `dialogs.c` | public |
| `show_confirm_dialog()` | `dialogs_show_confirm()` | `dialogs.c` | public |
| `refresh_ui()` | `ui_refresh_services()` | `ui.c` | public |
| `build_main_window()` | `ui_build_main_window()` | `ui.c` | public |
| `find_duplicate()` | `service_manager_find_dup()` | `service_manager.c` | `static` |
| `build_login_form()` | `build_login_form()` | `ui.c` | `static` |

**Module naming reference:**

| Module | Header | Owns |
|---|---|---|
| `master_auth.c` | `master_auth.h` | Master password setup, verify, recovery |
| `service_manager.c` | `service_manager.h` | Credential add, delete, list, search |
| `vault_storage.c` | `vault_storage.h` | Read/write `vault_creds.dat` & `vault_master.dat` |
| `encryption.c` | `encryption.h` | XOR encrypt and decrypt — no other dependencies |
| `dialogs.c` | `dialogs.h` | All GTK dialog construction and display |
| `ui.c` | `ui.h` | GTK window construction, wiring, refresh |
| `main.c` | — | Entry point only — init, launch, cleanup |

---

### 5. Encryption Workflow Encapsulation

**ver1 — XOR logic duplicated inline in multiple functions:**
```c
// Inside save function — mixes I/O with crypto
for (int i = 0; i < strlen(password); i++) {
    encrypted[i] = password[i] ^ master_pass[i % strlen(master_pass)];
}
fwrite(encrypted, 1, len, file);

// Inside load function — same XOR written again
for (int i = 0; i < len; i++) {
    decrypted[i] = raw[i] ^ master_pass[i % strlen(master_pass)];
}
```

**ver2 — Single responsibility. `encryption.c` is the only place encryption logic lives:**
```c
// encryption.c — owns the algorithm entirely
void encryption_encrypt(const char *input, const char *key, char *output) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i < strlen(input); i++) {
        output[i] = input[i] ^ key[i % key_len];
    }
}

void encryption_decrypt(const char *input, const char *key, char *output) {
    encryption_encrypt(input, key, output);  // XOR is symmetric
}

// vault_storage.c — delegates entirely, knows nothing about XOR
void vault_storage_save_creds(AppState *app) {
    encryption_encrypt(raw_password, app->session_master, encrypted_buf);
    fwrite(encrypted_buf, 1, len, file);
}
```

Swapping XOR for AES now requires changing **only `encryption.c`** — zero other files touched.

---

### 6. Data File Separation

**ver1 — One file stores everything mixed together:**
```
vault.dat  ←  master password hash + all credentials interleaved
```

**ver2 — Separated by concern, each file has single responsibility:**
```
vault.dat          ←  vault metadata / general state
vault_master.dat   ←  encrypted master password record only
vault_creds.dat    ←  encrypted credential entries only
```

This means `vault_storage_save_master()` and `vault_storage_save_creds()` are fully independent operations — changing the master password never touches credential storage and vice versa.

---

### 7. Cleanup Null Safety

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

### 8. Static Scope — Information Hiding

ver2 explicitly marks internal functions `static`, preventing accidental linkage from other translation units:

| Symbol | ver1 scope | ver2 scope |
|---|---|---|
| `service_manager_find_dup()` | global linkage | `static` in `service_manager.c` |
| `build_login_form()` | global linkage | `static` in `ui.c` |
| `build_main_layout()` | global linkage | `static` in `ui.c` |
| `on_login_clicked()` (GTK callback) | global linkage | `static` in `ui.c` |
| `on_delete_clicked()` (GTK callback) | global linkage | `static` in `ui.c` |
| `on_add_clicked()` (GTK callback) | global linkage | `static` in `ui.c` |

Only functions declared in a `.h` header have external linkage. Everything else is module-private.

---

## 🏛️ Architecture

```
╔══════════════════════════════════════════════════════════════════╗
║                        UI Layer                                  ║
║                                                                  ║
║   ┌──────────────────────────────────────────────────────────┐  ║
║   │                        ui.c                              │  ║
║   │                                                          │  ║
║   │  ui_build_main_window()    ui_refresh_services()         │  ║
║   │  [static] build_login_form()                             │  ║
║   │  [static] on_login_clicked / on_add_clicked / on_delete  │  ║
║   └──────────────────────────────────────────────────────────┘  ║
║                                                                  ║
║   ┌──────────────────────────────────────────────────────────┐  ║
║   │                      dialogs.c                           │  ║
║   │                                                          │  ║
║   │  dialogs_show_error()   dialogs_show_input()             │  ║
║   │  dialogs_show_confirm()                                  │  ║
║   └──────────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════════╗
║                    Business Logic Layer                          ║
║                                                                  ║
║  ┌───────────────────────┐    ┌──────────────────────────────┐  ║
║  │     master_auth.c     │    │       service_manager.c      │  ║
║  │                       │    │                              │  ║
║  │ master_auth_verify()  │    │ service_manager_load_all()   │  ║
║  │ master_auth_recover() │    │ service_manager_add()        │  ║
║  │ master_auth_setup()   │    │ service_manager_delete()     │  ║
║  └───────────────────────┘    │ [static] find_dup()          │  ║
║                               └──────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════════╗
║                   Infrastructure Layer                           ║
║                                                                  ║
║  ┌─────────────────────────────┐  ┌─────────────────────────┐  ║
║  │       vault_storage.c       │  │      encryption.c        │  ║
║  │                             │  │                          │  ║
║  │ vault_storage_save_creds()  │─▶│ encryption_encrypt()     │  ║
║  │ vault_storage_load_creds()  │◀─│ encryption_decrypt()     │  ║
║  │ vault_storage_save_master() │  │                          │  ║
║  │ vault_storage_load_master() │  │  (no AppState or GTK     │  ║
║  └─────────────────────────────┘  │   dependency — pure      │  ║
║                                   │   char buffer utility)   │  ║
║  Data files:                      └─────────────────────────┘  ║
║    vault.dat                                                     ║
║    vault_creds.dat    ← credentials only                         ║
║    vault_master.dat   ← master password only                     ║
╚══════════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════════╗
║                     Foundation Layer                             ║
║                                                                  ║
║  ┌─────────────────────────┐  ┌──────────────────────────────┐  ║
║  │       app_state.h       │  │        credential.h          │  ║
║  │                         │  │                              │  ║
║  │ typedef AppState {      │  │ typedef Credential {         │  ║
║  │   session_master,       │  │   service_name,              │  ║
║  │   credentials,          │  │   username,                  │  ║
║  │   main_window,          │  │   password                   │  ║
║  │   login_success         │  │ }                            │  ║
║  │ }                       │  │                              │  ║
║  │ #define MAX_LEN         │  │                              │  ║
║  └─────────────────────────┘  └──────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════╝
```

**Dependency rule:** UI → Business Logic → Infrastructure → Foundation. No upward dependencies. No circular includes.

---

## 🧩 Module Breakdown

### `src/main.c` — ~25 lines
Minimal entry point. Calls `gtk_init()`, allocates `AppState` with `g_new0()`, launches the UI, runs `gtk_main()`, then calls `app_cleanup()`. Nothing else lives here.

### `src/master_auth.c` / `include/master_auth.h`
All authentication logic. `master_auth_setup()` handles first-run master password creation. `master_auth_verify()` compares input against `vault_master.dat`. `master_auth_recover()` validates the recovery answer and allows password reset. No GTK widget construction happens here — dialog prompts are delegated to `dialogs.c`.

### `src/service_manager.c` / `include/service_manager.h`
Credential list management. `service_manager_add()` appends to `app->credentials` and triggers a vault save. `service_manager_delete()` removes by index. `service_manager_load_all()` populates the list from the decrypted `vault_creds.dat`. The `static` helper `service_manager_find_dup()` prevents storing the same service name twice.

### `src/vault_storage.c` / `include/vault_storage.h`
Encrypted disk persistence, split across two data files. `vault_storage_save_creds()` and `vault_storage_load_creds()` operate on `vault_creds.dat`. `vault_storage_save_master()` and `vault_storage_load_master()` operate on `vault_master.dat`. All encryption is delegated to `encryption.c` — no XOR logic lives here.

### `src/encryption.c` / `include/encryption.h`
Pure encryption utility — no GTK, no `AppState`, no file I/O. `encryption_encrypt()` and `encryption_decrypt()` operate only on char buffers and a key string. Because XOR is symmetric, decrypt delegates directly to encrypt. Swapping the algorithm requires touching **only this file**.

### `src/dialogs.c` / `include/dialogs.h`
All GTK dialog construction in one place. `dialogs_show_error()`, `dialogs_show_input()`, and `dialogs_show_confirm()` centralize every pop-up in the application. Other modules call these instead of constructing `GtkDialog` inline.

### `src/ui.c` / `include/ui.h`
GTK window construction and event wiring. `ui_build_main_window()` constructs the full application layout. `ui_refresh_services()` clears and repopulates the credential list view from `app->credentials`. All GTK signal callbacks (`on_login_clicked`, `on_add_clicked`, `on_delete_clicked`) are `static` — invisible outside this file.

---

## 📂 File Structure

```
Updated_GTK_password_vault/
│
├── data/
│   ├── vault.dat              ← Vault metadata
│   ├── vault_creds.dat        ← Encrypted credential entries
│   └── vault_master.dat       ← Encrypted master password record
│
├── include/
│   ├── app_state.h            ← AppState struct + constants
│   ├── credential.h           ← Credential struct definition
│   ├── dialogs.h              ← GTK dialog declarations
│   ├── encryption.h           ← encryption_encrypt / encryption_decrypt
│   ├── master_auth.h          ← master_auth_* declarations
│   ├── service_manager.h      ← service_manager_* declarations
│   ├── ui.h                   ← ui_* declarations
│   └── vault_storage.h        ← vault_storage_* declarations
│
└── src/
    ├── main.c                 ← Entry point (~25 lines)
    ├── dialogs.c              ← All GTK dialogs
    ├── encryption.c           ← XOR encrypt/decrypt
    ├── master_auth.c          ← Master password & recovery
    ├── service_manager.c      ← Credential management
    ├── ui.c                   ← GTK window & callbacks
    └── vault_storage.c        ← vault_creds.dat & vault_master.dat I/O
```

---

## 🔐 Encryption Workflow

### Adding a Credential
```
User Input (plaintext password)
        │
        ▼
service_manager_add(AppState *app, ...)
        │
        ▼
encryption_encrypt(password, app->session_master, encrypted_buf)
        │
        ▼
Appended to app->credentials (stored encrypted)
        │
        ▼
vault_storage_save_creds(AppState *app)  ──▶  vault_creds.dat
```

### Retrieving Credentials
```
vault_creds.dat
        │
        ▼
vault_storage_load_creds(AppState *app)
        │
        ▼
encryption_decrypt(encrypted_buf, app->session_master, plaintext)
        │
        ▼
Loaded into app->credentials
        │
        ▼
ui_refresh_services(AppState *app)  ──▶  GTK list view
```

### Master Password Flow
```
First Run                          Subsequent Runs
    │                                    │
    ▼                                    ▼
master_auth_setup()              master_auth_verify()
    │                                    │
    ▼                                    ▼
encryption_encrypt(master, key)  encryption_decrypt(stored, key)
    │                                    │
    ▼                                    ▼
vault_storage_save_master()      Compare → set app->login_success
```

> ⚠️ **Note:** The current implementation uses XOR-based encryption, suitable for a lab context. For production use, AES-256 with PBKDF2 key derivation is strongly recommended.

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

**Example — `service_manager.h`:**
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
void service_manager_add(AppState *app, const char *service,
                          const char *username, const char *password);
```

**Example — `encryption.h`:**
```c
/**
 * @brief Encrypts a plaintext buffer using XOR with the provided key.
 *
 * @param input   Null-terminated plaintext string to encrypt.
 * @param key     Null-terminated key derived from the master password.
 * @param output  Caller-allocated buffer to receive the encrypted result.
 * @return void
 */
void encryption_encrypt(const char *input, const char *key, char *output);
```

**Example — `dialogs.h`:**
```c
/**
 * @brief Displays a modal error dialog with the given message.
 *
 * @param parent   The parent GtkWindow for the dialog, or NULL.
 * @param message  Null-terminated error message string to display.
 * @return void
 */
void dialogs_show_error(GtkWidget *parent, const char *message);
```

---

## 🤖 AI Refactoring Prompts: ver1 → ver2

These are the exact prompts used to drive each refactoring pass from the monolithic ver1 to the modular ver2.

---

### Prompt 1 — Split monolith into modules

```
I have a single monolithic C file called main.c that contains GTK window
construction, master password authentication, credential management, XOR
encryption, dialog management, and file I/O all in one place.

Refactor it into the following modular file structure without changing any logic:

  include/app_state.h      — AppState struct and all #define constants
  include/credential.h     — Credential struct definition
  include/dialogs.h        — declarations for GTK dialog functions
  include/encryption.h     — declarations for encrypt/decrypt functions
  include/master_auth.h    — declarations for authentication functions
  include/service_manager.h — declarations for credential CRUD functions
  include/ui.h             — declarations for UI construction functions
  include/vault_storage.h  — declarations for file I/O functions

  src/main.c               — gtk_init, AppState allocation, ui launch, gtk_main only
  src/dialogs.c            — all GTK dialog construction
  src/encryption.c         — XOR encrypt and decrypt only
  src/master_auth.c        — master password setup, verify, recovery
  src/service_manager.c    — credential add, delete, load, search
  src/ui.c                 — GTK window construction and signal wiring
  src/vault_storage.c      — vault_creds.dat and vault_master.dat read/write

  data/vault.dat           — vault metadata
  data/vault_creds.dat     — credential entries (separate from master)
  data/vault_master.dat    — master password record (separate from credentials)

Rules:
- Do not change any logic, algorithms, or values
- Each .c file must #include only the headers it actually needs
- No global variables — all shared state goes into AppState defined in app_state.h
- Header files must use include guards (#ifndef / #define / #endif)
- GTK signal callbacks must be declared static in ui.c
- Master password storage and credential storage must be in separate data files
```

---

### Prompt 2 — Eliminate global variables with `AppState`

```
The following C file uses global variables for all application state:
  GtkWidget *main_window, GList *credential_list, char master_pass[256],
  gboolean login_success.

Refactor by:
1. Creating a typedef struct called AppState in include/app_state.h that holds
   all runtime state as named fields:
     session_master[MAX_LEN], credentials, main_window, login_success

2. Creating a separate typedef struct called Credential in include/credential.h:
     service_name[MAX_LEN], username[MAX_LEN], password[MAX_LEN]

3. Removing all global variable declarations

4. Declaring AppState *app in main() using g_new0(AppState, 1)

5. Updating every function that previously read from globals to instead accept
   AppState* as its first parameter

6. GTK signal callbacks receive AppState* via gpointer user_data:
     AppState *app = (AppState*)user_data;

Do not change any logic. Show the updated function signatures and both structs.
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
  Functions           → module_verb_object() style (see table below)
  Header guards       → UPPER_SNAKE_CASE_H

Function naming table by module:
  master_auth.c    → master_auth_setup(), master_auth_verify(), master_auth_recover()
  service_manager.c → service_manager_add(), service_manager_delete(),
                      service_manager_load_all()
  vault_storage.c  → vault_storage_save_creds(), vault_storage_load_creds(),
                      vault_storage_save_master(), vault_storage_load_master()
  encryption.c     → encryption_encrypt(), encryption_decrypt()
  dialogs.c        → dialogs_show_error(), dialogs_show_input(), dialogs_show_confirm()
  ui.c             → ui_build_main_window(), ui_refresh_services()

Apply every rename at both the definition site and all call sites.
Do not change any logic or struct field values.
```

---

### Prompt 4 — Separate data files by concern

```
Currently all persistent data is written to a single vault.dat file,
mixing master password data and credential entries together.

Refactor vault_storage.c to use two separate data files:

  vault_master.dat  — stores ONLY the encrypted master password record
  vault_creds.dat   — stores ONLY the encrypted credential entries

Create four separate functions:
  vault_storage_save_master(AppState *app)  — writes to vault_master.dat only
  vault_storage_load_master(AppState *app)  — reads from vault_master.dat only
  vault_storage_save_creds(AppState *app)   — writes to vault_creds.dat only
  vault_storage_load_creds(AppState *app)   — reads from vault_creds.dat only

After this change:
- Changing the master password must NOT rewrite credential data
- Adding a credential must NOT rewrite master password data
- Both files must use encryption_encrypt() / encryption_decrypt() for all data

Do not change any logic outside vault_storage.c.
```

---

### Prompt 5 — Encapsulate encryption behind `encryption.c`

```
Currently, XOR encryption logic is written inline in multiple places —
once inside the save function and once inside the load function:

  for (int i = 0; i < strlen(password); i++)
      encrypted[i] = password[i] ^ master_pass[i % strlen(master_pass)];

Refactor by:
1. Creating encryption_encrypt() and encryption_decrypt() in src/encryption.c:
   - Both accept: const char *input, const char *key, char *output
   - Since XOR is symmetric, encryption_decrypt() may call encryption_encrypt()
   - These functions must have NO dependency on AppState or GTK

2. Removing all inline XOR logic from vault_storage.c and anywhere else

3. Replacing every inline encryption site with encryption_encrypt() or
   encryption_decrypt() using app->session_master as the key

After this change, swapping the encryption algorithm requires editing
only encryption.c — no other file should need modification.
```

---

### Prompt 6 — Centralise all GTK dialogs into `dialogs.c`

```
Currently, GTK dialog construction (GtkDialog, gtk_message_dialog_new, etc.)
is scattered inline across multiple functions in different source files.

Refactor by:
1. Creating src/dialogs.c and include/dialogs.h

2. Moving all GTK dialog construction into three functions:
     void dialogs_show_error(GtkWidget *parent, const char *message)
     char *dialogs_show_input(GtkWidget *parent, const char *prompt)
     gboolean dialogs_show_confirm(GtkWidget *parent, const char *question)

3. Replacing every inline GtkDialog construction site in master_auth.c,
   service_manager.c, and ui.c with the appropriate dialogs_* call

Rules:
- dialogs.c may depend on GTK but must NOT depend on AppState
- dialogs_show_input() returns a newly allocated string — caller must g_free() it
- dialogs_show_confirm() returns TRUE if user confirmed, FALSE otherwise
```

---

### Prompt 7 — Add null safety to cleanup

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

### Prompt 8 — Mark internal functions as `static`

```
In each .c module, identify every function that is only used within that
single file and is NOT declared in the corresponding .h header.
Add the static keyword to each such function to enforce information hiding.

Functions to mark static:

  service_manager.c:
    — service_manager_find_dup()

  ui.c:
    — build_login_form()
    — build_main_layout()
    — on_login_clicked()        (GTK callback)
    — on_add_clicked()          (GTK callback)
    — on_delete_clicked()       (GTK callback)

Do not change any logic. Only add the static keyword to the listed functions.
```

---

### Prompt 9 — Add Doxygen documentation to all function declarations

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
- GTK callbacks with unused parameters should note them as (unused)
- Do not change any function signatures or logic
- Apply to every function in all 8 header files
```

---

### Prompt 10 — Full end-to-end refactor (single master prompt)

> Use this to perform the entire ver1 → ver2 transformation in one pass.

```
I have a monolithic GTK password manager written in C as a single main.c file.
Refactor it completely into a professional modular architecture following all
of these rules simultaneously:

FILE STRUCTURE
- Split into: src/main.c, src/dialogs.c, src/encryption.c, src/master_auth.c,
  src/service_manager.c, src/ui.c, src/vault_storage.c
- Create 8 headers in include/: app_state.h, credential.h, dialogs.h,
  encryption.h, master_auth.h, service_manager.h, ui.h, vault_storage.h
- Use #ifndef include guards on all headers
- Separate data storage into: data/vault.dat, data/vault_creds.dat,
  data/vault_master.dat

GLOBAL VARIABLES
- Remove all global variables
- Bundle all runtime state into typedef struct AppState in include/app_state.h:
    session_master[MAX_LEN], credentials (GList*), main_window, login_success
- Define typedef struct Credential in include/credential.h:
    service_name, username, password
- Allocate AppState with g_new0(AppState, 1) in main()
- Pass AppState* as first parameter to every function that needs state
- GTK callbacks receive it via gpointer user_data, cast to AppState*

NAMING CONVENTIONS
- Local variables and parameters: snake_case
- Struct type names: PascalCase (AppState, Credential)
- Struct members: snake_case
- Functions: module_verb_object() — see naming table in Prompt 3
- Header guards: UPPER_SNAKE_CASE_H

DATA FILES
- vault_storage_save_master / vault_storage_load_master → vault_master.dat only
- vault_storage_save_creds / vault_storage_load_creds   → vault_creds.dat only
- These two operations must be fully independent of each other

ENCRYPTION
- All XOR logic moves exclusively to encryption.c
- encryption.c must have zero dependency on AppState or GTK
- vault_storage.c delegates all crypto to encryption_encrypt / encryption_decrypt

DIALOGS
- All GtkDialog construction moves to dialogs.c
- dialogs.c must NOT depend on AppState
- Other modules call dialogs_show_error / dialogs_show_input / dialogs_show_confirm

CLEANUP SAFETY
- Null-check every handle before calling g_list_free_full, gtk_widget_destroy, g_free

STATIC SCOPE
- Mark all internal-only functions static in their .c files
- GTK signal callbacks must always be static
- Only functions declared in .h headers have external linkage

DOCUMENTATION
- Add @brief/@param/@return Doxygen block to every function declaration
  in every header file

OUTPUT: Provide all files as separate clearly labeled code blocks.
Do not change any game logic, values, or algorithms beyond what is listed above.
```

---

## 🧠 Design Principles Applied

| Principle | ver1 Status | ver2 Implementation |
|---|---|---|
| **Separation of Concerns** | ❌ All logic in one file | ✅ Auth, credentials, encryption, dialogs, storage, and UI each isolated |
| **Dependency Injection** | ❌ Functions silently read globals | ✅ `AppState*` passed explicitly to every function that needs state |
| **Single Responsibility** | ❌ `main.c` does everything | ✅ Each `.c` file has exactly one clearly defined purpose |
| **Data Encapsulation** | ❌ All state globally mutable | ✅ State owned by `AppState`; internal helpers marked `static` |
| **Module Namespacing** | ❌ Flat inconsistent naming | ✅ `master_auth_*`, `service_manager_*`, `vault_storage_*`, `encryption_*`, `dialogs_*`, `ui_*` |
| **DRY** | ❌ XOR logic duplicated, dialogs scattered | ✅ Encryption in `encryption.c` only; dialogs in `dialogs.c` only |
| **Information Hiding** | ❌ All functions globally linked | ✅ Internal functions and GTK callbacks marked `static` |
| **Null Safety** | ❌ Unconditional free in cleanup | ✅ Every resource null-checked before free |
| **Layered Architecture** | ❌ No layers | ✅ UI → Logic → Infrastructure → Foundation; zero circular dependencies |
| **Extensibility** | ❌ Changing crypto or dialogs touches multiple files | ✅ Swap encryption in `encryption.c` only; add dialog types in `dialogs.c` only |

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
gcc src/*.c -o password_vault.exe \
    -Iinclude \
    $(pkg-config --cflags --libs gtk+-3.0)
```

### Manual (Linux)
```bash
gcc src/*.c -o password_vault \
    -Iinclude \
    $(pkg-config --cflags --libs gtk+-3.0)
```

---

*Password Vault — Advanced Programming Laboratory · CSE Discipline · February 2026*
