# GTK Password Vault

A secure, local password manager built with GTK4 and C.

## Features

- **AES-256-GCM** encryption of all credential fields at rest
- **PBKDF2-HMAC-SHA256** (600,000 iterations) master password derivation
- **Auto-lock** after 5 minutes of inactivity
- **Clipboard auto-clear** after 30 seconds
- **Password generator** with configurable charset and length
- **Password strength meter** with live feedback
- **Category sidebar** for organizing credentials
- **Full-text search** across title, username, URL, notes
- **Backup/restore** vault to file
- **Recovery phrase** (12-word) for vault access recovery
- **Light/Dark themes** (Google Material light + Catppuccin dark)

## Dependencies

| Library    | Version | Purpose                    |
|------------|---------|----------------------------|
| GTK4       | ≥ 4.10  | GUI                        |
| SQLite3    | ≥ 3.35  | Local encrypted database   |
| OpenSSL    | ≥ 3.0   | AES-GCM, PBKDF2, RAND      |

### Ubuntu / Debian

```bash
sudo apt install libgtk-4-dev libsqlite3-dev libssl-dev pkg-config gcc
```

### Fedora / RHEL

```bash
sudo dnf install gtk4-devel sqlite-devel openssl-devel gcc
```

### Arch

```bash
sudo pacman -S gtk4 sqlite openssl base-devel
```

## Build

```bash
make          # compile → build/gtk-password-vault
make run      # build + run
make clean    # remove build/
```

## Security Notes

- The master password is **never stored**; only a PBKDF2-derived verifier blob is kept.
- All credential fields (title, username, password, URL, notes, category) are individually AES-256-GCM encrypted.
- Sensitive memory (AES keys, decrypted passwords) is wiped with `OPENSSL_cleanse` before freeing.
- The database is a standard SQLite3 file (`~/.local/share/gtk-password-vault/vault.db`).  
  Back it up regularly with the built-in backup feature.

## Architecture

```
include/         Public headers (mirrors src/ layout)
src/
  main.c         Entry point
  core/          App lifecycle, session state
  auth/          Master password + recovery phrase auth
  crypto/        AES-256-GCM engine, PBKDF2, base64
  database/      SQLite init, CRUD, search
  security/      Auto-logout, clipboard, pw strength/generator, backup
  ui/            GTK4 widgets: app window, login, vault, dialogs, theme
  utils/         Logger
resources/css/   Light and dark CSS themes
```

## License

MIT
