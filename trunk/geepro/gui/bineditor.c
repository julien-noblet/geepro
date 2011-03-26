/* $Revision: 1.5 $ */
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
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include "bineditor.h"
#include "checksum.h"

enum
{
    CHANGED,
    LAST_SIGNAL
};

enum
{
    CHKSUM_GBE,
    CHKSUM_START,
    CHKSUM_STOP,
    CHKSUM_START_SK,
    CHKSUM_STOP_SK,
    CHKSUM_ADDR_CB,
    CHKSUM_ADDR_SB,
    CHKSUM_ADDR_BO,
    CHKSUM_ALGO_CBX,
    CHKSUM_CALC_BT,
    CHKSUM_CALC_ET,
    CHKSUM_CALC_VER,
    CHKSUM_CALC_WRITE,
    CHKSUM_LAST
};

static char clear_guard = 0;
static guint bineditor_signals[LAST_SIGNAL] = {0};
static void gui_bineditor_class_init(GuiBineditorClass *cl);
static void gui_bineditor_init(GuiBineditor *be);
static void gui_beneditor_destroy(GtkObject *obj);
static void gui_bineditor_draw(cairo_t *cr, GuiBineditor *be, int vxx, int vyy, char pr);

GtkType gui_bineditor_get_type(void)
{
    static GtkType bineditor_type = 0;
    
    if(!bineditor_type){

	GtkTypeInfo bineditor_info =
	{
	    "GuiBineditor",
	    sizeof(GuiBineditor),
	    sizeof(GuiBineditorClass),
	    (GtkClassInitFunc) gui_bineditor_class_init,
	    (GtkObjectInitFunc) gui_bineditor_init,
	};
	
	bineditor_type = gtk_type_unique(gtk_vbox_get_type(), &bineditor_info);
    }

    return bineditor_type;
}

static void gui_bineditor_class_init(GuiBineditorClass *cl)
{
    GtkObjectClass *oc = (GtkObjectClass*)cl;

    bineditor_signals[CHANGED] = g_signal_new(
	"changed", 
	G_TYPE_FROM_CLASS(cl), 
	G_SIGNAL_ACTION | G_SIGNAL_ACTION,
	G_STRUCT_OFFSET(GuiBineditorClass, bineditor),
	NULL, NULL,
	g_cclosure_marshal_VOID__VOID,
	G_TYPE_NONE, 0
    );
    oc->destroy = gui_beneditor_destroy;
}

static void gui_beneditor_destroy(GtkObject *obj)
{
    GuiBineditor *be;

    g_return_if_fail(obj != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(obj));

    be = GUI_BINEDITOR(obj);    
    if(be->vfind) free(be->vfind);
    be->vfind = NULL;
    if(be->vreplace) free(be->vreplace);
    be->vreplace = NULL;
}

GtkWidget *gui_bineditor_new(GtkWindow *wmain)
{
    GtkWidget *wg;

    g_return_val_if_fail(wmain != NULL, NULL);
    g_return_val_if_fail(GTK_IS_WINDOW(wmain), NULL);

    wg = GTK_WIDGET(g_object_new(GUI_TYPE_BINEDITOR, NULL));
    ((GuiBineditor*)wg)->wmain = (GtkWidget*)wmain;
    return wg;
}

static void gui_bineditor_emit_signal(GuiBineditor *wg)
{ 
    gtk_signal_emit(GTK_OBJECT(wg), bineditor_signals[CHANGED]);
}

static void gui_bineditor_kill(GtkWidget *wg, GtkWidget *wgg)
{
    gtk_widget_destroy(wgg);
}

static void gui_dialog_ok(GuiBineditor *be, const char *stock_img, const char *title, const char *msg)
{
    GtkWidget *dlg, *wg0, *wg1;

    g_return_if_fail(be->wmain != NULL);
    dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    wg0 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(dlg), 10);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox), wg0);
    wg1 = gtk_image_new_from_stock(stock_img, GTK_ICON_SIZE_DIALOG);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    wg1 = gtk_label_new(msg);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));    
    gtk_widget_destroy(dlg);
}

static void gui_bineditor_calc_checksum(GtkWidget *wg, GtkWidget **wgg)
{
    int alg, start, stop, start_sk1, stop_sk1, addr, bytes;    
    char tmp[32], *fmt, chk;
    GuiBineditor *be = (GuiBineditor *)wgg[CHKSUM_GBE];

    g_return_if_fail(be->buffer != NULL);
    g_return_if_fail(be->buffer_size != 0);
    
    alg = gtk_combo_box_get_active(GTK_COMBO_BOX(wgg[CHKSUM_ALGO_CBX]));
    start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_START]));
    start_sk1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_START_SK]));
    stop = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_STOP]));
    stop_sk1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_STOP_SK]));
    addr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_ADDR_SB]));
    chk = GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_CB])->active;
    g_return_if_fail((alg != -1) || (start != -1) || (stop != -1) || (addr != -1) || (start_sk1 != -1) || (stop_sk1 != -1));

    switch(alg){
	case 0: bytes = 1; fmt = "0x%02X"; break; /* LRC    */
	case 1: bytes = 2; fmt = "0x%04X"; break; /* CRC-16 */
	case 2: bytes = 4; fmt = "0x%08X"; break; /* CRC-32 */
	default: bytes = 0; fmt = "----" ; break;
    }

    if(!chk) bytes = 0;
    be->sum = checksum_calculate(alg, be->buffer_size, be->buffer, start, stop, start_sk1, stop_sk1, addr, addr + bytes);
    memset(tmp, 0, 32);
    sprintf(tmp, fmt, be->sum);
    gtk_entry_set_text(GTK_ENTRY(wgg[CHKSUM_CALC_ET]), tmp);
    if(chk){
	gtk_widget_set_sensitive(wgg[CHKSUM_CALC_VER], TRUE);
	gtk_widget_set_sensitive(wgg[CHKSUM_CALC_WRITE], TRUE);
    }
}

static void gui_bineditor_checksum_write(GtkWidget *wg, GtkWidget **wgg)
{
    const char *tmp;
    int alg, addr, bo, bytes, i;
    GuiBineditor *be = (GuiBineditor *)wgg[CHKSUM_GBE];
    
    tmp = gtk_entry_get_text(GTK_ENTRY(wgg[CHKSUM_CALC_ET]));
    g_return_if_fail(tmp != NULL);
    
    if(*tmp == 0){
	gui_dialog_ok((GuiBineditor *)wgg[CHKSUM_GBE], GTK_STOCK_INFO, "Message", "No calculated checksum.");
	return;
    }
    alg = gtk_combo_box_get_active(GTK_COMBO_BOX(wgg[CHKSUM_ALGO_CBX]));
    addr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_ADDR_SB]));
    bo = GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_BO])->active;
    g_return_if_fail((alg != -1) || (addr != -1) || (bo != -1));

    switch(alg){
	case 0: bytes = 1; break; /* LRC    */
	case 1: bytes = 2; break; /* CRC-16 */
	case 2: bytes = 4; break; /* CRC-32 */
	default: bytes = 0;break;
    }

    for(i = 0; (i < bytes) && ((i+addr) < be->buffer_size); i++)
	if(bo)
	    be->buffer[addr + (bytes - i - 1)] = (be->sum >> (i*8)) & 0xff;
	else
	    be->buffer[addr + i] = (be->sum >> (i*8)) & 0xff;

    if(bytes){
	gui_bineditor_emit_signal((GuiBineditor*)wgg[CHKSUM_GBE]);
	gtk_widget_queue_draw(be->drawing_area);
	gui_dialog_ok(be, GTK_STOCK_DIALOG_INFO, "Message", "Checksum updated.");	
    }
}

static void gui_bineditor_checksum_verify(GtkWidget *wg, GtkWidget **wgg)
{
    const char *tmp;
    int alg, addr, bo, i, bytes;
    GuiBineditor *be = (GuiBineditor *)wgg[CHKSUM_GBE];
        
    tmp = gtk_entry_get_text(GTK_ENTRY(wgg[CHKSUM_CALC_ET]));
    g_return_if_fail(tmp != NULL);
    
    if(*tmp == 0){
	gui_dialog_ok((GuiBineditor *)wgg[CHKSUM_GBE], GTK_STOCK_INFO, "Message", "No calculated checksum.");
	return;
    }
    alg = gtk_combo_box_get_active(GTK_COMBO_BOX(wgg[CHKSUM_ALGO_CBX]));
    addr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_ADDR_SB]));
    bo = GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_BO])->active;
    g_return_if_fail((alg != -1) || (addr != -1) || (bo != -1));    

    switch(alg){
	case 0: bytes = 1; break; /* LRC    */
	case 1: bytes = 2; break; /* CRC-16 */
	case 2: bytes = 4; break; /* CRC-32 */
	default: bytes = 0;break;
    }

    for(i = 0; (i < bytes) && ((i+addr) < be->buffer_size); i++)
	if(bo){
	    if(be->buffer[addr + (bytes - i - 1)] != ((be->sum >> (i*8)) & 0xff)) break;
	}else{
	    if(be->buffer[addr + i] != ((be->sum >> (i*8)) & 0xff)) break;
	}

    if(i != bytes)
	gui_dialog_ok(be, GTK_STOCK_DIALOG_WARNING, "Message", "Checksum inconsistent.");
    else
	gui_dialog_ok(be, GTK_STOCK_DIALOG_INFO, "Message", "Checksum consistent.");
}

static void gui_bineditor_chksum_chkbox(GtkWidget *wg, GtkWidget **wgg)
{
    gtk_widget_set_sensitive(wgg[CHKSUM_ADDR_SB], GTK_TOGGLE_BUTTON(wg)->active);
    gtk_widget_set_sensitive(wgg[CHKSUM_CALC_VER], FALSE);
    gtk_widget_set_sensitive(wgg[CHKSUM_CALC_WRITE], FALSE);
    gtk_entry_set_text(GTK_ENTRY(wgg[CHKSUM_CALC_ET]), "");
}

static void gui_bineditor_checksum(GtkWidget *wg, GuiBineditor *be)
{
    static GtkWidget *wgg[CHKSUM_LAST];
    GtkWidget *dialog;
    GtkWidget *tab, *wg0;
    GtkObject *adj;
    
    dialog = gtk_dialog_new_with_buttons("Checksum", GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL
    );

    /* tabelka pakujaca */
    tab = gtk_table_new(5, 5, FALSE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), tab);
    gtk_container_set_border_width(GTK_CONTAINER(tab), 10);

    wgg[CHKSUM_GBE] = (GtkWidget *)be;
    wg0 = gtk_label_new("Start address: ");
    gtk_misc_set_alignment(GTK_MISC(wg0), 0, 0.5);
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,1, 0,1, GTK_FILL,0, 0,10);
    wg0 = gtk_label_new("Stop address: ");
    gtk_misc_set_alignment(GTK_MISC(wg0), 0, 0.5);
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,1, 1,2, GTK_FILL,0, 0,10);

    wg0 = gtk_label_new("  Start lack: ");
    gtk_misc_set_alignment(GTK_MISC(wg0), 0, 0.5);
    gtk_table_attach(GTK_TABLE(tab), wg0, 3,4, 0,1, GTK_FILL,0, 0,10);
    wg0 = gtk_label_new("  Stop lack: ");
    gtk_misc_set_alignment(GTK_MISC(wg0), 0, 0.5);
    gtk_table_attach(GTK_TABLE(tab), wg0, 3,4, 1,2, GTK_FILL,0, 0,10);

    adj = gtk_adjustment_new(0, 0, be->buffer_size, 1,1,1);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 0,1, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_START] = wg0;
    adj = gtk_adjustment_new(be->buffer_size, 0, be->buffer_size, 1,1,1);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 1,2, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_STOP] = wg0;
    adj = gtk_adjustment_new(0, 0, be->buffer_size, 1,1,1);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 0,1, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_START_SK] = wg0;
    adj = gtk_adjustment_new(0, 0, be->buffer_size, 1,1,1);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 1,2, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_STOP_SK] = wg0;

    wg0 = gtk_check_button_new_with_label("Checksum address:");
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,2, 2,3, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_ADDR_CB] = wg0;    
    gtk_signal_connect(GTK_OBJECT(wg0),"toggled", GTK_SIGNAL_FUNC(gui_bineditor_chksum_chkbox), wgg);
    adj = gtk_adjustment_new(0, 0, be->buffer_size, 1,1,1);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 2,3, 2,3, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_widget_set_sensitive(wg0, FALSE);
    wgg[CHKSUM_ADDR_SB] = wg0;
    wg0 = gtk_check_button_new_with_label("Big Endian");
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 2,3, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_ADDR_BO] = wg0;

    wg0 = gtk_label_new("Algorythm: ");
    gtk_misc_set_alignment(GTK_MISC(wg0), 0, 0.5);
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,1, 3,4, GTK_FILL,0, 0,10);
    wg0 = gtk_combo_box_new_text();
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,5, 3,4, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_ALGO_CBX] = wg0;    

    /* wybor algorytmu */
    gtk_combo_box_append_text(GTK_COMBO_BOX(wg0), CHECKSUM_ALGO_LRC);
    gtk_combo_box_append_text(GTK_COMBO_BOX(wg0), CHECKSUM_ALGO_CRC16);
    gtk_combo_box_append_text(GTK_COMBO_BOX(wg0), CHECKSUM_ALGO_CRC32);
    gtk_combo_box_set_active(GTK_COMBO_BOX(wg0), 0);

    wg0 = gtk_button_new_with_label("Calculate");
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,1, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_signal_connect(GTK_OBJECT(wg0), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_calc_checksum), wgg);
    wgg[CHKSUM_CALC_BT] = wg0;

    wg0 = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    wg0 = gtk_entry_new();

    gtk_table_attach(GTK_TABLE(tab), wg0, 2,3, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_entry_set_editable(GTK_ENTRY(wg0), FALSE);
    wgg[CHKSUM_CALC_ET] = wg0;    

    wg0 = gtk_button_new_with_label("Verify");
    gtk_table_attach(GTK_TABLE(tab), wg0, 3,4, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_widget_set_sensitive(wg0, FALSE);
    gtk_signal_connect(GTK_OBJECT(wg0), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_checksum_verify), wgg);
    wgg[CHKSUM_CALC_VER] = wg0;

    wg0 = gtk_button_new_with_label("WRITE");
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_widget_set_sensitive(wg0, FALSE);
    gtk_signal_connect(GTK_OBJECT(wg0), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_checksum_write), wgg);
    wgg[CHKSUM_CALC_WRITE] = wg0;

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/******************************************************************************************************************/
#ifndef NO_PRINTER_SUPPORT
/* rysowanie wydruku */
static void gui_bineditor_draw_page(GtkPrintOperation *op, GtkPrintContext *cx, int page_nr, GuiBineditor *be)
{
    cairo_t *cr = gtk_print_context_get_cairo_context(cx);
    gui_bineditor_draw(cr, be, gtk_print_context_get_width(cx), gtk_print_context_get_height(cx), 1);
    cairo_destroy(cr);
}

static void gui_bineditor_print(GtkWidget *wg, GuiBineditor *be)
{
    static GtkPrintSettings *ps = NULL;
    GtkPrintOperationResult res;
    GtkPrintOperation *op;

    op = gtk_print_operation_new();

    gtk_print_operation_set_print_settings(op, ps);
    gtk_print_operation_set_n_pages(op, 1);
    gtk_print_operation_set_unit(op, GTK_UNIT_MM);
    g_signal_connect(op, "draw_page", G_CALLBACK(gui_bineditor_draw_page), be);

    res = gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
    
    if(res == GTK_PRINT_OPERATION_RESULT_APPLY)
	ps = gtk_print_operation_get_print_settings(op);
}
#endif

/* jak nie znajdzie zwraca -1, inaczej adres */
static int gui_bineditor_search_lo(GuiBineditor *be, const char *find, int addr, char cs, int cn)
{
    int x, i;
    char d;

    for(; addr < be->buffer_size; addr++){
	for(x = i = 0; (i < cn) && ((addr + i) < be->buffer_size); i++){
	    d = be->buffer[addr + i];
	    if(cs) d = tolower(d);
	    if(d == find[i]) x++;
	}
	if(x == cn) return addr; /* jesli trafiony to wyjdz */
    }

    return -1;
}

static char gui_bineditor_dialog_search(GuiBineditor *be, char flg, const char *find, const char *repl)
{    
    GtkWidget *wg, *wg0, *wg1, *wg2;
    int x;
    
    if(flg)
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL, 
	    "Replace", 1,
	    "Skip", 2,
	    "All", 3,
	    "One", 4,
	    GTK_STOCK_CANCEL, 5,
	    NULL
	); 
    else
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL, 
	    "Next", 1,
	    GTK_STOCK_CANCEL, 5,
	    NULL
	);    

    wg0 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(wg)->vbox), wg0);
    wg1 = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
    wg1 = gtk_table_new(2,2, FALSE);
    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);

    if(flg){
	wg2 = gtk_label_new("Find: ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 0,1, GTK_FILL,0, 0,10);
	wg2 = gtk_label_new(find);
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 0,1, GTK_FILL,0, 0,10);

	wg2 = gtk_label_new("Replace to: ");	
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 1,2, GTK_FILL,0, 0,10);
	wg2 = gtk_label_new(repl);
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 1,2, GTK_FILL,0, 0,10);
    } else {
	wg2 = gtk_label_new("Find: ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 0,2, GTK_FILL,0, 0,10);
	wg2 = gtk_label_new(find);
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 0,2, GTK_FILL,0, 0,10);
    }

    gtk_widget_show_all(wg);
    x = gtk_dialog_run(GTK_DIALOG(wg));
    gtk_widget_destroy(wg);

    return x;
}

static char *gui_str_to_memory(GuiBineditor *be, const char *rpl, int *pic)
{
    GtkWidget *wg, *wg0, *wg1, *wg2;
    char tdg[8];
    char *tmp = NULL, *stack;
    int len = strlen(rpl);
    int i = 0, dgcn = 0, x = 0, hex = 0;
    
    char fg_str = 0; /* znaleziono lancuch */
    char fg_bck = 0; /* znak poprzedzony '\' */
    char *fg_error = NULL;
    char n, *z, *p;
        
    if(!(tmp = (char *)malloc(sizeof(char) * len + 1))) return NULL;
    *pic = 0;    
    stack = tmp;
    
    for(i = 0; i < len+1 && !fg_error; i++){
	n = rpl[i];
	if(dgcn > 4) fg_error = "Digit too large, only 4 digits allowed (eg 0xff)";
	switch(n){
	    case '"' : if(!fg_bck){
			    fg_str = !fg_str;
		    	    if(!fg_str) { dgcn = 0; hex =0; }
			    continue;
			}
	               break;
	    case '\\': if(!fg_str) fg_error = "Illegal character occured: '\\'"; 
		       if(!fg_bck) fg_bck = 2; else break;
		       continue;
	    case   0:
	    case ' ':  if(fg_str) break;
		       if(dgcn){
		    	  tdg[dgcn] = 0;
			  z = strchr(tdg, 'x');
			  if(((z - tdg) > 1) && z) { fg_error = "Illegal digit base"; continue; }
			  p = z;
			  z = z ? z + 1 : tdg;
			  x = strtol(z, NULL, p ? 16:10);
			  if((x > 255) || (x < 0)) { fg_error = "Only digit in range 0..255 allowed"; continue; }
			  if(p)
			    if(strchr(p + 1, 'x'))  { fg_error = "More then one 'x' in digit string occured"; continue; }
			  *stack++ = x;			  
		       }
	               dgcn = 0;
		       hex = 0;
		       continue;
	}
	if(n == 0) break;
	if(fg_str){
	    *stack++ = n;
	    if(fg_bck) fg_bck--;
	    continue;
	}
	if(dgcn == 0) memset(tdg, 0, 5);
	n = tolower(n);
	if(((n >= '0') && (n <= '9')) || ((n >= 'a') && ( n <= 'f')) || (n == 'x')){
	    if(n == 'x') hex++;
	    if((n >= 'a') && ( n <= 'f') && !hex) fg_error = "Illegal character, to hex digit use 0x??";
	    tdg[dgcn++] = n;	
	}else
	    fg_error = "Illegal character";
    }
    if(fg_str) fg_error = "Missing ending \" in input";
    if(fg_error){
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
	    GTK_STOCK_OK, 1,
	    NULL
	);    
	wg0 = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(wg)->vbox), wg0);
	wg1 = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
	wg1 = gtk_table_new(2,2, FALSE);
	gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);

	wg2 = gtk_label_new("Input str: ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 0,1, GTK_FILL,0, 0,10);

	wg2 = gtk_label_new(rpl);
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 0,1, GTK_FILL,0, 0,10);

	wg2 = gtk_label_new("Error: ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 1,2, GTK_FILL,0, 0,10);

	wg2 = gtk_label_new(fg_error);
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 1,2, GTK_FILL,0, 0,10);
	g_signal_connect_swapped(GTK_OBJECT(wg), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(wg));    
	gtk_widget_show_all(wg);
	gtk_dialog_run(GTK_DIALOG(wg));
	free(tmp);
	return NULL;
    }    
    *pic = stack - tmp;
    *stack = 0;
    return tmp;
}

static void gui_bineditor_search(GuiBineditor *be, const char *find, const char *repl, int settings)
{
    GtkWidget *wg, *wg0, *wg1, *wg2;
    int addr = 0, inc = 0, pic = 0, i;
    char *rpl = NULL, *fnd = NULL, exit;

    if(!*find) return;

    g_return_if_fail(be->vfind == NULL);
    be->vfind = (char*) malloc(sizeof(char) * strlen(find) + 1);
    g_return_if_fail(be->vfind != NULL);
    strcpy(be->vfind, find);

    if(*repl){
	g_return_if_fail(be->vreplace == NULL);
	be->vreplace = (char*) malloc(sizeof(char) * strlen(repl) + 1);
	g_return_if_fail(be->vreplace != NULL);
	strcpy(be->vreplace, repl);
    }

    if(*repl) rpl = gui_str_to_memory(be, repl, &pic);
    fnd = gui_str_to_memory(be, find, &inc);

    if(fnd){
/* szukaj i ew zastap */
	if(settings & 1) /* jesli ma byc case-insensitive to lancuch porownywany ma miec male litery */
	    for(i=0; i < inc; i++) fnd[i] = tolower(fnd[i]);
	if(settings & 2) addr = be->address_mark;
	if(*repl) settings &= ~8;
	for(exit = 1; exit; addr += inc){
	    if((addr = gui_bineditor_search_lo(be, fnd, addr, settings & 1, inc)) == -1) break;
	    be->address_hl_start = addr;
	    be->address_hl_end = addr + inc;
	    if(be->address_hl_end > be->adj->value + be->adj->page_size) be->adj->value = addr - (addr % be->grid_cols);
	    gtk_widget_queue_draw(be->drawing_area);
	    if(!*repl || !(settings & 8)){
		switch( gui_bineditor_dialog_search(be, *repl, find, repl) ){
/*cancel*/	    case 5:
		    case GTK_RESPONSE_NONE: exit = 0; continue;
/*one*/		    case 4: exit = 0; break;
/*replace*/	    case 1: break;
/*skip*/	    case 2: continue;
/*all*/		    case 3: settings |= 8; break;
		}
	    }
	    if(*repl && pic){
		memcpy(be->buffer + addr, rpl, pic);
		gui_bineditor_emit_signal(be);
	    }
	}
/* finalizacja */
	if(addr == -1){
	    wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_STOCK_OK, 1,
		NULL
	    );    
	    wg0 = gtk_hbox_new(FALSE, 0);
	    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
	    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(wg)->vbox), wg0);
	    wg1 = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
	    wg2 = gtk_label_new("No more matches");
	    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	    gtk_box_pack_start(GTK_BOX(wg0), wg2, FALSE, FALSE, 0);
	    g_signal_connect_swapped(GTK_OBJECT(wg), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(wg));    
	    gtk_widget_show_all(wg);
	    gtk_dialog_run(GTK_DIALOG(wg));
	}
    }
    if(rpl) free(rpl);
    if(fnd) free(fnd);    
}

static void gui_bineditor_find_ok(GtkWidget *wg, GtkWidget **wgg)
{
    int settings = 0;
    const char *find;
    const char *repl;
    char *rfind = NULL, *rrepl = NULL;
    
    find = gtk_entry_get_text((GtkEntry*)wgg[2]);
    repl = gtk_entry_get_text((GtkEntry*)wgg[3]);
    settings |= GTK_TOGGLE_BUTTON(wgg[4])->active ? 1:0; /* case insensitive */
    settings |= GTK_TOGGLE_BUTTON(wgg[5])->active ? 2:0; /* start from marked */
    settings |= GTK_TOGGLE_BUTTON(wgg[6])->active ? 4:0; /* dont ask */
    settings |= GTK_TOGGLE_BUTTON(wgg[7])->active ? 8:0; /* find all */

    if(!(rfind = (char*)malloc(strlen(find) + 1))){
	perror("{bineditor.c} static gui_bineditor_find_ok() ---> allocation error for rfind !!!\n");
	gui_bineditor_kill(wg, wgg[0]);        
	return;
    } 
    if(!(rrepl = (char*)malloc(strlen(repl) + 1))){
	perror("{bineditor.c} static gui_bineditor_find_ok() ---> allocation error for rrepl !!!\n");
	free(rfind);
	gui_bineditor_kill(wg, wgg[0]);        
	return;
    }
    strcpy(rfind, find);
    strcpy(rrepl, repl);    

    gui_bineditor_kill(wg, wgg[0]);        
    gui_bineditor_search((GuiBineditor *)wgg[1], rfind, rrepl, settings);

    if(rfind) free(rfind);
    if(rrepl) free(rrepl);
}

static void gui_bineditor_find_string(GtkWidget *wg, GuiBineditor *be)
{
    static GtkWidget *wgg[8];
    GtkWidget *wg0, *wg1, *wg2, *wg3;
        
    /* usuniecie zaznaczenia */
    be->address_hl_start = -1;
    be->address_hl_end = -1;
    gtk_widget_queue_draw(be->drawing_area);

    if(!be->buffer) return;
    
    wg0 = gtk_window_new(0);
    gtk_window_set_modal(GTK_WINDOW(wg0), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(wg0), FALSE);
    gtk_container_border_width(GTK_CONTAINER(wg0), 10);
    wgg[0] = wg0;    
    wgg[1] = (GtkWidget *)be;
    wg1 = gtk_table_new(3,6, FALSE);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    
    /* find */
    wg3 = gtk_vbox_new(FALSE,0);
    gtk_container_border_width(GTK_CONTAINER(wg3), 5);
    gtk_table_attach(GTK_TABLE(wg1), wg3, 0,3, 0,1, GTK_FILL,0, 0,0);
    wg2 = gtk_label_new("Find ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);
    wg2 = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);    
    wgg[2] = wg2;

    if(be->vfind){
	gtk_entry_set_text(GTK_ENTRY(wg2), be->vfind);
	free(be->vfind);
	be->vfind = NULL;
    }

    /* replace */
    if(be->properties & GUI_BINEDITOR_PROP_EDITABLE){
	wg3 = gtk_vbox_new(FALSE,0);
	gtk_container_border_width(GTK_CONTAINER(wg3), 5);
	gtk_table_attach(GTK_TABLE(wg1), wg3, 0,3, 1,2, GTK_FILL,0, 0,0);
	wg2 = gtk_label_new("Replace ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);
	wg2 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);    
	wgg[3] = wg2;
    }

    if(be->vreplace){
	gtk_entry_set_text(GTK_ENTRY(wg2), be->vreplace);
	free(be->vreplace);
	be->vreplace = NULL;
    }

    /* ignorowanie wilekosci znaku ASCII */
    wg2 = gtk_check_button_new_with_label("Case insensitive");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 2,3, GTK_FILL,0, 0,0);    
    wgg[4] = wg2;
    
    /* szukanie od pozycji markera */
    wg2 = gtk_check_button_new_with_label("Start from marked");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 3,4, GTK_FILL,0, 0,0);    
    wgg[5] = wg2;

    if(be->properties & GUI_BINEDITOR_PROP_EDITABLE){
	/* Podmien wszystkie bez pytaniaę */
	wg2 = gtk_check_button_new_with_label("Dont't ask");
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 2,3, GTK_FILL,0, 0,0);    
	wgg[6] = wg2;    
	/* Znajdx wszystkie  */
	wg2 = gtk_check_button_new_with_label("Find all");
	gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 3,4, GTK_FILL,0, 0,0);    
        wgg[7] = wg2;
    }

    wg2 = gtk_button_new_with_label("Cancel");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    gtk_signal_connect(GTK_OBJECT(wg2), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_kill), wg0);

    wg2 = gtk_button_new_with_label("  OK  ");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 2,3, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    gtk_signal_connect(GTK_OBJECT(wg2), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_find_ok), wgg);

    gtk_widget_show_all(wg0);
}

/******************************************************************************************************************/
/*
static void gui_bineditor_editor(GuiBineditor *be, int addr)
{
    GtkWidget *dlg, *wg0;
    char tmp[8];
    const char *text;
    int len = 0, i;
    char *rpl;
    
    g_return_if_fail(be->buffer != NULL);
    g_return_if_fail(be->buffer_size != 0);
    g_return_if_fail(addr < be->buffer_size);    
    dlg = gtk_dialog_new_with_buttons("Edit cell", GTK_WINDOW(be->wmain), GTK_DIALOG_MODAL, 
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
    );

    wg0 = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dlg)->vbox), wg0);
    sprintf(tmp,"0x%02X", be->buffer[addr]);
    gtk_entry_set_text(GTK_ENTRY(wg0), tmp);

    gtk_widget_show_all(dlg);
    if(gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT){
	text = gtk_entry_get_text(GTK_ENTRY(wg0));
	g_return_if_fail(text != NULL);
	rpl = NULL;
	if(*text) rpl = gui_str_to_memory(be, text, &len);
	g_return_if_fail(rpl != NULL);

	for(i = 0; (i < len) && ((i + addr) < be->buffer_size); i++)
						be->buffer[i + addr] = rpl[i];

	if(rpl) free(rpl);
	if(len) gui_bineditor_emit_signal(be);
    }
    gtk_widget_destroy(dlg);

}
*/
/******************************************************************************************************************/
static void gui_bineditor_jump_marker(GtkWidget *wg, GuiBineditor *be)
{
    be->address_mark_redo = be->adj->value;
    be->adj->value = be->address_mark;
    gtk_widget_set_sensitive(be->rjmp, TRUE);
    gtk_adjustment_value_changed(be->adj);    
}

static void gui_bineditor_redo_marker(GtkWidget *wg, GuiBineditor *be)
{
    be->adj->value = be->address_mark_redo;
    gtk_widget_set_sensitive(be->rjmp, FALSE);
    gtk_adjustment_value_changed(be->adj);    
}

static void gui_bineditor_clear_clc(GtkWidget *wg, GtkWidget **wgg)
{
    int s,e,i;
    
    if(clear_guard) return;
    clear_guard = 1;
    s = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[0]));
    e = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[1]));
    
    if(s > e){
	 s = e;
	 gtk_spin_button_set_value(GTK_SPIN_BUTTON(wgg[0]), s);
	 gtk_spin_button_set_value(GTK_SPIN_BUTTON(wgg[1]), e);
    }

    i = e - s;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(wgg[2]), i + 1);    
    clear_guard = 0;
}

/* cos nie chce ustawiac */
static void gui_bineditor_clear_cli(GtkWidget *wg, GtkWidget **wgg)
{
    int s,e;

    if(clear_guard) return;
    clear_guard = 1;

    s = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[0]));    
    e = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[3]));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(wgg[1]), s + e);

    clear_guard = 0;
}

static void gui_bineditor_clear_ok(GtkWidget *wg, GtkWidget **wgg)
{
    int s,i,d;
    GuiBineditor *be = (GuiBineditor*)wgg[4];

    s = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[0]));    
    i = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[2]));    
    d = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[3]));

    g_return_if_fail(be->buffer != NULL);

    memset(be->buffer + s, d, i);
    gui_bineditor_emit_signal(be);
    gtk_widget_destroy(wgg[5]);
    gtk_widget_queue_draw(be->drawing_area);
}

static void gui_bineditor_clear_buffer(GtkWidget *bt, GuiBineditor *be)
{
    static GtkWidget *wgg[6];
    GtkWidget *wg0, *wg1, *wg2;
    GtkObject *adj;
        
    if(!be->buffer) return;
    
    wg0 = gtk_window_new(0);
    gtk_window_set_modal(GTK_WINDOW(wg0), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(wg0), FALSE);
    gtk_container_border_width(GTK_CONTAINER(wg0), 10);
    wgg[4] = (GtkWidget *)be;
    wgg[5] = wg0;    
    
    wg1 = gtk_table_new(3,4, FALSE);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    
    wg2 = gtk_label_new("Start address ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 0,1, GTK_FILL,0, 0,0);
    wg2 = gtk_label_new("Stop address ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 1,2, GTK_FILL,0, 0,0);
    wg2 = gtk_label_new("Repetition ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 2,3, GTK_FILL,0, 0,0);
    wg2 = gtk_label_new("Filling byte ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 3,4, GTK_FILL,0, 0,0);

    adj = gtk_adjustment_new(0, 0, be->buffer_size, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 0,1, GTK_FILL | GTK_EXPAND, 0,0,0);
    gtk_signal_connect(GTK_OBJECT(wg2), "changed", GTK_SIGNAL_FUNC(gui_bineditor_clear_clc), wgg);
    wgg[0] = wg2;

    adj = gtk_adjustment_new(be->buffer_size, 0, be->buffer_size, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 1,2, GTK_FILL | GTK_EXPAND, 0,0,0);
    gtk_signal_connect(GTK_OBJECT(wg2), "changed", GTK_SIGNAL_FUNC(gui_bineditor_clear_clc), wgg);
    wgg[1] = wg2;

    adj = gtk_adjustment_new(be->buffer_size, 0, be->buffer_size, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 2,3, GTK_FILL | GTK_EXPAND, 0,0,0);
    gtk_signal_connect(GTK_OBJECT(wg2), "changed", GTK_SIGNAL_FUNC(gui_bineditor_clear_cli), wgg);
    wgg[2] = wg2;

    adj = gtk_adjustment_new(0, 0, 255, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 3,4, GTK_FILL | GTK_EXPAND, 0,0,0);
    wgg[3] = wg2;

    wg2 = gtk_button_new_with_label("Cancel");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    gtk_signal_connect(GTK_OBJECT(wg2), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_kill), wg0);

    wg2 = gtk_button_new_with_label("  OK  ");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 2,3, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    gtk_signal_connect(GTK_OBJECT(wg2), "clicked", GTK_SIGNAL_FUNC(gui_bineditor_clear_ok), wgg);
    
    gtk_widget_show_all(wg0);

}

/**************************************************************************************************************************/

#define SET_COLOR(be, idx, r,g,b)	be->colors[idx * 3] = r; be->colors[idx * 3 + 1] = g; be->colors[idx * 3 + 2] = b
#define GET_COLOR(be, idx)		be->colors[idx * 3], be->colors[idx * 3 + 1], be->colors[idx * 3 + 2]

void gui_bineditor_statusbar(GuiBineditor *be, char *tmp, char *str, ...)
{	
    va_list v;
        
    va_start(v, str);
    vsprintf(tmp, str, v);
    va_end(v);

    if(!be->statusbar) return;
    gtk_statusbar_pop(GTK_STATUSBAR(be->statusbar), be->statusbar_id);
    gtk_statusbar_push(GTK_STATUSBAR(be->statusbar), be->statusbar_id, tmp);
}

static char gui_dig2hex(char i){
    return i + (i < 10 ? '0'  : 'A' - 10 );
}

static int gui_bineditor_get_grid_addr(GuiBineditor *be, int xi, int yi, char *ascii_grid)
{
    int x, y, xa, address;
    
    x = xi - be->grid_start; xa = xi - be->ascii_start;
    y = yi - be->grid_top;
    *ascii_grid = 0;
    
    /* nie dotyczy pola komorek hex to wyjdz */
    if(x < 0 || y < 0 || xi > be->grid_end || !be->cell_width || !be->cell_height){
	if(xa < 0 || y < 0 || xi > be->ascii_end || !be->cell_width || !be->cell_height) return -1;
	
	xa /= be->ascii_space;
	y /= be->cell_height + 1; 

	address = y * be->grid_cols + xa + be->adj->value;
	*ascii_grid = 1;

	return address;
    }

    x /= be->cell_width;
    y /= be->cell_height + 1; 

    address = y * be->grid_cols + x + be->adj->value;
    return address;
}

static void gui_bineditor_exit_edit(GuiBineditor *be)
{
    be->edit_hex = 0;
    be->address_hl_start = -1;
    be->address_hl_end = -1;
    gtk_widget_set_sensitive(be->clear, be->clear_sens);
    gtk_widget_set_sensitive(be->mjmp, be->mjmp_sens);
    gtk_widget_set_sensitive(be->rjmp, be->rjmp_sens);
    gtk_widget_set_sensitive(be->find, be->find_sens);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->print, be->print_sens);
#endif
    gtk_widget_set_sensitive(be->chksum, be->chksum_sens);
}

static void gui_bineditor_off_edit(GuiBineditor *be)
{
    be->clear_sens = gtk_widget_get_sensitive(be->clear);
    gtk_widget_set_sensitive(be->clear, 0);
    be->mjmp_sens = gtk_widget_get_sensitive(be->mjmp);
    gtk_widget_set_sensitive(be->mjmp, 0);
    be->rjmp_sens = gtk_widget_get_sensitive(be->rjmp);
    gtk_widget_set_sensitive(be->rjmp, 0);
    be->find_sens = gtk_widget_get_sensitive(be->find);
    gtk_widget_set_sensitive(be->find, 0);
#ifndef NO_PRINTER_SUPPORT
    be->print_sens = gtk_widget_get_sensitive(be->print);
    gtk_widget_set_sensitive(be->print, 0);
#endif
    be->chksum_sens = gtk_widget_get_sensitive(be->chksum);
    gtk_widget_set_sensitive(be->chksum, 0);
}

static void gui_bineditor_mbutton(GtkWidget *wg, GdkEventButton *ev, GuiBineditor *be)
{
    int address;
    char ascii = 0;

//    if((ev->button != 1) && (ev->button != 3)) return;
    if(!be->buffer || !be->buffer_size) return;
    address = gui_bineditor_get_grid_addr(be, ev->x, ev->y, &ascii);
    if(address < 0) return;

    if(ev->button == 3){
	be->address_mark = address;
        gtk_widget_queue_draw(be->drawing_area);
	return;
    }

    if(ev->button != 1) return;

    if( ev->button == 1){
	if(be->edit_hex == 0){
	    be->edit_hex = ascii ? 2 : 1;
	    be->edit_hex_cursor = 0;
	    be->address_hl_start = address;
	    be->address_hl_end = address;
	    gui_bineditor_off_edit( be );
	} else {
	    gui_bineditor_exit_edit( be );
	}
	gtk_widget_queue_draw(be->drawing_area);
    }
}

static void gui_bineditor_hint(GtkWidget *wg, GdkEventMotion *ev, GuiBineditor *be)
{
    char tmp[256];
    int address;
    int data, offs;
    char ascii = 0;
    
    if(!be->buffer || !be->buffer_size) return;
    address = gui_bineditor_get_grid_addr(be, ev->x, ev->y, &ascii);
    if(address < 0){
	gui_bineditor_statusbar(be, tmp, "");
        return;
    }
    data = be->buffer[address];
    if(be->address_old_hint == address) return;
    if(address >= be->buffer_size){
	gui_bineditor_statusbar(be, tmp, "");
	return;
    }
    be->address_old_hint = address;
    offs = address - be->address_mark;
//    gui_bineditor_statusbar(be, tmp, " Address: 0x%x;   Data hex: 0x%x   Data dec: %i  Data ASCII: '%c'  |"
//    " Mark address: 0x%x  Mark offset hex: %c0x%x, dec: %i",
//	address, data, data, ((data > 0x1f) && (data < 0x80)) ? data : '.',
//	be->address_mark, offs < 0 ? '-':' ', abs(offs), offs
//    );

    sprintf(tmp, " %x:%x(%i) ", address, data & 0xff, data & 0xff);
    gtk_label_set_text(GTK_LABEL(be->info_addr), tmp);

    sprintf(tmp, " %x:%c%x(%i) ", be->address_mark, offs < 0 ? '-':' ',abs(offs), offs );
    gtk_label_set_text(GTK_LABEL(be->info_mark), tmp);

}

static void gui_bineditor_leave(GtkWidget *wg, GdkEventCrossing *ev, GuiBineditor *be)
{
    char tmp[2];
    /* oczyszczenie pola statusu */
    gui_bineditor_statusbar(be, tmp, "");
    gdk_window_set_cursor(wg->window, NULL);
}

static void gui_bineditor_enter(GtkWidget *wg, GdkEventCrossing *ev, GuiBineditor *be)
{
    GdkCursor *cursor;
    cursor = gdk_cursor_new(GDK_HAND2);
    gdk_window_set_cursor(wg->window, cursor);
    gdk_cursor_destroy(cursor);
}

//static void gui_bineditor_scroll(GtkWidget *wg, GdkEventScroll *ev, GuiBineditor *be)
//{
//printf("aaa\n");
//return;
//    int adj = gtk_adjustment_get_value(be->adj);
//    
//    if( ev->direction == GDK_SCROLL_UP ){
//	adj -= be->grid_cols;
//	if(adj < 0) adj = 0;
//    }
//    if( ev->direction == GDK_SCROLL_DOWN ){
//	adj += be->grid_cols;
//	if(adj >= be->buffer_size) adj = be->buffer_size - be->grid_cols;
//    }
//    adj = (adj / be->grid_cols) * be->grid_cols;
//    gtk_adjustment_set_value(be->adj, adj);
/*
    int x = gtk_adjustment_get_value(be->adj);
    
    if(ev->direction == GDK_SCROLL_UP) x -= be->grid_cols * 2;
    if(ev->direction == GDK_SCROLL_DOWN) x += be->grid_cols * 2;

    x = x - (x % be->grid_cols);

    if( be->adj->upper - x <= be->adj->page_size )
	x = be->adj->upper - be->adj->page_size;	
    
    gtk_adjustment_set_value(be->adj, x);
*/

//}

static void gui_bineditor_slider(GtkAdjustment *wig, GuiBineditor *wg, GuiBineditor *be)
{
    gtk_widget_queue_draw(wg->drawing_area);
}

static  void gui_cairo_line(cairo_t *cr, int x0, int y0, int x1, int y1)
{
    cairo_move_to(cr, x0, y0);
    cairo_line_to(cr, x1, y1);
}

static void gui_cairo_outtext(cairo_t *cr, char *tmp, int x, int y, char *str, ...)
{
    va_list varg;

    va_start(varg, str);
    vsprintf(tmp, str, varg);
    va_end(varg);
    
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, tmp);
}

static void gui_bineditor_set_hl(cairo_t *cr, GuiBineditor *be, char mrk, char nc)
{
    if( !nc ){
	cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL)); 
	return;
    }
    if(mrk)
	cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_HL_MRK));
    else
	cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_HL));
}

/* poprawic dla wydruku */
static void gui_bineditor_draw(cairo_t *cr, GuiBineditor *be, int vxx, int vyy, char print)
{
    char tmp[16], rg, mrk, hl=0;
    int i, j,kx, ky, xc, yc, cx, zx, xam, addr, xa, z, max_addr, xx, yy, n, pagesize, bl_pos;
    
    /* obliczenie pozycji i parametrow wyswietlania w zalezniosci od rozmiarow okna */
#define nx	2	/* ilosc znakow w celi */
#define border	4	/* piksele otaczajace znak */
#define fx	6 	/* szerokosc fontu */
#define fy	10	/* wysokosc fontu */
#define ac	8 	/* ilosc cyfr adresu */
#define ry	26	/* szerokosc belki */
#define underln_pos 3   /* pozycja pod znakiem */

    kx = fx + border;
    ky = fy + border;
    /* szerokosc pola adresu */
    zx = (ac + 2) * kx;
    cx = nx * kx;
    /* ilosci cel */
    xc = (((vxx - zx - 2*kx) / kx) / 3);
    yc = (vyy - ry) / (ky + 1);
    xam = (vxx + zx + kx + xc * (cx - kx)) / 2;
    pagesize = xc * yc;

    if(!print){
	be->cell_width  = cx; 
	be->cell_height = ky;
	be->grid_start  = zx;
	be->grid_end    = xam - cx;
        be->grid_cols   = xc;
	be->grid_rows   = yc;
	be->grid_top    = ry;
	be->ascii_start = xam;
	be->ascii_end   = xam + kx * xc;
	be->ascii_space = kx;
		
	be->adj->page_size = pagesize;
	be->adj->page_increment = xc;
	be->adj->step_increment = xc;
    }

    max_addr = be->buffer_size;
    addr = be->adj->value / xc;
    addr *= xc;

    /* ustawienie parametrów fontu */
    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);

    /* czyszczenie ekranu */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_GRID_BG));
    cairo_rectangle(cr, 0, 0, vxx, vyy);
    cairo_fill(cr);
    cairo_stroke(cr);    
    /* górna belka */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_DESC_BG));
    cairo_rectangle(cr, 0, 0, vxx, ky + 5);
    cairo_fill(cr);
    cairo_stroke(cr);    

    /* narysowanie siatki */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_GRID));    
    for(i = 0, yy = ry + ky; i < yc; i++, yy += ky + 1) gui_cairo_line(cr, 0, yy, vxx, yy);
    for(i = 0, xx = zx; i < xc + 1; i++, xx += cx ) gui_cairo_line(cr, xx, 0, xx, vyy);
    cairo_stroke(cr);

    /* opis belki */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_DESC));    
    for(i = 0, xx = zx; i < xc; i++, xx += cx)
	gui_cairo_outtext(cr, tmp, xx + 1 , 4 + fy, "%c%c", gui_dig2hex(i / 16), gui_dig2hex(i % 16));
    gui_cairo_outtext(cr, tmp, border , 4 + fy, "Address");
    gui_cairo_outtext(cr, tmp, xam , 4 + fy, "ASCII");
    cairo_stroke(cr);

    /* wypelnienie cel */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL));    
    for(j = 0, yy = ry + fy; (j < yc) && (addr < max_addr); j++, yy += ky + 1){
	for(z = 0; z < 8; z++) 
	    gui_cairo_outtext(cr, tmp, kx*(8-z), yy, "%c", gui_dig2hex( (addr >> (z*4)) & 0xf));

	for(i = 0, xx = zx, xa = xam; (i < xc) && (addr < max_addr); i++, xx += cx, xa += kx, addr++ ){
	    n = 0;
	    if(be->buffer) n = be->buffer[addr];
	    mrk = be->address_mark == addr;
	    hl  = (addr >= be->address_hl_start) && (addr <= be->address_hl_end);
	    bl_pos = (be->edit_hex_cursor & 1) * (fx + 2);
	    if(mrk){
		cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_MARKED_BG));
	        cairo_rectangle(cr, xx, yy - fy, cx, ky - 1);
	        cairo_rectangle(cr, xa, yy - fy, kx - 1, ky - 1);
		cairo_fill(cr);
		cairo_stroke(cr);
		cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_MARKED));
		cairo_stroke(cr);
	    }

	    rg = (n > 0x1f) && (n < 0x80);

	    if(hl){
		gui_bineditor_set_hl( cr, be, mrk, 1);
		if( be->edit_hex != 2)
	    	    cairo_rectangle(cr, xx, yy - fy, cx, ky - 1);
		if( be->edit_hex != 1)
	    	    cairo_rectangle(cr, xa, yy - fy, kx - 1, ky - 1);
		cairo_fill(cr);
		cairo_stroke(cr);
		cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL));
	    }	    

	    gui_cairo_outtext(cr, tmp, xx + 1, yy , "%c%c", gui_dig2hex(n / 16), gui_dig2hex(n % 16));

	    if(!rg){
		if((!mrk && !hl) || (( be->edit_hex == 1) && hl)){
		    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_UD));
	    	    cairo_rectangle(cr, xa, yy - fy, kx - 1, ky - 1);
		    cairo_fill(cr);
		    cairo_stroke(cr);
		}
	    } else
		gui_cairo_outtext(cr, tmp, xa, yy, "%c", n);
	    
	    if(mrk || !rg || hl){
		cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL));
		cairo_stroke(cr);
	    }
	    
	    // underline
	    if( be->edit_hex && hl){
		if( be->edit_hex != 2)
		    gui_cairo_line(cr, xx + 1 + bl_pos, yy + underln_pos, xx + 1 + fx + 1 + bl_pos, yy + underln_pos);
		if( be->edit_hex != 1)
		    gui_cairo_line(cr, xa + 1 + bl_pos, yy + underln_pos, xa + 1 + fx + 1 + bl_pos, yy + underln_pos);
		cairo_stroke(cr);
	    }
	}
    }
    cairo_stroke(cr);
}

static gboolean gui_bineditor_expose(GtkWidget *wg, GdkEventExpose *ev, GuiBineditor *be)
{
    cairo_t  *cr;

    gtk_widget_grab_focus(wg);
    cr = gdk_cairo_create(wg->window);
    cairo_rectangle(cr, ev->area.x, ev->area.y, ev->area.width, ev->area.height);
    cairo_clip(cr);

    be->rfsh = 1;
    gui_bineditor_draw(cr, be, wg->allocation.width, wg->allocation.height, 0);

    cairo_destroy(cr);
    return FALSE;
}

static void gui_bineditor_configure(GtkWidget *wg, GdkEventExpose *ev, GuiBineditor *be)
{
    /* mozna to w przyszlosci zmienic, teraz przy resize suwak bedzie ustawiany na 0 */
    be->adj->value = 0;
    gtk_adjustment_value_changed(be->adj);        
}

static void gui_bineditor_cursor_decrement(GuiBineditor *be, int v )
{
    if( be->address_hl_start < v){
	be->edit_hex_cursor = 0;	
        return;
    }
    if(((be->edit_hex_cursor -= v) < 0) || (be->edit_hex == 2)){
	if(v == 1) 
	    be->edit_hex_cursor = 1;
	else
	    be->edit_hex_cursor &= 1;
	if( (be->edit_hex == 2) || ( be->edit_hex_cursor < 0) ) be->edit_hex_cursor = 0;
	be->address_hl_start -= v;
	be->address_hl_end -=v;
	if(be->address_hl_start < 0) be->address_hl_start = 0;
	if(be->address_hl_end < 0) be->address_hl_end = 0;
    }
}

static void gui_bineditor_cursor_increment(GuiBineditor *be, int v )
{
    if(((be->edit_hex_cursor += v) > 1) || (be->edit_hex == 2)){
	if((v == 1) || (be->edit_hex == 2)) 
	    be->edit_hex_cursor = 0;
	else
	    be->edit_hex_cursor &= 1;
	be->address_hl_start += v;
	be->address_hl_end +=v;
	if(be->address_hl_start >= be->buffer_size) be->address_hl_start -= v;
	if(be->address_hl_end >= be->buffer_size) be->address_hl_end -= v;
    }
}

static void gui_bineditor_edit(GuiBineditor *be, int key)
{
    int val;
    if(be->edit_hex == 1){
	if((key >= 'A') && (key <= 'F')) key = key - 'A' + 'a';
	if(!(((key >= '0') && (key <= '9')) || ((key >= 'a') && (key <= 'f')))) return;
	val = key - '0';
	if((key >= 'a') && (key <= 'f')) val = 10 + key - 'a';
	val &= 0x0f;
	if(be->edit_hex_cursor == 0)
	    be->buffer[ be->address_hl_start] = (be->buffer[ be->address_hl_start] & 0x0f) | (val << 4);
	else
	    be->buffer[ be->address_hl_start] = (be->buffer[ be->address_hl_start] & 0xf0) | val;
    } else
	be->buffer[ be->address_hl_start] = key;
    gui_bineditor_cursor_increment( be, 1 );
}

static void gui_bineditor_keystroke(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    int key = ev->keyval;
    int adj;
    
    if( be->edit_hex == 0 ) return; // ignore if edit mode not active
    
    // cursor
    switch(key){
	case GDK_KEY_Left:  gui_bineditor_cursor_decrement( be, 1 ); break;
	case GDK_KEY_Right: gui_bineditor_cursor_increment( be, 1 ); break;
	case GDK_KEY_Up:    gui_bineditor_cursor_decrement( be, be->grid_cols ); break;
	case GDK_KEY_Down:  gui_bineditor_cursor_increment( be, be->grid_cols ); break;
	case GDK_KEY_Home:  be->edit_hex_cursor = 0;
			    be->address_hl_start = be->address_hl_end = (be->address_hl_start / be->grid_cols) * be->grid_cols;
			    break;
	case GDK_KEY_End:   be->edit_hex_cursor = 0;
			    be->address_hl_start = ((be->address_hl_start / be->grid_cols) + 1) * be->grid_cols - 1;
			    if(be->address_hl_start >= be->buffer_size) be->address_hl_start = be->buffer_size - 1;
			    be->address_hl_end = be->address_hl_start;
			    break;
/*
	case GDK_KEY_Page_Up: be->edit_hex_cursor = 0;
	                      be->address_hl_start -= be->grid_cols * be->grid_rows;
	                      if(be->address_hl_start < 0) be->address_hl_start = 0;
	                      be->address_hl_start = be->address_hl_end = (be->address_hl_start / be->grid_cols) * be->grid_cols;
	                      break;
	case GDK_KEY_Page_Down: be->edit_hex_cursor = 0;
	                        be->address_hl_start += be->grid_cols * be->grid_rows - 1;
				be->address_hl_start = ((be->address_hl_start / be->grid_cols) + 1) * be->grid_cols - 1;
				if(be->address_hl_start >= be->buffer_size) be->address_hl_start = be->buffer_size - 1;
	                        be->address_hl_end = be->address_hl_start;
	                        break;
*/
	case GDK_KEY_Tab:   be->edit_hex ^= 0x03; 
			    if(be->edit_hex == 2) be->edit_hex_cursor = 0; 
			    break;
	default: if((key >= 32) && (key < 128)) gui_bineditor_edit( be, key); break;
    }

    //scroll if outside grid window
    adj = gtk_adjustment_get_value(be->adj);
    if( be->address_hl_start < adj){
	adj -= be->grid_cols;
	if(adj < 0) adj = 0;
	gtk_adjustment_set_value(be->adj, adj);
    }
    if( be->address_hl_start >= adj + be->grid_cols * be->grid_rows){
	adj += be->grid_cols;
	if(adj >= be->buffer_size) adj = be->buffer_size - be->grid_cols;
	gtk_adjustment_set_value(be->adj, adj);
    }
    gtk_widget_queue_draw(be->wmain);
}

static void gui_bineditor_focus_out(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    be->edit_hex = 0; 
    be->address_hl_end = be->address_hl_start = -1;
    gui_bineditor_exit_edit( be );
}

static void gui_bineditor_focus_in(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    be->edit_hex = 0; 
    be->address_hl_end = be->address_hl_start = -1;

    gtk_widget_set_sensitive(be->clear, TRUE);
    gtk_widget_set_sensitive(be->mjmp,  TRUE);
    gtk_widget_set_sensitive(be->find,  TRUE);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->print,  TRUE);
#endif
    gtk_widget_set_sensitive(be->rjmp,  FALSE);
    gtk_widget_set_sensitive(be->chksum, TRUE);    
}

static void gui_bineditor_init(GuiBineditor *be)
{
    GtkWidget *wg0, *wg1, *wg2;

    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));

    be->edit_hex = 0;
    be->address_mark = 0;
    be->address_mark_redo = 0;
    be->address_old_hint = -1;
    be->address_hl_start = -1;
    be->address_hl_end = -1;
    be->adj = NULL;
    be->drawing_area = NULL;
    be->tb = NULL;
    be->buffer_size = 0;
    be->buffer = NULL;    
    be->statusbar = NULL;
    be->statusbar_id = 0;
    be->properties = ~0;
    be->vfind = NULL;
    be->vreplace = NULL;
    be->rfsh = 0; /* wymus przerysowanie calosci */

    SET_COLOR(be, GUI_BINEDITOR_COLOR_UD, 1, 0.5, 0.5);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_HL, 0.8, 0.8, 0.8);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_HL_MRK, 0.6, 0.6, 0.6);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_MARKED_BG, 0.3,0.3,0.3);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_MARKED, 0, 1, 0);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL, 0, 0, 0);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_DESC, 0, 1, 1);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_GRID, 0.8, 0.8, 0.8);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_GRID_BG, 1, 1, 0.9);
    SET_COLOR(be, GUI_BINEDITOR_COLOR_DESC_BG, 0.3, 0.3, 1);

/* toolbar */
    be->tb = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(be), be->tb, FALSE, FALSE, 0);    

    /* Czyszczenie bufora */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->clear = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Clear buffer", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_clear_buffer), be);

    /* Szukaj ciagu znaków */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->find = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Find string", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_find_string), be);

    /* Suma kontrolna */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->chksum = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Calculate & insert checksum", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_checksum), be);

    /* Skocz pod marker */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->mjmp = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Jump to marker", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_jump_marker), be);

    /* Powroc */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->rjmp = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Redo jump", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_redo_marker), be);

#ifndef NO_PRINTER_SUPPORT
    /* Drukarka */
    wg1 = gtk_image_new_from_stock(GTK_STOCK_PRINT, GTK_ICON_SIZE_SMALL_TOOLBAR);
    be->print = gtk_toolbar_append_item(GTK_TOOLBAR(be->tb), NULL, "Print", 
	NULL, wg1, GTK_SIGNAL_FUNC(gui_bineditor_print), be);
#endif

/* zacieniowanie przycisków */
    gtk_widget_set_sensitive(be->clear, FALSE);
    gtk_widget_set_sensitive(be->mjmp, FALSE);
    gtk_widget_set_sensitive(be->find, FALSE);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->print, FALSE);
#endif
    gtk_widget_set_sensitive(be->rjmp, FALSE);
    gtk_widget_set_sensitive(be->chksum, FALSE);    

    /* info fields */
    wg1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(be), wg1, FALSE, FALSE, 0);
    be->info_addr = gtk_label_new("0000:00");
    wg2 = gtk_frame_new(NULL);
    gtk_widget_set_size_request( wg2, 120, 20);
    gtk_frame_set_shadow_type(GTK_FRAME(wg2), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(wg2), be->info_addr);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);
    be->info_mark = gtk_label_new("0000:00");
    wg2 = gtk_frame_new(NULL);
    gtk_widget_set_size_request( wg2, 120, 20);
    gtk_container_add(GTK_CONTAINER(wg2), be->info_mark);
    gtk_frame_set_shadow_type(GTK_FRAME(wg2), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);

/* utworzenie wklesnietego pola na pole rysunkowe CAIRO i suwak*/
    wg0 = gtk_frame_new(NULL);    
    gtk_frame_set_shadow_type(GTK_FRAME(wg0), GTK_SHADOW_IN);

    wg1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    gtk_container_add(GTK_CONTAINER(be), wg0);    

    /* dodanie suwaka */
    be->adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1, 1, 0, 0));
    wg2 = gtk_vscrollbar_new(be->adj);
    gtk_box_pack_end(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(be->adj), "value_changed", GTK_SIGNAL_FUNC(gui_bineditor_slider), be);

    /* utworzenie obszaru rysunkowego i podpięcie do niego sygnalów */
    be->drawing_area = gtk_drawing_area_new();
    GTK_WIDGET_SET_FLAGS( be->drawing_area, GTK_CAN_FOCUS);
    gtk_widget_set_events(be->drawing_area, 
	GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_MASK
	| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK
    );

    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"expose_event",GTK_SIGNAL_FUNC(gui_bineditor_expose), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"configure_event",GTK_SIGNAL_FUNC(gui_bineditor_configure), be);
//    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"scroll_event",GTK_SIGNAL_FUNC(gui_bineditor_scroll), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"button_press_event",GTK_SIGNAL_FUNC(gui_bineditor_mbutton), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"motion_notify_event",GTK_SIGNAL_FUNC(gui_bineditor_hint), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"leave_notify_event",GTK_SIGNAL_FUNC(gui_bineditor_leave), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"enter_notify_event",GTK_SIGNAL_FUNC(gui_bineditor_enter), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"key_press_event",GTK_SIGNAL_FUNC(gui_bineditor_keystroke), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"focus_out_event",GTK_SIGNAL_FUNC(gui_bineditor_focus_out), be);
    gtk_signal_connect(GTK_OBJECT(be->drawing_area),"focus_in_event",GTK_SIGNAL_FUNC(gui_bineditor_focus_in), be);
    gtk_box_pack_end(GTK_BOX(wg1), be->drawing_area, TRUE, TRUE, 1);
    gtk_widget_show_all(wg0);
}

void gui_bineditor_set_buffer(GuiBineditor *be, int bfsize, unsigned char *buffer)
{
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(buffer != NULL);

    be->buffer_size = bfsize;
    be->buffer = bfsize ? buffer: NULL;
    be->adj->lower = 0;
    be->adj->upper = bfsize;
    be->adj->page_size = 0;
    be->adj->value = 0;
    gtk_adjustment_value_changed(be->adj);
    gtk_widget_set_sensitive(be->clear, bfsize);
    gtk_widget_set_sensitive(be->mjmp, bfsize);
    gtk_widget_set_sensitive(be->find, bfsize);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->print, bfsize);
#endif
    gtk_widget_set_sensitive(be->chksum, bfsize);
/* powinno się teraz calosc przerysowac */
    be->rfsh = 0; /* wymus przerysowanie calosci */
    gtk_widget_queue_draw(be->drawing_area);
}

void gui_bineditor_set_colors(GuiBineditor *be, GuiBineditorColors color, float r, float g, float b)
{
    int tmp = color;
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(tmp < GUI_BINEDITOR_COLOR_LAST);
    
    tmp *= 3;
    be->colors[color + 0] = r;
    be->colors[color + 1] = g;
    be->colors[color + 2] = b;
}

static void gui_widget_show(GtkWidget *wg, gboolean flag)
{
    if(flag)
	gtk_widget_show(wg);
    else
	gtk_widget_hide(wg);    
}

void gui_bineditor_set_properties(GuiBineditor *be, GuiBineditorProperties pr)
{
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(be->tb != NULL);
    g_return_if_fail(GTK_IS_TOOLBAR(be->tb));

    be->properties = pr;

    gui_widget_show(be->tb,     pr & GUI_BINEDITOR_PROP_TOOLBAR);
    gui_widget_show(be->find,   pr & GUI_BINEDITOR_PROP_FIND);
    gui_widget_show(be->mjmp,   pr & GUI_BINEDITOR_PROP_MJMP);
    gui_widget_show(be->rjmp,   pr & GUI_BINEDITOR_PROP_RJMP);
    gui_widget_show(be->calc,   pr & GUI_BINEDITOR_PROP_CALC);
#ifndef NO_PRINTER_SUPPORT
    gui_widget_show(be->print,  pr & GUI_BINEDITOR_PROP_PRINT);
#endif
    gui_widget_show(be->clear,  (pr & GUI_BINEDITOR_PROP_CLEAR) && (pr & GUI_BINEDITOR_PROP_EDITABLE));
    gui_widget_show(be->chksum, (pr & GUI_BINEDITOR_PROP_CHKSUM) && (pr & GUI_BINEDITOR_PROP_EDITABLE));
}

void gui_bineditor_connect_statusbar(GuiBineditor *be, GtkWidget *sb)
{
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(sb != NULL);
    g_return_if_fail(GTK_IS_STATUSBAR(sb));
    g_return_if_fail(be->statusbar == NULL);

    be->statusbar = sb;
    be->statusbar_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), "bineditor_statusbar");
}

void gui_bineditor_redraw(GuiBineditor *be)
{
    gtk_widget_queue_draw(be->wmain);
}
