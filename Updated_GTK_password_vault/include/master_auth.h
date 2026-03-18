#ifndef MASTER_AUTH_H
#define MASTER_AUTH_H

#include "app_state.h"
#include <gtk/gtk.h>
#include <stdbool.h>

// Prompts the user for the master password at startup (handles both new and existing)
bool ma_prompt_master_password(GtkWindow *parent, char session_master[]);

// Change the master password (GTK button callback)
void ma_change_master_password(GtkButton *button, gpointer data);

// Check if master password file exists
bool ma_master_exists(void);

#endif // MASTER_AUTH_H
