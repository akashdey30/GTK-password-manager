#include "master_auth.h"
#include "vault_storage.h"
#include "dialogs.h"
#include "encryption.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ------------------- Check if master exists -------------------
// Delegates to vault_storage so MASTER_FILE stays in one place
bool ma_master_exists(void) {
    return vs_master_exists();
}

// ------------------- Prompt for master password -------------------
bool ma_prompt_master_password(GtkWindow *parent, char session_master[]) {
    if (!ma_master_exists()) {
        // No master password exists, prompt user to create one
        GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                   "No master password found. Please create a new one.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        char new_pass[MAX_LEN] = {0};
        if (!dl_prompt_new_master(parent, new_pass)) {
            return false; // user cancelled
        }

        if (!vs_save_master(new_pass)) {
            GtkWidget *err = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Failed to save master password.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            return false;
        }

        strncpy(session_master, new_pass, MAX_LEN-1);
        return true;
    } else {
        // Master exists → prompt for verification
        char input_pass[MAX_LEN] = {0};
        if (!dl_prompt_verify_master(parent, input_pass)) {
            return false;
        }

        if (!vs_verify_master(input_pass)) {
            GtkWidget *err = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Incorrect master password.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            return false;
        }

        strncpy(session_master, input_pass, MAX_LEN-1);
        return true;
    }
}

// ------------------- Change master password -------------------
void ma_change_master_password(GtkButton *button, gpointer data) {
    AppState *state = (AppState *)data;

    char new_pass[MAX_LEN] = {0};
    if (!dl_prompt_new_master(GTK_WINDOW(state->window), new_pass)) {
        return; // user cancelled
    }

    if (!vs_save_master(new_pass)) {
        GtkWidget *err = gtk_message_dialog_new(GTK_WINDOW(state->window), GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                "Failed to save new master password.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    strncpy(state->session_master, new_pass, MAX_LEN-1);

    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(state->window), GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                            "Master password changed successfully.");
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}
