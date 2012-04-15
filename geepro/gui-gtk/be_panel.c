/* $Revision: 1.8 $ */
/* binary editor
 * Copyright (C) 2007 Krzysztof Komarnicki
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
#include <gdk/gdkkeysyms.h>
#include "bineditor.h"
#include "../src/script.h"
#include "../intl/lang.h"
#include "be_buffer.h"
#include "be_panel.h"
#include "be_bitmap.h"
#include "be_asmview.h"
#include "be_textedit.h"
#include "be_stencil.h"
#include "../src/checksum.h"

typedef void (*gui_bineditor_tmpl_cb)(GuiBineditor *be, GtkWidget *ctx,  void *str);
#define GUI_BE_CB(x) ((gui_bineditor_tmpl_cb)(x))

typedef struct {
    GtkWidget *rad0, *rad1;
    GtkWidget *from, *to;
    GtkWidget *pattern;    
    GuiBineditor *be;
} gui_clear_str;

typedef struct
{
    GtkWidget *find;
    GtkWidget *repl;
    GtkWidget *replace;
    GtkWidget *r0, *r1,*r2, *r3;
    GtkWidget *ci, *c0,*c1, *c2;
} gui_find_str;

typedef struct {
    GtkWidget *addr;
    GtkWidget *count;
    GtkWidget *arg;
    GtkWidget *sub,*add,*div,*mul,*or,*and,*xor;
    GtkWidget *shl,*sal,*shr,*sar,*rol,*ror;
    GtkWidget *bx0, *bx1, *bx2, *bx3, *bx4, *bx5, *bx6, *bx7;
    GtkWidget *bx;
} gui_be_bm_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *addr;
    GtkWidget *count;
    GtkWidget *all;
    GtkWidget *split;
    GtkWidget *merge;
    GtkWidget *xchg;
    GtkWidget *reorg;
    GtkWidget *bits;
    GtkWidget *vb;
    GtkWidget *a[32];  // address bits
} gui_be_org_str;

typedef struct {
    GtkWidget *mask[8];
    GuiBineditor *be;
    GtkWidget *width;
    GtkWidget *height;
    GtkWidget *rev;    
} gui_be_bmp_str;

typedef struct {
    GuiBineditor *be;    
    GtkWidget *start;
    GtkWidget *stop;
    GtkWidget *count;
} gui_be_cut_str;

typedef struct {
    GtkWidget *start;    
} gui_be_copy_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *count;    
    GtkWidget *proc;
    GtkWidget *fsel;
} gui_be_asm_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *start;    
    GtkWidget *width;
    GtkWidget *height;
} gui_be_text_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *size;
} gui_be_aux_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *algo;
    GtkWidget *start;
    GtkWidget *stop;
    GtkWidget *result;
} gui_be_sum_str;

typedef struct {
    GuiBineditor *be;
    GtkWidget *size;
} gui_be_resize_str;

typedef struct {
    GtkWidget *foffs;
    GtkWidget *start;
    GtkWidget *count;
    char *fname;
    FILE *fh;
    long fsize;
} gui_be_open_str;

typedef struct {
    GtkWidget *start;
    GtkWidget *count;
    char *fname;
} gui_be_save_str;

/****************************************************************************************************************/

static void gui_bineditor_clear_exec( GuiBineditor *be, GtkWidget *ctx, gui_clear_str *str)
{
    unsigned int from, to;
    const char *pattern;

    from = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->from));
    to   = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->to));
    pattern = gtk_entry_get_text(GTK_ENTRY(str->pattern));

    gui_bineditor_buff_clr(be->priv->buff, from, to, pattern);
}

static void gui_bineditor_find_exec( GuiBineditor *be, GtkWidget *ctx, gui_find_str *str  )
{
    GtkWidget *dlg, *hb;
    unsigned int from, to, flen, rlen, resp;
    char repl, mode, ci, ret;
    const char *find, *replace;
    unsigned char *fstr, *rstr;
    char skip = 0;
    int error;

    find = gtk_entry_get_text(GTK_ENTRY(str->find));    
    replace = gtk_entry_get_text(GTK_ENTRY(str->replace));    
    repl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->repl));

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->r0))) mode = BE_MODE_STRING;
//    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->r1))) mode = BE_MODE_REGEX;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->r2))) mode = BE_MODE_HEX;
//    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->r3))) mode = BE_MODE_BINARY;

    from = 0; to = 0;
    gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &from, &to);

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->c0))){
	from = 0;
	to = be->priv->buff->size - 1;
    }
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->c1))){
	from = be->priv->address_mark;
	to = be->priv->buff->size - from - 1;
    }
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->c2))){
	from = be->priv->clpb_start;
	to = be->priv->clpb_end;
    }

    ci = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->ci));

    fstr = gui_bineditor_buff_pattern2data(find, &flen, &error);
    if( error ){
	dlg = gtk_message_dialog_new(GTK_WINDOW(be->priv->wmain), 
	    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_CLOSE,
	    NULL
	);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), TXT_BE_FIND_ERR_FIND);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy( dlg );
	return;
    }
    rstr = gui_bineditor_buff_pattern2data(replace, &rlen, &error);
    if( error ){
	dlg = gtk_message_dialog_new(GTK_WINDOW(be->priv->wmain), 
	    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_CLOSE,
	    NULL
	);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), TXT_BE_FIND_ERR_REPL);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy( dlg );
	return;
    }
    if(( repl ) && ( flen != rlen )){
	dlg = gtk_message_dialog_new(GTK_WINDOW(be->priv->wmain), 
	    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_CLOSE,
	    NULL
	);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), TXT_BE_FIND_ERR_NEQ);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy( dlg );
	return;
    }

    for(;; from++){
	if((ret = gui_bineditor_buff_find(be->priv->buff, (const char *)fstr, flen, &from, to, ci)) == 1){
	    if(!skip){
		gui_bineditor_marker_set_item(be, GUI_BINEDITOR_MARKER_FOUND, GUI_BINEDITOR_MARKER_HEX | GUI_BINEDITOR_MARKER_ASCII, from, from + flen - 1);
		gui_bineditor_show_grid( be, from, from + flen - 1 );
		dlg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->priv->wmain), 0, NULL);
		if( repl ){
		    gtk_dialog_add_button(GTK_DIALOG(dlg), TXT_BE_FIND_ALL_BT, 1);
		    gtk_dialog_add_button(GTK_DIALOG(dlg), TXT_BE_FIND_REPL_BT, 2);
		}
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CANCEL, 3);
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_MEDIA_NEXT, 4);
		ctx = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
		hb = gtk_hbox_new(FALSE,20);
		gtk_container_add(GTK_CONTAINER(ctx), hb);
		gtk_box_pack_start(GTK_BOX(hb), gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG), FALSE, FALSE, 0);
        	gtk_box_pack_start(GTK_BOX(hb), gtk_label_new(TXT_BE_FIND_MATCH), FALSE, FALSE, 0);
		gtk_widget_show_all( ctx );            
		resp = gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy( dlg );
		if(resp == 1) skip = 1;
	        if(resp == 3) break;
	        if(resp == 4) continue;
	    } else {
		resp = 2;
	    }
	    if(resp == 2){
		gui_bineditor_buff_history_add(be->priv->buff, from, rlen, (unsigned char *)rstr);
		memcpy(be->priv->buff->data + from, rstr, rlen);
	    }
	} else {
	    dlg = gtk_message_dialog_new(GTK_WINDOW(be->priv->wmain), 
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
		GTK_MESSAGE_INFO,
		GTK_BUTTONS_CLOSE,
		NULL
	    );
	    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), TXT_BE_FIND_NO_MATCH);
	    gtk_dialog_run(GTK_DIALOG(dlg));
	    gtk_widget_destroy( dlg );
    	    return;
	}
    }

    gui_bineditor_marker_set_item(be, GUI_BINEDITOR_MARKER_FOUND, GUI_BINEDITOR_MARKER_HEX | GUI_BINEDITOR_MARKER_ASCII, from, from + flen - 1);
    gui_bineditor_show_grid( be, from, from + flen - 1 );
    
    if(fstr) free( fstr );
    if(rstr) free( rstr );
}

static void gui_bineditor_manipulator_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_bm_str *str )
{
    unsigned int start, count;
    int arg, i;
    char func;
    char rel[8];
    
    for( i = 0; i < 8; i++) rel[i] = i;

    start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->addr));        
    count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count));        
    arg = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->arg));        

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->sub))) func = 0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->add))) func = 1;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->mul))) func = 2;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->div))) func = 3;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->or ))) func = 4;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->and))) func = 5;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->xor))) func = 6;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->shl))) func = 7;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->sal))) func = 8;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->shr))) func = 9;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->sar))) func = 10;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->rol))) func = 11;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->ror))) func = 12;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->bx ))){
	func = 13;
	rel[0] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx0));
	rel[1] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx1));
	rel[2] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx2));
	rel[3] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx3));
	rel[4] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx4));
	rel[5] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx5));
	rel[6] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx6));
	rel[7] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->bx7));
    }
    
    gui_bineditor_buff_bman(be->priv->buff, start, count, arg, func, rel);    
}

static void gui_bineditor_organizer_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_org_str *str )
{
    unsigned int addr, count, i;
    char op = 0;
    char reorg[32];
    
    addr  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->addr));        
    count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count));        

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->split))) op = 0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->merge))) op = 1;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->xchg ))) op = 2;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->reorg))) op = 3;
    
    for( i =0; i < 32; i++){
	 reorg[i] = -1;
	 if( str->a[i] ) reorg[i] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->a[i]));
    }
    gui_bineditor_buff_reorg(be->priv->buff, addr, count, op, reorg);	    
}

static void gui_be_cut_get_values(gui_be_cut_str *str, int *start, int *count, int *stop)
{
    *start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start));
    *count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count));
    *stop  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->stop));
}

static void gui_bineditor_cut_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_cut_str *str )
{
    int start, stop, count;
    
    gui_be_cut_get_values( str, &start, &count, &stop );
    gui_bineditor_buff_cut( be->priv->buff, start, count, stop );
}

static void gui_bineditor_copy_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_copy_str *str  )
{
    gui_bineditor_buff_copy( be->priv->buff, gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start)));
}

static void gui_bineditor_bined_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_bmp_str *str )
{
    unsigned int width, height, i;
    unsigned char mask, br;
  
    width  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->width));
    height = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->height)); 
    br = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->rev));

    mask = 0;
    for(i = 0; i < 8; i++) 
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(str->mask[i]))) mask |= 1 << i;
        
    gui_bineditor_bitmap( be, width, height, mask, br);
}

static void gui_bineditor_asm_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_asm_str *str  )
{
    const char *text;
    
    if(be->priv->core_name != NULL) 
		g_free(be->priv->core_name);

    text = gtk_entry_get_text(GTK_ENTRY(str->proc));
    be->priv->core_name = (char *)malloc( strlen(text) + 1 );
    if(be->priv->core_name == NULL){
	printf("memory error: gui_bineditor_asm_exec()\n");
	return;
    }
    strcpy(be->priv->core_name, text);
    be->priv->core_count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count));
    

    gui_bineditor_asm_view(be, be->priv->core_count, be->priv->core_name);
}

static void gui_bineditor_text_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_text_str *str  )
{
    gui_bineditor_buff_texted(be, 
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start)),
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->width)),
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->height))
    );
}

static void gui_bineditor_aux_destr( GtkWidget *wg, GuiBineditor *p )
{
    if(p->priv->aux_buffer) free(p->priv->aux_buffer);
    p->priv->aux_buffer = NULL;
    p->priv->aux_size = 0;
    gtk_widget_set_sensitive(p->priv->aux, TRUE);
}

static void gui_bineditor_aux_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_aux_str *str )
{
    unsigned int size;
    GtkWidget *aux;
    
    size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->size));
    be->priv->aux_buffer = (char *)malloc(size);
    if(be->priv->aux_buffer == NULL){
	printf("memory error: gui_bineditor_aux_exec()\n");
	return;
    }
    be->priv->aux_size = size;
    memset(be->priv->aux_buffer, 0, size);

    be->priv->aux_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(be->priv->aux_win), 640,480);
    gtk_window_set_title(GTK_WINDOW(be->priv->aux_win), TXT_BE_WINTIT_AUX);

    aux = gui_bineditor_new(GTK_WINDOW(be->priv->aux_win));

    gui_bineditor_set_buffer(GUI_BINEDITOR(aux), size, (unsigned char *)be->priv->aux_buffer);
    gtk_container_add(GTK_CONTAINER(be->priv->aux_win), GTK_WIDGET(aux));
    gtk_widget_show_all(be->priv->aux_win);
    gtk_widget_hide(GUI_BINEDITOR(aux)->priv->aux);
    gtk_widget_hide(GUI_BINEDITOR(aux)->priv->stenc);
    gtk_widget_set_sensitive(be->priv->aux, FALSE);
    g_signal_connect(G_OBJECT(be->priv->aux_win), "destroy", G_CALLBACK(gui_bineditor_aux_destr), be );

}

static void gui_bineditor_sum_exec( GtkWidget *wg, gui_be_sum_str *str )
{
    unsigned int start, stop;
    ChecksumAlgo algo;
    int result = 0;
    const char *id;
    char res[16];


    start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start));
    stop  = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->stop));
    id    = gtk_combo_box_get_active_id(GTK_COMBO_BOX(str->algo));
    
    switch(*id){
	case 'A': algo = CHECKSUM_ALG_LRC; break;
	case 'B': algo = CHECKSUM_ALG_CRC16; break;
	case 'C': algo = CHECKSUM_ALG_CRC32; break;
	default : algo = CHECKSUM_ALG_LRC;
    }

    result = checksum_calculate(algo, str->be->priv->buff->size, str->be->priv->buff->data, start, stop, 0,0, 0,0 );

    if( algo == CHECKSUM_ALG_CRC32) sprintf(res, "0x%X%X%X%X%X%X%X%X",
	(result >> 28 ) & 0xf,
	(result >> 24 ) & 0xf,
	(result >> 20 ) & 0xf,
	(result >> 16 ) & 0xf,
	(result >> 12 ) & 0xf,
	(result >> 8  ) & 0xf,
	(result >> 4  ) & 0xf,
	(result       ) & 0xf
     );

    if( algo == CHECKSUM_ALG_CRC16) sprintf(res, "0x%X%X%X%X",
	(result >> 12 ) & 0xf,
	(result >> 8  ) & 0xf,
	(result >> 4  ) & 0xf,
	(result       ) & 0xf
     );

    if( algo == CHECKSUM_ALG_LRC) sprintf(res, "0x%X%X",
	(result >> 4  ) & 0xf,
	(result       ) & 0xf
     );

    gtk_entry_set_text(GTK_ENTRY(str->result), res);
}

static void gui_bineditor_resize_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_resize_str *str )
{
    unsigned int new_size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->size));

    if(new_size <= be->priv->buff->size) return;
    
    be->priv->buff->size = new_size;
    be->priv->buff->data = realloc(be->priv->buff->data, new_size);

    if(be->priv->buff->data == NULL){
	printf("memory error in gui_bineditor_resize_exec()\n");
	exit(-1);
    }
    gui_bineditor_set_buffer(be, new_size, be->priv->buff->data);
}

static void gui_bineditor_open_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_open_str *str )
{
    gui_bineditor_buff_file_insert(be->priv->buff, str->fh, 
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->foffs)),
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start)),
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count))    
    );
}

static void gui_bineditor_save_exec( GuiBineditor *be, GtkWidget *ctx, gui_be_save_str *str )
{
    gui_bineditor_buff_file_save(be->priv->buff, 
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->start)),
	gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count))    
    );
}

/****************************************************************************************************************************************/

static void gui_bineditor_build_open( GuiBineditor *be, GtkWidget *ctx, gui_be_open_str *str)
{
    GtkWidget *wg;

    str->fh = fopen(str->fname, "r");
    if(str->fh == NULL){
	printf("error open file gui_bineditor_build_open()\n");
	return;
    }
    
    fseek(str->fh, 0L, SEEK_END);
    str->fsize = ftell(str->fh);
    fseek(str->fh, 0L, SEEK_SET);    

    str->foffs = gtk_spin_button_new_with_range( 0, str->fsize - 1, 1);
    str->start = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    str->count = gtk_spin_button_new_with_range( 1, be->priv->buff->size, 1);

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new( TXT_BE_OPEN_FOFFS));
    gtk_container_add(GTK_CONTAINER(wg), str->foffs);
    gtk_container_add(GTK_CONTAINER(ctx), wg);

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new( TXT_BE_OPEN_START));
    gtk_container_add(GTK_CONTAINER(wg), str->start);
    gtk_container_add(GTK_CONTAINER(ctx), wg);

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new( TXT_BE_OPEN_COUNT));
    gtk_container_add(GTK_CONTAINER(wg), str->count);
    gtk_container_add(GTK_CONTAINER(ctx), wg);
// add guard    
}

static void gui_bineditor_build_save( GuiBineditor *be, GtkWidget *ctx, gui_be_save_str *str)
{
    GtkWidget *wg;

    str->start = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    str->count = gtk_spin_button_new_with_range( 1, be->priv->buff->size, 1);

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new( TXT_BE_OPEN_START));
    gtk_container_add(GTK_CONTAINER(wg), str->start);
    gtk_container_add(GTK_CONTAINER(ctx), wg);

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new( TXT_BE_OPEN_COUNT));
    gtk_container_add(GTK_CONTAINER(wg), str->count);
    gtk_container_add(GTK_CONTAINER(ctx), wg);
// add guard    
}

static void gui_bineditor_build_resize( GuiBineditor *be, GtkWidget *ctx, gui_be_resize_str *str)
{
    GtkWidget *wg;
    
    str->be = be;
    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new(TXT_BE_RESIZE_RES));
    str->size = gtk_spin_button_new_with_range( be->priv->buff->size, be->priv->buff->size * 16, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->size), be->priv->buff->size);
    gtk_container_add(GTK_CONTAINER(wg), str->size);    
}

static void gui_bineditor_build_sum( GuiBineditor *be, GtkWidget *ctx, gui_be_sum_str *str)
{
    GtkWidget *wg, *bt;
    unsigned int from = 0, to = 0;

    if(be->priv->buff->size < 2) return;

    if(!gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &from, &to)){
	from = 0;
	to = be->priv->buff->size - 1;
    }

    str->be = be;
    str->algo = gtk_combo_box_text_new();
    gtk_container_add(GTK_CONTAINER(ctx), str->algo);    
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(str->algo), (const char *)"A", TXT_BE_SUM_ALGO_LRC);
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(str->algo), (const char *)"B", TXT_BE_SUM_ALGO_CRC16);
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(str->algo), (const char *)"C", TXT_BE_SUM_ALGO_CRC32);
    gtk_combo_box_set_active(GTK_COMBO_BOX(str->algo), 0);

    str->start = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 2, 1);
    str->stop  = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->start), from);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), to);
    gtk_container_add(GTK_CONTAINER(ctx), str->start);    
    gtk_container_add(GTK_CONTAINER(ctx), str->stop);    

    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    str->result = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(wg), str->result);    
    bt = gtk_button_new_with_label(TXT_BE_SUM_RUN);
    gtk_container_add(GTK_CONTAINER(wg), bt);    
    gtk_editable_set_editable(GTK_EDITABLE(str->result), FALSE);
    g_signal_connect(G_OBJECT(bt), "pressed", G_CALLBACK(gui_bineditor_sum_exec), str );
}

static void gui_bineditor_build_aux( GuiBineditor *be, GtkWidget *ctx, gui_be_aux_str *str)
{
    GtkWidget *wg;
    unsigned int r;    
    
    str->be = be;
    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new(TXT_BE_AUX_SIZE));
    r = be->priv->buff->size;
    if(r < 16) r = 32;
    str->size = gtk_spin_button_new_with_range( 16, r, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->size), r);
    gtk_container_add(GTK_CONTAINER(wg), str->size);    
}

static void gui_bineditor_build_bined( GuiBineditor *be, GtkWidget *ctx, gui_be_bmp_str *str)
{
    GtkWidget *wg, *lb, *ww, *frm;
    int i;

    str->width  = gtk_spin_button_new_with_range( 0, 256, 1);
    str->height = gtk_spin_button_new_with_range( 0, 256, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->width), 8);    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->height), 8);    

    wg = gtk_table_new(2, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(ctx), wg);
    lb = gtk_label_new(TXT_BE_BMP_WIDTH);
    gtk_misc_set_alignment(GTK_MISC(lb), 0.0f, 0.5f);
    gtk_table_attach_defaults(GTK_TABLE(wg), lb, 0,1, 0,1);
    lb = gtk_label_new(TXT_BE_BMP_HEIGHT);
    gtk_misc_set_alignment(GTK_MISC(lb), 0.0f, 0.5f);
    gtk_table_attach_defaults(GTK_TABLE(wg), lb, 0,1, 1,2);    
    gtk_table_attach_defaults(GTK_TABLE(wg), str->width,  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->height, 1,2, 1,2);


    frm = gtk_frame_new(TXT_BE_BMP_BS);
    gtk_container_add(GTK_CONTAINER(ctx), frm);
    ww = gtk_table_new(2, 8, TRUE);
    gtk_container_add(GTK_CONTAINER(frm), ww);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("7"), 0,1, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("6"), 1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("5"), 2,3, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("4"), 3,4, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("3"), 4,5, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("2"), 5,6, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("1"), 6,7, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(ww), gtk_label_new("0"), 7,8, 0,1);

    for(i = 8; i; i--){
            str->mask[i - 1] = gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(str->mask[i - 1]), TRUE);
            gtk_table_attach_defaults(GTK_TABLE(ww), str->mask[i - 1], i - 1, i, 1,2);
    }
    
    str->rev = gtk_check_button_new_with_label(TXT_BE_BMP_REV);
    gtk_container_add(GTK_CONTAINER(ctx), str->rev);    

}

void gui_bineditor_find_repl(GtkToggleButton *tg, gui_find_str *str)
{
    if(gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( tg )))
	gtk_widget_set_sensitive(str->replace, 1);        
    else
	gtk_widget_set_sensitive(str->replace, 0);    
}

/* Find and replace */
static void gui_bineditor_build_find_string( GuiBineditor *be, GtkWidget *ctx, gui_find_str *str )
{
    GtkWidget *hb, *table;
    /* entries */
    gtk_box_pack_start(GTK_BOX(ctx), gtk_label_new(TXT_BE_FIND_ENTRY), FALSE, FALSE, 2);        
    str->find = gtk_entry_new();    
    gtk_box_pack_start(GTK_BOX(ctx), str->find, FALSE, TRUE, 2);        
    gtk_box_pack_start(GTK_BOX(ctx), gtk_label_new(TXT_BE_REPLACE_ENTRY), FALSE, FALSE, 2);        
    hb = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx), hb, FALSE, TRUE, 2);        
    str->repl = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(hb), str->repl, FALSE, FALSE, 2);            
    str->replace = gtk_entry_new();    
    gtk_container_add(GTK_CONTAINER(hb), str->replace);                
    gtk_widget_set_sensitive(str->replace, 0);    

    /* parameters */
    hb = gtk_frame_new(TXT_BE_FIND_ST_LABEL);
    gtk_frame_set_label_align(GTK_FRAME(hb), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(ctx), hb);
    table = gtk_table_new(4, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(hb), table);
    str->r0 = gtk_radio_button_new_with_label(NULL, TXT_BE_FIND_ST_STRING);
//    str->r1 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->r0), TXT_BE_FIND_ST_REGEXP);
    str->r2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->r0), TXT_BE_FIND_ST_HEX);
//    str->r3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->r0), TXT_BE_FIND_ST_BIN);
    gtk_table_attach_defaults(GTK_TABLE(table), str->r0, 0,1, 0,1);
//    gtk_table_attach_defaults(GTK_TABLE(table), str->r1, 0,1, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(table), str->r2, 0,1, 2,3);    
//    gtk_table_attach_defaults(GTK_TABLE(table), str->r3, 0,1, 3,4);    
    str->ci = gtk_check_button_new_with_label(TXT_BE_FIND_ST_CI);
    str->c0 = gtk_radio_button_new_with_label(NULL, TXT_BE_FIND_ST_BEGIN);
    str->c1 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->c0), TXT_BE_FIND_ST_CURSOR);
    str->c2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->c0), TXT_BE_FIND_ST_MARKED);
    gtk_table_attach_defaults(GTK_TABLE(table), str->ci, 1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(table), str->c0, 1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(table), str->c1, 1,2, 2,3);    
    gtk_table_attach_defaults(GTK_TABLE(table), str->c2, 1,2, 3,4);    
    g_signal_connect(G_OBJECT(str->repl), "toggled", G_CALLBACK(gui_bineditor_find_repl), str );
}

/**/
void gui_bineditor_bx( GtkWidget *tb, GtkWidget *wg, const char *lbl, int x0, int x1, int y0, int y1)
{
    GtkWidget *hb;

    hb = gtk_hbox_new(FALSE, 0);    
    gtk_table_attach_defaults(GTK_TABLE(tb), hb,  x0,x1, y0,y1);
    gtk_container_add(GTK_CONTAINER(hb), gtk_label_new( lbl ));
    gtk_container_add(GTK_CONTAINER(hb), wg);
}

void gui_bineditor_bmbx_set_sensitive( gui_be_bm_str *str, char x)
{
    gtk_widget_set_sensitive(str->bx0, x);    
    gtk_widget_set_sensitive(str->bx1, x);    
    gtk_widget_set_sensitive(str->bx2, x);    
    gtk_widget_set_sensitive(str->bx3, x);    
    gtk_widget_set_sensitive(str->bx4, x);    
    gtk_widget_set_sensitive(str->bx5, x);    
    gtk_widget_set_sensitive(str->bx6, x);    
    gtk_widget_set_sensitive(str->bx7, x);    
}

void gui_bineditor_bx_sens(GtkToggleButton *tg, gui_be_bm_str *str)
{
    if(gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( tg ))){
	gui_bineditor_bmbx_set_sensitive( str, 1);
	gtk_widget_set_sensitive(str->arg, 0);    
    }else{
        gui_bineditor_bmbx_set_sensitive( str, 0);
	gtk_widget_set_sensitive(str->arg, 1);    
    }
}

static void gui_bineditor_build_manipulator( GuiBineditor *be, GtkWidget *ctx, gui_be_bm_str *str )
{
    GtkWidget *wg, *tb, *vb;
    unsigned int from = 0, to = 0;
// input parameters
    wg = gtk_table_new(3, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    gtk_table_attach_defaults(GTK_TABLE(wg), gtk_label_new(TXT_BE_BM_START_ADDRESS), 0,1, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(wg), gtk_label_new(TXT_BE_BM_COUNT), 0,1, 1,2);    
    gtk_table_attach_defaults(GTK_TABLE(wg), gtk_label_new(TXT_BE_BM_ARGUMENT), 0,1, 2,3);
    str->addr  = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);//gtk_entry_new();
    str->count = gtk_spin_button_new_with_range( 0, be->priv->buff->size, 1);//gtk_entry_new();
    str->arg   = gtk_spin_button_new_with_range( 0, 255, 1);//gtk_entry_new();

    if(!gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &from, &to)){
	from = 0;
	to = be->priv->buff->size - 1;
    }

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->addr), from);    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), to - from + 1);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->addr,  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->count, 1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->arg,   1,2, 2,3);
// functions to select
    wg = gtk_frame_new(TXT_BE_BM_FUNCTIONS);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    tb = gtk_table_new( 5, 4, TRUE);
    gtk_container_add(GTK_CONTAINER(wg), tb);    
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_BM_ARITHM), 0,1, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_BM_LOGIC),  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_BM_SHIFT),  2,3, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_BM_ROTATE), 3,4, 0,1);

    str->sub = gtk_radio_button_new_with_label(NULL, TXT_BE_BM_SUB);
    str->add = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_ADD);
    str->mul = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_MUL);
    str->div = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_DIV);
    str->or =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_OR);
    str->and = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_AND);
    str->xor = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_XOR);
    str->shl = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_SHL);
    str->sal = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_SAL);
    str->shr = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_SHR);
    str->sar = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_SAR);
    str->rol = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_ROL);
    str->ror = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_ROR);

    gtk_table_attach_defaults(GTK_TABLE(tb), str->sub, 0,1, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->add, 0,1, 2,3);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->mul, 0,1, 3,4);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->div, 0,1, 4,5);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->or,  1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->and, 1,2, 2,3);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->xor, 1,2, 3,4);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->shl, 2,3, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->sal, 2,3, 2,3);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->shr, 2,3, 3,4);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->sar, 2,3, 4,5);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->rol, 3,4, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->ror, 3,4, 2,3);
// bit exchg
    wg = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    vb = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(wg), vb);    
    str->bx = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->sub), TXT_BE_BM_BX);    
    gtk_container_add(GTK_CONTAINER(vb), str->bx);    
    tb = gtk_table_new( 4, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(vb), tb);    
    
    str->bx0 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx1 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx2 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx3 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx4 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx5 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx6 = gtk_spin_button_new_with_range( 0, 7, 1);
    str->bx7 = gtk_spin_button_new_with_range( 0, 7, 1);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx0), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx1), 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx2), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx3), 3);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx4), 4);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx5), 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx6), 6);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->bx7), 7);

    gui_bineditor_bx(tb, str->bx0, "0->", 0,1, 0,1);
    gui_bineditor_bx(tb, str->bx1, "1->", 0,1, 1,2);    
    gui_bineditor_bx(tb, str->bx2, "2->", 0,1, 2,3);    
    gui_bineditor_bx(tb, str->bx3, "3->", 0,1, 3,4);    
    gui_bineditor_bx(tb, str->bx4, "4->", 1,2, 0,1);    
    gui_bineditor_bx(tb, str->bx5, "5->", 1,2, 1,2);    
    gui_bineditor_bx(tb, str->bx6, "6->", 1,2, 2,3);    
    gui_bineditor_bx(tb, str->bx7, "7->", 1,2, 3,4);    

    gui_bineditor_bmbx_set_sensitive( str, 0);
    g_signal_connect(G_OBJECT(str->bx), "toggled", G_CALLBACK(gui_bineditor_bx_sens), str );
}

static void gui_clear_radio_whole(GtkWidget *wg, gui_clear_str *str)
{
    if(!gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(str-> rad0))) return;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->from), 0);    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->to), str->be->priv->buff->size - 1);
    gtk_widget_set_sensitive(str->from, 0);    
    gtk_widget_set_sensitive(str->to, 0);
}

static void gui_clear_radio_marked(GtkWidget *wg, gui_clear_str *str)
{
    unsigned int from, to;
    if(!gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(str->rad1))) return;
    
    if( !gui_bineditor_marker_get_range(str->be, GUI_BINEDITOR_MARKER_SELECTED, &from, &to) ){
	from = 0;
	to = str->be->priv->buff->size - 1;
    }
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->from), from);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->to), to);
    gtk_widget_set_sensitive(str->from, 1);
    gtk_widget_set_sensitive(str->to, 1);
}

/* Clear buffer */
static void gui_bineditor_build_clear( GuiBineditor *be, GtkWidget *ctx,  gui_clear_str *str)
{
    GtkWidget *hb, *wg;
// Gui
    /* Radio buttons */
    str->rad0 = gtk_radio_button_new_with_label(NULL, TXT_BE_WHOLE_BUFFER);
    str->rad1 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->rad0), TXT_BE_MARKED_AREA);
    gtk_box_pack_start(GTK_BOX(ctx), str->rad0, FALSE, FALSE, 2);    
    gtk_box_pack_start(GTK_BOX(ctx), str->rad1, FALSE, FALSE, 2);        
    /* Address range */    
    hb = gtk_label_new(TXT_BE_ADDRESS_RANGE);
    gtk_box_pack_start(GTK_BOX(ctx), hb, FALSE, TRUE, 2);
    hb = gtk_hbox_new(FALSE, 0);    
	/* From */
	gtk_box_pack_start(GTK_BOX(ctx), hb, FALSE, FALSE, 2);        
	wg = gtk_label_new(TXT_BE_ADDRESS_FROM);
	gtk_box_pack_start(GTK_BOX(hb), wg, FALSE, FALSE, 4);        
	str->from = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->from), 0);
	gtk_container_add(GTK_CONTAINER(hb), str->from);            
	gtk_widget_set_sensitive(str->from, 0);    
	/* to */
	wg = gtk_label_new(TXT_BE_ADDRESS_TO);
	gtk_box_pack_start(GTK_BOX(hb), wg, FALSE, FALSE, 4);        
	str->to = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->to), be->priv->buff->size - 1);
	gtk_container_add(GTK_CONTAINER(hb), str->to);            
	gtk_widget_set_sensitive(str->to, 0);    
    /* Pattern */
    hb = gtk_label_new(TXT_BE_PATTERN);
    gtk_box_pack_start(GTK_BOX(ctx), hb, FALSE, FALSE, 2);
    str->pattern = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(ctx), str->pattern, FALSE, FALSE, 8);
    gtk_entry_set_text(GTK_ENTRY(str->pattern), "0xff");
// Signals
    g_signal_connect(G_OBJECT(str->rad0), "toggled", G_CALLBACK(gui_clear_radio_whole), str);
    g_signal_connect(G_OBJECT(str->rad1), "toggled", G_CALLBACK(gui_clear_radio_marked), str);    
}

static void gui_bineditor_as_sensitive(gui_be_org_str *str, char sens)
{
    int i;
    for(i = 0; i < 32; i++)
	if(str->a[i])
	    gtk_widget_set_sensitive(str->a[i], sens);

}

static void gui_bineditor_as_show(gui_be_org_str *str, GtkWidget *tb, const char *lbl, int row, int bl)
{
    int i;
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(lbl),  0,1, row, row + 1);
    for(i = 0; i < 8; i++)
	if(str->a[i + bl]){
	    gtk_table_attach_defaults(GTK_TABLE(tb), str->a[i + bl],  i + 1, i + 2, row, row + 1);
	    gtk_widget_set_sensitive(str->a[i + bl], FALSE);
	}
}

static char gui_bineditor_get_bits_size( unsigned int size )
{
    unsigned int i;
    unsigned int mask = 1 << 31;

    if( size > 0) size--; else return 0;
        
    for(i = 32; (i > 0) && !( size & mask); i--, mask >>= 1);
    return i - 1;
}

static GtkWidget *gui_bineditor_set_as(gui_be_org_str *str, unsigned int size)
{    
    GtkWidget *wg, *tb;
    int i, bits;

    bits = gui_bineditor_get_bits_size(size);
    wg = gtk_frame_new(TXT_BE_ORG_REORG_ADDRS);
    if(!bits) return wg;

    tb = gtk_table_new((bits / 8) + 1, 9, TRUE);
    gtk_container_add(GTK_CONTAINER(wg), tb);

    for(i = 0; i < 32; i++){
	if( i <= bits)
	    str->a[i] = gtk_spin_button_new_with_range( 0, bits, 1);
	else
	    str->a[i] = NULL;
    }

    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+0"),  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+1"),  2,3, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+2"),  3,4, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+3"),  4,5, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+4"),  5,6, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+5"),  6,7, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+6"),  7,8, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new("+7"),  8,9, 0,1);

    gui_bineditor_as_show( str, tb, "00:", 1, 0);
    if(bits > 7)
	gui_bineditor_as_show( str, tb, "08:", 2, 8);
    if(bits > 15)
	gui_bineditor_as_show( str, tb, "16:", 3, 16);
    if(bits > 23)
	gui_bineditor_as_show( str, tb, "24:", 4, 24);
    return wg;
}

static void gui_be_org_bit_rst( gui_be_org_str *str )
{
    gtk_widget_destroy(str->bits);
    str->bits = gui_bineditor_set_as(str, gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count)));
    gtk_box_pack_start(GTK_BOX(str->vb), str->bits, FALSE, FALSE, 10);
    gui_bineditor_as_sensitive( str, gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(str->reorg)) );
    gtk_widget_show_all(str->vb);
}

static void gui_be_org_bt_sig(GtkWidget *wg, gui_be_org_str *str)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->addr), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), str->be->priv->buff->size);
    gui_be_org_bit_rst( str );
}

static void gui_be_org_reorg_rad(GtkWidget *wg, gui_be_org_str *str)
{
    gui_bineditor_as_sensitive( str, gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(str->reorg)) );
}

static void gui_be_org_chval(GtkWidget *wg, gui_be_org_str *str)
{
    unsigned int start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->addr));
    unsigned int count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(str->count));

    if( start + count >= str->be->priv->buff->size - 1){
	count = str->be->priv->buff->size - start;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), count);	
    }
}

static void gui_be_org_unfoc(GtkWidget *wg, GdkEvent *ev, gui_be_org_str *str)
{
    gui_be_org_bit_rst( str );
}

static void gui_bineditor_build_organizer( GuiBineditor *be, GtkWidget *ctx, gui_be_org_str *str )
{
    GtkWidget *wg, *vb, *lb;
    unsigned int start = 0, count = 0, to = 0;
    
    if(!gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &start, &to)){
	start = 0;
	to = be->priv->buff->size - 1;
    }

// initial values
    str->be = be;
    count = to - start;
    if(count < 0) count = 0;
    if(start < 0) start = 0;
    if(!(count | start )){
	start = 0;
	count = be->priv->buff->size;
    }
    
// input parameters
    wg = gtk_table_new(3, 2, TRUE);
    gtk_box_pack_start(GTK_BOX(ctx), wg, TRUE, TRUE, 5);
    lb = gtk_label_new(TXT_BE_BM_START_ADDRESS);
    gtk_misc_set_alignment(GTK_MISC(lb), 0.0f, 0.5f);
    gtk_table_attach_defaults(GTK_TABLE(wg), lb, 0,1, 0,1);
    lb = gtk_label_new(TXT_BE_BM_COUNT);
    gtk_misc_set_alignment(GTK_MISC(lb), 0.0f, 0.5f);
    gtk_table_attach_defaults(GTK_TABLE(wg), lb, 0,1, 1,2);    
    str->addr  = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);//gtk_entry_new();
    str->count = gtk_spin_button_new_with_range( 0, be->priv->buff->size, 1);//gtk_entry_new();
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->addr), start);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), count);
    str->all  = gtk_button_new_with_label(TXT_BE_ORG_WHOLE);

    gtk_table_attach_defaults(GTK_TABLE(wg), str->addr,  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->count, 1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(wg), str->all,   1,2, 2,3);
// functions to select
    wg = gtk_frame_new(TXT_BE_ORG_FUNCTIONS);
    gtk_container_add(GTK_CONTAINER(ctx), wg);    
    vb = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), vb);    

    str->split = gtk_radio_button_new_with_label(NULL, TXT_BE_ORG_SPLIT);
    str->merge = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->split), TXT_BE_ORG_MERGE);
    str->xchg  = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->split), TXT_BE_ORG_XCHG);
    str->reorg = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(str->split), TXT_BE_ORG_REORG);

    gtk_container_add(GTK_CONTAINER(vb), str->split);    
    gtk_container_add(GTK_CONTAINER(vb), str->merge);    
    gtk_container_add(GTK_CONTAINER(vb), str->xchg);    
    gtk_container_add(GTK_CONTAINER(vb), str->reorg);    
// address bits
    str->vb = vb;
    str->bits = gui_bineditor_set_as(str, count);
    gtk_box_pack_start(GTK_BOX(str->vb), str->bits, FALSE, FALSE, 10);
// signal connects
    g_signal_connect(G_OBJECT(str->reorg), "toggled", G_CALLBACK(gui_be_org_reorg_rad), str);
    g_signal_connect(G_OBJECT(str->all), "pressed", G_CALLBACK(gui_be_org_bt_sig), str);
    g_signal_connect(G_OBJECT(str->addr),  "value-changed", G_CALLBACK(gui_be_org_chval), str);    
    g_signal_connect(G_OBJECT(str->count), "value-changed", G_CALLBACK(gui_be_org_chval), str);    
    g_signal_connect(G_OBJECT(str->addr),  "focus-out-event", G_CALLBACK(gui_be_org_unfoc), str);    
    g_signal_connect(G_OBJECT(str->count), "focus-out-event", G_CALLBACK(gui_be_org_unfoc), str);    

}

static void gui_be_cut_start(GtkWidget *wg, gui_be_cut_str *str)
{
    int start, stop, count;
    
    gui_be_cut_get_values( str, &start, &count, &stop );

    if(start > stop){
	stop = start;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), stop);
    }

    if(start - stop != count - 1){
	stop = start + count - 1;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), stop);
    }
    
    if(start + count >= str->be->priv->buff->size){
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), str->be->priv->buff->size - start);
    }
}

static void gui_be_cut_stop(GtkWidget *wg, gui_be_cut_str *str)
{
    int start, stop, count;
    
    gui_be_cut_get_values( str, &start, &count, &stop );

    if(stop < start){
	stop = start;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), stop);
    }

    if( stop >= str->be->priv->buff->size){
	stop = str->be->priv->buff->size - 1;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), stop);
    }

    if(start - stop != count - 1){
	count = stop - start + 1;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), count);
    }
}

static void gui_bineditor_build_cut( GuiBineditor *be, GtkWidget *ctx, gui_be_cut_str *str )
{
    GtkWidget *tb;
    unsigned int start, count, to;

// initial values
    if( !gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &start, &to) ){
	start = 0;
	to = 0;
    }

    str->be = be;
    count = to - start;
    if(count <= 0) count = 1;
    if(start < 0) start = 0;

// input parameters
    tb = gtk_table_new(3, 2, TRUE);	
    gtk_box_pack_start(GTK_BOX(ctx), tb, TRUE, TRUE, 5);
    str->start = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    str->stop  = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    str->count = gtk_spin_button_new_with_range( 1, be->priv->buff->size, 1);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->start), start);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), count);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->stop), start + count - 1);

    gtk_table_attach_defaults(GTK_TABLE(tb), str->start,  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->stop,   1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->count,  1,2, 2,3);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_CUT_START), 0,1, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_CUT_STOP),  0,1, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_CUT_COUNT), 0,1, 2,3);
// signals
    g_signal_connect(G_OBJECT(str->start), "value-changed", G_CALLBACK(gui_be_cut_start), str);
    g_signal_connect(G_OBJECT(str->stop),  "value-changed", G_CALLBACK(gui_be_cut_stop),  str);
    g_signal_connect(G_OBJECT(str->count), "value-changed", G_CALLBACK(gui_be_cut_start), str);
}

static void gui_bineditor_build_copy( GuiBineditor *be, GtkWidget *ctx, gui_be_copy_str *str )
{
    GtkWidget *wg;
    str->start = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON( str->start ), be->priv->edit_addr_cursor);
    wg = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx), wg, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(wg), gtk_label_new(TXT_BE_COPY_ADDRESS));    
    gtk_container_add(GTK_CONTAINER(wg), str->start);    
}

static void gui_be_asm_fsel(GtkWidget *w, gui_be_asm_str *str )
{
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new(TXT_BE_TITWIN_SELECT_CORE, GTK_WINDOW(str->be->priv->wmain), 
	GTK_FILE_CHOOSER_ACTION_OPEN,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	NULL
    );
    if(str->be->priv->core_name != NULL)
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), str->be->priv->core_name);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, TXT_BE_ASM_CORE);
    gtk_file_filter_add_pattern(filter, "*.brain");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT){
	char *fname;
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_entry_set_text(GTK_ENTRY(str->proc), fname);
	g_free( fname );
    }

    gtk_widget_destroy( dialog );
}

static void gui_bineditor_build_asm( GuiBineditor *be, GtkWidget *ctx, gui_be_asm_str *str )
{
    GtkWidget *wg;    

    str->be = be;
// widgets    
    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctx), wg);
    str->fsel = gtk_button_new_with_label(TXT_BE_ASM_FSEL);
    gtk_box_pack_start(GTK_BOX(wg), str->fsel, FALSE, FALSE, 0);
    str->proc = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(wg), str->proc, TRUE, TRUE, 0);
    gtk_editable_set_editable(GTK_EDITABLE(str->proc), FALSE);

    if(be->priv->core_name)
	gtk_entry_set_text(GTK_ENTRY(str->proc), be->priv->core_name);
    
    wg = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctx), wg);
    gtk_box_pack_start(GTK_BOX(wg), gtk_label_new(TXT_BE_ASM_COUNT), FALSE, FALSE, 0);    
    str->count = gtk_spin_button_new_with_range( 0, 255, 1);
    gtk_box_pack_start(GTK_BOX(wg), str->count, TRUE, TRUE, 0);    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->count), be->priv->core_count);
// signals
    g_signal_connect(G_OBJECT(str->fsel), "pressed", G_CALLBACK(gui_be_asm_fsel), str);
}

static void gui_bineditor_build_text( GuiBineditor *be, GtkWidget *ctx, gui_be_text_str *str )
{
    GtkWidget *tb;    
    unsigned int from = 0, to = 0;

    if(!gui_bineditor_marker_get_range(be, GUI_BINEDITOR_MARKER_SELECTED, &from, &to)) from = 0;

    tb = gtk_table_new(3, 2, TRUE);	
    gtk_box_pack_start(GTK_BOX(ctx), tb, TRUE, TRUE, 5);
    str->start  = gtk_spin_button_new_with_range( 0, be->priv->buff->size - 1, 1);
    str->width  = gtk_spin_button_new_with_range( 0, 80, 1);
    str->height = gtk_spin_button_new_with_range( 1, be->priv->buff->size, 1);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->start),  from);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->width),  16);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(str->height), 16);

    gtk_table_attach_defaults(GTK_TABLE(tb), str->start,  1,2, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->width,  1,2, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), str->height, 1,2, 2,3);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_TEXT_START),  0,1, 0,1);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_TEXT_WIDTH),  0,1, 1,2);
    gtk_table_attach_defaults(GTK_TABLE(tb), gtk_label_new(TXT_BE_TEXT_HEIGHT), 0,1, 2,3);
// add values guard !
}

/***************************************************************************************************************************************/
static void gui_bineditor_dialog_tmpl(GuiBineditor *be, void *str, gui_bineditor_tmpl_cb build, gui_bineditor_tmpl_cb exec, const char *title)
{
    int RESPONSE;
    GtkWidget *dlg, *ctx;
    
    if(!be->priv->buff->data) return;
    dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(be->priv->wmain), 0, 
	    GTK_STOCK_OK, GTK_RESPONSE_OK,
	    (exec != NULL) ? GTK_STOCK_CANCEL : NULL, GTK_RESPONSE_CANCEL,
	    NULL
	  );
    ctx = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    if(build != NULL){ 
	build( be, ctx, str);
	gtk_widget_show_all( ctx );            
    }
    RESPONSE = gtk_dialog_run(GTK_DIALOG(dlg));    
    if(( RESPONSE == GTK_RESPONSE_OK) && ( exec != NULL)) exec( be, ctx, str);
    gtk_widget_destroy( dlg );
}

void gui_bineditor_clear_buffer(GtkWidget *bt, GuiBineditor *be)
{
    gui_clear_str str;
    str.be = be;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_clear), GUI_BE_CB(gui_bineditor_clear_exec), TEXT(BE_WIN_TIT_CLEAR));
} 

void gui_bineditor_find_string(GtkWidget *wg, GuiBineditor *be)
{
    gui_find_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_find_string), GUI_BE_CB(gui_bineditor_find_exec), TEXT(BE_WIN_TIT_FIND_AND_REPLACE));
}

void gui_bineditor_manipulator(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_bm_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_manipulator), GUI_BE_CB(gui_bineditor_manipulator_exec), TEXT(BE_WIN_TIT_MANIPULATOR));
}

void gui_bineditor_organizer(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_org_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_organizer), GUI_BE_CB(gui_bineditor_organizer_exec), TEXT(BE_WIN_TIT_ORGANIZER));
}

void gui_bineditor_bined(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_bmp_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_bined), GUI_BE_CB(gui_bineditor_bined_exec), TEXT(BE_WIN_TIT_BMPEDIT));
}

void gui_bineditor_cut(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_cut_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_cut), GUI_BE_CB(gui_bineditor_cut_exec), TEXT(BE_WIN_TIT_CUT));
}

void gui_bineditor_copy(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_copy_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_copy), GUI_BE_CB(gui_bineditor_copy_exec), TEXT(BE_WIN_TIT_COPY));
}

void gui_bineditor_asmview(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_asm_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_asm), GUI_BE_CB(gui_bineditor_asm_exec), TEXT(BE_WIN_TIT_ASMVIEWER));
}

void gui_bineditor_texted(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_text_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_text), GUI_BE_CB(gui_bineditor_text_exec), TEXT(BE_WIN_TIT_TEXT));
}

void gui_bineditor_undo(GtkWidget *wg, GuiBineditor *be)
{
    gui_bineditor_buff_history(be->priv->buff, GUI_BE_UNDO);
}

void gui_bineditor_redo(GtkWidget *wg, GuiBineditor *be)
{
    gui_bineditor_buff_history(be->priv->buff, GUI_BE_REDO);
}

void gui_bineditor_aux(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_aux_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_aux), GUI_BE_CB(gui_bineditor_aux_exec), TEXT(BE_WIN_TIT_AUX));
}

void gui_bineditor_checksum(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_sum_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_sum), NULL, TEXT(BE_WIN_TIT_SUM));
}

void gui_bineditor_resize(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_resize_str str;
    gui_bineditor_dialog_tmpl(be, &str, GUI_BE_CB(gui_bineditor_build_resize), GUI_BE_CB(gui_bineditor_resize_exec), TEXT(BE_WIN_TIT_RESIZE));
}

void gui_bineditor_stencil(GtkWidget *wg, GuiBineditor *be)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    char *fname = NULL;

    dialog = gtk_file_chooser_dialog_new(TXT_BE_STC_WINTIT, GTK_WINDOW(be->priv->wmain), 
	GTK_FILE_CHOOSER_ACTION_OPEN,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	NULL
    );

// gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), name);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, TXT_BE_STC_FE);
    gtk_file_filter_add_pattern(filter, "*.stc");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    gtk_widget_destroy( dialog );
    if( fname != NULL ) gui_bineditor_stencil_run(be, fname);
    g_free( fname );
}

static void gui_bineditor_open_(GuiBineditor *be, gui_be_open_str *str)
{
    gui_bineditor_dialog_tmpl(be, str, GUI_BE_CB(gui_bineditor_build_open), GUI_BE_CB(gui_bineditor_open_exec), TEXT(BE_WIN_TIT_OPEN));
}

static void gui_bineditor_save_(GuiBineditor *be, gui_be_save_str *str)
{
    gui_bineditor_dialog_tmpl(be, str, GUI_BE_CB(gui_bineditor_build_save), GUI_BE_CB(gui_bineditor_save_exec), TEXT(BE_WIN_TIT_SAVE));
}

void gui_bineditor_open(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_open_str str;
    GtkWidget *dialog;

    str.fname = NULL;
    str.fh = NULL;
    dialog = gtk_file_chooser_dialog_new(TXT_BE_OPEN_WINTIT, GTK_WINDOW(be->priv->wmain), 
	GTK_FILE_CHOOSER_ACTION_OPEN,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	NULL
    );

// gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), name);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	str.fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    gtk_widget_destroy( dialog );
    if( str.fname != NULL ){ 
	gui_bineditor_open_(be, &str);
	g_free( str.fname );
    }
    if(str.fh) fclose(str.fh);
}

void gui_bineditor_write(GtkWidget *wg, GuiBineditor *be)
{
    gui_be_save_str str;
    GtkWidget *dialog;

    str.fname = NULL;
    dialog = gtk_file_chooser_dialog_new(TXT_BE_SAVE_WINTIT, GTK_WINDOW(be->priv->wmain), 
	GTK_FILE_CHOOSER_ACTION_SAVE,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL
    );

// gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), name);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	str.fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    gtk_widget_destroy( dialog );
    if( str.fname != NULL ){ 
	gui_bineditor_save_(be, &str);
	g_free( str.fname );
    }
}
