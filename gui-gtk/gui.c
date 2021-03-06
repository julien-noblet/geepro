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

#include "../intl/lang.h"
#include "../src/files.h"
#include "../src/buffer.h"
#include "../src/main.h"
#include "gui.h"
#include "gui_xml.h"
#include "../src/chip.h"
#include "../pixmaps/img_idx.h"
#include "../src/parport.h"
#include "../pixmaps/xpms.c"
#include "../drivers/hwdriver.h"
#include "../src/iface.h"
#include "icons_xpm.c"
#include "icons_xpm.h"
#include "../src/checksum.h"
#include "../src/error.h"
#include "gui_dialog.h"
#include "../src/programmer.h"
#include "gui_buffer.h"

typedef struct chip_tree_ chip_tree;
struct chip_tree_
{
    char *name;
    chip_tree  *branch;
    char branch_flag;
//    void *key;
    chip_tree *next;
};

typedef struct 
{
    GtkWidget *wg;
    int	counter;
} gui_local_1;

typedef struct 
{
    geepro *gep;
    void *ptr;
} gui_local_2;

static void gui_chip_tree_add(geepro *gep, const char *path, char *name, chip_tree **tree);
static void gui_chip_tree_view_create(geepro *gep, chip_tree *tree);
static void gui_chip_tree_view_free( chip_tree *tree );
static void gui_help(geepro *gep);
static void gui_chip_tree_add_node(const char *path, char *name, chip_tree **br, int col);
static void gui_test_hw(GtkWidget *wg, geepro *gep);

/***************************************************************************************************/
void gui_action_icon_set()
{
    GtkIconFactory *ifact = gtk_icon_factory_new();

//    gtk_icon_factory_add(ifact, "geepro-logo", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( LOGO_ICON )));
    gtk_icon_factory_add(ifact, "geepro-read-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( READ_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-read-eeprom-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( READ_EEPROM_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-sign-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( SIGN_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-write-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( WRITE_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-write-eeprom-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( WRITE_EEPROM_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-erase-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( ERASE_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-testblank-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( TESTBLANK_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-verify-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( VERIFY_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-verify-eeprom-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( VERIFY_EEPROM_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-lockbit-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( LOCKBIT_ACTION_ICON )));
    gtk_icon_factory_add(ifact, "geepro-lockbreak-action", gtk_icon_set_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data( LOCKBREAK_ACTION_ICON )));

    gtk_icon_factory_add_default(ifact);
}

char gui_test_connection(geepro *gep)
{
    if(hw_test_conn()) return 0;
    gui_test_hw( NULL, gep );
    gui_dialog_box(gep, "[ER][TEXT]Programmer unplugged ![/TEXT][BR]OK", NULL, NULL);
    return 1;    
}




void gui_stat_rfsh(geepro *gep)
{
    const char *cname;
    
    if( !gep->ifc ) return;    
    cname = iface_get_chip_name( gep->ifc );
    if( !cname ) cname = "Not selected";
    gtk_entry_set_text( GTK_ENTRY(GUI(gep->gui)->dev_entry ), cname );
    gui_buffer_refresh( GUI(gep->gui)->buffer );
}

static int gui_exit_program(GtkWidget *wg, geepro *gep, geepro *gep1)
{ 

    if(GTK_IS_WINDOW(wg) == TRUE) 
			    gep = gep1;

    if(gui_dialog_box(gep, "[QS][TEXT]"QUIT_MSG"[/TEXT][BR]  NO  [BR]  YES  ") == 2){
	 gui_kill_me( gep );
	 return FALSE;
    }
    return TRUE;
}

void gui_exit(geepro *gep)
{
    gui_exit_program(NULL, gep, gep);
}

void gui_load_error_msg(geepro *gep, const char *fname, const char *err)
{
    if(err[0] != '!') 
	gui_error_box(gep, "Error loading file:\n%s\n%s", fname, err);
    else
	gui_dialog_box(gep, "[WN][TEXT]%s[/TEXT][BR]OK", err + 1); 
}

/***************************************************************************************************************************/
// adding action button to tool bar


static void gui_invoke_action(GtkWidget *wg, gui_action *ga)
{
    int x = 0;
    geepro *gep = (geepro*)(ga->root);
    if(!ga->action){
	gui_dialog_box(
	    gep, 
	    "[ER][TEXT]"
	    "Plugin internal error. Action button registered, but action function = NULL."
	    "[/TEXT][BR] OK "
	);
	return;
    }
// program, compare -> test bin file for changes
    if( !strcmp(ga->name, "geepro-write-action") ) gui_buffer_check_valid( GUI(gep->gui)->buffer );
    if( !strcmp(ga->name, "geepro-write-eeprom-action") ) gui_buffer_check_valid( GUI(gep->gui)->buffer );
    if( !strcmp(ga->name, "geepro-verify-action") ) gui_buffer_check_valid( GUI(gep->gui)->buffer );
    if( !strcmp(ga->name, "geepro-verify-eeprom-action") ) gui_buffer_check_valid( GUI(gep->gui)->buffer );
    gep->action = 1;
    gui_test_hw( NULL, gep );
    if( !gui_test_connection( gep ) ){
	x = ((chip_act_func)ga->action)(ga->root);
    } 
    else;
	//x = -1;        //changed by Reuben
		

    if( x ) gui_dialog_box( gep, 
		"[ER][TEXT]"
	        "Action returned error: %i"
		"[/TEXT][BR] OK ", x
	    ); 

    gui_buffer_refresh(GUI(gep->gui)->buffer);
    gep->action = 0;
}

static int gui_add_bt_action(geepro *gep, const char *stock_name, const char *tip, chip_act_func action)
{
    gui_action *tmp, *new_tie;
    
    if(!(new_tie = malloc(sizeof(gui_action)))){
	printf("{gui.c} gui_add_br_action() --> out of memory.\n");
	return -1;
    }
    
    new_tie->next = NULL;

    new_tie->root = gep;
    new_tie->action = action;

    if(!(new_tie->name = malloc(strlen(stock_name) + 1))){
	printf("{gui.c} gui_add_br_action() --> out of memory.\n");
    } else {
	strcpy(new_tie->name, stock_name);
    }    

    new_tie->widget = gtk_tool_button_new_from_stock( stock_name );
    g_signal_connect(G_OBJECT(new_tie->widget), "clicked", G_CALLBACK(gui_invoke_action), new_tie);
    gtk_tool_item_set_tooltip_text( new_tie->widget, tip );
    gtk_toolbar_insert(GTK_TOOLBAR(GUI(gep->gui)->toolbox), new_tie->widget, -1);
    
    if(!GUI(gep->gui)->action){
	GUI(gep->gui)->action = new_tie;
	return 0;
    }    
    
    for(tmp = GUI(gep->gui)->action; tmp->next; tmp = tmp->next);
    
    tmp->next = new_tie;
    return 0;
}

static void gui_rem_bt_action(gui *g)
{
    gui_action *tmp, *x;
    tmp = g->action;
    while( tmp ){
	x = tmp->next;
	free( tmp->name );
	gtk_widget_destroy(tmp->widget);
	free(tmp);
	tmp = x;
    }
    g->action = NULL;
}

static int gui_add_action_list(s_iface_chip *desc, s_iface_chip_action *act, void *ptr)
{
    gui_add_bt_action((geepro*)ptr, act->name, act->tip, act->action);
    return 0;
}

static int gui_add_action(geepro *gep)
{
    iface_chip_get_actions(gep->ifc->chp, F_IFACE_ACTION(gui_add_action_list), gep);
    gtk_widget_show_all(GTK_WIDGET(GUI(gep->gui)->toolbox));
    return 0;
}

static void gui_chip_free(geepro *gep)
{
    if( iface_get_selected_chip( gep->ifc ) ){
	gui_rem_bt_action(GUI(gep->gui));
    }
    iface_unselect_chip( gep->ifc );
}

/***************************************************************************************************************************/

static void gui_add_buffer( s_buffer *bf, s_buffer_list *it, geepro *gep)
{
    GtkWidget *wg, *lbl;
    wg = gui_buffer_new( GUI(gep->gui)->buffer, it, GUI(gep->gui)->wmain);
    lbl = gtk_label_new(it->name);
    gtk_label_set_angle(GTK_LABEL(lbl), 90);
    gtk_notebook_append_page( GTK_NOTEBOOK(GUI(gep->gui)->buffer_tabs), wg, lbl);
}

static void gui_chip_select(geepro *gep, const char *name)
{
    s_buffer *bf;

    // destroy menu, free buffer memory 
    gui_chip_free( gep );

    // select new chip as current
    if( pgm_select_chip(gep, name) ){
	gui_dialog_box( gep,
	    "[ER][TEXT]Cannot set selected chip '%s'.[/TEXT][BR] OK ", name
	);
	return;
    }

    gui_buffer_delete( GUI(gep->gui)->buffer );
    bf = (s_buffer *)iface_get_chip_buffer(gep->ifc);
    // actualize buffer object 
    buffer_get_list(bf, BUFFER_CB(gui_add_buffer), gep);
    gtk_widget_show_all(GUI(gep->gui)->buffer_tabs);

    // add action buttons to menu    
    gui_add_action(gep);

    // autostart for choosed chip, if defined
    pgm_autostart( gep );

    gui_stat_rfsh(gep);
}

static void gui_rfsh_gtk(void)
{
    while(gtk_events_pending()) gtk_main_iteration();
}

static void gui_about(GtkWidget *wg, geepro *gep)
{ 
    GtkWidget *a;
    a = gtk_about_dialog_new();
    
    gtk_about_dialog_set_program_name( GTK_ABOUT_DIALOG( a ), EPROGRAM_NAME );    
    gtk_about_dialog_set_version( GTK_ABOUT_DIALOG( a ), EVERSION );    
    gtk_about_dialog_set_license_type( GTK_ABOUT_DIALOG( a ), GTK_LICENSE_GPL_2_0 );    
    gtk_about_dialog_set_website( GTK_ABOUT_DIALOG( a ), "http://"ESRCURL );    
    gtk_about_dialog_set_authors( GTK_ABOUT_DIALOG( a ), (const char *[])EAUTHORS );
    gtk_about_dialog_set_logo( GTK_ABOUT_DIALOG( a ), NULL );

    gtk_dialog_run( GTK_DIALOG(a) );    
    gtk_widget_destroy( a );
    
    
}

static void gui_chip_callback(s_iface_chip *chip, s_iface_chip_list *it, gui_local_2 *loc)
{
    geepro *gep = loc->gep;
    chip_tree **tree = (chip_tree **)loc->ptr;
    
    if( !it || !gep ) return;
    if( !it->path || !it->name ) return;
    if( !gep->gui ) return;
    gui_chip_tree_add( gep, (char *)it->path, (char*)it->name, tree);
}

static void gui_chip_tree_build(geepro *gep)
{
    chip_tree *tree = NULL;
    gui_local_2  local;

    local.gep = gep;
    local.ptr = &tree;
    iface_chip_get_list(gep->ifc->chp, F_IFACE_CHIP(gui_chip_callback), &local);
    gui_chip_tree_view_create( gep, tree);
    gui_chip_tree_view_free( tree );
}

static void gui_build_iface_menu(s_iface_device *ifc, s_iface_devlist *dev, GtkWidget *wg, int iter )
{
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wg), dev->name);    
    if(ifc->selected){
	if(!strcmp(ifc->selected->name, dev->name)) gtk_combo_box_set_active(GTK_COMBO_BOX(wg), iter);
    }
}

static int gui_iface_sel(GtkWidget *wg, geepro *gep)
{
    char *name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wg));

    if(!name) return 0;
    iface_device_select( gep->ifc->dev, name );    
    gui_stat_rfsh(gep);
    gui_test_hw(NULL, gep);
    return 0;
}

static GtkWidget *gui_iface_list(geepro *gep)
{
    GtkWidget *combox;
    combox = gtk_combo_box_text_new();
    iface_device_get_list(gep->ifc->dev, IFACE_F_DEVICE(gui_build_iface_menu), GTK_COMBO_BOX(combox));
    g_signal_connect(G_OBJECT(combox), "changed", G_CALLBACK(gui_iface_sel), gep);
    return combox;
}

static void gui_add_iface_combox(geepro *gep)
{    
    GtkWidget *tmp = gui_iface_list(gep);

    gtk_box_pack_start(GTK_BOX(GUI(gep->gui)->table), tmp, FALSE, FALSE, 5);
    GUI(gep->gui)->iface = tmp;

    gtk_widget_show(tmp);
}

static void gui_iface_rescan(geepro *gep)
{
    if( GUI(gep->gui)->iface ){
	gtk_widget_destroy( GUI(gep->gui)->iface );
	GUI(gep->gui)->iface = NULL;
    }
    gui_add_iface_combox( gep );
}

static void gui_prog_sel(GtkWidget *wg, geepro *gep)
{
    char *name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wg));

    if(!name) return;
    // remove old driver
    gtk_widget_destroy(GUI(gep->gui)->iface);
    GUI(gep->gui)->iface = NULL;
    // remove old menu
    gui_xml_destroy(GUI(gep->gui)->xml);    
    pgm_select_driver(gep, name); // select new driver
    gui_iface_rescan( gep );
    gui_test_hw( NULL, gep );
}

static void gui_build_prg_menu(s_iface_driver *ifc, s_iface_driver_list *it, gui_local_1 *tmp)
{
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tmp->wg), it->name);
    if( !ifc->selected ) return;
    if( !ifc->selected->name ) return;
    if( !strcmp(it->name, ifc->selected->name) ){
	gtk_combo_box_set_active(GTK_COMBO_BOX(tmp->wg), tmp->counter);
    }
    tmp->counter++;
}

static GtkWidget *gui_prog_list(geepro *gep)
{
    GtkWidget *combox;
    gui_local_1 tmp;

    combox = gtk_combo_box_text_new();
    tmp.wg = combox;
    tmp.counter = 0;
    iface_driver_get_list(gep->ifc->drv, (f_iface_driver_callback)gui_build_prg_menu, &tmp);
    g_signal_connect(G_OBJECT(combox), "changed", G_CALLBACK(gui_prog_sel), gep);
    GUI(gep->gui)->prog_combox = combox;

    return combox;
}

/********************************************************************************************************************/
static void gui_config(GtkWidget *wg, geepro *gep)
{
    printf("config\n");
}

static void gui_set_default(geepro *gep)
{
    gtk_widget_show_all(GUI(gep->gui)->wmain);
    gui_stat_rfsh(gep);
}

#define GUI_STATUSBAR_ID_BUFFER 1

//void gui_set_statusbar(geepro *gep, char *tmp, char *str, ...)
//{	
//    va_list v;
//    
//    va_start(v, str);
//    vsprintf(tmp, str, v);
//    va_end(v);

//    gtk_statusbar_pop(GUI(gep->gui)->status_bar, GUI_STATUSBAR_ID_BUFFER);
//    gtk_statusbar_push(GUI(gep->gui)->status_bar, GUI_STATUSBAR_ID_BUFFER, tmp);
//}

/***********************************************************************************************************************/
/* dodaje ikonki do listy ikon i obrazkow aplikacji */
#define GUI_ICON_NEW(name, xpm)\
    pixb = gdk_pixbuf_new_from_xpm_data((const char **)xpm);\
    gis = gtk_icon_set_new_from_pixbuf(pixb);\
    gtk_icon_factory_add(gifac, name, gis);\
    gtk_icon_factory_add_default(gifac);

static void gui_add_images(geepro *gep)
{
    GtkIconFactory *gifac = gtk_icon_factory_new();
    GtkIconSet *gis;
    GdkPixbuf *pixb;    
    
    /* pozycje przelaczników */
    
    GUI(gep->gui)->icon_size = gtk_icon_size_register("dupa", 10, 25);

    GUI_ICON_NEW(GUI_DIPSW_ON, sw_on_xpm)
    GUI_ICON_NEW(GUI_DIPSW_OFF, sw_off_xpm)
    GUI_ICON_NEW(GUI_JUMPER_UP, sw_on_xpm)
    GUI_ICON_NEW(GUI_JUMPER_DN, sw_off_xpm)
    GUI_ICON_NEW(GUI_JUMPER_CLOSE, sw_on_xpm)
    GUI_ICON_NEW(GUI_JUMPER_OPEN, sw_off_xpm)

}


static void gui_select_chip(geepro *gep, const char *name)
{
    store_set(gep->store, "LAST_CHIP_SELECTED", name);
    gui_chip_select(gep, name);
}

static void gui_chip_tree_selected(GtkTreeSelection *tree, geepro *gep)
{
    GtkTreeIter iter;
    GtkTreeModel *model = NULL;
    char *name = NULL;
    char flag = 0;
    
    if( gtk_tree_selection_get_selected(tree, &model, &iter) ) 
	    gtk_tree_model_get( model, &iter, 0, &name, 1, &flag, -1);
    if( !name ) return;
    if( flag ) gui_select_chip( gep, name);
    g_free( name );        
}

void gui_setup_chip_selection_tree(geepro *gep, GtkWidget *wg )
{
    GtkWidget *view;
    GtkTreeModel *model;

    view = gtk_tree_view_new();

    gtk_tree_view_set_headers_visible( GTK_TREE_VIEW( view ), FALSE );
    gtk_tree_view_set_enable_tree_lines( GTK_TREE_VIEW( view ), TRUE );
    gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ), -1, CHIP_SELECTION, gtk_cell_renderer_text_new(), "text", 0, NULL);

    model = GTK_TREE_MODEL( gtk_tree_store_new( 2, G_TYPE_STRING, G_TYPE_BOOLEAN ) );
    gtk_tree_view_set_model( GTK_TREE_VIEW( view ), model);        
    g_object_unref( model );
    GUI(gep->gui)->chip_select_store = gtk_tree_view_get_model( GTK_TREE_VIEW( view ) );
    g_signal_connect( G_OBJECT( gtk_tree_view_get_selection(GTK_TREE_VIEW( view))), "changed", G_CALLBACK( gui_chip_tree_selected ), gep);
    gtk_container_add(GTK_CONTAINER( wg ), view);
}

static void gui_set_plug(geepro *gep)
{
    gtk_label_set_markup(
	GTK_LABEL( GUI(gep->gui)->connected_lbl ), 
	gep->plug ? LB_PLUGGED : LB_UNPLUGGED
    );
    if( gep->plug )
        gtk_image_set_from_stock( GTK_IMAGE(GUI(gep->gui)->connected), GTK_STOCK_CONNECT, GTK_ICON_SIZE_DND);    
    else
        gtk_image_set_from_stock( GTK_IMAGE(GUI(gep->gui)->connected), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DND);        
}

static void gui_test_hw(GtkWidget *wg, geepro *gep)
{
    char old;
    if( !gep->ifc || gep->action);
    old = gep->plug;
    gep->plug = hw_test_conn();
    if( old != gep->plug) 
	gui_set_plug( gep );
}

static gboolean gui_test_cyclic_hw(geepro *gep)
{
    if( !gep->ifc || gep->action ) return TRUE;
    iface_device_event( gep->ifc->dev );
    if( !hw_test_continue() ) return TRUE;
    gui_test_hw( NULL, gep);
    return TRUE;
}


void gui_usb_port_notify(s_iface_device *dev, s_iface_devlist *list, geepro *gep)
{
     gui_iface_rescan( gep );
}

static void gui_port_devices_set(geepro *gep )
{
    iface_device_connect_notify(gep->ifc->dev, IFACE_F_DEVICE_NOTIFY(gui_usb_port_notify), gep);    
}


void gui_run(geepro *gep)
{
    char *tmp;
    GUI(gep->gui)->gui_run = 0;

    gui_menu_setup( gep );
    gui_action_icon_set();
    gtk_notebook_set_current_page(GTK_NOTEBOOK(GUI(gep->gui)->notebook), 0);
    gui_chip_tree_build( gep );

    GUI(gep->gui)->gui_run = 1;
    gtk_widget_show_all(GUI(gep->gui)->wmain);    

    // It is hack for proper work of gtk_file_chooser() 
//    gui_dialog_box(gep, "[IF][TEXT]Annoying popup\n Hack for gtk_file_chooser_new() not crash.[/TEXT][BR]OK", NULL, NULL);    

    /* inicjowanie domyślnego plugina sterownika programatora */
    g_signal_emit_by_name(G_OBJECT(GUI(gep->gui)->prog_combox), "changed");
    // default combox setting
    tmp = NULL;
    if(!store_get(gep->store, "LAST_CHIP_SELECTED", &tmp)){
	if( tmp ) gui_chip_select(gep, tmp);
    }
    gtk_main(); /* jesli programator ok to startuj program inaczej wyjdź */
    gui_buffer_exit( GUI(gep->gui)->buffer );
}

void gui_kill_me(geepro *gep)
{
    MSG(TXT_EXIT);
    /* Usuniecie biezacego GUI zbudowanego o xml */
    gui_xml_destroy(GUI(gep->gui)->xml);
    free(GUI(gep->gui)->xml);
    gui_chip_free(gep);
    gtk_main_quit();
}

/**************************************************************************************************************************/

static char gui_progress_bar_exit = 0;

void gui_progress_break(geepro *gep)
{
    gui_progress_bar_exit = 1;
}

void gui_progress_bar_init(geepro *gep, const char *title, long range)
{
    GUI(gep->gui)->progress_win = gtk_dialog_new();
    gtk_window_set_resizable(GTK_WINDOW(GUI(gep->gui)->progress_win), FALSE);
    gtk_window_set_modal(GTK_WINDOW(GUI(gep->gui)->progress_win), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(GUI(gep->gui)->progress_win), GTK_WINDOW(GUI(gep->gui)->wmain));
    g_signal_connect(G_OBJECT(GUI(gep->gui)->progress_win), "destroy", G_CALLBACK(gui_progress_break), NULL);
    gtk_window_set_title(GTK_WINDOW(GUI(gep->gui)->progress_win), title);

    GUI(gep->gui)->progress_bar = gtk_progress_bar_new();
    gtk_container_add( GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(GUI(gep->gui)->progress_win))), GUI(gep->gui)->progress_bar );
    gui_rfsh_gtk();
    gui_progress_bar_exit = 0;
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(GUI(gep->gui)->progress_bar), TRUE);
    gtk_widget_show_all(GUI(gep->gui)->progress_win);
}

char gui_progress_bar_set(geepro *gep, long value, long max)
{
    long delta;
    
    if(max == 0) return 0;
    if( gui_progress_bar_exit ) return 1;
    if( value != 1){
	if((value != 0) || (value != max)){
	    delta = max / 100;
	    if(delta < 1) delta = 1;
    	    if( value % delta ) return 0;
	}
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GUI(gep->gui)->progress_bar), (gdouble)value / max);
    gui_rfsh_gtk();
    return 0;
}

void gui_progress_bar_free(geepro *gep)
{
    if(gui_progress_bar_exit) return;
    gtk_widget_destroy(GUI(gep->gui)->progress_win);
}

char gui_cmp_pls(geepro *gep, int a, int b)
{
    char test = (a < b) & !gui_progress_bar_exit;
    if(!test ) gui_progress_bar_free(gep);
    return test;
}

/**************************************************************************************************************************/

static int gui_dialog_exit = 0;

static void gui_dialog_box_close(GtkWidget *wg, char *p_i)
{
    gui_dialog_exit = 1; /* wyjście z petli */

    if(GTK_IS_WINDOW(wg) == TRUE){
	 gtk_widget_destroy(wg);
	 return;
    }
    
    gui_dialog_exit = *p_i;
//    gtk_widget_destroy(wg->parent->parent->parent);
    gtk_widget_destroy(
	gtk_widget_get_parent(
	    gtk_widget_get_parent(
		gtk_widget_get_parent(wg)
	    )
	)	
    );
}

/* do poprawy na gtk_dialog */
int gui_dialog_box(geepro *gp, const char *en, ...)
{
    GtkWidget *wg0, *wg1, *wgtab, *wdialog;
    char *image = NULL;
    char *markup, *title;
    char *fmt, *ft, *ex;
    char flag;
    char pbuttons[256];
    int button;
    va_list ap;

    gui_dialog_exit = 0;
    
    title = NULL;
    if(!strncmp(en, "[CR]", 4)){ image = GTK_STOCK_STOP; title = "Critical error !!!!!!"; }
    if(!strncmp(en, "[ER]", 4)){ image = GTK_STOCK_DIALOG_ERROR; title = "Error !!!"; }
    if(!strncmp(en, "[WN]", 4)){ image = GTK_STOCK_DIALOG_WARNING; title = "Warning !!!"; }
    if(!strncmp(en, "[IF]", 4)){ image = GTK_STOCK_DIALOG_INFO; title = "Information"; }
    if(!strncmp(en, "[HL]", 4)){ image = GTK_STOCK_HELP; title = "Help"; }
    if(!strncmp(en, "[QS]", 4)){ image = GTK_STOCK_DIALOG_QUESTION; title = "Question"; }
    if(!strncmp(en, "[AU]", 4)){ image = GTK_STOCK_DIALOG_AUTHENTICATION; title = "Authentication"; }
    if(!title){
	printf("{gui.c} gui_dialog_box() ---> missing message class token.\n");
	return -1;
    };

    if(!(fmt = malloc(strlen(en) + 1))){
	printf("{gui.c} gui_dialog_box() ---> memory allocation error.\n");
	return -1;
    };
    strcpy(fmt ,en);

    if(!(ft = strstr(fmt, "[TEXT]"))){
	printf("{gui.c} gui_dialog_box() ---> missing [TEXT] delimeter in format string.\n");
	free(fmt);
	return -1;
    }
    ft = strchr(ft, ']') + 1;

    if(!(ex = strstr(fmt, "[/TEXT]"))){
	printf("{gui.c} gui_dialog_box() ---> missing [/TEXT] delimeter in format string.\n");
	free(fmt);
	return -1;
    }
    *ex = '\0';

    /* utworzenie okna w pelni modalnego */
    wdialog = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_position(GTK_WINDOW(wdialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable(GTK_WINDOW(wdialog), FALSE);
    g_signal_connect(G_OBJECT(wdialog),"delete_event",G_CALLBACK(gui_dialog_box_close), NULL);
    gtk_window_set_title(GTK_WINDOW(wdialog), title);
    gtk_container_set_border_width(GTK_CONTAINER(wdialog), 10);
    gtk_window_set_modal(GTK_WINDOW(wdialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(wdialog), GTK_WINDOW(GUI(gp->gui)->wmain));

    /* tabela pakujaca */
    wgtab = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(wdialog), wgtab);    

    if(image){
	wg0 = gtk_image_new_from_stock( image, GTK_ICON_SIZE_DIALOG);
	gtk_grid_attach(GTK_GRID(wgtab), wg0, 0, 0, 1, 1);
    }

    /* dodanie tekstu */
    wg0 = gtk_label_new(NULL);
    va_start(ap, en);
    markup = g_markup_vprintf_escaped(ft, ap);
    va_end(ap);
    gtk_label_set_markup(GTK_LABEL(wg0), markup);
    g_free(markup);
    gtk_grid_attach(GTK_GRID(wgtab), wg0, 1, 0, 1, 1);

    /* przyciski */
    ft = strchr(ex + 1, ']') + 1; /* koniec klamerki */

    wg0 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 3);
    gtk_grid_attach(GTK_GRID(wgtab), wg0, 0, 1, 2, 1);

    button = 0;
    do{
	flag = 0;
	if(!strncmp(ft, "[BR]", 4)) flag = 1;
	if(!strncmp(ft, "[BL]", 4)) flag = 2;
	if(!flag) break;
	ft = strchr(ft, ']') + 1;
	ex = strchr(ft, '[');
	if(ex) *ex = '\0';    

	wg1 = gtk_button_new_with_label(ft);
	
	if(flag == 1) gtk_box_pack_end(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
	if(flag == 2) gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
	pbuttons[ button ] = button + 2;
        g_signal_connect(G_OBJECT(wg1),"clicked",G_CALLBACK(gui_dialog_box_close), (void*)(pbuttons + button) );
	button++;
	if(ex) *ex = '[';
	ft = ex;
    } while(ex  && (button < 256) );
    
    free(fmt);
    gtk_widget_show_all(wdialog);

    for(;!gui_dialog_exit;) gtk_main_iteration();

    return gui_dialog_exit - 1;
}

/**************************************************************************************************************************/
/* generator fali prostokatnej */

static void gui_test_set_period(GtkWidget *wg,  sqw_gen *sqg)
{
    sqg->period = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sqg->wper));
}

static void gui_test_set_duty(GtkWidget *wg,  sqw_gen *sqg)
{
    sqg->duty = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sqg->wdut));
}

static void gui_test_set_length(GtkWidget *wg,  sqw_gen *sqg)
{
    sqg->len = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sqg->wlen));
}

static void gui_test_set_sequence(GtkWidget *wg,  sqw_gen *sqg)
{
    sqg->seq = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sqg->wseq));
}

void gui_clk_sqw(gui *g, gui_sqw_generator gen)
{
    GtkWidget *wgm, *wg0, *wg1;
    GtkAdjustment *adj;    
    static sqw_gen sqg;

    sqg.generator = gen;
    sqg.parent = g;
    
    wgm = gtk_dialog_new_with_buttons("Square Wave Generator", 	GTK_WINDOW(g->wmain), GTK_DIALOG_MODAL, 
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, 
	GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, 
	NULL);
    
    gtk_container_set_border_width(GTK_CONTAINER(wgm), 10);

    wg0 = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(  gtk_dialog_get_content_area(GTK_DIALOG(wgm))  ), wg0);

    wg1 = gtk_label_new("Period [us]:");    
    gtk_grid_attach(GTK_GRID(wg0), wg1, 0, 0, 1, 1);
    wg1 = gtk_label_new("Duty cycle [%]:");    
    gtk_grid_attach(GTK_GRID(wg0), wg1, 0, 1, 1, 1);
    wg1 = gtk_label_new("Length [s]:");    
    gtk_grid_attach(GTK_GRID(wg0), wg1, 0, 2, 1, 1);
    wg1 = gtk_label_new("Sequence (32bit):");    
    gtk_grid_attach(GTK_GRID(wg0), wg1, 0, 3, 1, 1);

    adj = GTK_ADJUSTMENT(gtk_adjustment_new(100, 0.0, 1000000, 100, 0, 0));
    wg1 = gtk_spin_button_new(adj, 1, 0);
    sqg.wper = wg1;
    gtk_grid_attach(GTK_GRID(wg0), wg1, 1, 0, 1, 1);
    g_signal_connect(G_OBJECT(adj),"value_changed",G_CALLBACK(gui_test_set_period), &sqg);

    adj = GTK_ADJUSTMENT(gtk_adjustment_new(50, 0.0, 100, 1, 0, 0));
    wg1 = gtk_spin_button_new(adj, 1, 0);
    sqg.wdut = wg1;
    gtk_grid_attach(GTK_GRID(wg0), wg1, 1, 1, 1, 1);
    g_signal_connect(G_OBJECT(adj),"value_changed",G_CALLBACK(gui_test_set_duty), &sqg);

    adj = GTK_ADJUSTMENT(gtk_adjustment_new(1, 0.0, 60, 1, 0, 0));
    wg1 = gtk_spin_button_new(adj, 1, 0);
    sqg.wlen = wg1;
    gtk_grid_attach(GTK_GRID(wg0), wg1, 1, 2, 1, 1);
    g_signal_connect(G_OBJECT(adj),"value_changed",G_CALLBACK(gui_test_set_length), &sqg);

    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0.0, 0xffffffff, 1, 0, 0));
    wg1 = gtk_spin_button_new(adj, 1, 0);
    sqg.wseq = wg1;
    gtk_grid_attach(GTK_GRID(wg0), wg1, 1, 3, 1, 1);
    g_signal_connect(G_OBJECT(adj),"value_changed",G_CALLBACK(gui_test_set_sequence), &sqg);

    gtk_widget_show_all(wgm);

    while(gtk_dialog_run(GTK_DIALOG(wgm)) == GTK_RESPONSE_ACCEPT){
	gtk_widget_hide(wgm);
	sqg.generator(&sqg);
	gtk_widget_show_all(wgm);
    }
    gtk_widget_destroy(wgm);
}

char *gui_lookup_tag(const char *fmt, char *tmp, int size, const char *key_begin, const char *key_end)
{
    char *str, *x;
    tmp[0] = 0;
    if((str = strstr(fmt, key_begin))){
	str += strlen(key_begin);
	if(!(x = strstr(str, key_end))){
	    printf("[WARN] gui_checkbox(): missing %s in sequence %s ... %s\n", key_end, key_begin, key_end);
	    return NULL;
	};
	memset(tmp, 0, size);
	strncpy(tmp, str, x - str);
	return x + 1;
    }
    return NULL;
}

// global
static unsigned long gui_checkbox_result;

void gui_checkbox_action(GtkWidget *wg, int *value)
{
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wg)) )
	gui_checkbox_result |= *value;
    else
	gui_checkbox_result &= ~(*value);
}

unsigned long *gui_checkbox(geepro *gep, const char *fmt)
{
    GtkWidget *wd, *hbox, *vbox, *r;

    char *x = NULL, *str, t;
    char *tmp;
    int tmp_size;    
    int result = 0, lock;
    int cnt;

    tmp_size = strlen( fmt );
    if(!(tmp = (char *)malloc(tmp_size + 1))){
	printf("[WARN] gui_checkbox(): malloc error\n");
	return NULL;
    }
    gui_checkbox_result = 0;
    gui_lookup_tag(fmt, tmp, tmp_size, "[TITLE]", "[/TITLE]");
    wd = gtk_dialog_new_with_buttons(tmp, 
	GTK_WINDOW(GUI(gep->gui)->wmain), 
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	NULL 
    );    
    gui_lookup_tag(fmt, tmp, tmp_size, "[TYPE:", "]");
    if(!strcmp(tmp, "QS")) x = GTK_STOCK_DIALOG_QUESTION;
    if(!strcmp(tmp, "WN")) x = GTK_STOCK_DIALOG_WARNING;
    if(!strcmp(tmp, "ER")) x = GTK_STOCK_DIALOG_ERROR;
    if(!strcmp(tmp, "HL")) x = GTK_STOCK_HELP;
    if(!strcmp(tmp, "CR")) x = GTK_STOCK_STOP;
    if(!strcmp(tmp, "IF")) x = GTK_STOCK_DIALOG_INFO;
    if(!strcmp(tmp, "AU")) x = GTK_STOCK_DIALOG_AUTHENTICATION;
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(   gtk_dialog_get_content_area(GTK_DIALOG(wd))  ), hbox, TRUE, TRUE, 0);
    if( x ) 
	gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock( x, GTK_ICON_SIZE_DIALOG), FALSE, FALSE, 10 );
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_end(GTK_BOX(hbox), vbox, FALSE, 0, 0);    
    str = (char *)fmt; 
    cnt = 0;
    while( (str = gui_lookup_tag(str, tmp, tmp_size, "[CB:", "]")) && (cnt < MAX_CB_TABLE)){
	sscanf(tmp, " %i ", &result);
	if((x = strchr(tmp, ':'))) x++;
	t = *x;
	if((x = strchr(x, ':'))) x++;
	if((lock = strncmp(x, "LOCK:", 5)) == 0){
	    if((x = strchr(x, ':'))) x++;
	}
	r = gtk_check_button_new_with_label( x );
	gtk_box_pack_start( GTK_BOX(vbox), r, TRUE, TRUE, 2 );
	GUI(gep->gui)->cbtable[ cnt ] = result;
	g_signal_connect(G_OBJECT(r), "toggled", G_CALLBACK(gui_checkbox_action), (void *)(GUI(gep->gui)->cbtable + cnt)); // ptr as integer
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r), t == '1');
	gtk_widget_set_sensitive(GTK_WIDGET(r), !((lock == 0) && (t == '1')));
	cnt++;
    }
    
    gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(wd)));
    result = gtk_dialog_run(GTK_DIALOG(wd));    
    gtk_widget_destroy(GTK_WIDGET(wd));
    free(tmp);
    if(result != GTK_RESPONSE_ACCEPT) return NULL;
    return &gui_checkbox_result;
}

//*******************************************************************************************************************


typedef struct
{
    int i_par;
    int val;
    void *u_par;
    dlg_slider_cb cb;
} s_slider;

typedef struct gui_stack_ gui_stack;

struct gui_stack_
{
    GtkWidget *node;
    gui_stack *prev;
};

gui_stack *gstk = NULL;
gui_stack *sliders = NULL;
gui_stack *spins = NULL;

GtkWidget *dlg;

void gui_push(GtkWidget *it, gui_stack **stk)
{
    gui_stack *tmp;

    if(! stk ) return;
    if(!(tmp = (gui_stack *)malloc( sizeof(gui_stack) ))){
	ERR( E_T_MALLOC );
	return;
    }
    tmp->node = it;
    tmp->prev = *stk;
    *stk = tmp;
}

void gui_pop(gui_stack **stk)
{
    gui_stack *x;
    if( !stk ) return;
    if( !*stk ) return;
    x = *stk;
    *stk = (*stk)->prev;
    free( x );
}

void gui_free_stack(gui_stack **stk)
{
    while( !*stk ) gui_pop( stk );
}

GtkWidget *gui_get_stack(gui_stack *stk)
{
    if( ! stk ) return NULL;
    return stk->node;
}

void dialog_start(const char *title, int width, int height)
{
    dlg = gtk_dialog_new_with_buttons(title, NULL, 0, 
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	NULL
    );
    gtk_widget_set_size_request(dlg, width, height);
    
    gui_push( gtk_dialog_get_content_area(GTK_DIALOG( dlg )), &gstk);
}

void dialog_cleanup()
{
    gtk_widget_destroy( dlg );
    gui_free_stack( &gstk );
    while( !sliders ){
	if(sliders->node) free(sliders->node);
        gui_pop( &sliders );
    }
    while( !spins ){
	if(spins->node) free(spins->node);
        gui_pop( &spins );
    }

}

void dialog_end()
{
    gtk_widget_show_all( dlg ); 
    gtk_dialog_run( GTK_DIALOG( dlg ));
    dialog_cleanup();
}

void frame_start(const char *name)
{
    GtkWidget *tmp, *x;    
    
    tmp = gtk_frame_new( name );
    x = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER( tmp ), x );    
    gtk_container_add(GTK_CONTAINER( gui_get_stack( gstk ) ), tmp );
    gui_push( x, &gstk );
}

void frame_end()
{
    gui_pop( &gstk );
}

static void slider_cb(GtkRange *range, s_slider *ud)
{
    if( !ud || !range ) return;
    ud->val = gtk_range_get_value( range );    
    if(ud->cb) ud->cb( ud->val, ud->u_par, ud->i_par);
}

void slider_add(const char *label, int min, int max, int def, dlg_slider_cb cb, int i_par, void *u_par )
{
    GtkWidget *tmp, *x;
    s_slider *ss;
    
    if(!(ss = (s_slider *)malloc(sizeof( s_slider )))){
	ERR(E_T_MALLOC);
	return;
    }
    ss->i_par = i_par;
    ss->u_par = u_par;
    ss->cb = cb;
    tmp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    x = gtk_label_new( label );
    gtk_box_pack_start(GTK_BOX( tmp ), x, FALSE, FALSE, 10);    
    x = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 1);
    gtk_range_set_value( GTK_RANGE(x), def );    
    g_signal_connect(G_OBJECT(x), "value-changed", G_CALLBACK(slider_cb), ss);
    gui_push( (GtkWidget *)ss, &sliders);
    gtk_box_pack_start(GTK_BOX( tmp ), x, TRUE, TRUE, 10);    
    gtk_container_add(GTK_CONTAINER( gui_get_stack( gstk ) ), tmp );
}

static void spin_cb(GtkSpinButton *spin, s_slider *ud)
{
    if( !ud || !spin ) return;
    ud->val = gtk_spin_button_get_value( spin );    
    if(ud->cb) ud->cb( ud->val, ud->u_par, ud->i_par);
}


void spin_add(const char *label, int min, int max, int def, dlg_slider_cb cb, int i_par, void *u_par )
{
    GtkWidget *tmp, *x;
    s_slider *ss;
    
    if(!(ss = (s_slider *)malloc(sizeof( s_slider )))){
	ERR(E_T_MALLOC);
	return;
    }
    ss->i_par = i_par;
    ss->u_par = u_par;
    ss->cb = cb;
    tmp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    x = gtk_label_new( label );
    gtk_box_pack_start(GTK_BOX( tmp ), x, FALSE, FALSE, 10);    
    x = gtk_spin_button_new_with_range(min, max, 1);
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(x), def );    
    g_signal_connect(G_OBJECT(x), "value-changed", G_CALLBACK(spin_cb), ss);
    gui_push( (GtkWidget *)ss, &spins);
    gtk_box_pack_start(GTK_BOX( tmp ), x, TRUE, TRUE, 10);    
    gtk_container_add(GTK_CONTAINER( gui_get_stack( gstk ) ), tmp );
}

/***********************************************************************************************************************************/

static void gui_chip_tree_view_free( chip_tree *tree )
{
    chip_tree *tmp;
    while( tree ){
	if( tree->branch ) gui_chip_tree_view_free( tree->branch );		     
	if( tree->branch_flag ) free( tree->name );
	tmp = tree->next;
	free( tree );
	tree = tmp;	
    }
}

static void gui_chip_tree_view_create_node(geepro *gep, chip_tree *tree, GtkTreeStore *ts, GtkTreeIter *it)
{
    GtkTreeIter iter;    
    for( ;tree; tree = tree->next ){
	gtk_tree_store_append(ts, &iter, it);
	gtk_tree_store_set( ts, &iter, 0, tree->name, 1, !tree->branch_flag, -1);	
	if( tree->branch ){
	    gui_chip_tree_view_create_node(gep, tree->branch, ts, &iter);    
	    continue;
	}
    }
}

static void gui_chip_tree_view_create(geepro *gep, chip_tree *tree)
{
    GtkTreeStore *ts;
    ts = GTK_TREE_STORE(GUI(gep->gui)->chip_select_store);
    gui_chip_tree_view_create_node(gep, tree, ts, NULL);    
}

/**********************************************************************************************************************************/
static void gui_chip_tree_add(geepro *gep, const char *path, char *name, chip_tree **br)
{
    if(*path != '/'){
	printf("[WN] incorrect chip path name (expected '/'): \"%s\"", path);
	return;
    }        
    gui_chip_tree_add_node( path, name, br, 0);
}

static chip_tree *chip_add_node(chip_tree **br, char *name, chip_tree *branch_, void *key, char brf, int col)
{
    chip_tree *tm, *it;
    if(!( tm = (chip_tree *)malloc( sizeof(chip_tree)))){
	printf("ERR malloc\n");
	return *br;    
    }
    tm->branch = branch_;    
    tm->branch_flag = brf;
    tm->name = name;
//    tm->key = key;
    tm->next = NULL;
    if( *br == NULL ){
	*br = tm;
    } else {
	for( it = (*br); it->next; it = it->next);
	it->next = tm;
    }
    return tm;
}

// decomposing paths
static void gui_chip_tree_add_node(const char *path, char *name, chip_tree **br, int col)
{
    const char *tmp;
    char *link;
    int len;
    chip_tree *chpos, *tm;

    if(*path != '/'){ // finish
	tm = chip_add_node( br, name, NULL, NULL, 0, col);
	return;
    }
    path++;
    // take first link
    tmp = path;        
    for( ; *path; path++){
	if( *path != '/') continue;
	if( path == tmp ) break;	// first character
	if( *(path - 1) != '\\') break; // test if precedent character is '\', if so, ignore it
    }
    len = path - tmp;
    if(!(link = (char *)malloc( len + 1))){ // +1 for 0 flag
	printf("ERR malloc\n");
	return;
    };
    memset(link, 0, len + 1);
    strncpy(link, tmp, len);
    // now link is is acquired

    // looking link in queue
    for( chpos = *br; chpos; chpos = chpos->next){
	if( !chpos->branch_flag ) continue; // skip test if not branch
	if( !strcmp(chpos->name, link) ) break; // chpos will be branch address, or NULL if link not found
    }
    if( !chpos ){ // add new branch, and go deeper
	tm = chip_add_node( br, link, NULL, NULL, 1, col);	
	gui_chip_tree_add_node( path, name, &tm->branch, ++col);
	return;
    }
    // go deeper
    gui_chip_tree_add_node( path, name, &chpos->branch, ++col);
}

/*********************************************************************************************/

static void gui_help(geepro *gep)
{
    GtkWidget *wg, *wg1, *cta;
    GtkTextBuffer *gtb;
    FILE *f;
    char *text = NULL, *path = NULL;    
    int len;

    wg = gtk_dialog_new();
    gtk_widget_set_size_request(wg, 640, 480); // default window size
    gtk_window_set_title(GTK_WINDOW( wg), "Help");        
    cta = gtk_dialog_get_content_area( GTK_DIALOG( wg ) );
    wg1 = gtk_scrolled_window_new( NULL, NULL);
    gtk_box_pack_start(GTK_BOX(cta), wg1, TRUE, TRUE, 0);
    cta =  gtk_text_view_new_with_buffer( NULL );
    gtk_text_view_set_editable( GTK_TEXT_VIEW( cta ), FALSE);
    gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW( cta ), FALSE);
    gtb = gtk_text_view_get_buffer( GTK_TEXT_VIEW( cta ));
    gtk_container_add(GTK_CONTAINER( wg1 ), cta);
    // read file, temporary fixed path
//    if(cfp_get_string(gep->cfg, "/docfile", &path)){
//	printf("[WRN] Missing docfile variable in config\n");    
//    }
//printf( "--->%s \n", path);    
path = "./doc/doc_eng.txt";
    if( path ){
	if(!(f = fopen( path, "r"))){ 
//	    printf("[ERR] Missing help file\n");
	} else {
	    fseek(f, 0L, SEEK_END);    
	    len = ftell( f );
	    fseek(f, 0L, SEEK_SET);
	    if(len){
		text = (char *)malloc( len + 1 );
		if(fread( text, len, 1, f) != len){
//		    printf("[ERR] Read help file error\n");
		};	
		text[len] = 0;
	    }	
	    fclose( f );
	}
//        free( path );
    }
    if( text ){
	// set marker
	gtk_text_buffer_set_text( gtb, text, len );
	free( text );
	gtk_widget_show_all( wg );
	gtk_dialog_run( GTK_DIALOG( wg ) ); 
    }
    gtk_widget_destroy( wg );
}

void gui_menu_setup(geepro *gep)
{
    GtkWidget *wg0, *wg1, *wg2, *wg3, *wg4, *wg5;
    GtkToolItem *ti0;
    GtkStyle *style;

    GUI(gep->gui)->fct = -1;
    gtk_init(&gep->argc, &gep->argv);
    gui_add_images(gep);
    GUI(gep->gui)->action = NULL;
    GUI(gep->gui)->wmain = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(GUI(gep->gui)->wmain), gdk_pixbuf_new_from_xpm_data( LOGO_ICON ));
    g_signal_connect(G_OBJECT(GUI(gep->gui)->wmain), "delete_event", G_CALLBACK(gui_exit_program), gep);
    gtk_container_set_border_width(GTK_CONTAINER(GUI(gep->gui)->wmain), 1);
    gtk_window_set_title(GTK_WINDOW(GUI(gep->gui)->wmain), EPROGRAM_NAME " ver " EVERSION);
    gtk_widget_set_size_request(GUI(gep->gui)->wmain, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    gtk_widget_realize(GUI(gep->gui)->wmain);
    wg0 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);    
    gtk_container_add(GTK_CONTAINER(GUI(gep->gui)->wmain), wg0);
// connect/disconnect icon
    wg1 = gtk_frame_new( NULL );
    wg2 = gtk_button_new();
    wg4 = gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DND);    
    gtk_container_add(GTK_CONTAINER(wg2), wg4);
    g_signal_connect(G_OBJECT(wg2), "pressed", G_CALLBACK(gui_test_hw), gep);
    gtk_widget_set_tooltip_text(wg2, TEST_BUTTON_LB);
    gtk_container_add(GTK_CONTAINER(wg1), wg2);    
    GUI(gep->gui)->connected = wg4;

/* stripe Menu Bar */
    wg3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(wg0), wg3);
    gtk_box_pack_end(GTK_BOX(wg3), wg1, FALSE, FALSE, 0);
    wg4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(wg3), wg4, TRUE, TRUE, 0);
    wg3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(wg4), wg3, TRUE, TRUE, 0);
    wg1 = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(wg3), wg1, TRUE, TRUE, 0);
    style = gtk_widget_get_style( wg1 );
    wg5 = gtk_label_new(NULL);
    GUI(gep->gui)->connected_lbl = wg5;
    gtk_misc_set_padding(GTK_MISC(wg5), 10, 0);
    wg2 = gtk_event_box_new();
    if( style )
	gtk_widget_modify_bg( wg2, GTK_STATE_NORMAL, &style->bg[GTK_STATE_NORMAL]);
    gtk_container_add(GTK_CONTAINER(wg2), wg5);    
    gtk_box_pack_start(GTK_BOX(wg3), wg2, FALSE, FALSE, 0);

    gui_set_plug( gep );

/* Menu Help */
    wg2 = gtk_menu_item_new_with_label( MB_HELP );    
    gtk_menu_shell_append(GTK_MENU_SHELL(wg1), wg2);
    wg3 = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(wg2), wg3);
    /* help */
    wg2 = gtk_menu_item_new_with_label(MB_DOCUMENTATION);
    gtk_menu_shell_append(GTK_MENU_SHELL(wg3), wg2);    
    g_signal_connect(G_OBJECT(wg2), "activate", G_CALLBACK(gui_help), gep);
    /* spacer */
    wg2 = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(wg3), wg2);    
    /* about */
    wg2 = gtk_menu_item_new_with_label(MB_ABOUT_FILE);
    gtk_menu_shell_append(GTK_MENU_SHELL(wg3), wg2);    
    g_signal_connect(G_OBJECT(wg2), "activate", G_CALLBACK(gui_about), gep);

    /* spacer */
    wg2 = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(wg3), wg2);    
    /* exit */
    wg2 = gtk_menu_item_new_with_label(MB_EXIT_FILE);
    gtk_menu_shell_append(GTK_MENU_SHELL(wg3), wg2);    
    g_signal_connect(G_OBJECT(wg2), "activate", G_CALLBACK(gui_exit_program), gep);

/* toolbar */
    wg1 = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(wg1), GTK_TOOLBAR_ICONS);
    gtk_box_pack_start(GTK_BOX(wg4), wg1, FALSE, FALSE, 0);
    GUI(gep->gui)->toolbox = wg1;
    // static toolbar items

    ti0 = gtk_tool_button_new_from_stock( GTK_STOCK_PREFERENCES );
    g_signal_connect(G_OBJECT(ti0), "clicked", G_CALLBACK(gui_config), gep);
gtk_widget_set_sensitive(GTK_WIDGET(ti0), FALSE);
    gtk_tool_item_set_tooltip_text( ti0, CONFIG_TIP);
    gtk_toolbar_insert(GTK_TOOLBAR(wg1), ti0, -1);
    ti0 = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(wg1), ti0, -1);

/* Notebook */
    wg1 = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(wg1), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(wg0), wg1, TRUE, TRUE, 0);
    GUI(gep->gui)->notebook = wg1;

/* ------------------------------------------- NOTEBOOK pages ----------------------------------------------------------- */
/* ======================================== */
/* --> notebook page 1 'main page' <--- */
/* ======================================== */
    wg2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    wg5 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_end(GTK_BOX(wg2), wg5, FALSE, FALSE, 0);
    gtk_notebook_append_page( GTK_NOTEBOOK(wg1), wg2, gtk_label_new(LAB_NOTE_1) );
// Chip frame
    wg1 = gtk_frame_new(MB_DEVICE);
    gtk_container_set_border_width(GTK_CONTAINER(wg1), 3);
    gtk_box_pack_start(GTK_BOX(wg2), wg1, TRUE, TRUE, 0);
/* tabela pakujaca opis ukladu i bufor */
    wg3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(wg1), wg3);
    // selected chip name 
    wg4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    wg1 = gtk_label_new(DEVICE_ENTRY_LB);
    gtk_box_pack_start(GTK_BOX(wg4), wg1, FALSE, FALSE, 5);
    wg1 = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(wg1), FALSE);
    gtk_box_pack_start(GTK_BOX(wg4), wg1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(wg3), wg4, FALSE, FALSE, 0);
    GUI(gep->gui)->dev_entry = wg1;
    // Chip selection tree
    wg1 = gtk_scrolled_window_new( NULL, NULL);
    gtk_box_pack_start(GTK_BOX(wg3), wg1, TRUE, TRUE, 0);
    gui_setup_chip_selection_tree( gep, wg1 );

/* Programmer */
    GUI(gep->gui)->main_table = wg5; // point to attach XML GUI (view of programmer, dipsw etc )
    wg1 = gtk_frame_new(FR_NB_04_TITLE);
    gtk_box_pack_end(GTK_BOX(wg5), wg1, FALSE, FALSE, 0);
    wg3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GUI(gep->gui)->table = wg3;
    gtk_container_add(GTK_CONTAINER(wg1), wg3); 
    gtk_box_pack_start(GTK_BOX(wg3), wg1 = gui_prog_list(gep), FALSE, FALSE, 5);
    gui_add_iface_combox(gep);

/* Koniec inicjowania Gui */
    gui_set_default(gep);
    gui_xml_new(GUI(gep->gui)); /* zainicjowanie struktury gui_xml */
// test connection automaticaly
    g_timeout_add( 1000, (GSourceFunc)(gui_test_cyclic_hw), gep); // automatic test connection
    gui_port_devices_set( gep );
    
/* ======================================== */
/* --> notebook page 2 'Buffers' <--- */
/* ======================================== */
    GUI(gep->gui)->buffer_tabs = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(GUI(gep->gui)->buffer_tabs), GTK_POS_LEFT);
//    gtk_widget_set_sensitive(GTK_WIDGET(GUI(gep->gui)->buffer_tabs), FALSE);
//    gtk_widget_hide(GTK_WIDGET(GUI(gep->gui)->buffer_tabs));
    gtk_notebook_append_page( GTK_NOTEBOOK(GUI(gep->gui)->notebook), GUI(gep->gui)->buffer_tabs, gtk_label_new("Buffers") );
    if(gui_buffer_init((s_gui_buffer **)&GUI(gep->gui)->buffer, gep->store, GUI(gep->gui)->buffer_tabs)){
	ERR( "Creating buffers fail." );
    }
}

/*
static void gui_test_file( geepro *gep )
{
    long long time;

    const char *fname = gtk_entry_get_text(GTK_ENTRY(GUI(gep->gui)->file_entry));    

    if( !fname ) return;
    if( !fname[0] ) return;

    file_get_time(gep, &time, fname);

    if( GUI(gep->gui)->fct < 0){
	GUI(gep->gui)->fct = time;
	return;
    }
    
    if( GUI(gep->gui)->fct != time){
	if(gui_dialog_box(gep, "[QS][TEXT]"RELOAD_QUESTION"[/TEXT][BR]  NO  [BR]  YES  ", fname) == 2){
	    gui_refresh_button(NULL, gep);
	    return;
	}
    }
}


void gui_refresh_button(GtkWidget *wg, geepro *gep)
{
    const char *fname = gtk_entry_get_text(GTK_ENTRY(GUI(gep->gui)->file_entry));
    const char *err;


    err = file_load(gep, (char *)fname, -1, -1, -1);
    if( err ) 
	    gui_load_error_msg(gep, fname, err);
    else {
	gui_dialog_box(gep, "[IF][TEXT]File reloaded[/TEXT][BR]OK");
	
    }

    err = file_get_time(gep, &GUI(gep->gui)->fct, fname);
    if( err ) 
        gui_error_box(gep, "Error get creation time of file :\n%s\n%s", fname, err);    
    gui_buffer_refresh( GUI(gep->gui)->buffer );
}

*/

