#ifndef BACKUP_H
#define BACKUP_H

#include "global_types.h"

AppResult backup_export(const char *db_path, const char *dest_path);
AppResult backup_import(const char *src_path, const char *db_path);

#endif
