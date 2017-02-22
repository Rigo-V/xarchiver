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

#include <string.h>
#include "gzip.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"

static void xa_gzip_parse_output(gchar *, gpointer);

void xa_gzip_open (XArchive *archive)
{
	gchar *command;
	unsigned short int i;

	if (g_str_has_suffix (archive->escaped_path,".tar.gz") || g_str_has_suffix (archive->escaped_path,".tgz"))
	{
		archive->type = XARCHIVETYPE_TAR_GZ;
		archive->delete =	delete[archive->type];
		archive->add = 		add[archive->type];
		archive->extract = 	extract[archive->type];

		command = g_strconcat (tar, " tzvf " , archive->escaped_path, NULL );
		archive->files_size = 0;
		archive->nr_of_files = 0;
		archive->nc = 7;
		archive->parse_output = xa_tar_parse_output;
		xa_spawn_async_process (archive,command);
		g_free (command);

		if (archive->child_pid == 0)
			return;

		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 9; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time")),NULL};
		xa_create_liststore (archive,names);
	}
	else
	{
		archive->nc = 4;
		archive->parse_output = xa_gzip_parse_output;
		archive->nr_of_files = 1;

		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 6; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Compressed")),(_("Size")),(_("Ratio"))};
		xa_create_liststore (archive,names);

		command = g_strconcat ("gzip -l ",archive->escaped_path,NULL);
		xa_spawn_async_process (archive,command);
		g_free (command);

		if (archive->child_pid == 0)
			return;
	}
}

static void xa_gzip_parse_output (gchar *line, gpointer data)
{
	XArchive *archive = data;
	gchar *filename;
	gchar *basename;
	gpointer item[3];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);
	if (line[9] == 'c')
		return;

	/* Size */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[0] = line + a;
	n++;

	/* Compressed */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[1] = line + a;
	archive->files_size += g_ascii_strtoull(item[1],NULL,0);
	n++;

	/* Ratio */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[2] = line + a;
	n++;

	line[linesize-1] = '\0';
	filename = line+n;

	basename = g_path_get_basename(filename);
	if (basename == NULL)
		basename = g_strdup(filename);

	xa_set_archive_entries_for_each_row (archive,basename,item);
	g_free(basename);
}

gboolean gzip_extract(XArchive *archive,GSList *dummy)
{
	gchar *command = NULL,*filename = NULL,*dot,*filename_noext;
	GSList *list = NULL;

	filename = xa_remove_path_from_archive_name(archive->escaped_path);
	dot = strrchr(filename,'.');
	if (G_LIKELY(dot))
	{
		filename_noext = g_strndup(filename,(dot - filename));
		g_free(filename);
	}
	else
		filename_noext = filename;

	command = g_strconcat("sh -c \"gunzip -cd ",archive->escaped_path," > ",archive->extraction_path,"/",filename_noext,"\"",NULL);
	list = g_slist_append(list,command);
	return xa_run_command (archive,list);
}
