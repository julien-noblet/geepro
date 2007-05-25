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

#ifndef __gui_xml_h__
#define __gui_xml_h__
#include "gui.h"

#define GUI_XML(x)	((gui_xml*)x)

typedef struct _gui_xml_ev gui_xml_ev;
typedef void (*gui_xml_event)(gui_xml_ev *ev, int value, const char *sval);

struct _gui_xml_ev
{
    void *root_parent;
    char *id;
    int  type;
    void *widget;
    gui_xml_ev *next;
};

typedef struct
{
    void *parent;
    char suppress;  /* flaga stlumienia echa sygnalu */
    void *notebook;
    void *info;
    void *description;
    int  sw_size;
    gui_xml_event ev;
    gui_xml_ev *event;
} gui_xml;

typedef enum
{
    GUI_XML_INFO_ROOT = 1,
    GUI_XML_NOTEBOOK_ROOT,
    GUI_XML_BUTTON,
    GUI_XML_CHECK_BUTTON,
    GUI_XML_SPIN_BUTTON,
    GUI_XML_ENTRY
}gui_xml_ev_wg;

extern void *gui_xml_new(gui *g);
extern void gui_xml_register_event_func(gui_xml *g, gui_xml_event ev);
extern int gui_xml_build(gui_xml *g, char *xml, const char *section, const char *chip_name);
extern void *gui_xml_set_widget(gui_xml *g, gui_xml_ev_wg wg, const char *id, int val, char *sval);
extern void gui_xml_destroy(gui_xml *g);

#endif
