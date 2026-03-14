# Password Vault Application

## Code Architecture & Design Documentation

*Comparing Original vs. Refactored Architecture*

---

# 1. Project Overview

This document describes the full design and refactoring journey of a GTK-based desktop Password Vault application written in C. Starting from a single monolithic source file with global variables, mixed naming conventions, and no documentation, the project was progressively transformed through three distinct refactoring passes into a clean, modular, well-documented multi-file C project.

The documentation covers every design decision made, every naming convention applied, every structural pattern introduced, and every prompt used to drive the transformation — so that the complete reasoning and evolution of the codebase is fully traceable.

---

**Scope**

Three refactoring passes were applied to the original `main.c`: (1) naming conventions + global variable elimination, (2) Doxygen documentation, and (3) modular multi-file decomposition into header/source pairs.

---

# 2. The Original Codebase

The original codebase was a single file, `main.c`, containing all application logic. It implemented a complete GTK desktop password manager for Windows with the following capabilities:

- Master password authentication and session management
- Recovery question system for password reset
- Add, view, and manage stored credentials
- XOR-based encryption for credential data
- Encrypted persistent storage via `vault.dat`
- GTK-based GUI windows for login, dashboard, and credential management

## 2.1 Structural Problems

The original file suffered from several significant structural anti-patterns that made it difficult to maintain, test, and extend:

### 2.1.1 Global Variables

All application state — GTK widgets, credential lists, session flags — was declared at file scope. This meant any function could read or modify any piece of state at any time, with no encapsulation or clear ownership.

| **Before (Original)** | **After (Refactored)** |
|---|---|
| `GtkWidget *main_window;` | `/* removed -- now app->main_window */` |
| `GList *credential_list;` | `/* removed -- now app->credentials */` |
| `char master_pass[MAX_LEN];` | `/* removed -- now app->session_master */` |
| `gboolean login_success;` | `/* removed -- now app->login_success */` |

### 2.1.2 Inconsistent Naming

Identifiers mixed several naming styles within the same file: some functions used PascalCase (`LoadCredentials`, `SaveVault`), others used snake_case (`crypto_encrypt`, `refresh_ui`), and many variables used cryptic or abbreviated names that required reading the full function body to understand.

### 2.1.3 Monolithic Structure

All logic — struct definitions, encryption utilities, file I/O, authentication, UI construction, and the main entry point — lived in a single translation unit. There were no headers, no module boundaries, and no enforced interface between concerns.

### 2.1.4 No Documentation

No function carried a comment describing its purpose, parameters, or return value. Maintainers had to read the full body of every function to understand its contract.

---

# 3. Prompts Used

All three refactoring passes were driven by the following exact prompts. Each prompt is preserved verbatim.

## 3.1 Prompt 1 — Naming Conventions & Global Variable Elimination

---

**PROMPT 1**

*Design the code in such a way that it follows the following naming conventions. Remove the global variables and use them as structure elements instead. Modify the code accordingly.*

*Local variables — snake_case*

*Parameters — snake_case*

*Functions — verb_object() / action_target() style, module-prefixed (e.g. `ma_`, `vs_`, `ui_`)*

*Struct names — PascalCase*

*Struct members — snake_case*

---

## 3.2 Prompt 2 — Doxygen Documentation

---

**PROMPT 2**

*Now add comments for each function that follow the following convention:*

```c
/**
 * @brief Encrypts a plaintext string using the vault key.
 *
 * @param input   The plaintext string to encrypt.
 * @param key     The encryption key derived from the master password.
 * @param output  Buffer to receive the encrypted output.
 * @return void
 */
void crypto_encrypt(const char *input, const char *key, char *output);
```

---

## 3.3 Prompt 3 — Modular Decomposition into Header/Source Pairs

---

**PROMPT 3**

*Now, create header files with the following names, each containing the specified function declarations. Also create the corresponding .c source files for each header:*

- **app.h** — holds the `AppState` struct and initialization
- **auth.h** — `void ma_prompt_master_password(...); void ma_handle_recovery(...); gboolean ma_verify_master(...);`
- **vault.h** — `void vs_load_all_credentials(...); void vs_add_credential(...); void vs_delete_credential(...);`
- **crypto.h** — `void crypto_encrypt(...); void crypto_decrypt(...);`
- **file_io.h** — `void fio_save_vault(...); void fio_load_vault(...);`
- **ui/login_window.h** — `void ui_show_login_window(...);`
- **ui/dashboard_window.h** — `void ui_show_dashboard(...); void ui_refresh_services(...);`
- **ui/add_credential_window.h** — `void ui_show_add_credential(...);`
- **ui/view_credentials_window.h** — `void ui_show_credentials(...);`

---

# 4. Design Patterns Applied

## 4.1 Context Object Pattern (Prompt 1)

The most impactful structural change introduced in Prompt 1 was replacing all global variables with a single heap-allocated struct called `AppState`. Every callback and helper receives a pointer to this struct instead of reading or writing global state.

---

**Pattern Name**

Context Object (also called State Object or Parameter Object). The pattern bundles related state into a single struct and passes it by pointer through the call graph, eliminating global mutable state.

---

In the original code, any function could silently depend on or mutate globals like `master_pass`, `credential_list`, and `login_success`. The Context Object makes these dependencies explicit — a function that needs session state must accept an `AppState*` parameter, making the dependency visible at the call site.

**Before (Global State):**

```c
// Global variables anyone can modify
char master_pass[MAX_LEN];
GList *credential_list = NULL;
gboolean login_success = FALSE;

void someFunction() {
    // Function silently depends on globals
    if (login_success) { /* ... */ }
    credential_list = g_list_append(credential_list, new_entry);
}
```

**After (Context Object):**

```c
// All state in one struct
typedef struct {
    char session_master[MAX_LEN];
    GList *credentials;
    GtkWidget *main_window;
    gboolean login_success;
} AppState;

void someFunction(AppState *app) {
    // Dependencies explicit at call site
    if (app->login_success) { /* ... */ }
    app->credentials = g_list_append(app->credentials, new_entry);
}
```

## 4.2 Module Pattern (Prompt 3)

The third refactoring pass applied the Module pattern by splitting the monolithic file into cohesive header/source pairs. Each module exposes a small set of related functions and hides implementation details in the `.c` file.

**Modules Created:**

| Module | Responsibility | Approx. Lines |
|---|---|---|
| **app.h/.c** | `AppState` struct definition and initialization | ~60 |
| **auth.h/.c** | Master password verification and recovery | ~150 |
| **vault.h/.c** | Credential add, load, delete, and list management | ~180 |
| **crypto.h/.c** | XOR-based encryption and decryption utilities | ~80 |
| **file_io.h/.c** | Encrypted read/write of `vault.dat` | ~70 |
| **ui/login_window.h/.c** | GTK login window construction and callbacks | ~100 |
| **ui/dashboard_window.h/.c** | Dashboard window, service list, and refresh | ~150 |
| **ui/add_credential_window.h/.c** | Add credential form and submission | ~80 |
| **ui/view_credentials_window.h/.c** | Credential viewer window | ~70 |

## 4.3 Facade Pattern (ui layer)

The UI layer serves as a facade that integrates all other modules. The dashboard window includes `auth.h`, `vault.h`, and `file_io.h`, and provides entry-point functions like `ui_show_dashboard(AppState*)` that construct the full interface. This shields `main.c` from knowing about the internal modules.

## 4.4 Strategy Pattern (crypto.h)

The encryption and decryption logic is encapsulated behind a consistent interface in `crypto.h`. This means the encryption algorithm can be swapped (e.g., from XOR to AES) without changing any calling code in `vault.c` or `file_io.c`.

## 4.5 Separation of Concerns

Each module has a single, well-defined responsibility:

- **auth** knows only about password verification — it never touches file I/O directly
- **vault** knows only about credential list management — it never constructs UI widgets
- **crypto** is a pure utility — it has no dependency on `AppState` or GTK
- **file_io** handles only disk persistence — it delegates encryption to `crypto`
- **ui/*** handles only widget construction and signal wiring — it never encrypts data

---

# 5. Before-and-After Comparison

## 5.1 File Structure

| **Before (Original)** | **After (Refactored)** |
|---|---|
| 1 file (`main.c`) | 18 files (9 headers + 9 sources) |
| All logic in one translation unit | Clear module interfaces via headers |
| No module boundaries | Each module has a single responsibility |

## 5.2 State Management

| **Before (Original)** | **After (Refactored)** |
|---|---|
| Multiple global variables | 1 `AppState` struct passed by pointer |
| Silent side effects everywhere | State changes visible at call site |
| No ownership — anyone can mutate | Owner is caller who holds `AppState*` |
| Hard to pass to GTK callbacks | Passed cleanly via `gpointer user_data` |

## 5.3 Documentation

| **Before (Original)** | **After (Refactored)** |
|---|---|
| Zero function comments | Every function has `@brief/@param/@return` |
| Purpose guessed from body | Purpose described at declaration |
| No parameter descriptions | All parameters documented with types |
| No return-value descriptions | Return values and error states documented |

## 5.4 Naming

| **Before (Original)** | **After (Refactored)** |
|---|---|
| Mixed: `LoadCredentials`, `crypto_encrypt` | Uniform module-prefixed style throughout |
| Cryptic abbreviations | Descriptive names: `session_master`, `login_success` |
| Inconsistent struct casing | All structs PascalCase: `AppState`, `Credential` |
| No module prefix on functions | Clear prefixes: `ma_`, `vs_`, `fio_`, `ui_` |

---

# 6. Directory Structure

```
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
│
├── data/
│   └── vault.dat
│
└── Makefile
```

---

# 7. Module Dependency Graph

The diagram below shows which modules each source file includes. Arrows represent `#include` dependencies. `app.h` is included by every module and sits at the base of the dependency tree.

```
main.c
  |
  v
ui/login_window.h
  |
  v
ui/dashboard_window.h
  |
  +------------------+------------------+
  |                  |                  |
  v                  v                  v
auth.h           vault.h           file_io.h
  |                  |                  |
  v                  v                  v
app.h            app.h            crypto.h
                                       |
                                       v
                                    app.h  (foundation — included by all)
```

Notable dependency rules enforced during decomposition:

- `app.h` has no local dependencies — it includes only GTK and standard C headers.
- `crypto.h` has no dependency on `app.h` — it operates purely on char buffers.
- `file_io.c` delegates all encryption to `crypto.h` and never touches GTK.
- `auth.c` depends only on `app.h` and standard C — no UI dependencies.
- `main.c` includes only `app.h` and `ui/login_window.h`, keeping the entry point minimal.

---

# 8. Step-by-Step Evolution

## 8.1 Pass 1: Context Object + Naming Conventions

Applied by Prompt 1. The codebase remained a single file but was substantially restructured:

1. All global variables removed.
2. A new struct `AppState` created to hold all former globals.
3. All functions updated to accept `AppState*` as a parameter.
4. All identifiers renamed to follow `snake_case` / `verb_object()` / `PascalCase` rules.
5. Cryptic abbreviations replaced with descriptive names.
6. GTK callbacks now receive state through `gpointer user_data` cast to `AppState*` instead of globals.

## 8.2 Pass 2: Doxygen Documentation

Applied by Prompt 2. No logic was changed — only comments were added:

1. Every function declaration received a `@brief/@param/@return` block.
2. Every function definition received the same block immediately above it.
3. GTK signal-handler parameters that are unused were annotated with `(unused)`.
4. Struct members received inline `/** ... */` documentation.

## 8.3 Pass 3: Modular Decomposition

Applied by Prompt 3. The single documented file was split into 18 files:

1. 9 header files (`.h`) — one per module, each with include guards and Doxygen comments.
2. 9 source files (`.c`) — each including only its own header and its direct dependencies.
3. 1 minimal `main.c` — `gtk_init`, `g_new0(AppState)`, `ui_show_login_window()`, `gtk_main()`, `g_free`. Under 30 lines.
4. The function `ui_show_login_window()` was introduced in `login_window.h` to encapsulate the entire startup UI construction that was previously inlined in `main()`.
5. Module-prefixed function naming (`ma_`, `vs_`, `fio_`, `ui_`) was enforced to make the owning module clear at every call site.

---

# 9. Encryption Workflow

## 9.1 Adding a Credential

1. User inputs service name, username, and password via the GTK form.
2. `vs_add_credential(AppState *app, const char *service, const char *user, const char *pass)` is called.
3. `crypto_encrypt()` is applied to the password field using the session master key.
4. The encrypted credential is appended to `app->credentials`.
5. `fio_save_vault()` serializes and writes the updated list to `vault.dat`.

## 9.2 Retrieving a Credential

1. `fio_load_vault(AppState *app)` reads the encrypted `vault.dat` on startup.
2. Each stored password field is passed through `crypto_decrypt()` using the session master key.
3. Decrypted credentials are loaded into `app->credentials`.
4. The UI reads from `app->credentials` to populate the dashboard list.

> **Note:** The current implementation uses XOR-based encryption, which is lightweight and suitable for a lab context. For production use, a proven algorithm such as AES-256 with a properly derived key (e.g., via PBKDF2) is strongly recommended.

---

# 10. Documentation Standard

Each function follows the Doxygen block structure below. This convention is applied uniformly across all header files.

```c
/**
 * @brief Brief one-sentence description of what the function does.
 *
 * @param param_name  Description of the parameter and its expected range or format.
 * @param param_name  Description of the second parameter.
 * @return            Description of the return value, including error conditions.
 */
```

**Example:**

```c
/**
 * @brief Encrypts a plaintext string using XOR with the provided key.
 *
 * @param input   Null-terminated plaintext string to encrypt.
 * @param key     Null-terminated key string derived from the master password.
 * @param output  Caller-allocated buffer to receive the encrypted output.
 * @return void
 */
void crypto_encrypt(const char *input, const char *key, char *output);
```

---

# 11. Naming Conventions Reference

| **Category** | **Convention** | **Example** |
|---|---|---|
| Local variables | `snake_case` | `char master_pass[MAX_LEN];` |
| Parameters | `snake_case` | `const char *service_name` |
| Functions | `module_verb_object()` | `vs_load_all_credentials()` |
| Struct type names | `PascalCase` | `AppState`, `Credential` |
| Struct members | `snake_case` | `app->login_success` |
| Header guards | `UPPER_SNAKE_H` | `#ifndef CRYPTO_H` |
| Source files | `lowercase` | `file_io.c`, `auth.c` |

---

# 12. Benefits Achieved

## 12.1 Maintainability

- A developer looking for authentication logic goes directly to `auth.c`.
- A developer looking for encryption logic goes directly to `crypto.c`.
- Changes to one module require recompiling only that module and its dependents.

## 12.2 Readability

- Every function name describes what it does and which module it belongs to.
- Every parameter name describes what the parameter represents.
- Every function has a one-sentence description visible at the declaration.

## 12.3 Testability

- `crypto_encrypt` and `crypto_decrypt` are pure utility functions with no GTK or `AppState` dependencies — they can be unit-tested with simple char buffers.
- `vs_add_credential` and `vs_load_all_credentials` can be tested by constructing a temporary `AppState` without launching the GTK event loop.
- `fio_save_vault` and `fio_load_vault` can be tested against a temporary file path.

## 12.4 Extensibility

- Swapping the encryption algorithm requires changing only `crypto.c` — no other module needs modification.
- Adding a new credential field requires only extending the `Credential` struct in `app.h` and updating `vault.c` and `file_io.c`.
- Adding a new UI window requires only creating a new `ui/window_name.h/.c` pair and calling it from the dashboard.

---

*Advanced Programming Laboratory — CSE Discipline*
