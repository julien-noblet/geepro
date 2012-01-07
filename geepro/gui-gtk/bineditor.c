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
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include "bineditor.h"
#include "checksum.h"
#include "../intl/lang.h"

#define NO_PRINTER_SUPPORT

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
static void gui_beneditor_destroy(GObject *obj);
static void gui_bineditor_draw(cairo_t *cr, GuiBineditor *be, int vxx, int vyy, char pr);

GType gui_bineditor_get_type(void)
{
    static GType bineditor_type = 0;
    if(!bineditor_type){
	static const GTypeInfo bineditor_info =
	{
	    sizeof(GuiBineditorClass),
	    NULL, // base init
	    NULL, // base finalize
	    (GClassInitFunc) gui_bineditor_class_init,
	    NULL, // class finalize
	    NULL, // class data
	    sizeof(GuiBineditor),
	    0,
	    (GInstanceInitFunc)gui_bineditor_init
	};
	bineditor_type = g_type_register_static(GTK_TYPE_BOX, "GuiBineditor", &bineditor_info, (GTypeFlags)0);
    }

    return bineditor_type;
}

static void gui_bineditor_class_init(GuiBineditorClass *cl)
{
    GObjectClass *goc = (GObjectClass*)cl;

    bineditor_signals[CHANGED] = g_signal_new(
	"changed", 
	G_TYPE_FROM_CLASS(cl), 
	G_SIGNAL_ACTION | G_SIGNAL_ACTION,
	G_STRUCT_OFFSET(GuiBineditorClass, bineditor),
	NULL, NULL,
	g_cclosure_marshal_VOID__VOID,
	G_TYPE_NONE, 0
    );
    goc->finalize = gui_beneditor_destroy;

    g_type_class_add_private(goc, sizeof(GuiBineditorPrivate) );
}

static void gui_beneditor_destroy(GObject *obj)
{
    GuiBineditor *be;

    g_return_if_fail(obj != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(obj));

    be = GUI_BINEDITOR(obj);    
    if(be->priv->vfind) free(be->priv->vfind);
    be->priv->vfind = NULL;
    if(be->priv->vreplace) free(be->priv->vreplace);
    be->priv->vreplace = NULL;
}

GtkWidget *gui_bineditor_new(GtkWindow *wmain)
{
    GtkWidget *wg;

    g_return_val_if_fail(wmain != NULL, NULL);
    g_return_val_if_fail(GTK_IS_WINDOW(wmain), NULL);

    wg = GTK_WIDGET(g_object_new(GUI_TYPE_BINEDITOR, NULL));
    ((GuiBineditor*)wg)->priv->wmain = (GtkWidget*)wmain;
    return wg;
}

static void gui_bineditor_emit_signal(GuiBineditor *wg)
{ 
    g_signal_emit(G_OBJECT(wg), 0, bineditor_signals[CHANGED]);
}

static void gui_bineditor_kill(GtkWidget *wg, GtkWidget *wgg)
{
    gtk_widget_destroy(wgg);
}

static void gui_dialog_ok(GuiBineditor *be, const char *stock_img, const char *title, const char *msg)
{
    GtkWidget *dlg, *wg0, *wg1;

    g_return_if_fail(be->priv->wmain != NULL);
    dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    wg0 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(dlg), 10);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), wg0);
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

    g_return_if_fail(be->priv->buffer != NULL);
    g_return_if_fail(be->priv->buffer_size != 0);
    
    alg = gtk_combo_box_get_active(GTK_COMBO_BOX(wgg[CHKSUM_ALGO_CBX]));
    start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_START]));
    start_sk1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_START_SK]));
    stop = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_STOP]));
    stop_sk1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_STOP_SK]));
    addr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(wgg[CHKSUM_ADDR_SB]));
    chk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_CB]));
    g_return_if_fail((alg != -1) || (start != -1) || (stop != -1) || (addr != -1) || (start_sk1 != -1) || (stop_sk1 != -1));

    switch(alg){
	case 0: bytes = 1; fmt = "0x%02X"; break; /* LRC    */
	case 1: bytes = 2; fmt = "0x%04X"; break; /* CRC-16 */
	case 2: bytes = 4; fmt = "0x%08X"; break; /* CRC-32 */
	default: bytes = 0; fmt = "----" ; break;
    }

    if(!chk) bytes = 0;
    be->priv->sum = checksum_calculate(alg, be->priv->buffer_size, be->priv->buffer, start, stop, start_sk1, stop_sk1, addr, addr + bytes);
    memset(tmp, 0, 32);
    sprintf(tmp, fmt, be->priv->sum);
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
    bo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_BO]));
    g_return_if_fail((alg != -1) || (addr != -1) || (bo != -1));

    switch(alg){
	case 0: bytes = 1; break; /* LRC    */
	case 1: bytes = 2; break; /* CRC-16 */
	case 2: bytes = 4; break; /* CRC-32 */
	default: bytes = 0;break;
    }

    for(i = 0; (i < bytes) && ((i+addr) < be->priv->buffer_size); i++)
	if(bo)
	    be->priv->buffer[addr + (bytes - i - 1)] = (be->priv->sum >> (i*8)) & 0xff;
	else
	    be->priv->buffer[addr + i] = (be->priv->sum >> (i*8)) & 0xff;

    if(bytes){
	gui_bineditor_emit_signal((GuiBineditor*)wgg[CHKSUM_GBE]);
	gtk_widget_queue_draw(be->priv->drawing_area);
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
    bo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[CHKSUM_ADDR_BO]));
    g_return_if_fail((alg != -1) || (addr != -1) || (bo != -1));    

    switch(alg){
	case 0: bytes = 1; break; /* LRC    */
	case 1: bytes = 2; break; /* CRC-16 */
	case 2: bytes = 4; break; /* CRC-32 */
	default: bytes = 0;break;
    }

    for(i = 0; (i < bytes) && ((i+addr) < be->priv->buffer_size); i++)
	if(bo){
	    if(be->priv->buffer[addr + (bytes - i - 1)] != ((be->priv->sum >> (i*8)) & 0xff)) break;
	}else{
	    if(be->priv->buffer[addr + i] != ((be->priv->sum >> (i*8)) & 0xff)) break;
	}

    if(i != bytes)
	gui_dialog_ok(be, GTK_STOCK_DIALOG_WARNING, "Message", "Checksum inconsistent.");
    else
	gui_dialog_ok(be, GTK_STOCK_DIALOG_INFO, "Message", "Checksum consistent.");
}

static void gui_bineditor_chksum_chkbox(GtkWidget *wg, GtkWidget **wgg)
{
    gtk_widget_set_sensitive(wgg[CHKSUM_ADDR_SB], gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wg)));
    gtk_widget_set_sensitive(wgg[CHKSUM_CALC_VER], FALSE);
    gtk_widget_set_sensitive(wgg[CHKSUM_CALC_WRITE], FALSE);
    gtk_entry_set_text(GTK_ENTRY(wgg[CHKSUM_CALC_ET]), "");
}

static void gui_bineditor_checksum(GtkWidget *wg, GuiBineditor *be)
{
    static GtkWidget *wgg[CHKSUM_LAST];
    GtkWidget *dialog;
    GtkWidget *tab, *wg0;
    GtkAdjustment *adj;
    
    dialog = gtk_dialog_new_with_buttons("Checksum", GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL
    );

    /* pack table */
    tab = gtk_table_new(5, 5, FALSE);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), tab);
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

    adj = gtk_adjustment_new(0, 0, be->priv->buffer_size, 1,1,0);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 0,1, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_START] = wg0;
    adj = gtk_adjustment_new(be->priv->buffer_size, 0, be->priv->buffer_size, 1,1,0);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 1,2, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_STOP] = wg0;
    adj = gtk_adjustment_new(0, 0, be->priv->buffer_size, 1,1,0);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 0,1, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_START_SK] = wg0;
    adj = gtk_adjustment_new(0, 0, be->priv->buffer_size, 1,1,0);
    wg0 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 1,2, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_STOP_SK] = wg0;

    wg0 = gtk_check_button_new_with_label("Checksum address:");
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,2, 2,3, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_ADDR_CB] = wg0;    
    g_signal_connect(G_OBJECT(wg0),"toggled", G_CALLBACK(gui_bineditor_chksum_chkbox), wgg);
    adj = gtk_adjustment_new(0, 0, be->priv->buffer_size, 1,1,0);
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
    wg0 = gtk_combo_box_text_new();
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,5, 3,4, GTK_FILL | GTK_EXPAND,0, 0,10);
    wgg[CHKSUM_ALGO_CBX] = wg0;    

    /* algorithm selection */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wg0), CHECKSUM_ALGO_LRC);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wg0), CHECKSUM_ALGO_CRC16);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wg0), CHECKSUM_ALGO_CRC32);
    gtk_combo_box_set_active(GTK_COMBO_BOX(wg0), 0);

    wg0 = gtk_button_new_with_label("Calculate");
    gtk_table_attach(GTK_TABLE(tab), wg0, 0,1, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    g_signal_connect(G_OBJECT(wg0), "clicked", G_CALLBACK(gui_bineditor_calc_checksum), wgg);
    wgg[CHKSUM_CALC_BT] = wg0;

    wg0 = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_table_attach(GTK_TABLE(tab), wg0, 1,2, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    wg0 = gtk_entry_new();

    gtk_table_attach(GTK_TABLE(tab), wg0, 2,3, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_editable_set_editable(GTK_EDITABLE(wg0), FALSE);
    wgg[CHKSUM_CALC_ET] = wg0;    

    wg0 = gtk_button_new_with_label("Verify");
    gtk_table_attach(GTK_TABLE(tab), wg0, 3,4, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_widget_set_sensitive(wg0, FALSE);
    g_signal_connect(G_OBJECT(wg0), "clicked", G_CALLBACK(gui_bineditor_checksum_verify), wgg);
    wgg[CHKSUM_CALC_VER] = wg0;

    wg0 = gtk_button_new_with_label("WRITE");
    gtk_table_attach(GTK_TABLE(tab), wg0, 4,5, 4,5, GTK_FILL | GTK_EXPAND,0, 0,10);
    gtk_widget_set_sensitive(wg0, FALSE);
    g_signal_connect(G_OBJECT(wg0), "clicked", G_CALLBACK(gui_bineditor_checksum_write), wgg);
    wgg[CHKSUM_CALC_WRITE] = wg0;

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/******************************************************************************************************************/
#ifndef NO_PRINTER_SUPPORT
/* draw printing */
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

    for(; addr < be->priv->buffer_size; addr++){
	for(x = i = 0; (i < cn) && ((addr + i) < be->priv->buffer_size); i++){
	    d = be->priv->buffer[addr + i];
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
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL, 
	    "Replace", 1,
	    "Skip", 2,
	    "All", 3,
	    "One", 4,
	    GTK_STOCK_CANCEL, 5,
	    NULL
	); 
    else
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL, 
	    "Next", 1,
	    GTK_STOCK_CANCEL, 5,
	    NULL
	);    

    wg0 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(wg))), wg0);
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
	wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
	    GTK_STOCK_OK, 1,
	    NULL
	);    
	wg0 = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(wg))), wg0);
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
	g_signal_connect_swapped(G_OBJECT(wg), "response", G_CALLBACK(gtk_widget_destroy), G_OBJECT(wg));    
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

    g_return_if_fail(be->priv->vfind == NULL);
    be->priv->vfind = (char*) malloc(sizeof(char) * strlen(find) + 1);
    g_return_if_fail(be->priv->vfind != NULL);
    strcpy(be->priv->vfind, find);

    if(*repl){
	g_return_if_fail(be->priv->vreplace == NULL);
	be->priv->vreplace = (char*) malloc(sizeof(char) * strlen(repl) + 1);
	g_return_if_fail(be->priv->vreplace != NULL);
	strcpy(be->priv->vreplace, repl);
    }

    if(*repl) rpl = gui_str_to_memory(be, repl, &pic);
    fnd = gui_str_to_memory(be, find, &inc);

    if(fnd){
/* szukaj i ew zastap */
	if(settings & 1) /* jesli ma byc case-insensitive to lancuch porownywany ma miec male litery */
	    for(i=0; i < inc; i++) fnd[i] = tolower(fnd[i]);
	if(settings & 2) addr = be->priv->address_mark;
	if(*repl) settings &= ~8;
	for(exit = 1; exit; addr += inc){
	    if((addr = gui_bineditor_search_lo(be, fnd, addr, settings & 1, inc)) == -1) break;
	    be->priv->address_hl_start = addr;
	    be->priv->address_hl_end = addr + inc;
	    if(be->priv->address_hl_end > gtk_adjustment_get_value(be->priv->adj) + gtk_adjustment_get_page_size(be->priv->adj)) 
							    gtk_adjustment_set_value(be->priv->adj, addr - (addr % be->priv->grid_cols));
	    gtk_widget_queue_draw(be->priv->drawing_area);
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
		memcpy(be->priv->buffer + addr, rpl, pic);
		gui_bineditor_emit_signal(be);
	    }
	}

	if(addr == -1){
	    wg = gtk_dialog_new_with_buttons(NULL, GTK_WINDOW(be->priv->wmain), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_STOCK_OK, 1,
		NULL
	    );    
	    wg0 = gtk_hbox_new(FALSE, 0);
	    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
	    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(wg))), wg0);
	    wg1 = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
	    wg2 = gtk_label_new("No more matches");
	    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	    gtk_box_pack_start(GTK_BOX(wg0), wg2, FALSE, FALSE, 0);
	    g_signal_connect_swapped(G_OBJECT(wg), "response", G_CALLBACK(gtk_widget_destroy), G_OBJECT(wg));    
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
    settings |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[4])) ? 1:0; /* case insensitive */
    settings |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[5])) ? 2:0; /* start from marked */
    settings |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[6])) ? 4:0; /* dont ask */
    settings |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wgg[7])) ? 8:0; /* find all */

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
    be->priv->address_hl_start = -1;
    be->priv->address_hl_end = -1;
    gtk_widget_queue_draw(be->priv->drawing_area);

    if(!be->priv->buffer) return;
    
    wg0 = gtk_window_new(0);
    gtk_window_set_modal(GTK_WINDOW(wg0), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(wg0), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
    wgg[0] = wg0;    
    wgg[1] = (GtkWidget *)be;
    wg1 = gtk_table_new(3,6, FALSE);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    
    /* find */
    wg3 = gtk_vbox_new(FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(wg3), 5);
    gtk_table_attach(GTK_TABLE(wg1), wg3, 0,3, 0,1, GTK_FILL,0, 0,0);
    wg2 = gtk_label_new("Find ");
    gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);
    wg2 = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);    
    wgg[2] = wg2;

    if(be->priv->vfind){
	gtk_entry_set_text(GTK_ENTRY(wg2), be->priv->vfind);
	free(be->priv->vfind);
	be->priv->vfind = NULL;
    }

    /* replace */
    if(be->priv->properties & GUI_BINEDITOR_PROP_EDITABLE){
	wg3 = gtk_vbox_new(FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(wg3), 5);
	gtk_table_attach(GTK_TABLE(wg1), wg3, 0,3, 1,2, GTK_FILL,0, 0,0);
	wg2 = gtk_label_new("Replace ");
	gtk_misc_set_alignment(GTK_MISC(wg2), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);
	wg2 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);    
	wgg[3] = wg2;
    }

    if(be->priv->vreplace){
	gtk_entry_set_text(GTK_ENTRY(wg2), be->priv->vreplace);
	free(be->priv->vreplace);
	be->priv->vreplace = NULL;
    }

    /* ignorowanie wilekosci znaku ASCII */
    wg2 = gtk_check_button_new_with_label("Case insensitive");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 2,3, GTK_FILL,0, 0,0);    
    wgg[4] = wg2;
    
    /* szukanie od pozycji markera */
    wg2 = gtk_check_button_new_with_label("Start from marked");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 0,1, 3,4, GTK_FILL,0, 0,0);    
    wgg[5] = wg2;

    if(be->priv->properties & GUI_BINEDITOR_PROP_EDITABLE){
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
    g_signal_connect(G_OBJECT(wg2), "clicked", G_CALLBACK(gui_bineditor_kill), wg0);

    wg2 = gtk_button_new_with_label("  OK  ");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 2,3, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    g_signal_connect(G_OBJECT(wg2), "clicked", G_CALLBACK(gui_bineditor_find_ok), wgg);

    gtk_widget_show_all(wg0);
}

/******************************************************************************************************************/
static void gui_bineditor_jump_marker(GtkWidget *wg, GuiBineditor *be)
{
    be->priv->address_mark_redo = gtk_adjustment_get_value(be->priv->adj);
    gtk_adjustment_set_value(be->priv->adj, be->priv->address_mark);
    gtk_widget_set_sensitive(be->priv->rjmp, TRUE);
    gtk_adjustment_value_changed(be->priv->adj);    
}

static void gui_bineditor_redo_marker(GtkWidget *wg, GuiBineditor *be)
{
    gtk_adjustment_set_value(be->priv->adj, be->priv->address_mark_redo);
    gtk_widget_set_sensitive(be->priv->rjmp, FALSE);
    gtk_adjustment_value_changed(be->priv->adj);    
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

    g_return_if_fail(be->priv->buffer != NULL);

    memset(be->priv->buffer + s, d, i);
    gui_bineditor_emit_signal(be);
    gtk_widget_destroy(wgg[5]);
    gtk_widget_queue_draw(be->priv->drawing_area);
}

static void gui_bineditor_clear_buffer(GtkWidget *bt, GuiBineditor *be)
{
    static GtkWidget *wgg[6];
    GtkWidget *wg0, *wg1, *wg2;
    GtkAdjustment *adj;
        
    if(!be->priv->buffer) return;
    
    wg0 = gtk_window_new(0);
    gtk_window_set_modal(GTK_WINDOW(wg0), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(wg0), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 10);
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

    adj = gtk_adjustment_new(0, 0, be->priv->buffer_size, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 0,1, GTK_FILL | GTK_EXPAND, 0,0,0);
    g_signal_connect(G_OBJECT(wg2), "changed", G_CALLBACK(gui_bineditor_clear_clc), wgg);
    wgg[0] = wg2;

    adj = gtk_adjustment_new((be->priv->buffer_size != 0) ? (be->priv->buffer_size - 1) : 0, 0,(be->priv->buffer_size != 0) ? (be->priv->buffer_size - 1) : 0 , 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 1,2, GTK_FILL | GTK_EXPAND, 0,0,0);
    g_signal_connect(G_OBJECT(wg2), "changed", G_CALLBACK(gui_bineditor_clear_clc), wgg);
    wgg[1] = wg2;

    adj = gtk_adjustment_new(be->priv->buffer_size, 0, be->priv->buffer_size, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 2,3, GTK_FILL | GTK_EXPAND, 0,0,0);
    g_signal_connect(G_OBJECT(wg2), "changed", G_CALLBACK(gui_bineditor_clear_cli), wgg);
    wgg[2] = wg2;

    adj = gtk_adjustment_new(0, 0, 255, 1,1,0);
    wg2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,3, 3,4, GTK_FILL | GTK_EXPAND, 0,0,0);
    wgg[3] = wg2;

    wg2 = gtk_button_new_with_label("Cancel");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 1,2, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    g_signal_connect(G_OBJECT(wg2), "clicked", G_CALLBACK(gui_bineditor_kill), wg0);

    wg2 = gtk_button_new_with_label("  OK  ");
    gtk_table_attach(GTK_TABLE(wg1), wg2, 2,3, 4,5, GTK_FILL | GTK_EXPAND, 0,0,15);
    g_signal_connect(G_OBJECT(wg2), "clicked", G_CALLBACK(gui_bineditor_clear_ok), wgg);
    
    gtk_widget_show_all(wg0);

}

/**************************************************************************************************************************/

#define SET_COLOR(be, idx, r,g,b)	be->priv->colors[idx * 3] = r; be->priv->colors[idx * 3 + 1] = g; be->priv->colors[idx * 3 + 2] = b
#define GET_COLOR(be, idx)		be->priv->colors[idx * 3], be->priv->colors[idx * 3 + 1], be->priv->colors[idx * 3 + 2]

void gui_bineditor_statusbar(GuiBineditor *be, char *tmp, char *str, ...)
{	
    va_list v;
        
    va_start(v, str);
    vsprintf(tmp, str, v);
    va_end(v);

    if(!be->priv->statusbar) return;
    gtk_statusbar_pop(GTK_STATUSBAR(be->priv->statusbar), be->priv->statusbar_id);
    gtk_statusbar_push(GTK_STATUSBAR(be->priv->statusbar), be->priv->statusbar_id, tmp);
}

static char gui_dig2hex(char i){
    return i + (i < 10 ? '0'  : 'A' - 10 );
}

static int gui_bineditor_get_grid_addr(GuiBineditor *be, int xi, int yi, char *ascii_grid)
{
    int x, y, xa, address, start_addr;

    x  = xi - be->priv->grid_start;  // horizontal position relative to start of hex grid
    xa = xi - be->priv->ascii_start; // horizontal position relative to start of ascii grid  
    y  = yi - be->priv->grid_top;    // vertical position common for hex and ascii grid
    *ascii_grid = 0;
    start_addr = gtk_adjustment_get_value(be->priv->adj) / be->priv->grid_cols;
    start_addr *= be->priv->grid_cols;

    // Compute grid cell index
    // if cursor is over hex grid or is away grid fields
    if(x < 0 || y < 0 || xi > be->priv->grid_end || !be->priv->cell_width || !be->priv->cell_height){
	if(xa < 0 || y < 0 || xi > be->priv->ascii_end || !be->priv->cell_width || !be->priv->cell_height) return -1;
	x = xa / be->priv->ascii_space;
	*ascii_grid = 1;
    } else {
	x /= be->priv->cell_width;
    }
    y /= be->priv->cell_height + 1; 

    address = y * be->priv->grid_cols + x + start_addr;

    return address;
}

static void gui_bineditor_exit_edit(GuiBineditor *be)
{
    be->priv->edit_hex = 0;
    be->priv->address_hl_start = -1;
    be->priv->address_hl_end = -1;
    gtk_widget_set_sensitive(be->priv->clear, be->priv->clear_sens);
    gtk_widget_set_sensitive(be->priv->mjmp, be->priv->mjmp_sens);
    gtk_widget_set_sensitive(be->priv->rjmp, be->priv->rjmp_sens);
    gtk_widget_set_sensitive(be->priv->find, be->priv->find_sens);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->priv->print, be->priv->print_sens);
#endif
    gtk_widget_set_sensitive(be->priv->chksum, be->priv->chksum_sens);
}

static void gui_bineditor_off_edit(GuiBineditor *be)
{
    be->priv->clear_sens = gtk_widget_get_sensitive(be->priv->clear);
    gtk_widget_set_sensitive(be->priv->clear, 0);
    be->priv->mjmp_sens = gtk_widget_get_sensitive(be->priv->mjmp);
    gtk_widget_set_sensitive(be->priv->mjmp, 0);
    be->priv->rjmp_sens = gtk_widget_get_sensitive(be->priv->rjmp);
    gtk_widget_set_sensitive(be->priv->rjmp, 0);
    be->priv->find_sens = gtk_widget_get_sensitive(be->priv->find);
    gtk_widget_set_sensitive(be->priv->find, 0);
#ifndef NO_PRINTER_SUPPORT
    be->priv->print_sens = gtk_widget_get_sensitive(be->priv->print);
    gtk_widget_set_sensitive(be->priv->print, 0);
#endif
    be->priv->chksum_sens = gtk_widget_get_sensitive(be->priv->chksum);
    gtk_widget_set_sensitive(be->priv->chksum, 0);
}

static void gui_bineditor_mbutton(GtkWidget *wg, GdkEventButton *ev, GuiBineditor *be)
{
    int address;
    char ascii = 0;


    if( gtk_widget_is_focus(wg) == FALSE ){
	gtk_widget_grab_focus( wg );
    }

    if(!be->priv->buffer || !be->priv->buffer_size) return;
    address = gui_bineditor_get_grid_addr(be, ev->x, ev->y, &ascii);
    if(address < 0) return;


    if(ev->button == 3){
	be->priv->address_mark = address;
        gtk_widget_queue_draw(be->priv->drawing_area);
	return;
    }

    if(ev->button != 1) return;

    if( ev->button == 1){
	if(be->priv->edit_hex == 0){
	    be->priv->edit_hex = ascii ? 2 : 1;
	    be->priv->edit_hex_cursor = 0;
	    be->priv->address_hl_start = address;
	    be->priv->address_hl_end = address;
	    gui_bineditor_off_edit( be );
	} else {
	    gui_bineditor_exit_edit( be );
	}
	gtk_widget_queue_draw(be->priv->drawing_area);
    }
}

static void gui_bineditor_hint(GtkWidget *wg, GdkEventMotion *ev, GuiBineditor *be)
{
    char tmp[256];
    int address;
    int data, offs;
    char ascii = 0;
    
    if(!be->priv->buffer || !be->priv->buffer_size) return;
    address = gui_bineditor_get_grid_addr(be, ev->x, ev->y, &ascii);
    if(address < 0){
	gui_bineditor_statusbar(be, tmp, "");
        return;
    }
    data = be->priv->buffer[address];
    if(be->priv->address_old_hint == address) return;
    if(address >= be->priv->buffer_size){
	gui_bineditor_statusbar(be, tmp, "");
	return;
    }
    be->priv->address_old_hint = address;
    offs = address - be->priv->address_mark;

    sprintf(tmp, " %x:%x(%i) ", address, data & 0xff, data & 0xff);
    gtk_label_set_text(GTK_LABEL(be->priv->info_addr), tmp);

    sprintf(tmp, " %x:%c%x(%i) ", be->priv->address_mark, offs < 0 ? '-':' ',abs(offs), offs );
    gtk_label_set_text(GTK_LABEL(be->priv->info_mark), tmp);

}

static void gui_bineditor_leave(GtkWidget *wg, GdkEventCrossing *ev, GuiBineditor *be)
{
    char tmp[2];
    /* oczyszczenie pola statusu */
    gui_bineditor_statusbar(be, tmp, "");
    gdk_window_set_cursor(gtk_widget_get_parent_window(wg), NULL);
}

static void gui_bineditor_enter(GtkWidget *wg, GdkEventCrossing *ev, GuiBineditor *be)
{
    GdkCursor *cursor;
    cursor = gdk_cursor_new(GDK_HAND2);
    gdk_window_set_cursor(gtk_widget_get_parent_window(wg), cursor);
    gdk_cursor_unref(cursor);
}

static void gui_bineditor_slider(GtkAdjustment *wig, GuiBineditor *wg, GuiBineditor *be)
{
    gtk_widget_queue_draw(wg->priv->drawing_area);
}

static void gui_bineditor_scroll(GtkWidget *wg, GdkEventScroll *ev, GuiBineditor *be)
{
    gdouble inc = gtk_adjustment_get_step_increment( GTK_ADJUSTMENT(be->priv->adj) );
    gdouble val = gtk_adjustment_get_value( GTK_ADJUSTMENT(be->priv->adj) );

    switch( ev->direction ){
	case GDK_SCROLL_UP:   val -= inc; break; 
	case GDK_SCROLL_DOWN: val += inc; break;
	default: return;
    }
    gtk_adjustment_set_value( GTK_ADJUSTMENT(be->priv->adj), val );
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

/* must be corrected for printing */
static void gui_bineditor_draw(cairo_t *cr, GuiBineditor *be, int vxx, int vyy, char print)
{
    char tmp[16], rg, mrk, hl=0;
    int i, j,kx, ky, xc, yc, cx, zx, xam, addr, xa, z, max_addr, xx, yy, n, pagesize, bl_pos;
    
    /* computing position and parameters for display */
#define nx	2	/* characters per cell */
#define border	4	/* pixels sorrounding character */
#define fx	6 	/* font width */
#define fy	10	/* font height */
#define ac	8 	/* digit counts for address */
#define ry	26	/* bar width */
#define underln_pos 3   /* position for underline below character*/

    kx = fx + border;
    ky = fy + border;
    /* width of the address field */
    zx = (ac + 2) * kx;
    cx = nx * kx;
    /* cell count */
    xc = (((vxx - zx - 2*kx) / kx) / 3);
    yc = (vyy - ry) / (ky + 1);
    xam = (vxx + zx + kx + xc * (cx - kx)) / 2;
    pagesize = xc * yc;

    if(!print){
	be->priv->cell_width  = cx; 
	be->priv->cell_height = ky;
	be->priv->grid_start  = zx;
	be->priv->grid_end    = xam - cx;
        be->priv->grid_cols   = xc;
	be->priv->grid_rows   = yc;
	be->priv->grid_top    = ry;
	be->priv->ascii_start = xam;
	be->priv->ascii_end   = xam + kx * xc;
	be->priv->ascii_space = kx;
		
	gtk_adjustment_set_page_size(be->priv->adj, pagesize);
	gtk_adjustment_set_page_increment(be->priv->adj, xc);
	gtk_adjustment_set_step_increment(be->priv->adj, xc);
    }

    max_addr = be->priv->buffer_size;
    addr = gtk_adjustment_get_value(be->priv->adj) / xc;
    addr *= xc;

    /* setting font parameters */
    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);

    /* clear screen */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_GRID_BG));
    cairo_rectangle(cr, 0, 0, vxx, vyy);
    cairo_fill(cr);
    cairo_stroke(cr);    
    /* upper bar */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_DESC_BG));
    cairo_rectangle(cr, 0, 0, vxx, ky + 5);
    cairo_fill(cr);
    cairo_stroke(cr);    

    /* drawing grid */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_GRID));    
    for(i = 0, yy = ry + ky; i < yc; i++, yy += ky + 1) gui_cairo_line(cr, 0, yy, vxx, yy);
    for(i = 0, xx = zx; i < xc + 1; i++, xx += cx ) gui_cairo_line(cr, xx, 0, xx, vyy);
    cairo_stroke(cr);

    /* bar description */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_DESC));    
    for(i = 0, xx = zx; i < xc; i++, xx += cx)
	gui_cairo_outtext(cr, tmp, xx + 1 , 4 + fy, "%c%c", gui_dig2hex(i / 16), gui_dig2hex(i % 16));
    gui_cairo_outtext(cr, tmp, border , 4 + fy, "Address");
    gui_cairo_outtext(cr, tmp, xam , 4 + fy, "ASCII");
    cairo_stroke(cr);

    /* filling cells */
    cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL));    
    for(j = 0, yy = ry + fy; (j < yc) && (addr < max_addr); j++, yy += ky + 1){
	for(z = 0; z < 8; z++) 
	    gui_cairo_outtext(cr, tmp, kx*(8-z), yy, "%c", gui_dig2hex( (addr >> (z*4)) & 0xf));

	for(i = 0, xx = zx, xa = xam; (i < xc) && (addr < max_addr); i++, xx += cx, xa += kx, addr++ ){
	    n = 0;
	    if(be->priv->buffer) n = be->priv->buffer[addr];
	    mrk = be->priv->address_mark == addr;
	    hl  = (addr >= be->priv->address_hl_start) && (addr <= be->priv->address_hl_end);
	    bl_pos = (be->priv->edit_hex_cursor & 1) * (fx + 2);
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
		if( be->priv->edit_hex != 2)
	    	    cairo_rectangle(cr, xx, yy - fy, cx, ky - 1);
		if( be->priv->edit_hex != 1)
	    	    cairo_rectangle(cr, xa, yy - fy, kx - 1, ky - 1);
		cairo_fill(cr);
		cairo_stroke(cr);
		cairo_set_source_rgb(cr, GET_COLOR(be, GUI_BINEDITOR_COLOR_TEXT_NORMAL));
	    }	    

	    gui_cairo_outtext(cr, tmp, xx + 1, yy , "%c%c", gui_dig2hex(n / 16), gui_dig2hex(n % 16));

	    if(!rg){
		if((!mrk && !hl) || (( be->priv->edit_hex == 1) && hl)){
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
	    if( be->priv->edit_hex && hl){
		if( be->priv->edit_hex != 2)
		    gui_cairo_line(cr, xx + 1 + bl_pos, yy + underln_pos, xx + 1 + fx + 1 + bl_pos, yy + underln_pos);
		if( be->priv->edit_hex != 1)
		    gui_cairo_line(cr, xa + 1 + bl_pos, yy + underln_pos, xa + 1 + fx + 1 + bl_pos, yy + underln_pos);
		cairo_stroke(cr);
	    }
	}
    }
    cairo_stroke(cr);
}

static gboolean gui_bineditor_draw_sig(GtkWidget *wg, cairo_t *cr, GuiBineditor *be)
{
    gui_bineditor_draw(cr, be, gtk_widget_get_allocated_width( wg ), gtk_widget_get_allocated_height( wg ), 0);  
    return FALSE;
}

static void gui_bineditor_cursor_decrement(GuiBineditor *be, int v )
{
    if( be->priv->address_hl_start < v){
	be->priv->edit_hex_cursor = 0;	
        return;
    }
    if(((be->priv->edit_hex_cursor -= v) < 0) || (be->priv->edit_hex == 2)){
	if(v == 1) 
	    be->priv->edit_hex_cursor = 1;
	else
	    be->priv->edit_hex_cursor &= 1;
	if( (be->priv->edit_hex == 2) || ( be->priv->edit_hex_cursor < 0) ) be->priv->edit_hex_cursor = 0;
	be->priv->address_hl_start -= v;
	be->priv->address_hl_end -=v;
	if(be->priv->address_hl_start < 0) be->priv->address_hl_start = 0;
	if(be->priv->address_hl_end < 0) be->priv->address_hl_end = 0;
    }
}

static void gui_bineditor_cursor_increment(GuiBineditor *be, int v )
{
    if(((be->priv->edit_hex_cursor += v) > 1) || (be->priv->edit_hex == 2)){
	if((v == 1) || (be->priv->edit_hex == 2)) 
	    be->priv->edit_hex_cursor = 0;
	else
	    be->priv->edit_hex_cursor &= 1;
	be->priv->address_hl_start += v;
	be->priv->address_hl_end +=v;
	if(be->priv->address_hl_start >= be->priv->buffer_size) be->priv->address_hl_start -= v;
	if(be->priv->address_hl_end >= be->priv->buffer_size) be->priv->address_hl_end -= v;
    }
}

static void gui_bineditor_edit(GuiBineditor *be, int key)
{
    int val;
    if(be->priv->edit_hex == 1){
	if((key >= 'A') && (key <= 'F')) key = key - 'A' + 'a';
	if(!(((key >= '0') && (key <= '9')) || ((key >= 'a') && (key <= 'f')))) return;
	val = key - '0';
	if((key >= 'a') && (key <= 'f')) val = 10 + key - 'a';
	val &= 0x0f;
	if(be->priv->edit_hex_cursor == 0)
	    be->priv->buffer[ be->priv->address_hl_start] = (be->priv->buffer[ be->priv->address_hl_start] & 0x0f) | (val << 4);
	else
	    be->priv->buffer[ be->priv->address_hl_start] = (be->priv->buffer[ be->priv->address_hl_start] & 0xf0) | val;
    } else
	be->priv->buffer[ be->priv->address_hl_start] = key;
    gui_bineditor_cursor_increment( be, 1 );
}

static void gui_bineditor_keystroke(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    int key = ev->keyval;
    int adj;

    if( be->priv->edit_hex == 0 ) return; // ignore if edit mode not active
    
    if(key == be->priv->key_left) gui_bineditor_cursor_decrement( be, 1 ); 
    if(key == be->priv->key_right) gui_bineditor_cursor_increment( be, 1 ); 
    if(key == be->priv->key_up) gui_bineditor_cursor_decrement( be, be->priv->grid_cols ); 
    if(key == be->priv->key_down) gui_bineditor_cursor_increment( be, be->priv->grid_cols ); 
    if(key == be->priv->key_home){
	be->priv->edit_hex_cursor = 0;
	be->priv->address_hl_start = be->priv->address_hl_end = (be->priv->address_hl_start / be->priv->grid_cols) * be->priv->grid_cols;
    }
    if(key == be->priv->key_end){
        be->priv->edit_hex_cursor = 0;
        be->priv->address_hl_start = ((be->priv->address_hl_start / be->priv->grid_cols) + 1) * be->priv->grid_cols - 1;
        if(be->priv->address_hl_start >= be->priv->buffer_size) be->priv->address_hl_start = be->priv->buffer_size - 1;
        be->priv->address_hl_end = be->priv->address_hl_start;
    }
    if(key == be->priv->key_pgup){	
//	be->priv->edit_hex_cursor = 0;
//	be->priv->address_hl_start -= be->priv->grid_cols * be->priv->grid_rows;
//	if(be->priv->address_hl_start < 0) be->priv->address_hl_start = 0;
//	be->priv->address_hl_start = be->priv->address_hl_end = (be->priv->address_hl_start / be->priv->grid_cols) * be->priv->grid_cols;
    }
    if(key == be->priv->key_pgdn){	
//	be->priv->edit_hex_cursor = 0;
//      be->priv->address_hl_start += be->priv->grid_cols * be->priv->grid_rows - 1;
//	be->priv->address_hl_start = ((be->priv->address_hl_start / be->priv->grid_cols) + 1) * be->priv->grid_cols - 1;
//	if(be->priv->address_hl_start >= be->priv->buffer_size) be->priv->address_hl_start = be->priv->buffer_size - 1;
//      be->priv->address_hl_end = be->priv->address_hl_start;
    }

    if(key == be->priv->key_tab){	
        be->priv->edit_hex ^= 0x03; 
        if(be->priv->edit_hex == 2) be->priv->edit_hex_cursor = 0; 
    }
    if((key >= 32) && (key < 128)) gui_bineditor_edit( be, key); 

    //scroll if outside grid window
    adj = gtk_adjustment_get_value(be->priv->adj);
    if( be->priv->address_hl_start < adj){
	adj -= be->priv->grid_cols;
	if(adj < 0) adj = 0;
	gtk_adjustment_set_value(be->priv->adj, adj);
    }
    if( be->priv->address_hl_start >= adj + be->priv->grid_cols * be->priv->grid_rows){
	adj += be->priv->grid_cols;
	if(adj >= be->priv->buffer_size) adj = be->priv->buffer_size - be->priv->grid_cols;
	gtk_adjustment_set_value(be->priv->adj, adj);
    }
    gtk_widget_queue_draw(be->priv->wmain);
}

static void gui_bineditor_focus_out(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    be->priv->edit_hex = 0; 
    be->priv->address_hl_end = be->priv->address_hl_start = -1;
    gui_bineditor_exit_edit( be );
}

static void gui_bineditor_focus_in(GtkWidget *wg, GdkEventKey *ev, GuiBineditor *be)
{
    be->priv->edit_hex = 0; 
    be->priv->address_hl_end = be->priv->address_hl_start = -1;

    gtk_widget_set_sensitive(be->priv->clear, TRUE);
    gtk_widget_set_sensitive(be->priv->mjmp,  TRUE);
    gtk_widget_set_sensitive(be->priv->find,  TRUE);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->priv->print,  TRUE);
#endif
    gtk_widget_set_sensitive(be->priv->rjmp,  FALSE);
    gtk_widget_set_sensitive(be->priv->chksum, TRUE);    
}

static void gui_bineditor_init(GuiBineditor *be)
{
    GtkWidget *wg0, *wg1, *wg2;

    be->priv = G_TYPE_INSTANCE_GET_PRIVATE (be, GUI_TYPE_BINEDITOR, GuiBineditorPrivate);
    
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));

    be->priv->edit_hex = 0;
    be->priv->address_mark = 0;
    be->priv->address_mark_redo = 0;
    be->priv->address_old_hint = -1;
    be->priv->address_hl_start = -1;
    be->priv->address_hl_end = -1;
    be->priv->adj = NULL;
    be->priv->drawing_area = NULL;
    be->priv->tb = NULL;
    be->priv->buffer_size = 0;
    be->priv->buffer = NULL;    
    be->priv->statusbar = NULL;
    be->priv->statusbar_id = 0;
    be->priv->properties = ~0;
    be->priv->vfind = NULL;
    be->priv->vreplace = NULL;
    be->priv->rfsh = 0; /* force redrawing */

    be->priv->key_left = gdk_keyval_from_name("Left");
    be->priv->key_right = gdk_keyval_from_name("Right");
    be->priv->key_up = gdk_keyval_from_name("Up");
    be->priv->key_down = gdk_keyval_from_name("Down");
    be->priv->key_home = gdk_keyval_from_name("Home");
    be->priv->key_end = gdk_keyval_from_name("End");
    be->priv->key_pgup = gdk_keyval_from_name("Page_Up");
    be->priv->key_pgdn = gdk_keyval_from_name("Page_Down");
    be->priv->key_tab = gdk_keyval_from_name("Tab");

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

    be->priv->vbox = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(be), be->priv->vbox, TRUE, TRUE, 0);        

/* toolbar */
    be->priv->tb = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(be->priv->vbox), be->priv->tb, FALSE, FALSE, 0);    

    /* Clear buffer */
    be->priv->clear = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_CLEAR ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->clear), TIP_BE_CLEAR_BUFFER);
    g_signal_connect(G_OBJECT(be->priv->clear), "clicked", G_CALLBACK(gui_bineditor_clear_buffer), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->clear), -1);

    /* Find string */
    be->priv->find = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_FIND ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->find), TIP_BE_FIND_STRING);
    g_signal_connect(G_OBJECT(be->priv->find), "clicked", G_CALLBACK(gui_bineditor_find_string), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->find), -1);

    /* Checksum */
    be->priv->chksum = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_DIALOG_AUTHENTICATION ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->chksum), TIP_BE_CHECKSUM);
    g_signal_connect(G_OBJECT(be->priv->chksum), "clicked", G_CALLBACK(gui_bineditor_checksum), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->chksum), -1);

    /* Jump to marker */
    be->priv->mjmp = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_JUMP_TO ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->mjmp), TIP_BE_JUMP_TO_MARKER);
    g_signal_connect(G_OBJECT(be->priv->mjmp), "clicked", G_CALLBACK(gui_bineditor_jump_marker), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->mjmp), -1);

    /* Return from marker */
    be->priv->rjmp = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_GO_BACK ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->rjmp), TIP_BE_REDO_JUMP);
    g_signal_connect(G_OBJECT(be->priv->rjmp), "clicked", G_CALLBACK(gui_bineditor_redo_marker), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->rjmp), -1);

#ifndef NO_PRINTER_SUPPORT
    /* Printer */
    be->priv->print = GTK_WIDGET(gtk_tool_button_new_from_stock( GTK_STOCK_PRINT ));
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(be->priv->print), TIP_BE_PRINT);
    g_signal_connect(G_OBJECT(be->priv->print), "clicked", G_CALLBACK(gui_bineditor_print), be);
    gtk_toolbar_insert( GTK_TOOLBAR(be->priv->tb), GTK_TOOL_ITEM(be->priv->print), -1);
#endif

/* shadows buttons */
    gtk_widget_set_sensitive(be->priv->clear, FALSE);
    gtk_widget_set_sensitive(be->priv->mjmp, FALSE);
    gtk_widget_set_sensitive(be->priv->find, FALSE);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->priv->print, FALSE);
#endif
    gtk_widget_set_sensitive(be->priv->rjmp, FALSE);
    gtk_widget_set_sensitive(be->priv->chksum, FALSE);    

    /* info fields */
    wg1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(be->priv->vbox), wg1, FALSE, FALSE, 0);
    be->priv->info_addr = gtk_label_new("0000:00");
    wg2 = gtk_frame_new(NULL);
    
    gtk_widget_set_size_request( wg2, 120, 20);
    gtk_frame_set_shadow_type(GTK_FRAME(wg2), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(wg2), be->priv->info_addr);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);
    be->priv->info_mark = gtk_label_new("0000:00");
    wg2 = gtk_frame_new(NULL);
    gtk_widget_set_size_request( wg2, 120, 20);
    gtk_container_add(GTK_CONTAINER(wg2), be->priv->info_mark);
    gtk_frame_set_shadow_type(GTK_FRAME(wg2), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);

/* shadowed in drawing area for CAIRO and slider */
    wg0 = gtk_frame_new(NULL);    
    gtk_frame_set_shadow_type(GTK_FRAME(wg0), GTK_SHADOW_IN);

    wg1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    gtk_container_add(GTK_CONTAINER(be->priv->vbox), wg0);    

    /* add vertical slider */
    be->priv->adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1, 1, 0, 0));
    wg2 = gtk_vscrollbar_new(be->priv->adj);
    gtk_box_pack_end(GTK_BOX(wg1), wg2, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(be->priv->adj), "value_changed", G_CALLBACK(gui_bineditor_slider), be);

    /* create drawing area and connect signals */
    be->priv->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_can_focus(be->priv->drawing_area, TRUE);
    gtk_widget_set_events(be->priv->drawing_area, 
	GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK | GDK_POINTER_MOTION_MASK
	| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK
    );
    
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"draw",G_CALLBACK(gui_bineditor_draw_sig), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"scroll_event",G_CALLBACK(gui_bineditor_scroll), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"button_press_event",G_CALLBACK(gui_bineditor_mbutton), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"motion_notify_event",G_CALLBACK(gui_bineditor_hint), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"leave_notify_event",G_CALLBACK(gui_bineditor_leave), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"enter_notify_event",G_CALLBACK(gui_bineditor_enter), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"key_press_event",G_CALLBACK(gui_bineditor_keystroke), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"focus_out_event",G_CALLBACK(gui_bineditor_focus_out), be);
    g_signal_connect(G_OBJECT(be->priv->drawing_area),"focus_in_event",G_CALLBACK(gui_bineditor_focus_in), be);
    gtk_box_pack_end(GTK_BOX(wg1), be->priv->drawing_area, TRUE, TRUE, 1);
    gtk_widget_show_all(wg0);
}

void gui_bineditor_set_buffer(GuiBineditor *be, int bfsize, unsigned char *buffer)
{
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(buffer != NULL);

    be->priv->buffer_size = bfsize;
    be->priv->buffer = bfsize ? buffer: NULL;
    gtk_adjustment_set_lower(be->priv->adj, 0);
    gtk_adjustment_set_upper(be->priv->adj,bfsize);
    gtk_adjustment_set_page_size(be->priv->adj, 0);
    gtk_adjustment_set_value(be->priv->adj, 0);
    gtk_adjustment_value_changed(be->priv->adj);
    gtk_widget_set_sensitive(be->priv->clear, bfsize);
    gtk_widget_set_sensitive(be->priv->mjmp, bfsize);
    gtk_widget_set_sensitive(be->priv->find, bfsize);
#ifndef NO_PRINTER_SUPPORT
    gtk_widget_set_sensitive(be->priv->print, bfsize);
#endif
    gtk_widget_set_sensitive(be->priv->chksum, bfsize);
/* now everything to redraw */
    be->priv->rfsh = 0; 
    gtk_widget_queue_draw(be->priv->drawing_area);
}

void gui_bineditor_set_colors(GuiBineditor *be, GuiBineditorColors color, float r, float g, float b)
{
    int tmp = color;
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(tmp < GUI_BINEDITOR_COLOR_LAST);
    
    tmp *= 3;
    be->priv->colors[color + 0] = r;
    be->priv->colors[color + 1] = g;
    be->priv->colors[color + 2] = b;
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
    g_return_if_fail(be->priv->tb != NULL);
    g_return_if_fail(GTK_IS_TOOLBAR(be->priv->tb));

    be->priv->properties = pr;

    gui_widget_show(be->priv->tb,     pr & GUI_BINEDITOR_PROP_TOOLBAR);
    gui_widget_show(be->priv->find,   pr & GUI_BINEDITOR_PROP_FIND);
    gui_widget_show(be->priv->mjmp,   pr & GUI_BINEDITOR_PROP_MJMP);
    gui_widget_show(be->priv->rjmp,   pr & GUI_BINEDITOR_PROP_RJMP);
    gui_widget_show(be->priv->calc,   pr & GUI_BINEDITOR_PROP_CALC);
#ifndef NO_PRINTER_SUPPORT
    gui_widget_show(be->priv->print,  pr & GUI_BINEDITOR_PROP_PRINT);
#endif
    gui_widget_show(be->priv->clear,  (pr & GUI_BINEDITOR_PROP_CLEAR) && (pr & GUI_BINEDITOR_PROP_EDITABLE));
    gui_widget_show(be->priv->chksum, (pr & GUI_BINEDITOR_PROP_CHKSUM) && (pr & GUI_BINEDITOR_PROP_EDITABLE));
}

void gui_bineditor_connect_statusbar(GuiBineditor *be, GtkWidget *sb)
{
    g_return_if_fail(be != NULL);
    g_return_if_fail(GUI_IS_BINEDITOR(be));
    g_return_if_fail(sb != NULL);
    g_return_if_fail(GTK_IS_STATUSBAR(sb));
    g_return_if_fail(be->priv->statusbar == NULL);

    be->priv->statusbar = sb;
    be->priv->statusbar_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), "bineditor_statusbar");
}

void gui_bineditor_redraw(GuiBineditor *be)
{
    gtk_widget_queue_draw(be->priv->wmain);
}

