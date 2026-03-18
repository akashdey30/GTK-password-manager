#ifndef DIALOGS_H
#define DIALOGS_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "app_state.h"

// ------------------- Master Password Dialogs -------------------

// Prompt user to enter a new master password (first-time setup or change)
// Returns true if user entered and confirmed password, false if cancelled
bool dl_prompt_new_master(GtkWindow *parent, char out_pass[]);

// Prompt user to verify master password
// Returns true if user entered a password (caller checks validity)
bool dl_prompt_verify_master(GtkWindow *parent, char out_pass[]);

// ------------------- Credential Dialogs -------------------

// Add Credential dialog
void dl_add_credential_dialog(GtkButton *button, gpointer data);

// Search Credential dialog
void dl_search_credential_dialog(GtkButton *button, gpointer data);

// Delete Credential dialog
void dl_delete_credential_dialog(GtkButton *button, gpointer data);

// Show all credentials for a given service (triggered by service button click)
void dl_show_service_credentials(GtkButton *button, gpointer data);

#endif // DIALOGS_H
