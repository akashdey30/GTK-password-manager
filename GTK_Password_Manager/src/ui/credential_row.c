#include "ui/credential_row.h"
#include <glib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    Credential      *cred;
    CredRowCallback  on_edit;
    CredRowCallback  on_delete;
    CredRowCallback  on_copy;
    gpointer         user_data;
} RowData;

static void row_data_free(RowData *rd)
{
    credential_free(rd->cred);
    g_free(rd);
}

/* Return uppercase first letter for avatar */
static char avatar_char(const char *title)
{
    if (!title || !*title) return '?';
    return (char)toupper((unsigned char)title[0]);
}

static void on_edit_click  (GtkButton *b, gpointer d) { (void)b; RowData *rd=d; if(rd->on_edit) rd->on_edit(rd->cred, rd->user_data); }
static void on_delete_click(GtkButton *b, gpointer d) { (void)b; RowData *rd=d; if(rd->on_delete) rd->on_delete(rd->cred, rd->user_data); }
static void on_copy_click  (GtkButton *b, gpointer d) { (void)b; RowData *rd=d; if(rd->on_copy) rd->on_copy(rd->cred, rd->user_data); }

GtkWidget *credential_row_new(const Credential *c,
                               CredRowCallback   on_edit,
                               CredRowCallback   on_delete,
                               CredRowCallback   on_copy_password,
                               gpointer          user_data)
{
    RowData *rd = g_new0(RowData, 1);
    /* Deep copy credential */
    rd->cred = g_new0(Credential, 1);
    rd->cred->id       = c->id;
    rd->cred->title    = g_strdup(c->title    ? c->title    : "");
    rd->cred->username = g_strdup(c->username ? c->username : "");
    rd->cred->password = g_strdup(c->password ? c->password : "");
    rd->cred->url      = g_strdup(c->url      ? c->url      : "");
    rd->cred->notes    = g_strdup(c->notes    ? c->notes    : "");
    rd->cred->category = g_strdup(c->category ? c->category : "");
    rd->on_edit   = on_edit;
    rd->on_delete = on_delete;
    rd->on_copy   = on_copy_password;
    rd->user_data = user_data;

    /* Outer row */
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(row, "cred-row");
    g_object_set_data_full(G_OBJECT(row), "row-data", rd,
                            (GDestroyNotify)row_data_free);

    /* Avatar circle with initial */
    char av[3] = { avatar_char(c->title), '\0' };
    GtkWidget *avatar_lbl = gtk_label_new(av);
    gtk_widget_set_size_request(avatar_lbl, 40, 40);
    gtk_widget_add_css_class(avatar_lbl, "cred-avatar");
    gtk_widget_set_valign(avatar_lbl, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), avatar_lbl);

    /* Text stack */
    GtkWidget *text_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_hexpand(text_col, TRUE);
    gtk_widget_set_valign(text_col, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), text_col);

    GtkWidget *title_lbl = gtk_label_new(c->title ? c->title : "(no title)");
    gtk_widget_add_css_class(title_lbl, "cred-title");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(title_lbl), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(text_col), title_lbl);

    char *sub = g_strdup_printf("%s%s%s",
        c->username ? c->username : "",
        (c->username && c->url) ? " · " : "",
        c->url ? c->url : "");
    GtkWidget *sub_lbl = gtk_label_new(sub);
    g_free(sub);
    gtk_widget_add_css_class(sub_lbl, "cred-username");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(sub_lbl), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(text_col), sub_lbl);

    /* Action buttons (shown on hover via CSS / always visible for now) */
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_valign(btn_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), btn_box);

    GtkWidget *copy_btn = gtk_button_new_from_icon_name("edit-copy-symbolic");
    gtk_widget_set_tooltip_text(copy_btn, "Copy password");
    gtk_widget_add_css_class(copy_btn, "flat");
    g_signal_connect(copy_btn, "clicked", G_CALLBACK(on_copy_click), rd);
    gtk_box_append(GTK_BOX(btn_box), copy_btn);

    GtkWidget *edit_btn = gtk_button_new_from_icon_name("document-edit-symbolic");
    gtk_widget_set_tooltip_text(edit_btn, "Edit");
    gtk_widget_add_css_class(edit_btn, "flat");
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_click), rd);
    gtk_box_append(GTK_BOX(btn_box), edit_btn);

    GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_set_tooltip_text(del_btn, "Delete");
    gtk_widget_add_css_class(del_btn, "flat");
    gtk_widget_add_css_class(del_btn, "destructive-action");
    g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_click), rd);
    gtk_box_append(GTK_BOX(btn_box), del_btn);

    return row;
}
