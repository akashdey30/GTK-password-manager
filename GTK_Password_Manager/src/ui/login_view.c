#include "ui/login_view.h"
#include "ui/dialogs.h"
#include "auth/master_auth.h"
#include "auth/recovery_auth.h"
#include "crypto/crypto.h"
#include "crypto/aes_engine.h"
#include "database/db_init.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

/* ── Private state ───────────────────────────────────── */
typedef struct {
    GtkWidget          *password_entry;
    GtkWidget          *confirm_entry;
    GtkWidget          *confirm_row;
    GtkWidget          *status_label;
    GtkWidget          *submit_btn;
    char               *db_path;
    LoginSuccessCallback on_success;
    gpointer             user_data;
    bool                 setup_mode;
} LoginViewData;

static void lvd_free(LoginViewData *d)
{
    g_free(d->db_path);
    g_free(d);
}

static void set_status(LoginViewData *d, const char *msg, bool error)
{
    gtk_label_set_text(GTK_LABEL(d->status_label), msg);
    if (error)
        gtk_widget_add_css_class(d->status_label, "error");
    else
        gtk_widget_remove_css_class(d->status_label, "error");
}

/* ── Recovery phrase setup popup shown after vault creation ── */
typedef struct {
    char          *db_path;
    uint8_t        key[AES_KEY_LEN];
    LoginSuccessCallback on_success;
    gpointer       user_data;
    GtkWidget     *dlg;
} PostCreateData;

static void post_create_free(PostCreateData *p)
{
    aes_wipe(p->key, AES_KEY_LEN);
    g_free(p->db_path);
    g_free(p);
}

static void on_post_create_done(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    PostCreateData *p = user_data;
    gtk_window_close(GTK_WINDOW(p->dlg));
    p->on_success(p->key, p->user_data);
}

static void show_post_create_dialog(LoginViewData *lv,
                                     const uint8_t  key[AES_KEY_LEN])
{
    PostCreateData *p = g_new0(PostCreateData, 1);
    p->db_path    = g_strdup(lv->db_path);
    memcpy(p->key, key, AES_KEY_LEN);
    p->on_success = lv->on_success;
    p->user_data  = lv->user_data;

    /* Generate and store phrase immediately */
    char *phrase = recovery_generate_phrase();
    if (phrase) {
        recovery_store_phrase(phrase, lv->db_path, key);
    }

    p->dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(p->dlg), "Save Your Recovery Phrase");
    gtk_window_set_modal(GTK_WINDOW(p->dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(p->dlg), 460, -1);
    gtk_window_set_resizable(GTK_WINDOW(p->dlg), FALSE);
    g_object_set_data_full(G_OBJECT(p->dlg), "post-create-data", p,
                            (GDestroyNotify)post_create_free);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 32);
    gtk_widget_set_margin_end(box, 32);
    gtk_widget_set_margin_top(box, 28);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_window_set_child(GTK_WINDOW(p->dlg), box);

    GtkWidget *title = gtk_label_new("Write Down Your Recovery Phrase");
    gtk_widget_add_css_class(title, "login-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *sub = gtk_label_new(
        "If you ever forget your master password, this 12-word phrase\n"
        "is the only way to recover your vault. Write it down and keep\n"
        "it somewhere safe — it will not be shown again.");
    gtk_widget_add_css_class(sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(sub), TRUE);
    gtk_label_set_justify(GTK_LABEL(sub), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), sub);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Phrase display box */
    GtkWidget *phrase_frame = gtk_frame_new(NULL);
    gtk_widget_set_margin_top(phrase_frame, 4);
    gtk_widget_set_margin_bottom(phrase_frame, 4);
    GtkWidget *phrase_lbl = gtk_label_new(phrase ? phrase : "(error generating phrase)");
    gtk_widget_set_margin_start(phrase_lbl, 16);
    gtk_widget_set_margin_end(phrase_lbl, 16);
    gtk_widget_set_margin_top(phrase_lbl, 12);
    gtk_widget_set_margin_bottom(phrase_lbl, 12);
    gtk_label_set_wrap(GTK_LABEL(phrase_lbl), TRUE);
    gtk_label_set_selectable(GTK_LABEL(phrase_lbl), TRUE);
    gtk_label_set_justify(GTK_LABEL(phrase_lbl), GTK_JUSTIFY_CENTER);
    /* Make phrase text stand out */
    gtk_widget_add_css_class(phrase_lbl, "cred-title");
    gtk_frame_set_child(GTK_FRAME(phrase_frame), phrase_lbl);
    gtk_box_append(GTK_BOX(box), phrase_frame);

    if (phrase) g_free(phrase);

    GtkWidget *warn = gtk_label_new(
        "⚠  This phrase is stored encrypted in your vault.\n"
        "Without it AND your master password you cannot recover your data.");
    gtk_widget_add_css_class(warn, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(warn), TRUE);
    gtk_label_set_justify(GTK_LABEL(warn), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(warn, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), warn);

    GtkWidget *done_btn = gtk_button_new_with_label("I've Written It Down — Enter Vault");
    gtk_widget_add_css_class(done_btn, "suggested-action");
    g_signal_connect(done_btn, "clicked", G_CALLBACK(on_post_create_done), p);
    gtk_box_append(GTK_BOX(box), done_btn);

    gtk_window_present(GTK_WINDOW(p->dlg));
}

/* ── submit handler ───────────────────────────────────── */
static void on_submit(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    LoginViewData *d = user_data;
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(d->password_entry));

    if (!pw || strlen(pw) < 8) {
        set_status(d, "Password must be at least 8 characters.", true);
        return;
    }

    if (d->setup_mode) {
        const char *confirm = gtk_editable_get_text(GTK_EDITABLE(d->confirm_entry));
        if (strcmp(pw, confirm) != 0) {
            set_status(d, "Passwords do not match.", true);
            return;
        }

        uint8_t salt[MASTER_SALT_LEN];
        uint8_t key[AES_KEY_LEN];
        if (!crypto_random_bytes(salt, MASTER_SALT_LEN) ||
            aes_derive_key(pw, salt, key) != APP_OK) {
            set_status(d, "Key derivation failed.", true);
            return;
        }

        char *verifier = crypto_encrypt_str(key, "GTK_VAULT_OK");
        if (!verifier) { set_status(d, "Encryption error.", true); return; }

        sqlite3 *db = NULL;
        db_open(d->db_path, &db);
        db_store_master(db, salt, verifier);
        db_close(db);
        g_free(verifier);

        /* Show recovery phrase dialog before entering vault */
        set_status(d, "", false);
        show_post_create_dialog(d, key);
        aes_wipe(key, AES_KEY_LEN);
        return;
    }

    /* Unlock existing vault */
    uint8_t key[AES_KEY_LEN];
    AppResult r = master_auth_verify(pw, d->db_path, key);
    if (r != APP_OK) {
        set_status(d, "Incorrect master password.", true);
        LOG_W("Login failed");
        return;
    }
    set_status(d, "", false);
    d->on_success(key, d->user_data);
    aes_wipe(key, AES_KEY_LEN);
}

static void on_entry_activate(GtkEntry *e, gpointer user_data)
{
    (void)e;
    LoginViewData *d = user_data;
    on_submit(NULL, d);
}

/* ═══════════════════════════════════════════════════════
   FORGOT PASSWORD — RECOVERY DIALOG
   Step 1: verify phrase  →  Step 2: set new password
   ═══════════════════════════════════════════════════════ */
typedef struct {
    char               *db_path;
    LoginSuccessCallback on_success;
    gpointer             user_data;
    GtkWidget           *dlg;
    GtkWidget           *phrase_entry;
    GtkWidget           *step1_box;
    GtkWidget           *rec_status;
    GtkWidget           *step2_box;
    GtkWidget           *new_pw_entry;
    GtkWidget           *confirm_pw_entry;
    GtkWidget           *step2_status;
} RecoveryData;

static void rec_free(RecoveryData *r)
{
    g_free(r->db_path);
    g_free(r);
}

static void on_rec_set_password(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    RecoveryData *r = user_data;

    const char *pw1 = gtk_editable_get_text(GTK_EDITABLE(r->new_pw_entry));
    const char *pw2 = gtk_editable_get_text(GTK_EDITABLE(r->confirm_pw_entry));

    if (!pw1 || strlen(pw1) < 8) {
        gtk_label_set_text(GTK_LABEL(r->step2_status),
                           "Password must be at least 8 characters.");
        return;
    }
    if (strcmp(pw1, pw2) != 0) {
        gtk_label_set_text(GTK_LABEL(r->step2_status), "Passwords do not match.");
        return;
    }

    uint8_t salt[MASTER_SALT_LEN];
    uint8_t key[AES_KEY_LEN];
    if (!crypto_random_bytes(salt, MASTER_SALT_LEN) ||
        aes_derive_key(pw1, salt, key) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(r->step2_status), "Key derivation failed.");
        return;
    }

    char *verifier = crypto_encrypt_str(key, "GTK_VAULT_OK");
    if (!verifier) {
        gtk_label_set_text(GTK_LABEL(r->step2_status), "Encryption error.");
        aes_wipe(key, AES_KEY_LEN);
        return;
    }

    sqlite3 *db = NULL;
    if (db_open(r->db_path, &db) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(r->step2_status), "Could not open database.");
        g_free(verifier);
        aes_wipe(key, AES_KEY_LEN);
        return;
    }
    db_store_master(db, salt, verifier);
    db_close(db);
    g_free(verifier);

    LOG_I("Master password reset via recovery phrase");
    gtk_window_close(GTK_WINDOW(r->dlg));
    r->on_success(key, r->user_data);
    aes_wipe(key, AES_KEY_LEN);
}

static void on_rec_verify_phrase(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    RecoveryData *r = user_data;

    const char *phrase = gtk_editable_get_text(GTK_EDITABLE(r->phrase_entry));
    if (!phrase || !*phrase) {
        gtk_label_set_text(GTK_LABEL(r->rec_status), "Please enter your recovery phrase.");
        return;
    }

    uint8_t dummy_key[AES_KEY_LEN];
    AppResult res = recovery_verify_phrase(phrase, r->db_path, dummy_key);
    aes_wipe(dummy_key, AES_KEY_LEN);

    if (res != APP_OK) {
        gtk_label_set_text(GTK_LABEL(r->rec_status),
                           "Recovery phrase incorrect. Please try again.");
        LOG_W("Recovery phrase verification failed");
        return;
    }

    LOG_I("Recovery phrase verified");
    gtk_widget_set_visible(r->step1_box, FALSE);
    gtk_widget_set_visible(r->step2_box, TRUE);
}

static void show_recovery_dialog(LoginViewData *lv)
{
    RecoveryData *r = g_new0(RecoveryData, 1);
    r->db_path    = g_strdup(lv->db_path);
    r->on_success = lv->on_success;
    r->user_data  = lv->user_data;

    r->dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(r->dlg), "Account Recovery");
    gtk_window_set_modal(GTK_WINDOW(r->dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(r->dlg), 420, -1);
    gtk_window_set_resizable(GTK_WINDOW(r->dlg), FALSE);
    g_object_set_data_full(G_OBJECT(r->dlg), "rec-data", r,
                            (GDestroyNotify)rec_free);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(outer, 32);
    gtk_widget_set_margin_end(outer, 32);
    gtk_widget_set_margin_top(outer, 28);
    gtk_widget_set_margin_bottom(outer, 24);
    gtk_window_set_child(GTK_WINDOW(r->dlg), outer);

    /* Step 1 */
    r->step1_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_append(GTK_BOX(outer), r->step1_box);

    GtkWidget *s1_title = gtk_label_new("Recover Your Vault");
    gtk_widget_add_css_class(s1_title, "login-title");
    gtk_widget_set_halign(s1_title, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(r->step1_box), s1_title);

    GtkWidget *s1_sub = gtk_label_new(
        "Enter your 12-word recovery phrase below.\n"
        "Words should be separated by spaces.");
    gtk_widget_add_css_class(s1_sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(s1_sub), TRUE);
    gtk_label_set_justify(GTK_LABEL(s1_sub), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(s1_sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(r->step1_box), s1_sub);

    gtk_box_append(GTK_BOX(r->step1_box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    r->phrase_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(r->phrase_entry),
                                   "word1 word2 word3 … word12");
    gtk_box_append(GTK_BOX(r->step1_box), r->phrase_entry);

    r->rec_status = gtk_label_new("");
    gtk_widget_add_css_class(r->rec_status, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(r->rec_status), TRUE);
    gtk_box_append(GTK_BOX(r->step1_box), r->rec_status);

    GtkWidget *verify_btn = gtk_button_new_with_label("Verify Phrase");
    gtk_widget_add_css_class(verify_btn, "suggested-action");
    g_signal_connect(verify_btn, "clicked", G_CALLBACK(on_rec_verify_phrase), r);
    gtk_box_append(GTK_BOX(r->step1_box), verify_btn);

    /* Step 2 */
    r->step2_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_visible(r->step2_box, FALSE);
    gtk_box_append(GTK_BOX(outer), r->step2_box);

    GtkWidget *s2_title = gtk_label_new("Set New Master Password");
    gtk_widget_add_css_class(s2_title, "login-title");
    gtk_widget_set_halign(s2_title, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(r->step2_box), s2_title);

    GtkWidget *s2_sub = gtk_label_new(
        "Recovery phrase verified.\nChoose a new master password.");
    gtk_widget_add_css_class(s2_sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(s2_sub), TRUE);
    gtk_label_set_justify(GTK_LABEL(s2_sub), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(s2_sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(r->step2_box), s2_sub);

    gtk_box_append(GTK_BOX(r->step2_box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    r->new_pw_entry = gtk_password_entry_new();
    gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(r->new_pw_entry), TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(r->new_pw_entry), "New master password");
    gtk_box_append(GTK_BOX(r->step2_box), r->new_pw_entry);

    r->confirm_pw_entry = gtk_password_entry_new();
    gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(r->confirm_pw_entry), TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(r->confirm_pw_entry), "Confirm new password");
    gtk_box_append(GTK_BOX(r->step2_box), r->confirm_pw_entry);

    r->step2_status = gtk_label_new("");
    gtk_widget_add_css_class(r->step2_status, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(r->step2_status), TRUE);
    gtk_box_append(GTK_BOX(r->step2_box), r->step2_status);

    GtkWidget *save_btn = gtk_button_new_with_label("Reset Password & Unlock");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_rec_set_password), r);
    gtk_box_append(GTK_BOX(r->step2_box), save_btn);

    gtk_window_present(GTK_WINDOW(r->dlg));
}

static void on_forgot(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    LoginViewData *d = user_data;

    sqlite3 *db = NULL;
    bool has_phrase = false;
    if (db_open(d->db_path, &db) == APP_OK) {
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db,
            "SELECT 1 FROM meta WHERE key='recovery_phrase' LIMIT 1;",
            -1, &stmt, NULL);
        has_phrase = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        db_close(db);
    }

    if (!has_phrase) {
        set_status(d,
            "No recovery phrase is set up for this vault. "
            "Delete the vault database and start over.",
            true);
        return;
    }

    show_recovery_dialog(d);
}

/* ── builder ─────────────────────────────────────────── */
GtkWidget *login_view_new(const char          *db_path,
                           LoginSuccessCallback  on_success,
                           gpointer              user_data)
{
    LoginViewData *d = g_new0(LoginViewData, 1);
    d->db_path    = g_strdup(db_path);
    d->on_success = on_success;
    d->user_data  = user_data;
    d->setup_mode = !master_auth_is_set(db_path);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(outer, 400, -1);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(card, "login-card");
    gtk_box_append(GTK_BOX(outer), card);

    GtkWidget *icon = gtk_image_new_from_icon_name("dialog-password");
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 48);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), icon);

    GtkWidget *title = gtk_label_new(d->setup_mode ? "Create Vault" : "Unlock Vault");
    gtk_widget_add_css_class(title, "login-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), title);

    GtkWidget *sub = gtk_label_new(
        d->setup_mode
        ? "Set a strong master password to secure your vault."
        : "Enter your master password to continue.");
    gtk_widget_add_css_class(sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(sub), true);
    gtk_label_set_justify(GTK_LABEL(sub), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), sub);

    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    d->password_entry = gtk_password_entry_new();
    gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(d->password_entry), true);
    gtk_entry_set_placeholder_text(GTK_ENTRY(d->password_entry), "Master password");
    g_signal_connect(d->password_entry, "activate", G_CALLBACK(on_entry_activate), d);
    gtk_box_append(GTK_BOX(card), d->password_entry);

    d->confirm_row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    d->confirm_entry = gtk_password_entry_new();
    gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(d->confirm_entry), true);
    gtk_entry_set_placeholder_text(GTK_ENTRY(d->confirm_entry), "Confirm password");
    g_signal_connect(d->confirm_entry, "activate", G_CALLBACK(on_entry_activate), d);
    gtk_box_append(GTK_BOX(d->confirm_row), d->confirm_entry);
    gtk_widget_set_visible(d->confirm_row, d->setup_mode);
    gtk_box_append(GTK_BOX(card), d->confirm_row);

    d->status_label = gtk_label_new("");
    gtk_widget_add_css_class(d->status_label, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(d->status_label), true);
    gtk_box_append(GTK_BOX(card), d->status_label);

    d->submit_btn = gtk_button_new_with_label(
        d->setup_mode ? "Create Vault" : "Unlock");
    gtk_widget_add_css_class(d->submit_btn, "suggested-action");
    g_signal_connect(d->submit_btn, "clicked", G_CALLBACK(on_submit), d);
    gtk_box_append(GTK_BOX(card), d->submit_btn);

    if (!d->setup_mode) {
        GtkWidget *forgot = gtk_button_new_with_label("Forgot password?");
        gtk_widget_add_css_class(forgot, "link");
        gtk_widget_set_halign(forgot, GTK_ALIGN_CENTER);
        g_signal_connect(forgot, "clicked", G_CALLBACK(on_forgot), d);
        gtk_box_append(GTK_BOX(card), forgot);
    }

    g_object_set_data_full(G_OBJECT(outer), "login-data", d,
                            (GDestroyNotify)lvd_free);
    return outer;
}
