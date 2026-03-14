# 🔐 GTK Password Vault

A modular **GTK-based password manager written in C** that securely stores and manages service credentials using encrypted vault storage.

This project was originally implemented as a **single monolithic file** and later refactored into a **modular architecture using AI-assisted refactoring prompts**. The refactor introduces clean separation of concerns, structured modules, and improved maintainability.

---

# 📌 Features

* Secure credential storage
* Master password authentication
* Password recovery system
* Encrypted vault storage
* GTK graphical interface
* Modular C architecture
* Separation of UI, logic, and storage layers

---

# 🏗 Architecture

The application follows a **layered architecture** to separate responsibilities and improve maintainability.

```
        GTK User Interface
             (ui.c)
                 │
                 ▼
     ┌──────────────────────┐
     │   Application Logic  │
     │                      │
     │ master_auth.c       │
     │ service_manager.c   │
     └──────────────────────┘
                 │
                 ▼
     ┌──────────────────────┐
     │     Storage Layer    │
     │                      │
     │ vault_storage.c      │
     │ encryption.c         │
     └──────────────────────┘
                 │
                 ▼
            Data Files
          (data/*.dat)
```

Each layer has a clearly defined responsibility:

| Layer   | Responsibility                           |
| ------- | ---------------------------------------- |
| UI      | GTK windows and user interaction         |
| Logic   | Authentication and credential management |
| Storage | Encryption and file persistence          |
| Data    | Encrypted vault files                    |

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

* Hold session master password
* Store GTK window pointers
* Track login state
* Maintain runtime credential list

This struct replaces the original **global variables** used in the monolithic version.

---

## Credential Model

**`credential.h`**

Defines the data structure representing stored credentials.

Typical fields include:

```
service_name
username
password
```

This structure is used by both the **service manager** and **vault storage modules**.

---

## Dialog Utilities

**`dialogs.h` / `dialogs.c`**

Provides reusable GTK dialog functions.

Examples:

* Error dialogs
* Confirmation prompts
* Information messages

Centralizing dialogs keeps the UI consistent.

---

## Encryption Module

**`encryption.h` / `encryption.c`**

Handles encryption and decryption of vault data.

Responsibilities:

```
crypto_encrypt()
crypto_decrypt()
```

Encryption currently uses a simple XOR-based method derived from the master password.
This module is isolated so stronger encryption methods can easily replace it.

---

## Master Authentication

**`master_auth.h` / `master_auth.c`**

Controls access to the vault.

Responsibilities:

* Verify master password
* Prompt login dialog
* Handle password recovery
* Reset master password when recovery answer is correct

This module acts as the **security gateway of the application**.

---

## Service Manager

**`service_manager.h` / `service_manager.c`**

Manages the in-memory credential list.

Responsibilities:

* Add credentials
* Remove credentials
* Retrieve credentials
* Manage credential data structures

This module contains the **core vault business logic**.

---

## User Interface

**`ui.h` / `ui.c`**

Implements the GTK graphical interface.

Responsibilities:

* Build login window
* Build vault dashboard
* Display credentials
* Connect UI actions to backend logic

This module coordinates communication between:

```
master_auth
service_manager
vault_storage
```

---

## Vault Storage

**`vault_storage.h` / `vault_storage.c`**

Handles file persistence of vault data.

Responsibilities:

* Load vault files
* Save encrypted credentials
* Manage vault data files

Data files used:

| File             | Purpose                           |
| ---------------- | --------------------------------- |
| vault.dat        | main encrypted vault              |
| vault_creds.dat  | stored credential entries         |
| vault_master.dat | master password and recovery info |

---

# 💾 Data Storage

All encrypted vault data is stored in the **data directory**.

```
data/
├── vault.dat
├── vault_creds.dat
└── vault_master.dat
```

These files contain encrypted credential information and authentication data.

---

# 🤖 AI Refactoring Prompts

The project was refactored using structured AI prompts to transform the original monolithic program into a modular architecture.

---

## Prompt 1 — Remove Global Variables

```
Refactor the C program to remove all global variables.

Introduce a struct named AppState to store all shared application state
such as GTK widgets, session data, and credential lists.

Update all functions to receive an AppState pointer instead of
accessing global variables.
```

---

## Prompt 2 — Add Doxygen Documentation

```
Add Doxygen-style comments to every function.

Each function must include:

@brief
@param descriptions
@return description
```

---

## Prompt 3 — Split the Monolithic File into Modules

```
Refactor the single large C file into multiple modules:

app_state
master_auth
service_manager
vault_storage
encryption
ui
dialogs

Each module should have a header file and source file.
```

---

## Prompt 4 — Standardize Naming Conventions

```
Rename functions using module prefixes:

ma_  → master authentication
vs_  → vault storage
crypto_ → encryption
ui_  → user interface
```

---

# 🛠 Build Instructions

### Install GTK development libraries

On Linux:

```
sudo apt install libgtk-3-dev
```

---

### Compile the project

```
gcc src/*.c -Iinclude `pkg-config --cflags --libs gtk+-3.0` -o password_vault
```

---

### Run the application

```
./password_vault
```

---

# 🎯 Learning Goals

This project demonstrates:

* Modular C architecture
* GTK GUI programming
* File-based encrypted storage
* Separation of UI and business logic
* AI-assisted code refactoring

---

# 📄 License

This project is provided for educational purposes.
