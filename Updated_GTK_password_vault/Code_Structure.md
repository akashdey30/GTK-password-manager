Password_vault/
│
├── data/                        # Runtime vault data storage
│   ├── vault.dat                # Encrypted credential entries (binary, XOR-encoded)
│   └── master.dat               # Encrypted master password record (binary, XOR-encoded)
│
├── include/                     # Header files (interfaces, declarations)
│   ├── credentials.h            # Credential struct definition (service, username, password)
│   ├── app_state.h              # AppState struct — GTK widgets + session master password
│   ├── dialogs.h                # GTK dialog function declarations (add, search, delete, show)
│   ├── encryption.h             # XOR_KEY constant + xor_buffer() prototype
│   ├── master_auth.h            # Master password setup, verify, and change declarations
│   ├── service_manager.h        # Credential add, delete, and search declarations
│   ├── ui.h                     # UI construction and service refresh declarations
│   └── vault_storage.h          # Vault file I/O declarations + VAULT_FILE / MASTER_FILE paths
│
└── src/                         # Source code implementations
    ├── encryption.c             # xor_buffer() — single XOR pass over a byte buffer
    ├── vault_storage.c          # Encrypted read/write for vault.dat and master.dat
    ├── master_auth.c            # Master password exists-check, prompt, and change logic
    ├── service_manager.c        # Credential add, delete, and search against vault
    ├── dialogs.c                # All GTK dialog construction (add, search, delete, show)
    ├── ui.c                     # GTK main window construction and service button refresh
    └── main.c                   # Entry point — GTK init, master auth flow, gtk_main loop
