/* $Revision: 1.2 $ */
/* hex, binary viewer, editor, kontrolka GTK
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
#include <string.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>
#include "gui_xml.h"

static void gui_xml_parse_element(gui *g, GtkWidget *wg, xmlDocPtr doc, xmlNode *cur, const char *parm);

static void gui_xml_container_add(gui *g, xmlNode *cur, xmlDocPtr doc, GtkWidget *parent, GtkWidget *child, char recursive, const char *parm)
{
    int b0=0, b1=0, b2=0, b3=0, flagx, flagy, spx, spy;
    char *pos;

    if(recursive && cur->xmlChildrenNode){
	gui_xml_parse_element(g, child, doc, cur->xmlChildrenNode, parm );
    }
    
    b0 = b1 = b2 = b3 = spx = 0;
    pos = (char *)xmlGetProp(cur, (unsigned char *)"expand");
    if(pos) b0 = !strcmp(pos, "true");
    pos = (char *)xmlGetProp(cur, (unsigned char *)"fill");
    if(pos) b1 = !strcmp(pos, "true");
    pos = (char *)xmlGetProp(cur, (unsigned char *)"border");
    if(pos) b2 = atoi(pos), spx=1;
    pos = (char *)xmlGetProp(cur, (unsigned char *)"box_end");
    if(pos) b3 = !strcmp(pos, "true");
    if(spx){
	if(b3)
	    gtk_box_pack_end(GTK_BOX(parent), child, b0, b1, b2);
	else
	    gtk_box_pack_start(GTK_BOX(parent), child, b0, b1, b2);
	return;
    }

    b0 = b1 = b2 = b3 = 0;
    pos = (char *)xmlGetProp(cur, (unsigned char *)"expandx");
    if(pos) b0 = !strcmp(pos, "true");    
    pos = (char *)xmlGetProp(cur, (unsigned char *)"expandy");
    if(pos) b1 = !strcmp(pos, "true");
    pos = (char *)xmlGetProp(cur, (unsigned char *)"fillx");
    if(pos) b2 = !strcmp(pos, "true");
    pos = (char *)xmlGetProp(cur, (unsigned char *)"filly");
    if(pos) b3 = !strcmp(pos, "true");

    if(b0) b0 = GTK_EXPAND;
    if(b1) b1 = GTK_EXPAND;
    if(b2) b2 = GTK_FILL;
    if(b3) b3 = GTK_FILL;

    flagx = b0 | b2;
    flagy = b1 | b3;    

    spx = spy = 0;    
    pos = (char *)xmlGetProp(cur, (unsigned char *)"spacex");
    if(pos) spx = atoi(pos);    
    pos = (char *)xmlGetProp(cur, (unsigned char *)"spacey");
    if(pos) spy = atoi(pos);
    pos = (char *)xmlGetProp(cur, (unsigned char *)"pos");

    if(pos){
	sscanf(pos, "%i, %i, %i, %i", &b0, &b1, &b2, &b3);
	gtk_table_attach(GTK_TABLE(parent), child, b0, b1, b2, b3, flagx, flagy, spx, spy);
    }else
	gtk_container_add(GTK_CONTAINER(parent), child);
}

static GtkWidget *gui_xml_frame_new(xmlNode *cur)
{
    char *label, *align, *shadow;
    int a0=0, a1=0;
    int bn;
    GtkWidget *tmp;

    label = (char *)xmlGetProp(cur, (unsigned char *)"label");
    align = (char *)xmlGetProp(cur, (unsigned char *)"align");
    shadow = (char *)xmlGetProp(cur, (unsigned char *)"shadow");
    tmp = gtk_frame_new(label);
    if(align){
	sscanf(align, "%i, %i", &a0, &a1);
	gtk_frame_set_label_align(GTK_FRAME(tmp), (float)a0 / 100, (float)a1 / 100);
    }
    if(shadow){
	bn = GTK_SHADOW_NONE;
	if(!strcmp(shadow, "in")) bn = GTK_SHADOW_IN;
	 else
	if(!strcmp(shadow, "out")) bn = GTK_SHADOW_OUT;		
	 else
	if(!strcmp(shadow, "etched_in")) bn = GTK_SHADOW_ETCHED_IN;
	 else
	if(!strcmp(shadow, "etched_out")) bn = GTK_SHADOW_ETCHED_OUT;
	gtk_frame_set_shadow_type(GTK_FRAME(tmp), bn);
    }
    return tmp;
}

static GtkWidget *gui_xml_box_new(xmlNode *cur, char dir)
{
    char *arg0;
    int arg1, arg2;

    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"expand");
    arg1 = FALSE; arg2 = 0;
    if(!strcmp(arg0, "true")) arg1 = TRUE;

    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"spacing");
    if(arg0) arg2 = atoi(arg0);

    if(dir) return gtk_hbox_new(arg1, arg2);
    return gtk_vbox_new(arg1, arg2);
}

static GtkWidget *gui_xml_dipsw(gui *g, xmlNode *cur)
{
    GtkWidget *wg0, *wg1, *wg2;
    char tmp[8], *arg0, *desc, rev = 0;
    int i=0, mask, len;
    long set = 0;
//    int size_x, size_y;
    
//    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"size");
//    if(arg0){
//	sscanf(arg0, "%i, %i", &size_x, &size_y);
	
//    }

    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"len");
    if(arg0) len = atoi(arg0);
    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"value");
    if(arg0) set = strtol(arg0, NULL, 0);
    desc = (char *)xmlGetProp(cur, (unsigned char *)"name");
    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"reversed");
    if(arg0) rev = !strcmp(arg0, "true");
    
    wg0 = gtk_table_new(i,2, FALSE);
    memset(tmp, 0, 8);
    if(len <= 0) return NULL;
    
    if(rev)
	for(mask = 1 << (len-1), i = 0; i < len; i++,mask >>= 1){
	    sprintf(tmp, "%X", len - i);
	    wg1 = gtk_label_new(tmp);
	    wg2 = gtk_image_new_from_stock(set & mask ? GUI_DIPSW_ON : GUI_DIPSW_OFF, g->icon_size);
	    gtk_table_attach(GTK_TABLE(wg0), wg1, i,i+1, 0,1, 0,0, 0,0);
	    gtk_table_attach(GTK_TABLE(wg0), wg2, i,i+1, 1,2, 0,0, 0,0);
	}
    else
	for(mask = 1, i = 0; i < len; i++,mask <<= 1){
	    sprintf(tmp, "%X", i + 1);
	    wg1 = gtk_label_new(tmp);
	    wg2 = gtk_image_new_from_stock(set & mask ? GUI_DIPSW_ON : GUI_DIPSW_OFF, g->icon_size);
	    gtk_table_attach(GTK_TABLE(wg0), wg1, i,i+1, 0,1, 0,0, 0,0);
	    gtk_table_attach(GTK_TABLE(wg0), wg2, i,i+1, 1,2, 0,0, 0,0);
	}

    if(desc){
	wg1 = wg0;
	wg0 = gtk_frame_new(desc);
	gtk_frame_set_label_align(GTK_FRAME(wg0), 0.5, 0.5);
	gtk_container_add(GTK_CONTAINER(wg0), wg1);
    }

    wg2 = gtk_hbox_new(FALSE,0);
    gtk_container_set_border_width(GTK_CONTAINER(wg2), 5);
    gtk_container_set_border_width(GTK_CONTAINER(wg0), 3);
    gtk_box_pack_start(GTK_BOX(wg2), wg0, TRUE, FALSE, 0);
    return wg2;
}

static GtkWidget *gui_xml_jumper(gui *g, xmlNode *cur)
{
    GtkWidget *wg0, *wg1;
    char state, *idx_up, *idx_dn;

    state = 0;
    idx_up = (char *)xmlGetProp(cur, (unsigned char *)"set");
    if(idx_up) state = !strcmp(idx_up, "1");
    idx_up = (char *)xmlGetProp(cur, (unsigned char *)"name_up");
    idx_dn = (char *)xmlGetProp(cur, (unsigned char *)"name_dn");

    wg0 = gtk_table_new(1,3, FALSE);
    if(idx_up){
	wg1 = gtk_label_new(idx_up);
	gtk_table_attach(GTK_TABLE(wg0), wg1, 0,1, 0,1, 0,0, 0,0);
    }
    wg1 = gtk_image_new_from_stock(state ? GUI_DIPSW_ON : GUI_DIPSW_OFF, g->icon_size);
    gtk_table_attach(GTK_TABLE(wg0), wg1, 0,1, 1,2, 0,0, 0,0);
    if(idx_dn){
	wg1 = gtk_label_new(idx_dn);
	gtk_table_attach(GTK_TABLE(wg0), wg1, 0,1, 2,3, 0,0, 0,0);
    }
    return wg0;
}

static GtkWidget *gui_xml_table(gui *g, xmlNode *cur)
{
    int x=0, y=0;
    char *tmp=NULL, eq=0;
    
    tmp = (char *)xmlGetProp(cur, (unsigned char *)"dim");
    if(!tmp) return NULL;
    sscanf(tmp, "%i, %i", &x, &y);

    tmp = (char *)xmlGetProp(cur, (unsigned char *)"equal");
    if(tmp) eq = !strcmp(tmp, "true");

    if(!x || !y) return NULL;
    return gtk_table_new(x, y, eq);
}

void gui_xml_signal_register(gui *g, GtkWidget *wg, char *id, char *signal, char *wg_name)
{

}

static GtkWidget *gui_xml_chbutton(gui *g, xmlNode *cur)
{
    GtkWidget *tmp;
    GtkWidget *tmp_2, * tmp_3, *tmp_4, *tmp_1;
    GtkStyle *style;    
    long fs;
    char rl = 0, *arg, *id;

    if(!(id = (char *)xmlGetProp(cur, (unsigned char *)"id"))){
	return NULL;
    };
    arg = (char *)xmlGetProp(cur, (unsigned char *)"place");
    if(arg) rl = !strcmp(arg, "right");
    
    tmp = gtk_alignment_new( rl ? 1:0, 0.5,0,0);
    tmp_2 = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(tmp), tmp_2);
    tmp_3 = gtk_check_button_new();
    gui_xml_signal_register(g, tmp_3, id, "toggled", "check_button");
    tmp_4 = gtk_label_new((char *)xmlGetProp(cur, (unsigned char *)"label"));
    style = gtk_widget_get_style(tmp_4);
    fs = pango_font_description_get_size(style->font_desc);
    pango_font_description_set_size(style->font_desc, fs / 1.3);
    gtk_widget_modify_font(tmp_4,style->font_desc);
    pango_font_description_set_size(style->font_desc, fs );

    arg = (char *)xmlGetProp(cur, (unsigned char *)"sensitive");
    if(arg){ 
	if(!strcmp(arg, "false"))
	    gtk_widget_set_sensitive(GTK_WIDGET(tmp_3), 0);
    }
    if(rl){
	tmp_1 = tmp_4;
	tmp_4 = tmp_3;
	tmp_3 = tmp_1;
    }
    gtk_box_pack_start(GTK_BOX(tmp_2), tmp_3, FALSE,FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(tmp_2), tmp_4, FALSE,FALSE, 0); 

    return tmp; 
}

static GtkWidget *gui_xml_button(gui *g, xmlNode *cur)
{
    GtkWidget *tmp;
    char *arg, *id;
        
    if(!(id = (char *)xmlGetProp(cur, (unsigned char *)"id"))){
	return NULL;
    };
    tmp = gtk_button_new_with_label((char *)xmlGetProp(cur, (unsigned char *)"label"));
    arg = (char *)xmlGetProp(cur, (unsigned char *)"sensitive");
    if(arg){ 
	if(!strcmp(arg, "false"))
	    gtk_widget_set_sensitive(GTK_WIDGET(tmp), 0);
    }
    gui_xml_signal_register(g, tmp, id, "clicked", "button");
    return tmp;
}

static GtkWidget *gui_xml_spinbutton(gui *g, xmlNode *cur)
{
    GtkWidget *tmp1;
    GtkAdjustment *adj;
    char *arg, *id;
    int max=0;    
    
    if(!(id = (char *)xmlGetProp(cur, (unsigned char *)"id"))){
	return NULL;
    };

    arg = (char *)xmlGetProp(cur, (unsigned char *)"max");
    if(arg) max = strtol(arg, NULL, 0);
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0.0, max, 1, 16, 16));    
    tmp1 = gtk_spin_button_new(adj, 1, 0);
    arg = (char *)xmlGetProp(cur, (unsigned char *)"size");
    if(arg){ 
	int xx=0, yy=0;
	sscanf(arg, "%i, %i", &xx, &yy);
	gtk_widget_set_usize(GTK_WIDGET(tmp1), xx, yy);
    }

    arg = (char *)xmlGetProp(cur, (unsigned char *)"sensitive");
    if(arg){ 
	if(!strcmp(arg, "false"))
	    gtk_widget_set_sensitive(GTK_WIDGET(tmp1), 0);
    }
    gui_xml_signal_register(g, tmp1, id, "value_changed", "spin_button");
    return tmp1;
}

static GtkWidget *gui_xml_entry(gui *g, xmlNode *cur)
{
    GtkWidget *tmp;
    char *arg, *id;
    
    if(!(id = (char *)xmlGetProp(cur, (unsigned char *)"id"))){
	return NULL;
    };

    tmp = gtk_entry_new();
    arg = (char *)xmlGetProp(cur, (unsigned char *)"sensitive");
    if(arg){ 
	if(!strcmp(arg, "false"))
	    gtk_widget_set_sensitive(GTK_WIDGET(tmp), 0);
    }
    
    arg = (char *)xmlGetProp(cur, (unsigned char *)"editable");
    if(arg){ 
	if(!strcmp(arg, "false"))
	    gtk_entry_set_editable(GTK_ENTRY(tmp), 0);
    }
    
    arg = (char *)xmlGetProp(cur, (unsigned char *)"size");
    if(arg){ 
	int xx;
	sscanf(arg, "%i", &xx);
	gtk_entry_set_width_chars(GTK_ENTRY(tmp), xx);
    }

    gui_xml_signal_register(g, tmp, id, "changed", "entry");    
    return tmp;
}

static GtkWidget *gui_xml_image(gui *g, xmlNode *cur)
{
    return gtk_image_new_from_file((char *)xmlGetProp(cur, (unsigned char *)"src"));
}

static GtkWidget *gui_xml_label(gui *g, xmlNode *cur)
{
    GtkWidget *tmp;
    char *arg;
    
    tmp = gtk_label_new((char *)xmlGetProp(cur, (unsigned char *)"text"));
    arg = (char *)xmlGetProp(cur, (unsigned char *)"align");
    if(arg){ 
	int xx, yy;
	sscanf(arg, "%i, %i", &xx, &yy);
	gtk_misc_set_alignment(GTK_MISC(tmp), (float)xx / 100, (float)yy / 100);
    }
    return tmp;
}

static void gui_xml_parse_element(gui *g, GtkWidget *wg, xmlDocPtr doc, xmlNode *cur, const char *parm)
{
    char x = 0;
    char *arg0;

    if(parm == NULL) 
	x = 1;
    else{
	if(*parm == 0) x = 1;
    }
    for(; cur != NULL; cur = cur->next){
	/* tagi kluczowe */
	if(!strcmp((char*)cur->name,"if")){
	    arg0 = (char *)xmlGetProp(cur, (unsigned char *)"chip");
	    if(!strcmp(arg0, "none") && x) 
	      gui_xml_parse_element(g, wg, doc, cur->xmlChildrenNode, parm );
	    else
	      if(strstr(arg0, parm) && x) gui_xml_parse_element(g, wg, doc, cur->xmlChildrenNode, NULL);
	} 
	else if(!strcmp((char*)cur->name,"description")){
	    arg0 = (char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    gtk_label_set_text(GTK_LABEL(g->chip_desc), arg0);
	} 
	/**** elementy GUI ****/
	/* kontenery */
	else if(!strcmp((char*)cur->name,"frame"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_frame_new(cur), 1, parm);
	else if(!strcmp((char*)cur->name,"vbox"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_box_new(cur, 0), 1, parm);
	else if(!strcmp((char*)cur->name,"hbox"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_box_new(cur, 1), 1, parm);
	else if(!strcmp((char*)cur->name, "table"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_table(g, cur), 1, parm);
	/* kontrolki */
	else if(!strcmp((char*)cur->name,"jumper"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_jumper(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name, "dipswitch"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_dipsw(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name,"image"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_image(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name,"label"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_label(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name, "button"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_button(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name, "chbutton"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_chbutton(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name, "spinbutton"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_spinbutton(g, cur), 0, NULL);
	else if(!strcmp((char*)cur->name, "entry"))
	    gui_xml_container_add(g, cur, doc, wg, gui_xml_entry(g, cur), 0, NULL);
/* w przyszlosci dodac pozostale elementy w razie potrzeby */	

    }    
}

/* parsowanie glównego poziomu */
static void gui_xml_parser(gui *g, xmlDocPtr doc, const char *parm)
{
    xmlNode *cur;
    GtkWidget *tmp, *lab;

    if(!(cur = xmlDocGetRootElement(doc))) return;
    
    for(cur = cur->xmlChildrenNode; cur != NULL; cur=cur->next){
	if(!strcmp((char*)cur->name,"info")){
	    g_return_if_fail(g->main_table != NULL);
	    tmp = gtk_vbox_new(FALSE, 3);
	    gui_xml_parse_element(g, GTK_WIDGET(tmp), doc, cur->xmlChildrenNode, parm);
	    gtk_table_attach_defaults(GTK_TABLE(g->main_table), tmp, 1,2, 0, 2);
	    gtk_widget_show_all(GTK_WIDGET(g->main_table));
	}
	if(!strcmp((char*)cur->name,"notebook")){
	    lab = gtk_label_new((char *)xmlGetProp(cur, (unsigned char *)"name"));
	    tmp = gtk_vbox_new(FALSE, 0);
	     gui_xml_parse_element(g, tmp, doc, cur->xmlChildrenNode, parm);
	    gtk_notebook_append_page(GTK_NOTEBOOK(g->notebook), tmp, lab);
	    gtk_widget_show_all(GTK_WIDGET(g->notebook));
	}
    }    
}

int gui_xml_create(gui *g, char *xml, const char *section, const char *chip_name)
{
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;
    
    LIBXML_TEST_VERSION
    
    if(!(ctxt = xmlNewParserCtxt())){
	printf("Error {gui_xml.c} --> gui_xml_create(): Failed to allocate xml parser context.\n");
	return -1;
    };
    
    if(!strncmp(xml, "file://", 7))
	doc = xmlCtxtReadFile(ctxt, xml + 7, NULL, XML_PARSE_DTDVALID);
    else
	doc = xmlCtxtReadMemory(ctxt, xml, strlen(xml), "noname.xml", NULL, XML_PARSE_DTDVALID);

    if(doc == NULL){
	printf("Error {gui_xml.c} --> gui_xml_create(): Failed to parse xml string\n");
    } else {
	if(ctxt->valid)
	    gui_xml_parser(g, doc, chip_name);
	else 
	    printf("Error {gui_xml.c} --> gui_xml_create(): Failed to validate xml.\n");
	xmlFreeDoc(doc);
    }
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
// ?
//    xmlMemoryDump();
    return 0;
}
/*
void gui_drv_field_init(geepro *gep, const char *title)
{
    GtkWidget *wg0, *wg1;

    wg0 = gtk_frame_new(title);
    gtk_container_border_width(GTK_CONTAINER(wg0), 3);
    gtk_table_attach_defaults(GTK_TABLE(GUI(gep->gui)->main_table), wg0,  1, 2, 0, 2);
    wg1 = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(wg0), wg1);
    GUI(gep->gui)->drv_vbox = wg1;
}

void gui_drv_field_destroy(geepro *gep)
{
    if(GUI(gep->gui)->drv_vbox) gtk_widget_destroy(GTK_WIDGET(GUI(gep->gui)->drv_vbox)->parent);
    GUI(gep->gui)->drv_vbox = NULL;
}
*/




