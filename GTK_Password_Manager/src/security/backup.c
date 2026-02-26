#include "security/backup.h"
#include "utils/logger.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

AppResult backup_export(const char *db_path, const char *dest_path)
{
    GError *err = NULL;
    char *contents = NULL;
    gsize length   = 0;

    if (!g_file_get_contents(db_path, &contents, &length, &err)) {
        LOG_E("Backup export read failed: %s", err->message);
        g_error_free(err);
        return APP_ERR_IO;
    }

    if (!g_file_set_contents(dest_path, contents, (gssize)length, &err)) {
        LOG_E("Backup export write failed: %s", err->message);
        g_error_free(err);
        g_free(contents);
        return APP_ERR_IO;
    }

    g_free(contents);
    LOG_I("Backup exported to %s (%zu bytes)", dest_path, length);
    return APP_OK;
}

AppResult backup_import(const char *src_path, const char *db_path)
{
    /* Validate source is a SQLite3 file */
    FILE *f = fopen(src_path, "rb");
    if (!f) { LOG_E("Cannot open backup file"); return APP_ERR_IO; }
    char magic[16] = {0};
    fread(magic, 1, 15, f);
    fclose(f);
    if (strncmp(magic, "SQLite format 3", 15) != 0) {
        LOG_E("Not a valid vault backup");
        return APP_ERR_IO;
    }

    GError *err = NULL;
    char *contents = NULL;
    gsize length   = 0;
    if (!g_file_get_contents(src_path, &contents, &length, &err)) {
        g_error_free(err);
        return APP_ERR_IO;
    }
    /* Overwrite current db */
    if (!g_file_set_contents(db_path, contents, (gssize)length, &err)) {
        g_error_free(err);
        g_free(contents);
        return APP_ERR_IO;
    }
    g_free(contents);
    LOG_I("Backup imported from %s", src_path);
    return APP_OK;
}
