/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2011 Krzysztof Komarnicki
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "be_textedit.h"
#include "../intl/lang.h"

#define GET_COLOR( idx )	tx->be->priv->colors[idx*3 + 0],tx->be->priv->colors[idx*3 + 1],tx->be->priv->colors[idx*3 + 2]

static void gui_bineditor_text_ev(GtkTextBuffer *wg, gui_bineditor_text_str *tx);
static void gui_bineditor_text_insert_ev(GtkTextBuffer *wg, GtkTextIter *it, gchar *text, gint len, gui_bineditor_text_str *tx);

static void gui_bineditor_text_destroy(GtkWidget *wg, gui_bineditor_text_str *tx)
{
    gtk_widget_set_sensitive( tx->be->priv->texted, TRUE );
    tx->be->priv->texted_str = NULL;
    free( tx );
}

char gui_bineditor_text_rfsh( GuiBineditor *be )
{
    GtkWidget *dlg;
    const gchar *val,*val_;
    char r;
    int i, len;
    gui_bineditor_text_str *tx = (gui_bineditor_text_str *)be->priv->texted_str; 

    if( !tx ) return 0;


    return 0;
}

void gui_bineditor_text_editor(GuiBineditor *be, unsigned int start, unsigned int len)
{
    gui_bineditor_text_str *tx;
    GtkWidget *fr, *wg;

    tx = (gui_bineditor_text_str *)malloc(sizeof(gui_bineditor_text_str));
    if( !tx ){
	printf("ERR: gui_bineditor_bitmap() -> memory allocation problem.\n");
	return;    
    }
    tx->be = be;
    tx->base_addr = start;
    tx->ed_len = len;
    be->priv->texted_str = (void *)tx; 
    gtk_widget_set_sensitive( be->priv->texted, FALSE );
    tx->wmain = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title(GTK_WINDOW(tx->wmain),TEXT(WINTITLE_TEXTED));
    g_signal_connect(G_OBJECT(tx->wmain), "destroy", G_CALLBACK(gui_bineditor_text_destroy), tx);
    if( be->priv->icon )
	gtk_window_set_icon( GTK_WINDOW( tx->wmain ), gdk_pixbuf_new_from_xpm_data( be->priv->icon ));
    gtk_window_set_keep_above(GTK_WINDOW(tx->wmain), TRUE);
    gtk_widget_set_size_request( tx->wmain, 320, 320); 
    tx->ctx = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); 
    gtk_container_add(GTK_CONTAINER(tx->wmain), tx->ctx);
    tx->tb_line = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3); 
    gtk_box_pack_start(GTK_BOX(tx->ctx), tx->tb_line, FALSE, FALSE, 0 );

    fr = gtk_frame_new( NULL );    
    gtk_box_pack_start(GTK_BOX(tx->ctx), fr, TRUE, TRUE, 0);
    wg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fr), wg);



//    g_signal_connect(G_OBJECT(tx->ed_buffer), "changed", G_CALLBACK(gui_bineditor_text_ev), tx);
//    g_signal_connect(G_OBJECT(tx->ed_buffer), "insert-text", G_CALLBACK(gui_bineditor_text_insert_ev), tx);
    gtk_widget_show_all( tx->wmain );
}

