cmake_minimum_required(VERSION 3.16)
project(GTKPasswordVault CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find dependencies via pkg-config ──────────────────────────
find_package(PkgConfig REQUIRED)

# Try standard pkg-config first; fall back to manual paths if broken
pkg_check_modules(GTK3    gtk+-3.0)
pkg_check_modules(OPENSSL openssl)
pkg_check_modules(SQLITE3 sqlite3)

# Manual fallback for environments with broken pkg-config (e.g. missing libtiff)
if(NOT GTK3_FOUND)
    message(WARNING "pkg-config could not find gtk+-3.0 — using manual include paths")
    set(GTK3_INCLUDE_DIRS
        /usr/include/gtk-3.0
        /usr/include/glib-2.0
        /usr/lib/x86_64-linux-gnu/glib-2.0/include
        /usr/include/pango-1.0
        /usr/include/harfbuzz
        /usr/include/cairo
        /usr/include/gdk-pixbuf-2.0
        /usr/include/atk-1.0)
    set(GTK3_LIBRARIES gtk-3 gdk-3 pangocairo-1.0 pango-1.0 gobject-2.0 glib-2.0 cairo atk-1.0)
    set(GTK3_FOUND TRUE)
endif()

if(NOT OPENSSL_FOUND)
    message(WARNING "pkg-config could not find openssl — using default paths")
    set(OPENSSL_LIBRARIES ssl crypto)
    set(OPENSSL_FOUND TRUE)
endif()

if(NOT SQLITE3_FOUND)
    message(WARNING "pkg-config could not find sqlite3 — using default paths")
    set(SQLITE3_LIBRARIES sqlite3)
    set(SQLITE3_FOUND TRUE)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${GTK3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

link_directories(
    ${GTK3_LIBRARY_DIRS}
    ${OPENSSL_LIBRARY_DIRS}
    ${SQLITE3_LIBRARY_DIRS}
)

add_definitions(${GTK3_CFLAGS_OTHER})

# ── Source files ──────────────────────────────────────────────
set(SOURCES
    main.cpp
    src/utils/file_logger.cpp
    src/crypto/aes_gcm_encryption.cpp
    src/crypto/crypto_utils.cpp
    src/security/password_strength_evaluator.cpp
    src/db/sqlite_vault_storage.cpp
    src/auth/master_auth.cpp
    src/ui/service_manager.cpp
    src/ui/gtk_dialog_provider.cpp
    src/ui/ui.cpp
)

# ── Main executable ───────────────────────────────────────────
add_executable(password_vault ${SOURCES})

target_link_libraries(password_vault
    ${GTK3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

# std::filesystem — explicitly link on GCC < 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(password_vault stdc++fs)
endif()

# ── Compiler flags ────────────────────────────────────────────
target_compile_options(password_vault PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Ensure data/ directory exists after build ─────────────────
add_custom_command(TARGET password_vault POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/data"
    COMMENT "Creating data/ directory")

message(STATUS "GTK Password Vault — build configured")
