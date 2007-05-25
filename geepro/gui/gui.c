/* $Revision: 1.7 $ */
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
#include "../src/gepro.h"
#include "../src/files.h"
#include "../src/buffer.h"
#include "../src/main.h"
#include "gui.h"
#include "gui_xml.h"
#include "../src/chip.h"
#include "../pixmaps/img_idx.h"
#include "../src/parport.h"
#include "../pixmaps/xpms.c"
#include "../drivers/hwplugin.h"
#include "../src/iface.h"

#include "bineditor.h"

typedef struct{
    int		idx;
    GtkWidget 	*wg;
    void	*next;
} fo_idx;

char		dialog_exit, dialog_code;
void		*p_wgts[256];	/* przechowuje wska�niki do widget�w */
conf_state	program_state;
fo_idx		*fo_mg = NULL, *fo_mg_first = NULL;
char		fo_cpage = 3; /* bo 3 zakladki maja zostac */
fo_idx		*ao_mg = NULL, *ao_mg_first = NULL;
char		progress_bar_exit;


typedef struct
{
    int 	id;
    GdkPixmap	*image;
    void	*next;
} rimg;

rimg  *r_img_root = NULL;

void gui_dynamic_widget_add(fo_idx **fo, fo_idx **ff ,int idx ,int type, char *name, int value, void *fnc, void *parm, char pos);
void gui_dynamic_widget_purge(fo_idx **fo, fo_idx **ff);
void gui_dialog_bt(GtkWidget *wg, char *name);
void gui_add_action(geepro *gep, char *stock_name, gchar *tip, int id);
void gui_device_menu_create(chip_plugins *plg, GtkWidget *wg, geepro *gep);
/***************************************************************************************************/

void gui_drawable_rfsh(GtkWidget *wg)
{
    GdkRectangle temp;
    temp.x = temp.y = 0;
    temp.width = wg->allocation.width;
    temp.height = wg->allocation.height;
    gtk_widget_draw(wg, &temp);
}

void gui_free_images()
{
    void *tmp;
    rimg *r_img;
    for(r_img = r_img_root; r_img;){
	 tmp = ((rimg *)r_img)->next;
	 free(r_img);
	 r_img = tmp; 
    }
}

int gui_register_image(geepro *gep, char **img)
{
    GdkBitmap *mask;
    rimg *r_img;
    rimg *new_image;
    int  idx;

    new_image = (rimg *)malloc(sizeof(rimg));
    new_image->next = NULL;
    new_image->image = gdk_pixmap_create_from_xpm_d(GTK_WIDGET(GUI(gep->gui)->wmain)->window, &mask, NULL, (gchar **)img);

    if(!r_img_root)
    {
	new_image->id = 1;
	r_img_root = new_image;
	atexit(gui_free_images);
	return 1;
    }

    for(idx = 1, r_img = r_img_root; ((rimg *)r_img)->next; idx++ ) r_img = ((rimg *)r_img)->next;
    idx++;
    new_image->id = idx;
    ((rimg *)r_img)->next = new_image;
    return idx;
}

GdkPixmap *gui_get_image(int id)
{
    rimg *r_img;
    for(r_img = r_img_root; r_img; r_img = ((rimg *)r_img)->next )
	if( ((rimg *)r_img)->id == id) return ((rimg *)r_img)->image;
    return NULL;
}

/***************************************************************************************************/
void gui_stat_rfsh(geepro *gep)
{
    char tmp_str[40];

    if(gep->chp){
	gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->dev_entry), gep->chp->chip_name);
	sprintf(tmp_str, "0x%x", gep->chp->dev_size); 
	gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->buffer_entry), tmp_str);  
	sprintf(tmp_str, "0x%x", gep->chp->checksum); 
	gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->crc_entry), tmp_str);  
    } else {
	gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->dev_entry), "--------");
	sprintf(tmp_str,"0x%x", 0); gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->buffer_entry), tmp_str);  
	sprintf(tmp_str,"0x%x", 0); gtk_entry_set_text(GTK_ENTRY(GUI(gep->gui)->crc_entry), tmp_str);  
    }
//  - dopisac odswiezenie status bar
//    GUI(gep->gui)->pict_view = (GUI(gep->gui)->pcb) ? GUI(gep->gui)->pict_willem : GUI(gep->gui)->pict_pcb;
//    gui_draw_dip_switch(gep);
//    gui_draw_pict(gep);
//    gui_viewer_rfsh(gep);
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

static void gui_load_file(GtkWidget *w, geepro *gep)
{ 
    GtkWidget *wg;    

    wg = gtk_file_chooser_dialog_new(
	    "Open file", GUI(gep->gui)->wmain, 
	    GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
	    GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, 
	    GTK_RESPONSE_ACCEPT, NULL
	);

    if(gtk_dialog_run(GTK_DIALOG(wg)) == GTK_RESPONSE_ACCEPT){
	char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wg));
	if(file_load(gep, fname)){
	    gui_error_box(gep, "Error loading file");
	}
	g_free(fname);
    }

    gtk_widget_destroy(wg);    
}

static void gui_save_file(GtkWidget *w, geepro *gep)
{ 
    GtkWidget *wg;    

    wg = gtk_file_chooser_dialog_new(
	    "Save file", GUI(gep->gui)->wmain, 
	    GTK_FILE_CHOOSER_ACTION_SAVE,GTK_STOCK_CANCEL,
	    GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
	    GTK_RESPONSE_ACCEPT, NULL
	);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(wg), TRUE);


    if(gtk_dialog_run(GTK_DIALOG(wg)) == GTK_RESPONSE_ACCEPT){
	char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wg));
	if(file_save(gep, fname)){
	    gui_error_box(gep, "Error saving file");
	}
	g_free(fname);
    }

    gtk_widget_destroy(wg);    

}

void gui_device_sel(GtkWidget *wg, geepro *gep) 
{ 
    char *name = NULL;
    chip *tmp;
    chip_plugins *plg = gep->ifc->plugins;
    
    /* jesli nie mozna pobrac nazwy ukladu to wyjdz */
    if(!GTK_BIN(wg)->child) return;
    if(!GTK_IS_LABEL(GTK_BIN(wg)->child)) return;
    /* pobierz nazwe wybranego ukladu */
    gtk_label_get(GTK_LABEL(GTK_BIN(wg)->child), &name);

    if(!(tmp = chip_lookup_chip(plg, name))){
	gui_error_box(gep, NO_CHIP_PLUGIN);
	return;
    }

    /* Usuniecie bieżacego ukladu */
    if(gep->chp){
//	    gui_adv_option_purge(gep);
/* ten sposob do modyfikacji */
	    if(gep->chp->read_chip)
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_READ]));
	    if(gep->chp->verify_chip)	    
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_VERIFY]));
	    if(gep->chp->read_sig_chip)	
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_SIGNATURE]));
	    if(gep->chp->test_chip)	
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_TESTBLANK]));
	    if(gep->chp->write_chip)
	    	gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_WRITE]));
	    if(gep->chp->erase_chip)	
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_ERASE]));
	    if(gep->chp->lock_chip)	
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_LOCK]));
	    if(gep->chp->unlock_chip)	
		gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[CHIP_UNLOCK]));
/*
    Wersja na przyszłą wersję:
    for(i = 0; i < GUI_MAX_ACTIONS; i++)
	if(gep->chp->action[i]) gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->action[i]));
*/
	    free(gep->chp->buffer);
    }
    /* Ustawienie nowego bieżacego ukladu */
    gep->chp = tmp; 

    if(!(tmp->buffer = (char *)malloc(tmp->dev_size))){
	printf("{gui.c} gui_device_sel() ---> memory alocation for buffer error.\n" );
	gep->chp = NULL;
	return;
    }
    memset(tmp->buffer, 0, tmp->dev_size);

/* do usuniecia */
    SET_BUFFER(tmp->buffer);

    gui_bineditor_set_buffer(GUI(gep->gui)->bineditor, tmp->dev_size, (unsigned char*)tmp->buffer);
    
/* do modyfikacji */
	if(tmp->read_chip)
	    gui_add_action(gep, GTK_STOCK_CONVERT, READ_CHIP_TIP, CHIP_READ );
	if(tmp->verify_chip)	    
	    gui_add_action(gep, GTK_STOCK_COPY, VERIFY_CHIP_TIP, CHIP_VERIFY );
	if(tmp->read_sig_chip)	
	    gui_add_action(gep, GTK_STOCK_COLOR_PICKER, ID_CHIP_TIP, CHIP_SIGNATURE );
	if(tmp->test_chip)	
	    gui_add_action(gep, GTK_STOCK_DIALOG_QUESTION, BLANK_CHIP_TIP, CHIP_TESTBLANK );
	if(tmp->write_chip)	
	    gui_add_action(gep, GTK_STOCK_EDIT, PROGRAM_CHIP_TIP, CHIP_WRITE );
	if(tmp->erase_chip)	
	    gui_add_action(gep, GTK_STOCK_DELETE, ERASE_CHIP_TIP, CHIP_ERASE );
	if(tmp->lock_chip)	
	    gui_add_action(gep, GTK_STOCK_DIALOG_AUTHENTICATION, LB_CHIP_TIP, CHIP_LOCK );
	if(tmp->unlock_chip)	
	    gui_add_action(gep, GTK_STOCK_CUT, ULB_CHIP_TIP, CHIP_UNLOCK );
/*
    Wersja na przyszłą wersję:
    for(i = 0; i < GUI_MAX_ACTIONS; i++)
	if(gep->chp->action[i]) gui_add_action(gep, i );
*/

//	GUI(gep->gui)->dpsw_state = tmp->dip_switch;

	if(tmp->autostart)
	     tmp->autostart(wg, gep);

	gui_stat_rfsh(gep);
}

void gui_rfsh_gtk(void)
{
    while(gtk_events_pending()) gtk_main_iteration();
}

void gui_about(GtkWidget *wg, geepro *gep)
{ 
    gui_dialog_box(gep, "[IF][TEXT]"ABOUT"[/TEXT][BR]  OK  "); 
}

/************************************************************************************************************************/

char gui_dig2hex(char i){
    return i + (i < 10 ? '0'  : 'A' - 10 );
}

char gui_ascii_filter(unsigned char i){
    if((i<33)||(i>127)) i='.';
    return i;
}



char *gui_bin2hex(char *str,int value, int length){
    int i;

    *( str += length) = 0;
    for(i = 0; i < length; i++, value >>= 4 ) 
	*(--str) = gui_dig2hex(value & 0x0f);    
    return str;
}

/************************************************************************************************************************/

void *gui_submenu_add(void *op, char *name, void *gep)
{
    GtkWidget *p, *grp;
    
    grp = gtk_menu_item_new_with_label(name);
    gtk_menu_bar_append(GTK_MENU(op), grp);
    p = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(grp), p);
    return p;
}

void gui_menu_chip_add(chip_plugins *plg, void *sm, void *gep)
{
    GtkWidget *tt;
    tt = gtk_menu_item_new_with_label(plg->menu_sel->chip_name);
    gtk_menu_append(GTK_MENU(sm), tt);
    gtk_signal_connect(GTK_OBJECT(tt), "activate", GTK_SIGNAL_FUNC(gui_device_sel), gep);
}

void gui_device_menu_create(chip_plugins *plg, GtkWidget *wg, geepro *gep)
{
    chip_menu_create(plg, wg, gui_submenu_add, gui_menu_chip_add, gep);
}

void gui_add_action(geepro *gep, char *stock_name, gchar *tip, int id)
{
    GtkWidget *tmp;

    tmp = gtk_image_new_from_stock(stock_name, gtk_toolbar_get_icon_size(GTK_TOOLBAR(GUI(gep->gui)->toolbox)));    
/* do usuniecia w przyszlosci */
    switch(id){
	case 0: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->read_chip), gep); break;
	case 1: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->verify_chip), gep); break;
	case 2: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->read_sig_chip), gep); break;
	case 3: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->test_chip), gep); break;
	case 4: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->write_chip), gep); break;
	case 5: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->erase_chip), gep); break;
        case 6: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->lock_chip), gep); break;	
	case 7: GUI(gep->gui)->action[id] = gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp, GTK_SIGNAL_FUNC(gep->chp->unlock_chip), gep); break;
    }

/* przyszla wersja */
//    gtk_toolbar_append_item(GTK_TOOLBAR(GUI(gep->gui)->toolbox), NULL, tip, NULL, tmp,  GTK_SIGNAL_FUNC(gep->chp->action[id]), gep);

}

/***************************************************************************************************************************/

static void gui_build_iface_menu(iface *ifc, int cl, char *name, char *dev, GtkWidget *wg)
{
    gtk_combo_box_append_text(GTK_COMBO_BOX(wg), name);
}

static void gui_iface_sel(GtkWidget *wg, geepro *gep)
{
    char *name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(wg));

    if(!name) return;
    if(iface_select_iface(gep->ifc, name)){
	gui_error_box(NULL,"Open device error !!!\n No such device.\n");
	gtk_combo_box_set_active(GTK_COMBO_BOX(wg), gep->ifc->ifc_sel);
	return;
    }

    gep->ifc->ifc_sel = gtk_combo_box_get_active(GTK_COMBO_BOX(wg));

    gui_stat_rfsh(gep);

    test_hw(NULL, gep);

/*!! dopisac zapis do pliku konfiguracyjnego nowego ustawienia */

}

static GtkWidget *gui_iface_list(geepro *gep)
{
    GtkWidget *combox;

    combox = gtk_combo_box_new_text();
    iface_search(gep->ifc, gep->ifc->cl, (iface_cb)gui_build_iface_menu, GTK_COMBO_BOX(combox));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combox), gep->ifc->ifc_sel);
    gtk_signal_connect(GTK_OBJECT(combox), "changed", GTK_SIGNAL_FUNC(gui_iface_sel), gep);

    return combox;
}

static void gui_add_iface_combox(geepro *gep)
{    
    GtkWidget *tmp = gui_iface_list(gep);
    
    gtk_table_attach(GTK_TABLE(GUI(gep->gui)->table), tmp, 2, 4, 1, 2, GTK_FILL | GTK_EXPAND, 0, 5, 5);
    GUI(gep->gui)->iface = tmp;
    gtk_widget_show(tmp);
}

static void gui_prog_sel(GtkWidget *wg, geepro *gep)
{
    char *name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(wg));
    iface_prg_api api;    

    if(!name) return;

    if(!(api = iface_get_func(gep->ifc, name))){
	gui_error_box(NULL,"Brak pluginu do programatora.\n");
	gtk_combo_box_set_active(GTK_COMBO_BOX(wg), gep->ifc->prog_sel);
	return;
    }
    gep->ifc->prog_sel = gtk_combo_box_get_active(GTK_COMBO_BOX(wg));
    /* usuniecie listy interfejsów*/
    gtk_widget_destroy(GUI(gep->gui)->iface);

//    ifc->ifc_sel = ??? odczytac z konfiguracji dla danego programatora
    gep->ifc->ifc_sel = 0;
    hw_destroy(gep);
    ___hardware_module___ = api;
    gep->ifc->cl = hw_get_iface();
    /* utworzenie wyboru interfaców */
    gui_add_iface_combox(gep);
    /* usuniecie menu */
    gui_xml_destroy(GUI(gep->gui)->xml);    
    /* wywolanie gui dla programatora */
    hw_gui_init(gep);

    /* inicjowanie portu, trzeba wysłac sygnał do interfejsu, ze został zmieniony i wymusiś zainicjowanie */
    gtk_signal_emit_by_name(GTK_OBJECT(GUI(gep->gui)->iface), "changed");    
//    printf("|-->programator %s\n", name);
/*!! dopisac zapis do pliku konfiguracyjnego nowego ustawienia */
}


static void gui_build_prg_menu(iface *ifc, char *name, GtkWidget *wg)
{
    gtk_combo_box_append_text(GTK_COMBO_BOX(wg), name);
}


static GtkWidget *gui_prog_list(geepro *gep)
{
    GtkWidget *combox;

    combox = gtk_combo_box_new_text();

    iface_list_prg(gep->ifc, (iface_prg_func)gui_build_prg_menu, GTK_COMBO_BOX(combox));

    gtk_combo_box_set_active(GTK_COMBO_BOX(combox), gep->ifc->prog_sel);
    gtk_signal_connect(GTK_OBJECT(combox), "changed", GTK_SIGNAL_FUNC(gui_prog_sel), gep);
    GUI(gep->gui)->prog_combox = combox;

    return combox;
}

/********************************************************************************************************************/
void gui_config(GtkWidget *wg, geepro *gep)
{
    printf("config\n");
}

void gui_set_default(geepro *gep)
{
//    gui_willem_set_defaults(gep);
    gtk_widget_show_all(GUI(gep->gui)->wmain);
    gui_stat_rfsh(gep);
}

#define GUI_STATUSBAR_ID_BUFFER 1

void gui_set_statusbar(geepro *gep, char *tmp, char *str, ...)
{	
    va_list v;
    
    va_start(v, str);
    vsprintf(tmp, str, v);
    va_end(v);

    gtk_statusbar_pop(GUI(gep->gui)->status_bar, GUI_STATUSBAR_ID_BUFFER);
    gtk_statusbar_push(GUI(gep->gui)->status_bar, GUI_STATUSBAR_ID_BUFFER, tmp);
}

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

void gui_menu_setup(geepro *gep)
{
    GtkWidget *wg0, *wg1, *wg2, *wg3;

    gtk_init(&gep->argc, &gep->argv);

    gui_add_images(gep);

    GUI(gep->gui)->wmain = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect(GTK_OBJECT(GUI(gep->gui)->wmain), "delete_event", GTK_SIGNAL_FUNC(gui_exit_program), gep);
    gtk_container_border_width(GTK_CONTAINER(GUI(gep->gui)->wmain), 1);
    gtk_window_set_title(GTK_WINDOW(GUI(gep->gui)->wmain), EPROGRAM_NAME " ver " EVERSION);
    gtk_widget_set_usize(GUI(gep->gui)->wmain, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    gtk_widget_realize(GUI(gep->gui)->wmain);

    wg0 = gtk_vbox_new(FALSE, 0);    
    gtk_container_add(GTK_CONTAINER(GUI(gep->gui)->wmain), wg0);

/* pasek Menu Bar */
    wg1 = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);

/* menu File */
    wg2 = gtk_menu_item_new_with_label(MB_FILE);
    gtk_menu_bar_append(GTK_MENU_BAR(wg1), wg2);
    wg3 = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(wg2), wg3);
    /* load file */
    wg2 = gtk_menu_item_new_with_label(MB_LOAD_BIN_FILE);
    gtk_menu_append(GTK_MENU(wg3), wg2);    
    gtk_signal_connect(GTK_OBJECT(wg2), "activate", GTK_SIGNAL_FUNC(gui_load_file), gep);
    /* save file */
    wg2 = gtk_menu_item_new_with_label(MB_SAVE_BIN_FILE);
    gtk_menu_append(GTK_MENU(wg3), wg2);    
    gtk_signal_connect(GTK_OBJECT(wg2), "activate", GTK_SIGNAL_FUNC(gui_save_file), gep);
    /* spacer */
    wg2 = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(wg3), wg2);    
    /* about */
    wg2 = gtk_menu_item_new_with_label(MB_ABOUT_FILE);
    gtk_menu_append(GTK_MENU(wg3), wg2);    
    gtk_signal_connect(GTK_OBJECT(wg2), "activate", GTK_SIGNAL_FUNC(gui_about), gep);
    /* exit */
    wg2 = gtk_menu_item_new_with_label(MB_EXIT_FILE);
    gtk_menu_append(GTK_MENU(wg3), wg2);    
    gtk_signal_connect(GTK_OBJECT(wg2), "activate", GTK_SIGNAL_FUNC(gui_exit_program), gep);

/* Menu device */
    wg2 = gtk_menu_item_new_with_label(MB_DEVICE);    
    gtk_menu_bar_append(GTK_MENU_BAR(wg1), wg2);
    wg3 = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(wg2), wg3);
    GUI(gep->gui)->mb_dev = wg3;

/* toolbar */
    wg1 = gtk_toolbar_new(TOOLBAR_PARAMS);
    gtk_box_pack_start(GTK_BOX(wg0), wg1, FALSE, FALSE, 0);
    GUI(gep->gui)->toolbox = wg1;
    /* stale pozycje paska */
    wg2 = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_append_item(GTK_TOOLBAR(wg1), NULL, OPEN_FILE_TIP, NULL, wg2, GTK_SIGNAL_FUNC(gui_load_file), gep);
    wg2 = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_append_item(GTK_TOOLBAR(wg1), NULL, SAVE_FILE_TIP, NULL, wg2, GTK_SIGNAL_FUNC(gui_save_file), gep);
    wg2 = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_append_item(GTK_TOOLBAR(wg1), NULL, PROPERTIES_TIP, NULL, wg2, GTK_SIGNAL_FUNC(gui_config), gep);
    gtk_toolbar_append_space(GTK_TOOLBAR(wg1));

/* Notebook */
    wg1 = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(wg1), GTK_POS_TOP);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    GUI(gep->gui)->notebook = wg1;
/* Status bar */
    wg2 = gtk_statusbar_new();
    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(wg2), FALSE);
    gtk_box_pack_start(GTK_BOX(wg0), wg2, FALSE, FALSE, 0);
    GUI(gep->gui)->status_bar = wg2;

/* ------------------------------------------- strony NOTEBOOKA ----------------------------------------------------------- */
/* ======================================== */
/* --> notebook page 1 'strona glowna' <--- */
/* ======================================== */
    wg2 = gtk_table_new(2, 2, FALSE); /* tabela pakujaca karty glownej */
    GUI(gep->gui)->main_table = wg2;
    wg3 = gtk_label_new(LAB_NOTE_1);
    gtk_notebook_append_page(GTK_NOTEBOOK(wg1), wg2, wg3);
/* Ramka ukladu */
    wg1 = gtk_frame_new("Uklad");
    gtk_container_border_width(GTK_CONTAINER(wg1), 3);
    gtk_table_attach_defaults(GTK_TABLE(wg2), wg1,  0, 1, 0, 1);
    /* tabela pakujaca opis ukladu i bufor */
    wg3 = gtk_table_new( 2, 4, FALSE);
    gtk_container_border_width(GTK_CONTAINER(wg3), 3);
    gtk_container_add(GTK_CONTAINER(wg1), wg3);
    /* Nazwa wybranego ukladu */    
    wg1 = gtk_label_new(DEVICE_ENTRY_LB);
    gtk_misc_set_alignment(GTK_MISC(wg1), 0, 0);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  0,1,0,1, GTK_FILL, 0, 0,0);
    wg1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(wg1), FALSE);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  1,2,0,1, GTK_FILL, 0, 0,0);
    GUI(gep->gui)->dev_entry = wg1;
    /* Rozmiar bufora/pamieci */    
    wg1 = gtk_label_new(SIZE_HEX_LB);
    gtk_misc_set_alignment(GTK_MISC(wg1), 0, 0);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  0,1,1,2, GTK_FILL, 0, 0,0);
    wg1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(wg1), FALSE);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  1,2,1,2, GTK_FILL, 0, 0,0);
    GUI(gep->gui)->buffer_entry = wg1;
    /* Suma CRC */    
    wg1 = gtk_label_new(CHECKSUM_LB);
    gtk_misc_set_alignment(GTK_MISC(wg1), 0, 0);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  0,1,2,3, GTK_FILL, 0, 0,0);
    wg1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(wg1), FALSE);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  1,2,2,3, GTK_FILL, 0, 0,0);
    GUI(gep->gui)->crc_entry = wg1;
    /* opis ukladu */
    wg1 = gtk_frame_new("Opis ukladu");
    gtk_table_attach_defaults(GTK_TABLE(wg3), wg1,  0, 2, 3, 4);
    wg3 = gtk_label_new("brak");
    gtk_container_add(GTK_CONTAINER(wg1), wg3);
    GUI(gep->gui)->chip_desc = wg3;

/* Ramka programatora */
    /* opcje programatora */
    wg1 = gtk_frame_new(FR_NB_04_TITLE);
    gtk_container_border_width(GTK_CONTAINER(wg1), 3);
    gtk_table_attach(GTK_TABLE(wg2), wg1,  0, 1, 1, 2,  GTK_FILL | GTK_EXPAND,0, 0,0);
    wg3 = gtk_table_new(3, 4, FALSE);
    GUI(gep->gui)->table = wg3;
    gtk_container_add(GTK_CONTAINER(wg1), wg3);    
    wg1 = gtk_label_new("Programator:");
    gtk_misc_set_alignment(GTK_MISC(wg1), 0, 0);
    gtk_table_attach( GTK_TABLE(wg3), wg1, 0, 2, 0, 1,  GTK_FILL, 0, 5, 5);
    wg1 = gtk_label_new("Interface:");
    gtk_misc_set_alignment(GTK_MISC(wg1), 0, 0);
    gtk_table_attach( GTK_TABLE(wg3), wg1, 0, 2, 1, 2,  GTK_FILL, 0, 5, 5);
    wg1 = gtk_button_new_with_label("Test Connection");
    gtk_signal_connect(GTK_OBJECT(wg1), "clicked", GTK_SIGNAL_FUNC(test_hw), gep);
    gtk_table_attach(GTK_TABLE(wg3), wg1,  3, 4, 2, 3,  GTK_FILL, 0 , 5, 5);
    gtk_table_attach(GTK_TABLE(wg3), gui_prog_list(gep),  2, 4, 0, 1, GTK_FILL | GTK_EXPAND, 0, 5, 5);
    gui_add_iface_combox(gep);


/* ======================================= */
/* -----> notebook page 2 'bufor' <------- */
/* ======================================= */
    wg1 = GUI(gep->gui)->notebook;
    wg0 = gui_bineditor_new(GUI(gep->gui)->wmain);
    gui_bineditor_connect_statusbar(GUI_BINEDITOR(wg0), GUI(gep->gui)->status_bar);
    wg3 = gtk_label_new("Bufor");
    gtk_notebook_append_page(GTK_NOTEBOOK(wg1), wg0, wg3);
    GUI(gep->gui)->bineditor = wg0;

/* Koniec inicjowania Gui */
    gui_set_default(gep);
    gui_xml_new(GUI(gep->gui)); /* zainicjowanie struktury gui_xml */
}

static char gui_progress_bar_exit = 0;

void gui_progress_break(geepro *gep)
{
    gui_progress_bar_exit=1;
}

void gui_progress_bar_init(geepro *gep, char *title, long range)
{
    GtkWidget *wg0, *wg1;

    wg0 = gtk_window_new(GTK_WINDOW_DIALOG);
    GUI(gep->gui)->progress_win = wg0;
    gtk_window_set_position(GTK_WINDOW(wg0), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable(GTK_WINDOW(wg0), FALSE);
    gtk_window_set_modal(GTK_WINDOW(wg0), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(wg0), GTK_WINDOW(GUI(gep->gui)->wmain));
    gtk_signal_connect(GTK_OBJECT(wg0), "destroy", GTK_SIGNAL_FUNC(gui_progress_break), NULL);
    gtk_window_set_title(GTK_WINDOW(wg0), title);

    wg1 = gtk_progress_bar_new_with_adjustment( GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, range, 1, 0, 0)));
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    GUI(gep->gui)->progress_bar = wg1;
    gui_rfsh_gtk();
    gui_progress_bar_exit = 0;

    gtk_widget_show_all(wg0);
}

char gui_progress_bar_set(geepro *gep, long value)
{
    if(gui_progress_bar_exit) return 1;
    gtk_progress_set_value(GTK_PROGRESS(GUI(gep->gui)->progress_bar), value);
    gui_rfsh_gtk();
    return 0;
}

void gui_progress_bar_free(geepro *gep)
{
    if(gui_progress_bar_exit) return;
    gtk_widget_destroy(GUI(gep->gui)->progress_win);
}

static int gui_dialog_exit = 0;

static void gui_dialog_box_close(GtkWidget *wg, int i)
{
    gui_dialog_exit = 1; /* wyjście z petli */

    if(GTK_IS_WINDOW(wg) == TRUE){
	 gtk_widget_destroy(wg);
	 return;
    }
    
    gui_dialog_exit = i;

    gtk_widget_destroy(wg->parent->parent->parent);
}


int gui_dialog_box(geepro *gp, const char *en, ...)
{
    GtkWidget *wg0, *wg1, *wgtab, *wdialog;
    char *image = NULL;
    char *markup, *title;
    char *fmt, *ft, *ex;
    char flag;
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
    gtk_signal_connect(GTK_OBJECT(wdialog),"delete_event",GTK_SIGNAL_FUNC(gui_dialog_box_close), NULL);
    gtk_window_set_title(GTK_WINDOW(wdialog), title);
    gtk_container_border_width(GTK_CONTAINER(wdialog), 10);
    gtk_window_set_modal(GTK_WINDOW(wdialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(wdialog), GTK_WINDOW(GUI(gp->gui)->wmain));

    /* tabela pakujaca */
    wgtab = gtk_table_new(2, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(wdialog), wgtab);    

    if(image){
	wg0 = gtk_image_new_from_stock( image, GTK_ICON_SIZE_DIALOG);
	gtk_table_attach(GTK_TABLE(wgtab), wg0, 0,1,0,1 ,GTK_FILL, GTK_FILL|GTK_EXPAND, 0,0);
    }

    /* dodanie tekstu */
    wg0 = gtk_label_new(NULL);
    va_start(ap, en);
    markup = g_markup_vprintf_escaped(ft, ap);
    va_end(ap);
    gtk_label_set_markup(GTK_LABEL(wg0), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(wgtab), wg0, 1,2,0,1 ,GTK_FILL, GTK_FILL|GTK_EXPAND, 0,0);

    /* przyciski */
    ft = strchr(ex + 1, ']') + 1; /* koniec klamerki */

    wg0 = gtk_hbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(wg0), 3);
    gtk_table_attach(GTK_TABLE(wgtab), wg0, 0,2,1,2 ,GTK_FILL, GTK_FILL, 0,0);

    button = 2;
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
        gtk_signal_connect(GTK_OBJECT(wg1),"clicked",GTK_SIGNAL_FUNC(gui_dialog_box_close), (void*)button++);
	if(ex) *ex = '[';
	ft = ex;
    } while(ex);
    
    free(fmt);
    gtk_widget_show_all(wdialog);

    for(;!gui_dialog_exit;) gtk_main_iteration();

    return gui_dialog_exit - 1;
}

void gui_run(geepro *gep)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(GUI(gep->gui)->notebook), 0);
    gui_device_menu_create(gep->ifc->plugins, GUI(gep->gui)->mb_dev, gep);
    gtk_widget_show_all(GUI(gep->gui)->wmain);
    test_uid(gep);
    /* inicjowanie domyślnego plugina strownika programatora */
    gtk_signal_emit_by_name(GTK_OBJECT(GUI(gep->gui)->prog_combox), "changed");
    gtk_main(); /* jesli programator ok to startuj program inaczej wyjdź */
}

void gui_kill_me(geepro *gep)
{
    printf("pa pa.\n");
    /* Usuniecie biezacego GUI zbudowanego o xml */
    gui_xml_destroy(GUI(gep->gui)->xml);
    free(GUI(gep->gui)->xml);
    gtk_main_quit();
    if(GET_BUFFER) free(GET_BUFFER);
}

void gui_dont_kill_me(GtkWidget *k, gpointer gd)
{
    gtk_widget_destroy(GTK_WIDGET(gd));
}

char gui_cmp_pls(geepro *gep, int a,int b)
{
    char test = a < b;
    if(!test ) gui_progress_bar_free(gep);
    return test;
}
