#include "ui/dialogs.h"
#include "security/password_generator.h"
#include "security/password_strength.h"
#include "security/backup.h"
#include "auth/master_auth.h"
#include "auth/recovery_auth.h"
#include "crypto/aes_engine.h"
#include "crypto/crypto.h"
#include "database/db_init.h"
#include "database/db_credentials.h"
#include "core/session.h"
#include "utils/logger.h"
#include <glib.h>
#include <string.h>

/* ── Shared helpers ───────────────────────────────── */
static GtkWidget *make_field(GtkWidget *box, const char *label, bool secret)
{
    GtkWidget *lbl = gtk_label_new(label);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), lbl);
    GtkWidget *entry;
    if (secret) {
        entry = gtk_password_entry_new();
        gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(entry), true);
    } else {
        entry = gtk_entry_new();
    }
    gtk_box_append(GTK_BOX(box), entry);
    return entry;
}

static void show_dialog(GtkWindow *parent, const char *title,
                         GtkWidget *content, int width, int height)
{
    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), title);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dlg), width, height);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), content);
    gtk_window_set_child(GTK_WINDOW(dlg), scroll);
    gtk_window_present(GTK_WINDOW(dlg));
}

/* ── Credential form ─────────────────────────────── */
typedef struct {
    GtkWidget          *title_e, *username_e, *password_e;
    GtkWidget          *url_e,   *notes_e,    *category_e;
    GtkWidget          *strength_bar, *strength_lbl;
    DialogSaveCallback  on_save;
    gpointer            user_data;
    int64_t             edit_id;
} CredFormData;

static void on_password_changed(GtkEditable *e, gpointer user_data)
{
    CredFormData *d = user_data;
    const char *pw = gtk_editable_get_text(e);
    PasswordStrengthResult r = password_check_strength(pw);
    gtk_level_bar_set_value(GTK_LEVEL_BAR(d->strength_bar), r.score / 20.0);
    gtk_label_set_text(GTK_LABEL(d->strength_lbl), r.label);
    static const char *cls[] = {"very-weak","weak","fair","strong","very-strong",NULL};
    for (int i = 0; cls[i]; i++)
        gtk_widget_remove_css_class(d->strength_bar, cls[i]);
    const char *names[] = {"very-weak","weak","fair","strong","very-strong"};
    gtk_widget_add_css_class(d->strength_bar, names[(int)r.level]);
}

static void on_cred_save(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    CredFormData *d = user_data;
    Credential c = {0};
    c.id       = d->edit_id;
    c.title    = (char*)gtk_editable_get_text(GTK_EDITABLE(d->title_e));
    c.username = (char*)gtk_editable_get_text(GTK_EDITABLE(d->username_e));
    c.password = (char*)gtk_editable_get_text(GTK_EDITABLE(d->password_e));
    c.url      = (char*)gtk_editable_get_text(GTK_EDITABLE(d->url_e));
    c.notes    = (char*)gtk_editable_get_text(GTK_EDITABLE(d->notes_e));
    c.category = (char*)gtk_editable_get_text(GTK_EDITABLE(d->category_e));
    if (!c.title || !*c.title) { LOG_W("Title required"); return; }
    if (d->on_save) d->on_save(&c, d->user_data);
    GtkWidget *w = GTK_WIDGET(btn);
    while (w && !GTK_IS_WINDOW(w)) w = gtk_widget_get_parent(w);
    if (w) gtk_window_close(GTK_WINDOW(w));
}

static GtkWidget *build_cred_form(const Credential  *prefill,
                                    DialogSaveCallback  on_save,
                                    gpointer            user_data,
                                    int64_t             edit_id)
{
    CredFormData *d = g_new0(CredFormData, 1);
    d->on_save = on_save; d->user_data = user_data; d->edit_id = edit_id;

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16); gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 8);   gtk_widget_set_margin_bottom(box, 8);

    d->title_e    = make_field(box, "Title *",          false);
    d->username_e = make_field(box, "Username / Email", false);
    d->password_e = make_field(box, "Password",         true);
    d->url_e      = make_field(box, "URL",              false);
    d->notes_e    = make_field(box, "Notes",            false);
    d->category_e = make_field(box, "Category",         false);

    d->strength_bar = gtk_level_bar_new_for_interval(0, 5);
    gtk_widget_add_css_class(d->strength_bar, "strength-bar");
    gtk_box_append(GTK_BOX(box), d->strength_bar);
    d->strength_lbl = gtk_label_new("");
    gtk_widget_set_halign(d->strength_lbl, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), d->strength_lbl);

    g_signal_connect(d->password_e, "changed", G_CALLBACK(on_password_changed), d);

    if (prefill) {
        if (prefill->title)    gtk_editable_set_text(GTK_EDITABLE(d->title_e),    prefill->title);
        if (prefill->username) gtk_editable_set_text(GTK_EDITABLE(d->username_e), prefill->username);
        if (prefill->password) gtk_editable_set_text(GTK_EDITABLE(d->password_e), prefill->password);
        if (prefill->url)      gtk_editable_set_text(GTK_EDITABLE(d->url_e),      prefill->url);
        if (prefill->notes)    gtk_editable_set_text(GTK_EDITABLE(d->notes_e),    prefill->notes);
        if (prefill->category) gtk_editable_set_text(GTK_EDITABLE(d->category_e), prefill->category);
    }

    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    g_object_set_data_full(G_OBJECT(save_btn), "form-data", d, g_free);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_cred_save), d);
    gtk_box_append(GTK_BOX(box), save_btn);
    return box;
}

void dialog_add_credential(GtkWindow *parent, DialogSaveCallback on_save, gpointer user_data)
{
    show_dialog(parent, "Add Credential",
                build_cred_form(NULL, on_save, user_data, 0), 440, 500);
}

void dialog_edit_credential(GtkWindow *parent, const Credential *c,
                              DialogSaveCallback on_save, gpointer user_data)
{
    show_dialog(parent, "Edit Credential",
                build_cred_form(c, on_save, user_data, c->id), 440, 500);
}

/* ── Password Generator ───────────────────────────── */
typedef struct {
    GtkWidget *result_entry, *len_spin;
    GtkWidget *upper_chk, *lower_chk, *digits_chk, *symbols_chk, *ambiguous_chk;
} GenDialogData;

static void on_generate(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    GenDialogData *d = user_data;
    PwGenOptions opts = {
        .length            = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->len_spin)),
        .use_upper         = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->upper_chk)),
        .use_lower         = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->lower_chk)),
        .use_digits        = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->digits_chk)),
        .use_symbols       = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->symbols_chk)),
        .exclude_ambiguous = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->ambiguous_chk)),
    };
    char *pw = password_generate(&opts);
    gtk_editable_set_text(GTK_EDITABLE(d->result_entry), pw);
    g_free(pw);
}

void dialog_generator(GtkWindow *parent)
{
    GenDialogData *d = g_new0(GenDialogData, 1);
    PwGenOptions def = password_gen_defaults();

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16); gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 12);   gtk_widget_set_margin_bottom(box, 12);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Length:"));
    d->len_spin = gtk_spin_button_new_with_range(4, 128, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->len_spin), def.length);
    gtk_box_append(GTK_BOX(row), d->len_spin);
    gtk_box_append(GTK_BOX(box), row);

    d->upper_chk     = gtk_check_button_new_with_label("Uppercase (A-Z)");
    d->lower_chk     = gtk_check_button_new_with_label("Lowercase (a-z)");
    d->digits_chk    = gtk_check_button_new_with_label("Digits (0-9)");
    d->symbols_chk   = gtk_check_button_new_with_label("Symbols (!@#…)");
    d->ambiguous_chk = gtk_check_button_new_with_label("Exclude ambiguous");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->upper_chk),     def.use_upper);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->lower_chk),     def.use_lower);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->digits_chk),    def.use_digits);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->symbols_chk),   def.use_symbols);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->ambiguous_chk), def.exclude_ambiguous);
    gtk_box_append(GTK_BOX(box), d->upper_chk);
    gtk_box_append(GTK_BOX(box), d->lower_chk);
    gtk_box_append(GTK_BOX(box), d->digits_chk);
    gtk_box_append(GTK_BOX(box), d->symbols_chk);
    gtk_box_append(GTK_BOX(box), d->ambiguous_chk);

    d->result_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(d->result_entry), FALSE);
    gtk_box_append(GTK_BOX(box), d->result_entry);

    GtkWidget *gen_btn = gtk_button_new_with_label("Generate");
    gtk_widget_add_css_class(gen_btn, "suggested-action");
    g_object_set_data_full(G_OBJECT(gen_btn), "gen-data", d, g_free);
    g_signal_connect(gen_btn, "clicked", G_CALLBACK(on_generate), d);
    gtk_box_append(GTK_BOX(box), gen_btn);

    on_generate(NULL, d);
    show_dialog(parent, "Password Generator", box, 380, 360);
}

/* ── Backup ───────────────────────────────────────── */
static void backup_save_cb(GObject *source, GAsyncResult *res, gpointer user_data)
{
    (void)user_data;
    GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, NULL);
    if (!file) return;
    const char *db_path = g_object_get_data(source, "db-path");
    char *path = g_file_get_path(file);
    if (db_path && path) backup_export(db_path, path);
    g_free(path);
    g_object_unref(file);
}

void dialog_backup(GtkWindow *parent, const char *db_path)
{
    GtkFileDialog *dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_title(GTK_FILE_DIALOG(dlg), "Export Vault Backup");
    char *name = g_strdup_printf("vault_backup%s", BACKUP_EXT);
    gtk_file_dialog_set_initial_name(GTK_FILE_DIALOG(dlg), name);
    g_free(name);
    g_object_set_data_full(G_OBJECT(dlg), "db-path", g_strdup(db_path), g_free);
    gtk_file_dialog_save(GTK_FILE_DIALOG(dlg), parent, NULL, backup_save_cb, NULL);
}

/* ── Recovery phrase setup ──────────────────────── */
void dialog_recovery_setup(GtkWindow *parent, const char *db_path, const uint8_t *key)
{
    char *phrase = recovery_generate_phrase();
    if (!phrase) return;
    recovery_store_phrase(phrase, db_path, key);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 20); gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 12);   gtk_widget_set_margin_bottom(box, 12);

    GtkWidget *hdr = gtk_label_new("Your New Recovery Phrase");
    gtk_widget_add_css_class(hdr, "cred-title");
    gtk_widget_set_halign(hdr, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), hdr);

    GtkWidget *sub = gtk_label_new(
        "Write this down and store it somewhere safe.\n"
        "It will not be shown again.");
    gtk_widget_add_css_class(sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(sub), TRUE);
    gtk_label_set_justify(GTK_LABEL(sub), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), sub);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *pl = gtk_label_new(phrase);
    gtk_label_set_selectable(GTK_LABEL(pl), TRUE);
    gtk_label_set_wrap(GTK_LABEL(pl), TRUE);
    gtk_label_set_justify(GTK_LABEL(pl), GTK_JUSTIFY_CENTER);
    gtk_widget_add_css_class(pl, "cred-title");
    gtk_widget_set_margin_top(pl, 8);
    gtk_widget_set_margin_bottom(pl, 8);
    gtk_box_append(GTK_BOX(box), pl);
    g_free(phrase);

    show_dialog(parent, "Recovery Phrase", box, 440, 260);
}

/* ── Confirm dialog ──────────────────────────────── */
typedef struct { gboolean result; GMainLoop *loop; } ConfirmData;

static void on_yes(GtkButton *b, gpointer ud)
{ (void)b; ConfirmData *c=ud; c->result=TRUE;  g_main_loop_quit(c->loop); }

static void on_no(GtkButton *b, gpointer ud)
{ (void)b; ConfirmData *c=ud; c->result=FALSE; g_main_loop_quit(c->loop); }

static void on_confirm_destroy(GtkWidget *w, gpointer ud)
{ (void)w; ConfirmData *c=ud; g_main_loop_quit(c->loop); }

gboolean dialog_confirm(GtkWindow *parent, const char *title, const char *message)
{
    ConfirmData cd = { FALSE, g_main_loop_new(NULL, FALSE) };

    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), title);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 360, -1);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24); gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);   gtk_widget_set_margin_bottom(box, 16);
    GtkWidget *lbl = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(lbl), TRUE);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    GtkWidget *cancel_btn  = gtk_button_new_with_label("Cancel");
    GtkWidget *confirm_btn = gtk_button_new_with_label("Delete");
    gtk_widget_add_css_class(confirm_btn, "destructive-action");
    g_signal_connect(cancel_btn,  "clicked", G_CALLBACK(on_no),  &cd);
    g_signal_connect(confirm_btn, "clicked", G_CALLBACK(on_yes), &cd);
    g_signal_connect(dlg, "destroy", G_CALLBACK(on_confirm_destroy), &cd);
    gtk_box_append(GTK_BOX(btn_row), cancel_btn);
    gtk_box_append(GTK_BOX(btn_row), confirm_btn);
    gtk_box_append(GTK_BOX(box), btn_row);
    gtk_window_set_child(GTK_WINDOW(dlg), box);
    gtk_window_present(GTK_WINDOW(dlg));

    g_main_loop_run(cd.loop);
    g_main_loop_unref(cd.loop);
    if (GTK_IS_WINDOW(dlg)) gtk_window_close(GTK_WINDOW(dlg));
    return cd.result;
}

/* ═══════════════════════════════════════════════════
   SETTINGS DIALOG
   Tab 1 – Security: change master pw, regen phrase
   Tab 2 – Saved Passwords: edit any credential's pw
   ═══════════════════════════════════════════════════ */

/* ── Change master password ── */
typedef struct {
    char      *db_path;
    Session   *session;
    GtkWidget *cur_pw_e;
    GtkWidget *new_pw_e;
    GtkWidget *conf_pw_e;
    GtkWidget *pw_status;
} ChangePwData;

static void on_change_master_pw(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    ChangePwData *d = user_data;

    const char *cur  = gtk_editable_get_text(GTK_EDITABLE(d->cur_pw_e));
    const char *new1 = gtk_editable_get_text(GTK_EDITABLE(d->new_pw_e));
    const char *new2 = gtk_editable_get_text(GTK_EDITABLE(d->conf_pw_e));

    if (!cur || !*cur) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "Enter your current password.");
        return;
    }
    if (!new1 || strlen(new1) < 8) {
        gtk_label_set_text(GTK_LABEL(d->pw_status),
                           "New password must be at least 8 characters.");
        return;
    }
    if (strcmp(new1, new2) != 0) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "New passwords do not match.");
        return;
    }

    uint8_t test_key[AES_KEY_LEN];
    if (master_auth_verify(cur, d->db_path, test_key) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "Current password is incorrect.");
        aes_wipe(test_key, AES_KEY_LEN);
        return;
    }
    aes_wipe(test_key, AES_KEY_LEN);

    uint8_t salt[MASTER_SALT_LEN];
    uint8_t new_key[AES_KEY_LEN];
    if (!crypto_random_bytes(salt, MASTER_SALT_LEN) ||
        aes_derive_key(new1, salt, new_key) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "Key derivation failed.");
        return;
    }

    char *verifier = crypto_encrypt_str(new_key, "GTK_VAULT_OK");
    if (!verifier) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "Encryption error.");
        aes_wipe(new_key, AES_KEY_LEN);
        return;
    }

    sqlite3 *db = NULL;
    if (db_open(d->db_path, &db) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(d->pw_status), "Could not open database.");
        g_free(verifier);
        aes_wipe(new_key, AES_KEY_LEN);
        return;
    }
    db_store_master(db, salt, verifier);
    db_close(db);
    g_free(verifier);

    session_unlock(d->session, new_key);
    aes_wipe(new_key, AES_KEY_LEN);

    gtk_editable_set_text(GTK_EDITABLE(d->cur_pw_e),  "");
    gtk_editable_set_text(GTK_EDITABLE(d->new_pw_e),  "");
    gtk_editable_set_text(GTK_EDITABLE(d->conf_pw_e), "");
    gtk_label_set_text(GTK_LABEL(d->pw_status), "✓ Master password changed successfully.");
    LOG_I("Master password changed from settings");
}

/* ── Regen recovery phrase button ── */
typedef struct {
    char      *db_path;
    Session   *session;
    GtkWindow *parent;
} RegenRecData;

static void on_regen_recovery(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    RegenRecData *r = user_data;
    dialog_recovery_setup(r->parent, r->db_path, r->session->aes_key);
}

/* ── Per-credential password edit row ── */
typedef struct {
    char      *db_path;
    Session   *session;
    int64_t    cred_id;
    GtkWidget *pw_entry;
    GtkWidget *status_lbl;
    GtkWidget *strength_bar;
    GtkWidget *strength_lbl;
} CredPwEditData;

static void on_cred_pw_strength_changed(GtkEditable *e, gpointer user_data)
{
    CredPwEditData *d = user_data;
    const char *pw = gtk_editable_get_text(e);
    PasswordStrengthResult r = password_check_strength(pw);
    gtk_level_bar_set_value(GTK_LEVEL_BAR(d->strength_bar), r.score / 20.0);
    gtk_label_set_text(GTK_LABEL(d->strength_lbl), r.label);
    static const char *cls[] = {"very-weak","weak","fair","strong","very-strong",NULL};
    for (int i = 0; cls[i]; i++)
        gtk_widget_remove_css_class(d->strength_bar, cls[i]);
    const char *names[] = {"very-weak","weak","fair","strong","very-strong"};
    gtk_widget_add_css_class(d->strength_bar, names[(int)r.level]);
}

static void on_save_cred_pw(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    CredPwEditData *d = user_data;
    const char *new_pw = gtk_editable_get_text(GTK_EDITABLE(d->pw_entry));

    if (!new_pw || !*new_pw) {
        gtk_label_set_text(GTK_LABEL(d->status_lbl), "Password cannot be empty.");
        return;
    }

    sqlite3 *db = NULL;
    if (db_open(d->db_path, &db) != APP_OK) {
        gtk_label_set_text(GTK_LABEL(d->status_lbl), "Could not open database.");
        return;
    }

    Credential *c = db_cred_get(db, d->session->aes_key, d->cred_id);
    if (!c) {
        db_close(db);
        gtk_label_set_text(GTK_LABEL(d->status_lbl), "Credential not found.");
        return;
    }

    g_free(c->password);
    c->password = g_strdup(new_pw);
    AppResult res = db_cred_update(db, d->session->aes_key, c);
    credential_free(c);
    db_close(db);

    if (res == APP_OK) {
        gtk_editable_set_text(GTK_EDITABLE(d->pw_entry), "");
        gtk_label_set_text(GTK_LABEL(d->status_lbl), "✓ Password updated.");
        LOG_I("Credential password updated from settings (id=%lld)",
              (long long)d->cred_id);
    } else {
        gtk_label_set_text(GTK_LABEL(d->status_lbl), "Failed to save.");
    }
}

static void build_credentials_tab(GtkWidget *box,
                                    char      *db_path,
                                    Session   *session)
{
    GtkWidget *hdr = gtk_label_new("Edit Saved Passwords");
    gtk_widget_add_css_class(hdr, "cred-title");
    gtk_widget_set_halign(hdr, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), hdr);

    GtkWidget *sub = gtk_label_new(
        "Expand a credential below to update its password.\n"
        "Use the edit button in the main vault to change other fields.");
    gtk_widget_add_css_class(sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(sub), TRUE);
    gtk_widget_set_halign(sub, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), sub);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    sqlite3 *db = NULL;
    if (db_open(db_path, &db) != APP_OK) {
        gtk_box_append(GTK_BOX(box), gtk_label_new("Could not load credentials."));
        return;
    }
    int count = 0;
    Credential **creds = db_cred_list_all(db, session->aes_key, &count);
    db_close(db);

    if (count == 0) {
        GtkWidget *empty = gtk_label_new("No credentials saved yet.");
        gtk_widget_add_css_class(empty, "login-subtitle");
        gtk_box_append(GTK_BOX(box), empty);
        g_free(creds);
        return;
    }

    for (int i = 0; i < count; i++) {
        Credential *c = creds[i];

        /* Expander label */
        char *lbl_txt = g_strdup_printf("%s   %s",
            c->title    && *c->title    ? c->title    : "(no title)",
            c->username && *c->username ? c->username : "");
        GtkWidget *expander = gtk_expander_new(lbl_txt);
        g_free(lbl_txt);

        GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_margin_start(inner, 16);
        gtk_widget_set_margin_top(inner, 6);
        gtk_widget_set_margin_bottom(inner, 10);

        GtkWidget *pw_lbl = gtk_label_new("New password:");
        gtk_widget_set_halign(pw_lbl, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), pw_lbl);

        GtkWidget *pw_entry = gtk_password_entry_new();
        gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(pw_entry), TRUE);
        gtk_entry_set_placeholder_text(GTK_ENTRY(pw_entry), "Enter new password");
        gtk_box_append(GTK_BOX(inner), pw_entry);

        GtkWidget *sbar = gtk_level_bar_new_for_interval(0, 5);
        gtk_widget_add_css_class(sbar, "strength-bar");
        gtk_box_append(GTK_BOX(inner), sbar);

        GtkWidget *slbl = gtk_label_new("");
        gtk_widget_set_halign(slbl, GTK_ALIGN_END);
        gtk_widget_add_css_class(slbl, "cred-username");
        gtk_box_append(GTK_BOX(inner), slbl);

        GtkWidget *status_lbl = gtk_label_new("");
        gtk_widget_set_halign(status_lbl, GTK_ALIGN_START);
        gtk_widget_add_css_class(status_lbl, "login-subtitle");
        gtk_box_append(GTK_BOX(inner), status_lbl);

        GtkWidget *save_btn = gtk_button_new_with_label("Update Password");
        gtk_widget_add_css_class(save_btn, "suggested-action");
        gtk_widget_set_halign(save_btn, GTK_ALIGN_START);

        CredPwEditData *rd = g_new0(CredPwEditData, 1);
        rd->db_path      = db_path;
        rd->session      = session;
        rd->cred_id      = c->id;
        rd->pw_entry     = pw_entry;
        rd->status_lbl   = status_lbl;
        rd->strength_bar = sbar;
        rd->strength_lbl = slbl;

        g_object_set_data_full(G_OBJECT(save_btn), "cred-pw-data", rd, g_free);
        g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_cred_pw), rd);
        g_signal_connect(pw_entry, "changed", G_CALLBACK(on_cred_pw_strength_changed), rd);

        gtk_box_append(GTK_BOX(inner), save_btn);
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(box), expander);

        credential_free(c);
    }
    g_free(creds);
}

/* ── Main settings dialog ── */
void dialog_settings(GtkWindow *parent, const char *db_path, Session *session)
{
    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Settings");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 500, 580);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_window_set_child(GTK_WINDOW(dlg), notebook);

    /* ── Tab 1: Security ── */
    GtkWidget *sec_scroll = gtk_scrolled_window_new();
    GtkWidget *sec_box    = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(sec_box, 24); gtk_widget_set_margin_end(sec_box, 24);
    gtk_widget_set_margin_top(sec_box, 20);   gtk_widget_set_margin_bottom(sec_box, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sec_scroll), sec_box);

    /* Change password section */
    GtkWidget *pw_hdr = gtk_label_new("Change Master Password");
    gtk_widget_add_css_class(pw_hdr, "cred-title");
    gtk_widget_set_halign(pw_hdr, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(sec_box), pw_hdr);

    ChangePwData *cpd = g_new0(ChangePwData, 1);
    cpd->db_path  = g_strdup(db_path);
    cpd->session  = session;
    cpd->cur_pw_e  = make_field(sec_box, "Current password",     true);
    cpd->new_pw_e  = make_field(sec_box, "New password",         true);
    cpd->conf_pw_e = make_field(sec_box, "Confirm new password", true);
    cpd->pw_status = gtk_label_new("");
    gtk_widget_add_css_class(cpd->pw_status, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(cpd->pw_status), TRUE);
    gtk_widget_set_halign(cpd->pw_status, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(sec_box), cpd->pw_status);

    GtkWidget *change_btn = gtk_button_new_with_label("Change Password");
    gtk_widget_add_css_class(change_btn, "suggested-action");
    gtk_widget_set_halign(change_btn, GTK_ALIGN_START);
    g_object_set_data_full(G_OBJECT(change_btn), "cpd", cpd,
                            (GDestroyNotify)g_free);
    /* Note: cpd->db_path also needs freeing; use a wrapper */
    g_signal_connect(change_btn, "clicked", G_CALLBACK(on_change_master_pw), cpd);
    gtk_box_append(GTK_BOX(sec_box), change_btn);

    gtk_box_append(GTK_BOX(sec_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Recovery phrase section */
    GtkWidget *rec_hdr = gtk_label_new("Recovery Phrase");
    gtk_widget_add_css_class(rec_hdr, "cred-title");
    gtk_widget_set_halign(rec_hdr, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(sec_box), rec_hdr);

    GtkWidget *rec_sub = gtk_label_new(
        "Generate a new 12-word recovery phrase to replace the current one.\n"
        "Store the new phrase somewhere safe.");
    gtk_widget_add_css_class(rec_sub, "login-subtitle");
    gtk_label_set_wrap(GTK_LABEL(rec_sub), TRUE);
    gtk_widget_set_halign(rec_sub, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(sec_box), rec_sub);

    RegenRecData *rrd = g_new0(RegenRecData, 1);
    rrd->db_path = g_strdup(db_path);
    rrd->session = session;
    rrd->parent  = GTK_WINDOW(dlg);

    GtkWidget *rec_btn = gtk_button_new_with_label("Generate New Recovery Phrase");
    gtk_widget_set_halign(rec_btn, GTK_ALIGN_START);
    g_object_set_data_full(G_OBJECT(rec_btn), "rrd", rrd,
                            (GDestroyNotify)g_free);
    g_signal_connect(rec_btn, "clicked", G_CALLBACK(on_regen_recovery), rrd);
    gtk_box_append(GTK_BOX(sec_box), rec_btn);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sec_scroll,
                              gtk_label_new("Security"));

    /* ── Tab 2: Saved Passwords ── */
    GtkWidget *cred_scroll = gtk_scrolled_window_new();
    GtkWidget *cred_box    = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(cred_box, 24); gtk_widget_set_margin_end(cred_box, 24);
    gtk_widget_set_margin_top(cred_box, 20);   gtk_widget_set_margin_bottom(cred_box, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(cred_scroll), cred_box);
    build_credentials_tab(cred_box, (char*)db_path, session);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cred_scroll,
                              gtk_label_new("Saved Passwords"));

    gtk_window_present(GTK_WINDOW(dlg));
}
