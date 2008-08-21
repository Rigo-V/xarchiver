/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include "extract_dialog.h"
#include "window.h"
#include "string_utils.h"
#include "support.h"

extern gboolean unrar,batch_mode;
extern Prefs_dialog_data *prefs_window;
gchar *rar;

static gchar *xa_multi_extract_archive(gchar *,gboolean,gboolean,gchar *);
static void xa_select_where_to_extract ( GtkButton*,Multi_extract_data * );
static void xa_remove_files_liststore (GtkWidget *,Multi_extract_data *);
static void xa_multi_extract_dialog_select_files_to_add ( GtkButton* , Multi_extract_data * );
static void remove_foreach_func (GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GList **);
static void xa_multi_extract_dialog_drag_data_received (GtkWidget *,GdkDragContext *,int x,int y,GtkSelectionData *,unsigned int,unsigned int,gpointer );
static const GtkTargetEntry drop_targets[] =
{
	{ "text/uri-list", 0, 0 },
};

Extract_dialog_data *xa_create_extract_dialog()
{
	GSList *radiobutton1_group = NULL;
	Extract_dialog_data *dialog_data;

	dialog_data = g_new0 (Extract_dialog_data,1);
	dialog_data->dialog1 = gtk_dialog_new();

	gtk_window_set_position (GTK_WINDOW (dialog_data->dialog1),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->dialog1),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_has_separator (GTK_DIALOG(dialog_data->dialog1),FALSE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog_data->dialog1),TRUE);

	option_tooltip = gtk_tooltips_new ();
	dialog_data->dialog_vbox1 = GTK_DIALOG (dialog_data->dialog1)->vbox;

	vbox1 = gtk_vbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (dialog_data->dialog_vbox1), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	label1 = gtk_label_new (_("Extract to:"));
	gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

	dialog_data->destination_path_entry = gtk_entry_new ();
	gtk_entry_set_editable(GTK_ENTRY(dialog_data->destination_path_entry),FALSE);
	gtk_box_pack_start (GTK_BOX (vbox1), dialog_data->destination_path_entry, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2), frame1, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), vbox3);

	dialog_data->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("All files"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->all_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio));

	dialog_data->selected_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Selected files"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->selected_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->selected_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->selected_radio));

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox2, FALSE, FALSE, 0);

	dialog_data->files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Files: "));
	gtk_box_pack_start (GTK_BOX (hbox2), dialog_data->files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->files_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->files_radio));

	dialog_data->entry2 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox2),dialog_data->entry2, TRUE,TRUE,0);
	gtk_entry_set_width_chars (GTK_ENTRY (dialog_data->entry2),10);
	gtk_widget_set_sensitive(dialog_data->entry2,FALSE);
	g_signal_connect (G_OBJECT (dialog_data->files_radio),"toggled",G_CALLBACK(xa_activate_entry),dialog_data);

	label2 = gtk_label_new (_("Files "));
	gtk_frame_set_label_widget (GTK_FRAME (frame1), label2);

	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2),frame2,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2),GTK_SHADOW_OUT);

	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame2),alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2),0,0,12,0);

	vbox5 = gtk_vbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (alignment2),vbox5);

	dialog_data->overwrite_check = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->overwrite_check,FALSE,FALSE,0);

	dialog_data->extract_full = gtk_check_button_new_with_mnemonic (_("Extract files with full path"));
	gtk_tooltips_set_tip(option_tooltip,dialog_data->extract_full , _("The archive's directory structure is recreated in the extraction directory"), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->extract_full, FALSE, FALSE, 0);

	dialog_data->touch = gtk_check_button_new_with_mnemonic (_("Touch files"));
	gtk_tooltips_set_tip (option_tooltip,dialog_data->touch, _("When this option is used, tar leaves the data modification times of the files it extracts as the times when the files were extracted, instead of setting it to the times recorded in the archive"), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->touch, FALSE,FALSE,0);

	dialog_data->fresh = gtk_check_button_new_with_mnemonic (_("Freshen existing files"));
	gtk_tooltips_set_tip (option_tooltip,dialog_data->fresh , _("Extract only those files that already exist on disk and that are newer than the disk copies"), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->fresh, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (dialog_data->fresh),"toggled",G_CALLBACK (fresh_update_toggled_cb) , dialog_data);

	dialog_data->update = gtk_check_button_new_with_mnemonic (_("Update existing files"));
	gtk_tooltips_set_tip (option_tooltip,dialog_data->update , _("This option performs the same function as the freshen one, extracting files that are newer than those with the same name on disk, and in addition it extracts those files that do not already exist on disk"), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->update, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (dialog_data->update),"toggled",G_CALLBACK (update_fresh_toggled_cb) , dialog_data);
	
	label3 = gtk_label_new (_("Options "));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), label3);

	vbox4 = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox4, TRUE, TRUE, 0);

	label4 = gtk_label_new (_("Directories Tree:"));
	gtk_box_pack_start (GTK_BOX (vbox4), label4, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow1, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy", GTK_POLICY_AUTOMATIC,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);

	model = gtk_tree_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	dialog_data->treeview3 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), dialog_data->treeview3);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog_data->treeview3), FALSE);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),1,GTK_SORT_ASCENDING);

	g_signal_connect (G_OBJECT (dialog_data->treeview3),"row-expanded",G_CALLBACK(xa_expand_dir),dialog_data->destination_path_entry);
	g_signal_connect (G_OBJECT (dialog_data->treeview3),"row-activated",G_CALLBACK(xa_row_activated),dialog_data->destination_path_entry);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (dialog_data->treeview3));
	g_signal_connect (sel,"changed",G_CALLBACK (xa_tree_view_row_selected),dialog_data->destination_path_entry);

	column = gtk_tree_view_column_new();
	dialog_data->renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,dialog_data->renderer,FALSE);
	gtk_tree_view_column_set_attributes(column,dialog_data->renderer, "stock-id",0,NULL);

	dialog_data->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,dialog_data->renderer,TRUE);
	gtk_tree_view_column_set_attributes( column,dialog_data->renderer,"text",1,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog_data->treeview3),column);
	g_signal_connect (dialog_data->renderer, "editing-canceled",G_CALLBACK (xa_cell_edited_canceled),dialog_data);
	g_signal_connect (dialog_data->renderer, "edited",G_CALLBACK (xa_cell_edited),dialog_data);

	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox1),hbox4,FALSE,FALSE,0);
  
	dialog_data->create_dir = gtk_button_new_with_mnemonic (_("Create New Dir"));
	gtk_box_pack_end (GTK_BOX (hbox4),dialog_data->create_dir,FALSE, FALSE,0);
	g_signal_connect (G_OBJECT(dialog_data->create_dir),"clicked",G_CALLBACK(xa_create_dir_button_pressed),dialog_data);

	hbox3 = gtk_hbox_new (FALSE,0);
	gtk_box_pack_start (GTK_BOX (vbox5),hbox3,TRUE,TRUE,0);

	label_password = gtk_label_new (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox3), label_password,FALSE,FALSE,0);

	dialog_data->password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox3), dialog_data->password_entry,TRUE,TRUE,0);
	gtk_entry_set_visibility (GTK_ENTRY (dialog_data->password_entry),FALSE);

	dialog_action_area1 = GTK_DIALOG (dialog_data->dialog1)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

	extract_button = gtk_button_new();
	extract_image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	extract_hbox = gtk_hbox_new(FALSE,4);
	extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	alignment3 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment3),extract_hbox);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_label,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(extract_button),alignment3);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),extract_button,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (extract_button, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->dialog1),GTK_RESPONSE_OK);
	xa_browse_dir(model,NULL,NULL);
	return dialog_data;
}

void xa_create_dir_button_pressed (GtkButton *button,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter child,iter;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_object_set(dialog->renderer,"editable",TRUE,NULL);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview3));
	if (selection)
	{
		gtk_tree_selection_get_selected(selection,&model,&iter);
		gtk_widget_set_sensitive(dialog->create_dir,FALSE);
		gtk_tree_store_append(GTK_TREE_STORE(model),&child,&iter);
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(dialog->treeview3),&path,&column);
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(dialog->treeview3),path);

		gtk_tree_path_down(path);
		gtk_tree_selection_select_iter(selection,&child);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(dialog->treeview3),path,column,TRUE);
		gtk_tree_path_free (path);
	}
}

void xa_cell_edited_canceled(GtkCellRenderer *renderer,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview3));
	if (selection)
	{
		g_object_set(dialog->renderer,"editable",FALSE,NULL);
		gtk_tree_selection_get_selected(selection,&model,&iter);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
		gtk_widget_set_sensitive(dialog->create_dir,TRUE);
	}
}

void xa_cell_edited (GtkCellRendererText *cell,const gchar *path_string,const gchar *new_text,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *previous_dir;
	gchar *fullname;
	gint result;

	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(dialog->treeview3));
	gtk_tree_model_get_iter (model, &iter,path);
	
	previous_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->destination_path_entry)));
	fullname = g_strconcat(previous_dir,"/",new_text,NULL);
	g_free(previous_dir);

	gtk_tree_store_set(GTK_TREE_STORE(model),&iter,0,"gtk-directory",1,new_text,2,fullname,-1);

	result = mkdir (fullname,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXGRP);
	if (result == -1)
	{
		gchar *msg = g_strdup_printf(_("Can't create directory \"%s\""),fullname);
		result = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,g_strerror(errno ));
		g_free (msg);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
	}
	else
	{
		dialog->string = gtk_tree_path_to_string (gtk_tree_model_get_path (model, &iter));
		dialog->signal_id = g_signal_connect_after(G_OBJECT(GTK_TREE_VIEW(dialog->treeview3)),"expose-event",G_CALLBACK(xa_treeview_exposed),dialog);
		gtk_entry_set_text(GTK_ENTRY(dialog->destination_path_entry),fullname);
	}
	g_free(fullname);
	gtk_tree_path_free(path);
	gtk_widget_set_sensitive(dialog->create_dir,TRUE);
	g_object_set(dialog->renderer,"editable",FALSE,NULL);
}

void xa_activate_entry(GtkToggleButton *button,gpointer data)
{
	Extract_dialog_data *dialog = data;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->files_radio)))
	{
		gtk_widget_set_sensitive (dialog->entry2,TRUE);
		gtk_widget_grab_focus (dialog->entry2);
	}
	else
		gtk_widget_set_sensitive (dialog->entry2,FALSE);
}

void fresh_update_toggled_cb (GtkToggleButton *button,Extract_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update),FALSE);
}

void update_fresh_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data)
{
	if (data->fresh == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fresh),FALSE);
}

void xa_set_extract_dialog_options(Extract_dialog_data *dialog_data,gint selected,XArchive *archive)
{
	gchar *archive_dir = NULL;
	gboolean flag = TRUE;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->extract_dialog[0] != -1)
		gtk_window_set_default_size (GTK_WINDOW(dialog_data->dialog1), prefs_window->extract_dialog[0], prefs_window->extract_dialog[1]);
	else
		gtk_widget_set_size_request (dialog_data->dialog1,-1,372);

	if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA)
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Decompress file"));
	else
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Extract files"));

	if (archive->type != XARCHIVETYPE_RPM)
	{
		if (selected)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->selected_radio),TRUE);
			gtk_widget_set_sensitive (dialog_data->selected_radio,TRUE);
		}
		else
		{
			gtk_widget_set_sensitive (dialog_data->selected_radio,FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog_data->all_files_radio),TRUE);
		}
	}
	else
		gtk_widget_set_sensitive (dialog_data->selected_radio,FALSE);

	if (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA || archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_RPM)
		flag = FALSE;
	gtk_widget_set_sensitive (dialog_data->extract_full,flag);

	if (archive->type != XARCHIVETYPE_TAR && archive->type != XARCHIVETYPE_TAR_GZ && archive->type != XARCHIVETYPE_TAR_LZMA && archive->type != XARCHIVETYPE_TAR_BZ2 && archive->type != XARCHIVETYPE_DEB)
		flag = FALSE;
	else
		flag = TRUE;
	gtk_widget_set_sensitive (dialog_data->touch,flag);

	if (archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_ARJ)
		flag = TRUE;
	else
		flag = FALSE;

	gtk_widget_set_sensitive(dialog_data->fresh,flag);
	gtk_widget_set_sensitive(dialog_data->update,flag);
	
	if (archive->extraction_path == NULL)
	{
		archive_dir = xa_remove_level_from_path (archive->path);
		gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry),archive_dir);
	}
	else
		gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry),archive->extraction_path);

	g_free(archive_dir);
	if (archive->has_passwd)
    {
		gtk_widget_set_sensitive (label_password,TRUE);
		gtk_widget_set_sensitive (dialog_data->password_entry,TRUE);
		if (archive->passwd != NULL)
			gtk_entry_set_text (GTK_ENTRY(dialog_data->password_entry),archive->passwd);
    }
	else
	{
		gtk_widget_set_sensitive (label_password,FALSE);
		gtk_widget_set_sensitive (dialog_data->password_entry,FALSE);
	}
	gtk_widget_show_all(dialog_data->dialog1);
}

void xa_parse_extract_dialog_options (XArchive *archive,Extract_dialog_data *dialog_data,GtkTreeSelection *selection)
{
	gchar *destination_path = NULL, *string;
	gboolean done = FALSE;
	GSList *names = NULL;
	GtkTreeModel *model;
	int response;

	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

    while (! done)
	{
		switch (gtk_dialog_run(GTK_DIALOG(dialog_data->dialog1)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			if (xa_main_window && (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA || archive->type == XARCHIVETYPE_BZIP2) )
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				xa_set_button_state (1,1,GTK_WIDGET_IS_SENSITIVE(save1),GTK_WIDGET_IS_SENSITIVE(close1),0,0,0,0,0);
				archive->status = XA_ARCHIVESTATUS_IDLE;
			}
			break;

			case GTK_RESPONSE_OK:
			destination_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog_data->destination_path_entry)));
			archive->extraction_path = xa_escape_bad_chars (destination_path , "$\'`\"\\!?* ()&|@#:;");

			if (strlen(archive->extraction_path) == 0)
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed where to extract the files!"),_("Please enter the extraction path.") );
				break;
			}
			if (archive->extraction_path[0] != '/')
			{
				gchar *cur_dir = g_get_current_dir();
				archive->extraction_path = g_strconcat(cur_dir, "/",archive->extraction_path,NULL);
				g_free (cur_dir);
			}
			if (archive->has_passwd)
				archive->passwd  = g_strdup (gtk_entry_get_text (GTK_ENTRY(dialog_data->password_entry)));

			if (archive->has_passwd && strlen(archive->passwd) == 0 )
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("This archive is encrypted!"),_("Please enter the password.") );
				break;
			}

			if (g_file_test (destination_path,G_FILE_TEST_IS_DIR) && access (destination_path, R_OK | W_OK | X_OK ))
			{
				gchar *utf8_path;
				gchar  *msg;

                utf8_path = g_filename_to_utf8 (destination_path, -1, NULL, NULL, NULL);
                msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."), utf8_path);
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't perform extraction!"),msg );
                g_free (utf8_path);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;
			g_free (destination_path);

			archive->overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->overwrite_check));
			archive->tar_touch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->touch));
			archive->full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->extract_full));
			archive->freshen   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->fresh));
			archive->update    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->update));

			gtk_widget_hide (dialog_data->dialog1);
			archive->status = XA_ARCHIVESTATUS_EXTRACT;
			/* Is the radiobutton Files selected? */
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->files_radio)))
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
				string = g_strdup (gtk_entry_get_text(GTK_ENTRY(dialog_data->entry2)));
				gtk_tree_model_foreach(model,(GtkTreeModelForeachFunc)select_matched_rows,string);
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc) xa_concat_filenames,&names);
				g_free(string);
			}
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->selected_radio)))
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc) xa_concat_filenames,&names);

			if (xa_main_window)
			{
				xa_set_button_state (0,0,0,0,0,0,0,0,0);
				gtk_widget_set_sensitive ( Stop_button ,TRUE);
				gtk_label_set_text(GTK_LABEL(total_label),_("Extracting files from archive, please wait..."));
			}
			(*archive->extract) (archive,names);
		}
	}
	gtk_widget_hide (dialog_data->dialog1);
}

void xa_browse_dir(GtkTreeStore *model,gchar *path, GtkTreeIter *parent)
{
	DIR *dir;
	struct dirent *list;
	gchar *fullname;
	GtkTreeIter iter;
	GtkTreeIter dummy;

	dir = opendir(path ? path : "/");

	if (dir == NULL)
		return;
	while ((list = readdir(dir)))
	{
		if (list->d_name[0] == '.')
			continue;
		fullname = g_strconcat (path ? path : "","/",list->d_name,NULL);
		if (g_file_test(fullname,G_FILE_TEST_IS_DIR))
		{
			gtk_tree_store_append(model,&iter,parent);
			gtk_tree_store_set(model,&iter,0,"gtk-directory",1,list->d_name,2,fullname,-1);
			gtk_tree_store_append(model,&dummy,&iter);
		}
		g_free(fullname);
	}
	closedir(dir);
}

void xa_tree_view_row_selected(GtkTreeSelection *selection, gpointer data)
{
	GtkEntry *entry = data;
	gchar *dir;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (selection,&model,&iter))
	{
		gtk_tree_model_get (model,&iter,2,&dir,-1);
		if (dir != NULL)
		{
			gtk_entry_set_text(entry,dir);
			g_free(dir);
		}
	}
}

void xa_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data)
{
	if (gtk_tree_view_row_expanded(tree_view,path))
		gtk_tree_view_collapse_row(tree_view,path);
	else
		gtk_tree_view_expand_to_path(tree_view,path);
}

void xa_expand_dir(GtkTreeView *tree_view,GtkTreeIter *iter,GtkTreePath *path,gpointer data)
{
	GtkEntry *entry = data;
	GtkTreeModel *model;
	gchar *dir;
	GtkTreeIter child;
	gchar *fullname;

	model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get(model,iter,2,&dir,-1);
	gtk_tree_model_iter_nth_child(model,&child,iter,0);
	gtk_tree_model_get(model,&child,1,&fullname,-1);
	if (fullname == NULL)
	{
		xa_browse_dir(GTK_TREE_STORE(model),dir,iter);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&child);
	}
	else
		g_free(fullname);
	
	gtk_entry_set_text(entry,dir);
	g_free(dir);
}

void xa_treeview_exposed (GtkWidget *widget,GdkEventExpose *event,gpointer data)
{
	Extract_dialog_data *dialog = data;

	GtkTreePath *path = gtk_tree_path_new_from_string (dialog->string);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(widget),path,NULL,FALSE,0,0);
	gtk_tree_path_free (path);
	g_signal_handler_disconnect (G_OBJECT(widget),dialog->signal_id);
}

Multi_extract_data *xa_create_multi_extract_dialog()
{
	Multi_extract_data *dialog_data;
	GtkWidget	*dialog_vbox1,*vbox1,*scrolledwindow1,*hbox1,*frame1,*alignment1,*vbox2,*hbox3,*remove_button,*add_button,*browse,
				*cancelbutton1;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GSList *radiobutton1_group = NULL;
	GtkTooltips *multi_tooltip;
	char *column_names[]= {(_("Archive Name")),(_("Size")),(_("Path")),NULL};
	int x;

	multi_tooltip = gtk_tooltips_new ();
	dialog_data = g_new0 (Multi_extract_data,1);
	dialog_data->multi_extract = gtk_dialog_new();

	gtk_window_set_position (GTK_WINDOW (dialog_data->multi_extract),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->multi_extract), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog_data->multi_extract),TRUE);
	gtk_widget_set_size_request(dialog_data->multi_extract,-1,300);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog_data->multi_extract),FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog_data->multi_extract), _("Multi-Extract"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog_data->multi_extract),GTK_WINDOW (xa_main_window));

	dialog_vbox1 = GTK_DIALOG (dialog_data->multi_extract)->vbox;
	vbox1 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);
	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

	dialog_data->files_liststore = gtk_list_store_new (3,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING);
	dialog_data->files_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog_data->files_liststore));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog_data->files_treeview) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);	

	for (x = 0; x < 3; x++)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ( column_names[x],renderer,"text",x,NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(dialog_data->files_treeview),column);
	}
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(dialog_data->files_treeview), TRUE);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),dialog_data->files_treeview);

	gtk_drag_dest_set (dialog_data->files_treeview,GTK_DEST_DEFAULT_ALL, drop_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (dialog_data->files_treeview),"drag-data-received",G_CALLBACK (xa_multi_extract_dialog_drag_data_received),dialog_data);
	
	hbox2 = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, TRUE, 0);

	add_button = gtk_button_new_from_stock ("gtk-add");
	gtk_box_pack_end (GTK_BOX (hbox2), add_button, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_button), FALSE);
	g_signal_connect ( (gpointer) add_button, "clicked", G_CALLBACK (xa_multi_extract_dialog_select_files_to_add),dialog_data);

	remove_button = gtk_button_new_from_stock ("gtk-remove");
	gtk_widget_set_sensitive (remove_button,FALSE);
	gtk_box_pack_end (GTK_BOX (hbox2), remove_button, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (remove_button), FALSE);
	g_signal_connect ( (gpointer) remove_button,"clicked", G_CALLBACK (xa_remove_files_liststore),dialog_data);
	g_signal_connect (G_OBJECT (dialog_data->files_liststore),"row-inserted",G_CALLBACK (xa_activate_remove_button),remove_button);

	/* Destination dirs frame */
	hbox1 = gtk_hbox_new (TRUE, 8);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, TRUE, 0);
	frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), frame1, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

	vbox2 = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), vbox2);
	hbox3 = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox3, FALSE, FALSE, 0);
	dialog_data->extract_to = gtk_radio_button_new_with_mnemonic (NULL, _("Extract to:"));
	gtk_box_pack_start (GTK_BOX (hbox3), dialog_data->extract_to, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->extract_to), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->extract_to));
	dialog_data->entry1 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox3), dialog_data->entry1, TRUE, TRUE, 0);
	gtk_entry_set_width_chars (GTK_ENTRY (dialog_data->entry1), 5);

	browse = gtk_button_new ();
	image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image1);
	gtk_box_pack_end(GTK_BOX (hbox3),browse, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(browse),image1);
	g_signal_connect( (gpointer) browse, "clicked", G_CALLBACK (xa_select_where_to_extract),dialog_data);

	dialog_data->extract_to_archive_name = gtk_radio_button_new_with_mnemonic (NULL, _("Extract to dir \"Archive Name\""));
	gtk_tooltips_set_tip (multi_tooltip,dialog_data->extract_to_archive_name,_("This option extracts archives in directories named with the archive names"),NULL);
	gtk_box_pack_start (GTK_BOX (vbox2), dialog_data->extract_to_archive_name, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->extract_to_archive_name), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->extract_to_archive_name));
	label1 = gtk_label_new (_("Destination dirs:"));
	gtk_frame_set_label_widget (GTK_FRAME (frame1), label1);

	/* Option frame */
	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_OUT);
	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame2), alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 12, 0);
	vbox3 = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment2), vbox3);
	dialog_data->overwrite = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->overwrite, FALSE, FALSE, 0);
	dialog_data->full_path = gtk_check_button_new_with_mnemonic (_("Extract pathnames"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->full_path, FALSE, FALSE, 0);
	label2 = gtk_label_new (_("Options:"));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), label2);

	dialog_action_area1 = GTK_DIALOG (dialog_data->multi_extract)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->multi_extract), cancelbutton1, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);
	
	extract_button = gtk_button_new();
	extract_image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	extract_hbox = gtk_hbox_new(FALSE,4);
	extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	alignment3 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment3),extract_hbox);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_label,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(extract_button),alignment3);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->multi_extract),extract_button,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (extract_button, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->multi_extract),GTK_RESPONSE_OK);
	return dialog_data;
}

void xa_multi_extract_dialog_select_files_to_add (GtkButton* button, Multi_extract_data *dialog)
{
	GtkWidget *file_selector;
	GSList *dummy = NULL;
	gint response;

	file_selector = gtk_file_chooser_dialog_new (_("Please select the archives you want to extract"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(file_selector),TRUE);
	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dummy = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_selector));
		g_slist_foreach( dummy, (GFunc) xa_add_files_liststore,dialog);
	}
	if (dummy != NULL)
		g_slist_free (dummy);
	gtk_widget_destroy(file_selector);
	return;
}

void xa_add_files_liststore (gchar *file_path, Multi_extract_data *dialog)
{
	GtkTreeIter iter;
	gchar *file_utf8,*_file_utf8;
	gchar *path;
	struct stat my_stat;
	unsigned long long int file_size;

	if (xa_detect_archive_type(file_path) < 0)
		return;

	stat (file_path,&my_stat);
	file_size = my_stat.st_size;	
	file_utf8 = g_filename_display_name (file_path);
	path = xa_remove_level_from_path(file_utf8);
	_file_utf8 = xa_remove_path_from_archive_name(file_utf8);
	g_free (file_utf8);
	file_utf8 = _file_utf8;
	gtk_list_store_append(dialog->files_liststore,&iter);
	gtk_list_store_set (dialog->files_liststore,&iter,0,file_utf8,1,file_size,2,path,-1);
	dialog->nr++;
	g_free (file_utf8);
	g_free (path);
}

void xa_remove_files_liststore (GtkWidget *widget, Multi_extract_data *multi_extract_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *rr_list = NULL;
	GList *node;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(multi_extract_data->files_treeview));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(multi_extract_data->files_treeview) );
	gtk_tree_selection_selected_foreach(sel, (GtkTreeSelectionForeachFunc) remove_foreach_func, &rr_list);

	for (node = rr_list; node != NULL; node = node->next)
	{
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference *) node->data);
		if (path)
		{
			if ( gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path) )
				gtk_list_store_remove(multi_extract_data->files_liststore, &iter);
			gtk_tree_path_free(path);
		}
	}
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter) == FALSE)
		gtk_widget_set_sensitive ( widget, FALSE);
	g_list_foreach(rr_list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free(rr_list);
}

static void remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GList **rowref_list)
{
	GtkTreeRowReference *rowref;

	rowref = gtk_tree_row_reference_new(model, path);
	*rowref_list = g_list_append(*rowref_list, rowref);
}

void xa_activate_remove_button (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, GtkWidget *remove_button)
{
	if (gtk_tree_model_get_iter_first(tree_model, iter) == TRUE)
		gtk_widget_set_sensitive (remove_button, TRUE );
}

static void xa_multi_extract_dialog_drag_data_received (GtkWidget *widget,GdkDragContext *context, int x,int y,GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data)
{
	Multi_extract_data *dialog_data = user_data;
	gchar **array = NULL;
	gchar *filename;
	unsigned int len = 0;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	array = gtk_selection_data_get_uris ( data );
	if (array == NULL)
	{
		xa_show_message_dialog(GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"",_("Sorry, I could not perform the operation!") );
		gtk_drag_finish (context,FALSE,FALSE,time);
		return;
	}
	gtk_drag_finish (context, TRUE, FALSE, time);
	while (array[len])
	{
		filename = g_filename_from_uri ( array[len] , NULL, NULL );
		xa_add_files_liststore (filename,dialog_data);
		g_free (filename);
		len++;
	}
	g_strfreev (array);
}

void xa_select_where_to_extract(GtkButton *widget,Multi_extract_data *dialog_data)
{
	GtkWidget *file_selector;
	gchar *dest_dir;
	gint response;

	file_selector = gtk_file_chooser_dialog_new (_("Please select the destination directory"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);
	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_selector));
		gtk_entry_set_text(GTK_ENTRY(dialog_data->entry1),dest_dir);
		g_free(dest_dir);
	}
	gtk_widget_destroy(file_selector);
}

void xa_parse_multi_extract_archive(Multi_extract_data *dialog)
{
	GtkTreeIter iter;
	gchar *filename = NULL,*file,*path,*message = NULL,*name,*dest_path = NULL;
	GString *output = g_string_new("");
	Progress_bar_data *pb_struct;
	gboolean overwrite,full_path;
	gint response;
	double percent = 0.0;

	gtk_widget_show_all(dialog->multi_extract);
run:
	response = gtk_dialog_run(GTK_DIALOG(dialog->multi_extract));
	if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_hide(dialog->multi_extract);
		return;
	}
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dialog->files_liststore), &iter) == FALSE)
	{
		xa_show_message_dialog(GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't multi-extract archives:"),_("You haven't added any of them!") );
		goto run;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->extract_to)))
	{
		dest_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->entry1)));
		if (strlen(dest_path) == 0)
		{
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed where to extract the files!"),_("Please fill the \"Extract to\" field!") );
			goto run;
		}
	}
	gtk_widget_hide(dialog->multi_extract);

	overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->overwrite));
	full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->full_path));
	double fraction = 1.0 / dialog->nr;
	pb_struct = xa_create_progress_bar();
	do
	{
		gtk_tree_model_get (GTK_TREE_MODEL(dialog->files_liststore),&iter,0,&file,2,&path,-1);
		filename = g_strconcat (path,"/",file,NULL);
		xa_increase_progress_bar(pb_struct,filename,percent,TRUE);
		g_free(file);
		g_free(path);
		message = xa_multi_extract_archive(filename,overwrite,full_path,dest_path);
		if (message != NULL)
		{
			name = g_strconcat(filename,": ",message,"\n",NULL);
			g_string_append(output,name);
		}
		g_free(filename);
		percent += fraction;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL(dialog->files_liststore),&iter));
	gtk_widget_destroy(pb_struct->progress_window);
	g_free(pb_struct);

	if (strlen(output->str) > 0)
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Some errors occurred:"),output->str );

	g_string_free(output,TRUE);
	if (dest_path != NULL)
		g_free(dest_path);

	batch_mode = FALSE;
}

static gchar *xa_multi_extract_archive(gchar *filename,gboolean overwrite,gboolean full_path,gchar *dest_path)
{
	XArchive *archive = NULL;
	gchar *dirname = NULL;
	gchar *new_path = NULL;
	gchar *_filename = NULL;

	if (dest_path == NULL)
	{
		_filename = strstr(filename,".");
		if (_filename)
			_filename = g_strndup(filename,(_filename-filename));
		else
			_filename = filename;
		dest_path	= xa_remove_level_from_path(_filename);
		dirname		= xa_remove_path_from_archive_name(_filename);
		new_path	= g_strconcat(dest_path,"/",dirname,NULL);
		g_free(dirname);
		g_free(dest_path);
		if (g_mkdir(new_path,0700) < 0)
		{
			g_free(new_path);	
			return strerror(errno);
		}
		dest_path = new_path;
	}
	archive = xa_init_archive_structure(xa_detect_archive_type(filename));
	archive->overwrite = overwrite;
	archive->full_path = full_path;
	archive->escaped_path = xa_escape_bad_chars (filename,"$\'`\"\\!?* ()&|@#:;");
	archive->extraction_path = g_strdup(dest_path);
	while (gtk_events_pending())
		gtk_main_iteration();
	(*archive->extract) (archive,NULL);
	xa_clean_archive_structure(archive);
	return NULL;
}
