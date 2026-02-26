# Building & Running in UCRT64 (MSYS2)

## Prerequisites

Make sure you are using the **UCRT64** terminal in MSYS2 (orange icon, not MINGW64 or MSYS).

---

## Step 1 — Install dependencies

Run this once. Skip if already installed.

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
           mingw-w64-ucrt-x86_64-sqlite3 \
           mingw-w64-ucrt-x86_64-openssl \
           mingw-w64-ucrt-x86_64-pkg-config \
           mingw-w64-ucrt-x86_64-gcc \
           mingw-w64-ucrt-x86_64-hicolor-icon-theme \
           mingw-w64-ucrt-x86_64-adwaita-icon-theme \
           make
```

---

## Step 2 — Navigate to the project

```bash
cd /c/Users/USER/Claude/Full_Password_Manager
```

---

## Step 3 — Verify dependencies are found

```bash
which gcc
which make
pkg-config --exists gtk4    && echo "gtk4    OK" || echo "gtk4    MISSING"
pkg-config --exists sqlite3 && echo "sqlite3 OK" || echo "sqlite3 MISSING"
pkg-config --exists openssl && echo "openssl OK" || echo "openssl MISSING"
```

All three should print OK before continuing.

---

## Step 4 — Build

```bash
make
```

To do a clean rebuild from scratch:

```bash
make clean && make
```

---

## Step 5 — Run

```bash
./build/gtk-password-vault
```

Or build and run in one command:

```bash
make run
```

---

## All steps in one block (copy-paste)

```bash
# Install dependencies (once)
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
           mingw-w64-ucrt-x86_64-sqlite3 \
           mingw-w64-ucrt-x86_64-openssl \
           mingw-w64-ucrt-x86_64-pkg-config \
           mingw-w64-ucrt-x86_64-gcc \
           mingw-w64-ucrt-x86_64-hicolor-icon-theme \
           mingw-w64-ucrt-x86_64-adwaita-icon-theme \
           make

# Navigate to project
cd /c/Users/USER/Claude/Full_Password_Manager

# Build
make

# Run
./build/gtk-password-vault
```

---

## Subsequent runs (after first install)

```bash
cd /c/Users/USER/Claude/Full_Password_Manager
make run
```

---

## Troubleshooting

### Rebuild after editing a file
```bash
make
```
Make will only recompile changed files automatically.

### Full clean rebuild
```bash
make clean && make
```

### If the window opens but shows no icons
```bash
pacman -S mingw-w64-ucrt-x86_64-adwaita-icon-theme
```

### Bundle DLLs to run the .exe outside MSYS2
```bash
cd /c/Users/USER/Claude/Full_Password_Manager
ldd build/gtk-password-vault.exe | grep ucrt64 | awk '{print $3}' | xargs -I{} cp {} build/
```
After this, `build/gtk-password-vault.exe` can be double-clicked from Windows Explorer.

---

## Vault database location

On first launch a master password setup screen appears.
The vault database is created at:

```
C:\Users\USER\AppData\Local\gtk-password-vault\vault.db
```
