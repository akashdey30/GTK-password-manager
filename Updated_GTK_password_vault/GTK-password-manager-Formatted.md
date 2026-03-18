# 🔐 GTK Password Vault

A modular **GTK-based password manager written in C** that securely stores and manages service credentials using encrypted vault storage.

This project was originally implemented as a **single monolithic file** and later refactored into a **modular architecture using AI-assisted refactoring prompts**. The refactor introduces clean separation of concerns, structured modules, and improved maintainability.

---

# 📌 Features

* Secure credential storage
* Master password authentication
* Password recovery system
* Encrypted vault storage (XOR keyed on master password)
* GTK graphical interface
* Modular C architecture
* Separation of UI, logic, and storage layers
* Zero global variables — all state in `AppState`
* Full Doxygen documentation on all public functions

---

# 🏗 Architecture

The application follows a **layered architecture** to separate responsibilities and improve maintainability.

```
        GTK User Interface
          (ui.c / dialogs.c)
                 │
                 ▼
     ┌──────────────────────┐
     │   Application Logic  │
     │                      │
     │  master_auth.c       │
     │  service_manager.c   │
     └──────────────────────┘
                 │
                 ▼
     ┌──────────────────────┐
     │   Infrastructure     │
     │                      │
     │  vault_storage.c     │
     │  encryption.c        │
     └──────────────────────┘
                 │
                 ▼
            Data Files
          (data/*.dat)
```

Each layer has a clearly defined responsibility:

| Layer          | Responsibility                                |
| -------------- | --------------------------------------------- |
| UI             | GTK windows, dialogs, and user interaction    |
| Logic          | Authentication and credential management      |
| Infrastructure | Encryption and file persistence               |
| Data           | Encrypted vault files                         |

**Dependency rule:** UI → Logic → Infrastructure → Foundation. No upward dependencies. No circular includes.

---

# 📂 Project Structure

```
Updated_GTK_password_vault/
│
├── data/
│   ├── vault.dat
│   ├── vault_creds.dat
│   └── vault_master.dat
│
├── include/
│   ├── app_state.h
│   ├── credential.h
│   ├── dialogs.h
│   ├── encryption.h
│   ├── master_auth.h
│   ├── service_manager.h
│   ├── ui.h
│   └── vault_storage.h
│
└── src/
    ├── dialogs.c
    ├── encryption.c
    ├── main.c
    ├── master_auth.c
    ├── service_manager.c
    ├── ui.c
    └── vault_storage.c
```

---

# 🧩 Module Overview

## App State

**`app_state.h`**

Defines the central application state structure.

Responsibilities:

* Hold session master password (`session_master[MAX_LEN]`)
* Store GTK main window pointer (`main_window`)
* Track login state (`login_success`)
* Maintain runtime credential list (`credentials` — `GList*`)

This struct replaces the original **global variables** used in the monolithic version.

---

## Credential Model

**`credential.h`**

Defines the data structure representing stored credentials.

Fields:

```
service_name[MAX_LEN]
username[MAX_LEN]
password[MAX_LEN]
```

This structure is used by both the **service manager** and **vault storage** modules.

> Note: The field is named `service_name` (not `service`) to follow the full descriptive `snake_case` convention applied throughout ver2.

---

## Dialog Utilities

**`dialogs.h` / `dialogs.c`**

Provides reusable GTK dialog functions.

Functions:

* `dialogs_show_error(GtkWidget *parent, const char *message)` — modal error pop-up
* `dialogs_show_input(GtkWidget *parent, const char *prompt)` — returns user-entered string (caller must `g_free()`)
* `dialogs_show_confirm(GtkWidget *parent, const char *question)` — returns `TRUE` if confirmed

**This module has no dependency on `AppState`** — it accepts only a `GtkWidget *parent` pointer. Centralizing dialogs keeps the UI consistent and keeps other modules free of inline `GtkDialog` construction.

---

## Encryption Module

**`encryption.h` / `encryption.c`**

Handles encryption and decryption of vault data.

Functions:

```
encryption_encrypt(const char *input, const char *key, char *output)
encryption_decrypt(const char *input, const char *key, char *output)
```

Encryption uses a XOR-based method **keyed on the master password** (`app->session_master`). The key is passed by the caller — this module has **zero dependency on `AppState` or GTK**. Because XOR is symmetric, `encryption_decrypt()` delegates directly to `encryption_encrypt()`. This module is isolated so stronger encryption can easily replace it by editing only this file.

---

## Master Authentication

**`master_auth.h` / `master_auth.c`**

Controls access to the vault.

Functions:

* `master_auth_setup()` — first-run master password creation, saves to `vault_master.dat`
* `master_auth_verify()` — compares input against stored record in `vault_master.dat`
* `master_auth_recover()` — validates recovery answer and allows password reset

No GTK widget construction happens in this module — all dialog prompts are delegated to `dialogs.c`. This module acts as the **security gateway of the application**.

---

## Service Manager

**`service_manager.h` / `service_manager.c`**

Manages the in-memory credential list.

Functions:

* `service_manager_add()` — appends to `app->credentials` and saves to `vault_creds.dat`
* `service_manager_delete()` — removes by index and rewrites `vault_creds.dat`
* `service_manager_load_all()` — populates `app->credentials` from decrypted `vault_creds.dat`

Internal helper `service_manager_find_dup()` is marked `static` — prevents storing the same service name twice and is invisible outside this file.

---

## User Interface

**`ui.h` / `ui.c`**

Implements the GTK graphical interface.

Functions:

* `ui_build_main_window()` — constructs the full application layout and wires all signals
* `ui_refresh_services()` — clears and repopulates the credential list view from `app->credentials`

All GTK signal callbacks (`on_login_clicked`, `on_add_clicked`, `on_delete_clicked`) are `static` — invisible outside `ui.c`. This module coordinates communication between `master_auth`, `service_manager`, and `vault_storage`.

---

## Vault Storage

**`vault_storage.h` / `vault_storage.c`**

Handles file persistence of vault data.

Functions:

* `vault_storage_save_creds()` / `vault_storage_load_creds()` — operate on `vault_creds.dat` only
* `vault_storage_save_master()` / `vault_storage_load_master()` — operate on `vault_master.dat` only

All encryption is delegated to `encryption.c` — no XOR logic lives here. The two file operations are fully independent: changing the master password never touches credential storage and vice versa.

Data files used:

| File              | Purpose                                     |
| ----------------- | ------------------------------------------- |
| `vault.dat`       | Vault metadata / general state              |
| `vault_creds.dat` | Encrypted credential entries only           |
| `vault_master.dat`| Encrypted master password and recovery info |

---

# 💾 Data Storage

All encrypted vault data is stored in the **data directory**.

```
data/
├── vault.dat
├── vault_creds.dat
└── vault_master.dat
```

`vault_creds.dat` and `vault_master.dat` are fully independent — modifying one never rewrites the other.

---

# 🤖 AI Refactoring Prompts

The project was refactored using structured AI prompts to transform the original monolithic program into a modular architecture.

---

## Prompt 1 — Split the Monolithic File into Modules

```
Refactor the single large C file into multiple modules:

  app_state      — AppState struct and all #define constants
  credential     — Credential struct (service_name, username, password)
  master_auth    — master password setup, verify, recovery
  service_manager — credential add, delete, load, search
  vault_storage  — vault_creds.dat and vault_master.dat read/write
  encryption     — XOR encrypt and decrypt using master key
  ui             — GTK window construction and signal wiring
  dialogs        — all GTK dialog construction (no AppState dependency)

Each module should have a header file and source file.
Use #ifndef include guards on all headers.
```

---

## Prompt 2 — Remove Global Variables

```
Refactor the C program to remove all global variables.

Introduce a struct named AppState in include/app_state.h to store all
shared application state:
  session_master[MAX_LEN], credentials (GList*), main_window, login_success

Introduce a struct named Credential in include/credential.h:
  service_name[MAX_LEN], username[MAX_LEN], password[MAX_LEN]

Update all functions to receive an AppState pointer instead of
accessing global variables. GTK callbacks receive it via gpointer user_data.
```

---

## Prompt 3 — Standardize Naming Conventions

```
Apply the following naming conventions consistently to every identifier:

  Local variables / parameters → snake_case
  Struct type names             → PascalCase  (AppState, Credential)
  Struct members                → snake_case
  Functions                     → module_verb_object() style
  Header guards                 → UPPER_SNAKE_CASE_H

Function naming table:
  master_auth_*     → master_auth_setup(), master_auth_verify(), master_auth_recover()
  service_manager_* → service_manager_add(), service_manager_delete(),
                      service_manager_load_all()
  vault_storage_*   → vault_storage_save_creds(), vault_storage_load_creds(),
                      vault_storage_save_master(), vault_storage_load_master()
  encryption_*      → encryption_encrypt(), encryption_decrypt()
  dialogs_*         → dialogs_show_error(), dialogs_show_input(), dialogs_show_confirm()
  ui_*              → ui_build_main_window(), ui_refresh_services()
```

---

## Prompt 4 — Separate Data Files by Concern

```
Refactor vault_storage.c to use two separate data files:

  vault_master.dat — stores ONLY the encrypted master password record
  vault_creds.dat  — stores ONLY the encrypted credential entries

After this change:
- Changing the master password must NOT rewrite credential data
- Adding a credential must NOT rewrite master password data
```

---

## Prompt 5 — Encapsulate Encryption

```
Move all XOR logic exclusively to encryption.c.

Create:
  encryption_encrypt(const char *input, const char *key, char *output)
  encryption_decrypt(const char *input, const char *key, char *output)

The key parameter is always app->session_master (passed by the caller).
These functions must have NO dependency on AppState or GTK.

Replace every inline XOR site in vault_storage.c with calls to these functions.
```

---

## Prompt 6 — Centralise GTK Dialogs

```
Move all GTK dialog construction to dialogs.c.

Create:
  void     dialogs_show_error(GtkWidget *parent, const char *message)
  char    *dialogs_show_input(GtkWidget *parent, const char *prompt)
  gboolean dialogs_show_confirm(GtkWidget *parent, const char *question)

Rules:
- dialogs.c must NOT depend on AppState
- dialogs_show_input() returns a newly allocated string — caller must g_free() it
```

---

## Prompt 7 — Add null safety to cleanup

```
Rewrite app_cleanup(AppState *app) so every resource is null-checked
before being freed:

  if (app->credentials)  g_list_free_full(app->credentials, g_free);
  if (app->main_window)  gtk_widget_destroy(app->main_window);
  g_free(app);
```

---

## Prompt 8 — Mark internal functions as `static`

```
Mark all functions that are only used within their own .c file as static.

  service_manager.c: service_manager_find_dup()
  ui.c: build_login_form(), build_main_layout(),
        on_login_clicked(), on_add_clicked(), on_delete_clicked()
```

---

## Prompt 9 — Add Doxygen Documentation

```
Add a Doxygen comment block to every function declaration in every header file:

/**
 * @brief One sentence describing what the function does.
 *
 * @param param_name  Description of the parameter.
 * @return            Description of the return value, or void.
 */
```

---

# 🛠 Build Instructions

### Install GTK development libraries (Linux)

```bash
sudo apt install libgtk-3-dev
```

### Compile and run (Windows MSYS2 / UCRT64)

```bash
cd /c/Users/USER/Desktop/password_vault
gcc -Iinclude src/*.c -o PasswordVault $(pkg-config --cflags --libs gtk+-3.0)
./PasswordVault.exe
```

### Compile and run (Linux)

```bash
gcc src/*.c -Iinclude $(pkg-config --cflags --libs gtk+-3.0) -o PasswordVault
./PasswordVault
```

---

# 🎯 Learning Goals

This project demonstrates:

* Modular C architecture with clean layering
* GTK GUI programming
* File-based encrypted storage separated by concern
* Separation of UI, business logic, and infrastructure
* Dependency injection via `AppState*`
* Information hiding with `static` scope
* AI-assisted code refactoring

---

# 📄 License

This project is provided for educational purposes.
