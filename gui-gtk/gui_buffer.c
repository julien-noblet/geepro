/* $Revision: 1.22 $ */
/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "../src/geepro.h"
#include "../src/files.h"
#include "../intl/lang.h"
#include "icons_xpm.h"
#include "bineditor.h"
#include "gui_buffer.h"

static void gui_buffer_load_error(s_gui_buffer_list *it, int err, const char *fname)
{
    GtkWidget *dlg;

    WRN("Error loading file '%s':%i", fname, err);
    dlg = gtk_message_dialog_new(
	it->wmain, GTK_DIALOG_DESTROY_WITH_PARENT, 
	GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
	"Error loading file '%s':%i", fname, err
    );
    gtk_dialog_run( GTK_DIALOG(dlg) );    
    gtk_widget_destroy( dlg );
}

static void gui_buffer_save_error(s_gui_buffer_list *it, int err, const char *fname)
{
    GtkWidget *dlg;
    WRN("Error saving file '%s':%i", fname, err);
    dlg = gtk_message_dialog_new(
	it->wmain, GTK_DIALOG_DESTROY_WITH_PARENT, 
	GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
	"Error saving file '%s':%i", fname, err
    );
    gtk_dialog_run( GTK_DIALOG(dlg) );    
    gtk_widget_destroy( dlg );
}

static void gui_shortcut_size(char *tmp_str, unsigned long size)
{
    unsigned int a, i;
    const char kx[5] = {'k','M','G','T','P'};
    char  b[3] = {'B',0,0};

    a = size;
    i = 0;
    while( (a >= 1024) && (i < 5)){
        b[0] = kx[i];
        b[1] = 'B';
        a = a / 1024;
        i++;
    }
    sprintf(tmp_str + strlen(tmp_str), "(%i %s)", a, b); 
}

static void gui_entry_update(GtkEntry *entry, long size, char *tmp_str)
{
    sprintf(tmp_str, "0x%x", (unsigned int)size); 
    gui_shortcut_size( tmp_str, size);
    gtk_entry_set_text(entry, tmp_str);  
}

static GtkWidget *gui_info_field(const char *title, GtkWidget **entry)
{
    GtkWidget *wg1, *wg2;
    wg1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    wg2 = gtk_label_new(title);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);
    wg2 = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(wg2), FALSE);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, TRUE, TRUE, 0);
    *entry = wg2;
    return wg1;
}

static void gui_add_file_filter(GtkWidget *wg, const char *name, const char *patterns)
{
    GtkFileFilter *filter;
    char tmp[16]; // maximal prefix size
    const char *pt, *i;

    if( !wg || !name || !patterns ) return;

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, name);
    for( pt = patterns; *pt; ){
	i = pt;
	for( ; *pt && (*pt != ':'); pt++ );
	if( (pt - i) > 15 ){
	    WRN("Prefix for file filter exceeds 15 characters - skipping.");
	    return;
	}
	memset(tmp, 0, 16);
	strncpy(tmp, i, pt - i);	
	gtk_file_filter_add_pattern(filter, tmp);
	if( *pt ) pt++;
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wg), filter);
}

static void gui_load_file(GtkWidget *w, s_gui_buffer_list *it)
{ 
    char *tmp;
    GtkWidget *wg;    
    int err;
    
    wg = gtk_file_chooser_dialog_new( 
	    "Open file", it->wmain, 
	    GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL
	);
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wg), FALSE);
    gui_add_file_filter(wg, "hex", "*.hex:*.HEX:*.Hex");
    gui_add_file_filter(wg, "srec", "*.srec:*.SREC:*.Srec:*.s19:*.S19");
    gui_add_file_filter(wg, "bin", "*.bin:*.BIN:*.Bin:*.rom:*.ROM:*.Rom");
    gui_add_file_filter(wg, "ALL", "*.*");
    if( (tmp = (char *)buffer_get_last_loaded_fname( it->buffer, it->parent->store )) )
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(wg), tmp);
    if(gtk_dialog_run(GTK_DIALOG(wg)) == GTK_RESPONSE_ACCEPT){
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wg));
	gtk_widget_destroy(wg);    
	if( (err = buffer_load(it->buffer, it->parent->store, tmp)))
	    gui_buffer_load_error( it, err, tmp );
	g_free( tmp );
    } else
	gtk_widget_destroy(wg);    
    gui_buffer_refresh( it->parent );
}

static void gui_load_file_at(GtkWidget *w, s_gui_buffer_list *it)
{
    char *tmp;
    GtkWidget *wg, *ca, *grid, *sb0, *sb1, *sb2, *wgi;    
    unsigned int bpos, fpos, count, flen;
    int err;

    wg = gtk_file_chooser_dialog_new( 
	    "Open file", it->wmain, 
	    GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL
	);
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wg), FALSE);
    gui_add_file_filter(wg, "ALL", "*.*");
    if( (tmp = (char *)buffer_get_last_loaded_at_fname( it->buffer,it->parent->store )) )
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(wg), tmp);
    if(gtk_dialog_run(GTK_DIALOG(wg)) == GTK_RESPONSE_ACCEPT){
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wg));
	wgi = gtk_dialog_new_with_buttons("Insert file to buffer", it->wmain, 
		GTK_DIALOG_DESTROY_WITH_PARENT,	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, 
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
	    );
        grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
	// labels
        gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new( DLG_INS_FILE_BUFFER_OFFSET ), 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new( DLG_INS_FILE_SIZE ), 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new( DLG_INS_FILE_OFFSET ), 0, 2, 1, 1);
	// spin buttons 
	sb0 = gtk_spin_button_new_with_range(0.0, (double)it->buffer->size - 1, 1.0);
        gtk_grid_attach(GTK_GRID(grid), sb0, 1, 0, 1, 1);
	flen = file_length( tmp );
	sb1 = gtk_spin_button_new_with_range(0.0, (double)flen, 1.0);
        gtk_grid_attach(GTK_GRID(grid), sb1, 1, 1, 1, 1);
	sb2 = gtk_spin_button_new_with_range(0.0, (double)flen, 1.0);
        gtk_grid_attach(GTK_GRID(grid), sb2, 1, 2, 1, 1);

        ca = gtk_dialog_get_content_area(GTK_DIALOG( wgi ));
        gtk_container_add(GTK_CONTAINER(ca), grid);
        gtk_widget_show_all( ca );
    
        if(gtk_dialog_run(GTK_DIALOG( wgi ) ) == GTK_RESPONSE_ACCEPT ){
	    bpos  = gtk_spin_button_get_value(GTK_SPIN_BUTTON( sb0 ));
	    count = gtk_spin_button_get_value(GTK_SPIN_BUTTON( sb1 ));
	    fpos  = gtk_spin_button_get_value(GTK_SPIN_BUTTON( sb2 ));	    
	    if((err = buffer_load_at(it->buffer, it->parent->store, tmp, bpos, fpos, count)))
	    							    gui_buffer_load_error( it, err, tmp );
	}
	gtk_widget_destroy( wgi );	
	g_free(tmp);	
    }
    gtk_widget_destroy(wg);    
    gui_buffer_refresh( it->parent );
}

static void gui_save_file(GtkWidget *w, s_gui_buffer_list *it)
{ 
    char *tmp;
    GtkWidget *wg;    
    int err;

    wg = gtk_file_chooser_dialog_new(
	    "Save file", it->wmain, 
	    GTK_FILE_CHOOSER_ACTION_SAVE,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
	    GTK_RESPONSE_ACCEPT, NULL
	);
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wg), FALSE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(wg), TRUE);
    gui_add_file_filter( wg, "hex", "*.hex:*.HEX:*.Hex");
    gui_add_file_filter( wg, "bin", "*.bin:*.BIN:*.Bin");
    gui_add_file_filter( wg, "srec", "*.srec:*.SREC:*.Srec:*s19:*.S19");
    gui_add_file_filter( wg, "ALL", "*.*");

    if( (tmp = (char *)buffer_get_last_saved_fname( it->buffer, it->parent->store )) )
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(wg), tmp);

    if(gtk_dialog_run(GTK_DIALOG(wg)) == GTK_RESPONSE_ACCEPT){
	char *tmp = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( wg ) );
	if((err = buffer_save(it->buffer, it->parent->store, tmp, 1)))
	    gui_buffer_save_error( it, err, tmp );	    	
	g_free(tmp);
    }
    gtk_widget_destroy(wg);    
}

/*******************************************************************************************************************************************************/

// Prevents of using pointer to destroy notebook pages when notebook itself was destructed, and pointer to notebook is no longer valid.
static void gui_buffer_destroy_notebook( GtkWidget *wg, s_gui_buffer *bf)
{
    bf->np = NULL;
}

char gui_buffer_init(s_gui_buffer **bf, store_str *st, GtkNotebook *nb)
{ 
    if( !bf || !nb) return 0;
    if( *bf ){
	ERR("bf != NULL");
	return -1;
    }
    MALLOC(*bf, s_gui_buffer , 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;	
    }
    memset(*bf, 0, sizeof( s_gui_buffer ));
    (*bf)->store = st;
    (*bf)->np = nb;
    g_signal_connect(G_OBJECT(nb), "destroy", G_CALLBACK(gui_buffer_destroy_notebook), *bf);
    return 0; 
}

void gui_buffer_exit(s_gui_buffer *bf)
{
    if( !bf ) return;
    gui_buffer_delete( bf );
    free( bf );
}

static void gui_buffer_set_file_entry( s_gui_buffer_list *it )
{
    const char *tmp;
    if((tmp = buffer_get_last_loaded_fname( it->buffer, it->parent->store ) )){
	gtk_entry_set_text(GTK_ENTRY(it->file_entry), tmp);
	gtk_editable_set_position(GTK_EDITABLE(it->file_entry), -1);
    }
}

static void gui_buffer_reload(GtkWidget *wg, s_gui_buffer_list *it)
{
    int err;
    if( (err = buffer_load(it->buffer, it->parent->store, NULL)))
        gui_buffer_load_error( it, err, "<reload>" );
    gui_buffer_refresh( it->parent );    
}

static void gui_device_buffer_info(s_gui_buffer_list *it, GtkWidget *wg)
{
    GtkWidget *wg1, *wg2;

    wg1 = gtk_frame_new( NULL );
    gtk_container_add( GTK_CONTAINER( wg ), wg1);
    wg2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add( GTK_CONTAINER( wg1 ), wg2);
    // Size of device
    gtk_container_add( GTK_CONTAINER( wg2 ), gui_info_field(SIZE_DEVICE_HEX_LB, &it->device_entry) );
    // Size of buffer
    gtk_container_add( GTK_CONTAINER( wg2 ), gui_info_field(SIZE_BUFFER_HEX_LB, &it->buffer_entry) );
    // CRC sum   
    gtk_container_add( GTK_CONTAINER( wg2 ), gui_info_field(CHECKSUM_LB, &it->crc_entry) );
    // file entry and reload button
    wg1 = gtk_frame_new( NULL );
    gtk_container_add( GTK_CONTAINER( wg ), wg1);
    wg2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add( GTK_CONTAINER( wg1 ), wg2);
    gtk_box_pack_start(GTK_BOX(wg2), gui_info_field(FILE_LB, &it->file_entry),  TRUE, TRUE, 0);
    gui_buffer_set_file_entry( it );
    wg1 = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(wg1), gtk_image_new_from_stock("gtk-refresh", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_tooltip_text(wg1, TEXT(GUI_RELOAD) );
    g_signal_connect(G_OBJECT(wg1), "pressed", G_CALLBACK(gui_buffer_reload), it);
    gtk_box_pack_start(GTK_BOX(wg2), wg1,  FALSE, FALSE, 0);

}

static void gui_bineditor_update(GuiBineditor *be, s_gui_buffer *bf)
{
    gui_buffer_refresh( bf );
}

static GtkWidget *gui_buffer_build_gui( s_gui_buffer *bf, s_gui_buffer_list *it, s_buffer_list *buff, GtkWindow *wmain)
{
    GtkWidget *wg0, *wg1;

    it->wmain = wmain;
    it->be = (GuiBineditor *)gui_bineditor_new( wmain );
    g_signal_connect(G_OBJECT(it->be), "changed", G_CALLBACK(gui_bineditor_update), bf);    
    gui_bineditor_set_icon( GUI_BINEDITOR(it->be), LOGO_ICON );
    gui_bineditor_tool_insert(GUI_BINEDITOR(it->be), GTK_TOOL_ITEM(gtk_separator_tool_item_new()), 0);
    gui_bineditor_set_buffer(GUI_BINEDITOR(it->be), buff->size, (unsigned char *)buff->data );

    wg1 = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_SAVE ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(wg1), TIP_BE_WRITE);    
    gui_bineditor_tool_insert(GUI_BINEDITOR(it->be),  GTK_TOOL_ITEM(wg1), 0);
    g_signal_connect(G_OBJECT(wg1), "clicked", G_CALLBACK(gui_save_file), it);

    wg1 = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_EDIT ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(wg1), TIP_BE_OPEN_AT);    
    gui_bineditor_tool_insert(GUI_BINEDITOR(it->be),  GTK_TOOL_ITEM(wg1), 0);
    g_signal_connect(G_OBJECT(wg1), "clicked", G_CALLBACK(gui_load_file_at), it);

    wg1 = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_OPEN ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(wg1), TIP_BE_OPEN);    
    gui_bineditor_tool_insert(GUI_BINEDITOR(it->be),  GTK_TOOL_ITEM(wg1), 0);
    g_signal_connect(G_OBJECT(wg1), "clicked", G_CALLBACK(gui_load_file), it);

    it->buffer = buff;
    it->parent = bf;
    
    wg0 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gui_device_buffer_info( it, wg0 );
    gtk_box_pack_start(GTK_BOX(wg0), GTK_WIDGET(it->be), TRUE, TRUE, 0);    

    return wg0;
}

GtkWidget *gui_buffer_new(s_gui_buffer *bf, s_buffer_list *buffer, GtkWindow *wmain)
{ 
    s_gui_buffer_list *tmp, *it;
    GtkWidget *wg;

    MALLOC(tmp, s_gui_buffer_list , 1){
	ERR_MALLOC_MSG;
	return NULL;	
    }

    memset(tmp, 0, sizeof( s_gui_buffer_list ));
    if(!(wg = gui_buffer_build_gui( bf, tmp, buffer, wmain ))){
	ERR("Build gui buffer failed.");
	free( tmp );
	return NULL;
    }
    if( !bf->list ){
	bf->list = tmp;
	return wg;
    }
    for(it = bf->list; it->next; it = it->next);
    it->next = tmp;
    
    return wg; 
}

void gui_buffer_delete( s_gui_buffer *bf)
{
    s_gui_buffer_list *it, *tmp; 

    if( !bf ) return;
    if( !bf->list ) return;
    
    for(it = bf->list; it; ){
	tmp = it->next;
	free( it );
	it = tmp;        
    }
    bf->list = NULL;

    // delete all pages from buffer and it content
    if( bf->np )
	while( gtk_notebook_get_n_pages( bf->np ) > 0) 
			gtk_notebook_remove_page( bf->np, -1);    
}

static void gui_checksum_rfsh(s_gui_buffer_list *bf)
{
    char tmp_str[40];
    
    sprintf(tmp_str, "0x%x", (unsigned int )buffer_checksum_count( bf->buffer, CHECKSUM_ALG_LRC )); 
    gtk_entry_set_text(GTK_ENTRY(bf->crc_entry), tmp_str);  
}

void gui_buffer_refresh(s_gui_buffer *bf)
{
    s_gui_buffer_list *it; 
    char tmp_str[40];

    if( !bf ) return;
    if( !bf->list ) return;
    
    for(it = bf->list; it; it = it->next){
	gui_entry_update(GTK_ENTRY(it->device_entry),it->buffer->size, tmp_str);
	gui_entry_update(GTK_ENTRY(it->buffer_entry),gui_bineditor_get_buffer_size( GUI_BINEDITOR(it->be) ), tmp_str);
	gui_checksum_rfsh( it );
	gui_buffer_set_file_entry( it );
    	gui_bineditor_redraw( it->be );
    }
/*
//    gui_checksum_recalculate( gep );
    gtk_widget_queue_draw( it->wmain );
    gui_bineditor_redraw( it->be );

*/
}

void gui_buffer_check_valid( s_gui_buffer *bf )
{
    GtkWidget *dlg;

    s_gui_buffer_list *it; 
    for(it = bf->list; it; it = it->next){
	if( buffer_file_check_valid( it->buffer ) ) continue;
	MSG("Buffer and file differs.");    
	dlg = gtk_message_dialog_new(
	    it->wmain, GTK_DIALOG_DESTROY_WITH_PARENT, 
	    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	    "File and buffer differs - reload ?"
	);
	if(gtk_dialog_run( GTK_DIALOG(dlg) ) == GTK_RESPONSE_YES){
	    gui_buffer_reload(NULL, it);
	}    
	gtk_widget_destroy( dlg );
    }
}

