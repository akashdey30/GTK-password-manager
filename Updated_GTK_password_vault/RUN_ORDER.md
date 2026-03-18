# 🚀 Run Order — GTK Password Vault

## Windows (MSYS2 / UCRT64)

```bash
cd /c/Users/USER/Desktop/C_PROJECTS/Updated_GTK_password_vault

gcc -Iinclude src/*.c -o PasswordVault $(pkg-config --cflags --libs gtk+-3.0)

./PasswordVault.exe
```

---

## Linux

```bash
cd ~/Updated_GTK_password_vault

gcc -Iinclude src/*.c -o PasswordVault $(pkg-config --cflags --libs gtk+-3.0)

./PasswordVault
```

---

## Notes

- Run from the **project root** (`Updated_GTK_password_vault/`) so the executable can find the `data/` directory at the relative path `data/vault_creds.dat` and `data/vault_master.dat`.
- The `data/` folder must exist before first run. If it is missing, create it:
  ```bash
  mkdir -p data
  ```
- On first launch, the vault has no master password set. You will be prompted to create one.
- Subsequent launches will prompt you to enter the master password to unlock the vault.
