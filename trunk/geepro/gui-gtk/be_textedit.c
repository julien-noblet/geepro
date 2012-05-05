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
#include "be_textedit.h"
#include "../intl/lang.h"

static void gui_bineditor_text_toolbar_setup(gui_bineditor_text_str *tx);

static void gui_bineditor_text_destroy(GtkWidget *wg, gui_bineditor_text_str *tx)
{
    gtk_widget_set_sensitive( tx->be->priv->texted, TRUE );
    free( tx );
    
}

void gui_bineditor_buff_texted(GuiBineditor *be, unsigned int start, unsigned int width, unsigned int height)
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
    tx->ed_width = width;
    tx->ed_height = height;


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
    fr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(wg), fr, TRUE, TRUE, 0);
    tx->draw_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(fr), tx->draw_area, TRUE, TRUE, 0);
    tx->vadj = gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    gtk_box_pack_end(GTK_BOX(fr), gtk_scrollbar_new( GTK_ORIENTATION_VERTICAL, tx->vadj ), FALSE, FALSE, 0);
    tx->hadj = gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    gtk_box_pack_end(GTK_BOX(wg), gtk_scrollbar_new( GTK_ORIENTATION_HORIZONTAL, tx->hadj ), FALSE, FALSE, 0);

//    g_signal_connect(G_OBJECT(s->vadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_vch), s);
//    g_signal_connect(G_OBJECT(s->hadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_hch), s);

    gui_bineditor_text_toolbar_setup( tx );
//    gui_bineditor_bmp_parameter_setup( s );
//    gui_bineditor_bmp_drawing_setup( s );
    gtk_widget_show_all( tx->wmain );
}

static void gui_bineditor_text_toolbar_setup(gui_bineditor_text_str *tx)
{

}

