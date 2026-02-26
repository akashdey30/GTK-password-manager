#ifndef DIALOGS_H
#define DIALOGS_H

#include <gtk/gtk.h>
#include "global_types.h"

typedef void (*DialogSaveCallback)(const Credential *c, gpointer user_data);

/* Add new credential */
void dialog_add_credential(GtkWindow         *parent,
                             DialogSaveCallback on_save,
                             gpointer           user_data);

/* Edit existing credential */
void dialog_edit_credential(GtkWindow         *parent,
                              const Credential  *c,
                              DialogSaveCallback on_save,
                              gpointer           user_data);

/* Password generator */
void dialog_generator(GtkWindow *parent);

/* Backup / restore */
void dialog_backup(GtkWindow  *parent,
                    const char *db_path);

/* Recovery phrase setup — shown after vault creation and from settings */
void dialog_recovery_setup(GtkWindow     *parent,
                             const char    *db_path,
                             const uint8_t *key);

/* Settings: change master password, reset recovery phrase, edit credentials */
void dialog_settings(GtkWindow     *parent,
                      const char    *db_path,
                      Session       *session);

/* Confirmation dialog */
gboolean dialog_confirm(GtkWindow  *parent,
                          const char *title,
                          const char *message);

#endif
