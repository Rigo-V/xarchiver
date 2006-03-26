/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
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
 
#include "bzip2.h"
 
FILE *stream = NULL;
extern int output_fd,error_fd, child_pid;
gchar *tmp;
int fd;
gboolean type;

void OpenBzip2 ( gboolean mode , gchar *path )
{
    if ( g_str_has_suffix ( path , ".tar.bz2") || g_str_has_suffix ( path , ".tar.bz") || g_str_has_suffix ( path , ".tbz") || g_str_has_suffix ( path , ".tbz2" ) )
	{
	    gchar *command = g_strconcat ("tar tfjv " , path, NULL );
	    compressor_pid = SpawnAsyncProcess ( command , 1 , 0 );
		g_free ( command );
		if ( compressor_pid == 0 ) return;
        dummy_size = 0;
        number_of_files = 0;
        number_of_dirs = 0;
		char *names[]= {(_("Filename")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
		CreateListStore ( 6, names , (GType *)types );
		SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,Bzip2Output, (gpointer) mode );
		SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
        CurrentArchiveType = 4;
        WaitExitStatus ( compressor_pid , NULL );
    }
    else 
	{
        bz_gz = TRUE;
        Bzip2Extract ( 0 );
	}
}

void Bzip2Extract ( gboolean flag )
{
    gchar *text;
    gchar *new_path;
    extract_window = prefs (0);
	gtk_dialog_set_default_response (GTK_DIALOG (extract_window), GTK_RESPONSE_OK);
	done = FALSE;
	while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (extract_window ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;
			
			case GTK_RESPONSE_OK:
			extract_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (entry1) ));
			if ( strlen ( extract_path ) > 0 )
			{
				done = TRUE;
				gchar *archive = StripPathFromFilename ( removed_bs_path );
				gchar *command = g_strconcat ( flag ? "gzip -dc " : "bzip2 -dc " , escaped_path , NULL );
				compressor_pid = SpawnAsyncProcess ( command , 1 , 0);
				g_free ( command );
				if ( compressor_pid == 0 ) return;
				//This to remove the suffix from the archive name
                if (g_str_has_suffix ( archive , flag ? ".gz" : ".bz2") ) archive [strlen(archive) - ( flag ? 3 : 4 ) ] = '\0';
                if (archive == NULL) new_path = JoinPathArchiveName ( extract_path , removed_bs_path );
                    else new_path = JoinPathArchiveName ( extract_path , archive );
				stream = fopen ( new_path , "w" );
				g_free ( new_path );
				if ( stream == NULL )
				{
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno));
                    done = FALSE;
                    break;					
				}
                text = g_strconcat (_("Extracting ") , flag ? "gzip" : "bzip2" , _(" file to "), extract_path, NULL );
                Update_StatusBar ( text );
                g_free (text);
                action = extract;
				GIOChannel *ioc = SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,ExtractToDifferentLocation, stream );
				SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
				//The 2nd parameter is set to NULL to read binary data 
				g_io_channel_set_encoding (ioc, NULL , NULL);
			}
			else response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Please select where to extract files !") );
			break;
    	}
	}
	gtk_widget_destroy ( extract_window );
    if (done == TRUE) WaitExitStatus ( compressor_pid , NULL);
}

gboolean Bzip2Output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar **fields;
	gchar *filename;
	gchar *line;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
        if ( line == NULL ) return TRUE;
		if (data) gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
		fields = split_line (line,5);
		filename = get_last_field (line,6);
		gtk_list_store_append (liststore, &iter);
		if ( g_str_has_prefix(fields[0] , "d") == FALSE) number_of_files++;
            else number_of_dirs++;
   		for ( x = 0; x < 5; x++)
		{
           if ( x == 2 )
            {
                gtk_list_store_set (liststore, &iter,x+1,atoll(fields[x]),-1);
                dummy_size += atoll(fields[x]);
            }
            else gtk_list_store_set (liststore, &iter,x+1,fields[x],-1);
		}
        gtk_list_store_set (liststore, &iter,0,filename,-1);
        gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
        while (gtk_events_pending() )
		    gtk_main_iteration();
		g_strfreev ( fields );
		g_free (line);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
}

gchar *OpenTempFile ( gboolean dummy , gchar *temp_path )
{
    gchar *command = NULL;
    tmp = g_strdup ("/tmp/xarchiver-XXXXXX");
	fd = g_mkstemp ( tmp );
    stream = fdopen ( fd , "w" );
    if ( stream == NULL)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno) );
        g_free (tmp);
        return NULL;
    }
    if ( temp_path == NULL) command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , escaped_path , NULL );
        else command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , temp_path , NULL );
    //g_print ("1) %s > %s\n",command,tmp);
    compressor_pid = SpawnAsyncProcess ( command , 0 , 0 );
	g_free ( command );
	if ( compressor_pid == 0 )
    {
        fclose ( stream );
        unlink ( tmp );
        g_free (tmp);
        return NULL;
    }
    GIOChannel *ioc = SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,ExtractToDifferentLocation, stream );
    SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
    //The 2nd parameter is set to NULL to read binary data
    g_io_channel_set_encoding (ioc, NULL , NULL);
    return tmp;
}

gboolean ExtractToDifferentLocation (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar buffer[65536];
	gsize bytes_read;
	GError *error = NULL;
    
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		while (gtk_events_pending() )
			gtk_main_iteration();
		if ( g_io_channel_read_chars ( ioc, buffer, sizeof(buffer), &bytes_read, &error ) != G_IO_STATUS_NORMAL )
		{
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, error->message);
			g_error_free (error);
			return FALSE;
		}
		//Write the content of the bzip/gzip extracted file to the file pointed by the file stream in data
		fwrite ( buffer, 1 , bytes_read , data );
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		fclose ( data );
        stream = NULL;
        //g_message ("Stream closed");
		g_io_channel_shutdown ( ioc,TRUE,NULL );
        g_io_channel_unref (ioc);
		return FALSE;
	}
}

void DecompressBzipGzip ( GString *list , gchar *path , gboolean dummy , gboolean add )
{
    type = dummy;
    gchar *command, *msg;
    int status;
    int waiting = TRUE;
    int ps;

    tmp = OpenTempFile ( dummy , NULL );
    if ( tmp == NULL ) return;
    msg = g_strconcat ( _("Decompressing tar file with ") , dummy ? "gzip" : "bzip2" , ", please wait..." , NULL );
    Update_StatusBar ( msg );
    g_free (msg);
    gtk_widget_show (viewport2);
    while (waiting)
    {
        ps = waitpid ( (pid_t)child_pid, &status, WNOHANG);
        if (ps < 0) waiting = FALSE;
        else
        {
            gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
            while (gtk_events_pending())
                gtk_main_iteration();
        }
    }
    if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			archive_error = TRUE;
			SetButtonState (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW
			(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while decompressing the archive.\nDo you want to view the shell output ?") );
			if (response == GTK_RESPONSE_YES) ShowShellOutput (NULL,FALSE);
            unlink ( tmp );
            g_free (tmp);
            return;
		}
    }
    if ( add ) command = g_strconcat ( "tar rvvf " , tmp , list->str , NULL );
        else command = g_strconcat ( "tar --delete -f " , tmp , list->str , NULL );
    waiting = TRUE;
    //g_print ("2) %s\n",command);
    compressor_pid = SpawnAsyncProcess ( command , 0 , 0);
    SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenOutput, NULL );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenError, NULL );
	g_free ( command );
    if ( compressor_pid == 0 )
    {
        unlink ( tmp );
        g_free (tmp);
        return;
    }
    while (waiting)
    {
        ps = waitpid ( (pid_t)child_pid, &status, WNOHANG);
        if (ps < 0) waiting = FALSE;
        else
        {
            gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
            while (gtk_events_pending())
                   gtk_main_iteration();
        }
    }
    if ( WIFEXITED(status) )
    {
	 	if ( WEXITSTATUS (status) )
	    {
		    archive_error = TRUE;
		    SetButtonState (1,1,0,0,0);
		    gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW
		    (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,
            add ? _("An error occurred while adding to the tar archive.\nDo you want to view the shell output ?") : _("An error occurred while deleting from the tar archive.\nDo you want to view the shell output ?") );
		    if (response == GTK_RESPONSE_YES) ShowShellOutput (NULL,FALSE);
            unlink ( tmp );
            g_free (tmp);
            return;
        }
    }
    msg = g_strconcat ( _("Recompressing tar file with ") , dummy ? "gzip" : "bzip2" , ", please wait..." , NULL );
    Update_StatusBar ( msg );
    g_free (msg);
    RecompressArchive ( status );
}

void RecompressArchive (gint status)
{
    if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			archive_error = TRUE;
			SetButtonState (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW
			(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while writing the tar archive.\nDo you want to view the shell output ?") );
			if (response == GTK_RESPONSE_YES) ShowShellOutput (NULL,FALSE);
			unlink ( tmp );
            g_free (tmp);
            return;
		}
	}
    //Recompress the temp archive in the original archive overwriting it
    stream = fopen ( removed_bs_path , "w" ) ;
    if ( stream == NULL)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno) );
        unlink ( tmp );
        g_free (tmp);
        return;
    }
    gchar *command = g_strconcat ( type ? "gzip -c " : "bzip2 -c " , tmp , NULL );
    //g_print ("3) %s > %s\n",command,removed_bs_path);
    compressor_pid = SpawnAsyncProcess ( command , 0 , 0 );
    g_free ( command );
	if ( compressor_pid == 0 )
    {
        unlink ( tmp );
        g_free (tmp);
        return;
    }
    GIOChannel *ioc = SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,ExtractToDifferentLocation, stream );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
	//The 2nd parameter is set to NULL to read binary data 
	g_io_channel_set_encoding (ioc, NULL , NULL);
    //This to reload the content of the archive to show the changes (deletion / adding)
    WaitExitStatus (compressor_pid , tmp );
}

void Bzip2Add ( gchar *filename , gboolean flag )
{
    stream = fopen ( removed_bs_path , "w" );
	if ( stream == NULL )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno));
        done = FALSE;
        return;					
	}
    gtk_widget_show ( viewport2 );
    gchar *command = g_strconcat ( flag ? "gzip -c " : "bzip2 -c " , filename , NULL );
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0);
	g_free ( command );
	if ( compressor_pid == 0 ) return;
	GIOChannel *ioc = SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , ExtractToDifferentLocation , stream );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
	//The 2nd parameter is set to NULL to read binary data 
	g_io_channel_set_encoding (ioc, NULL , NULL);
    WaitExitStatus (compressor_pid , NULL );
}

