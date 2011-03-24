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

/*
    Kontrolka emituje sygnal "changed" jesli zawartosc bufora ulegnie zmianie
*/

#ifndef __GUI_BINEDITOR_H__
#define __GUI_BINEDITOR_H__
#include <gtk/gtk.h>
#include <gtk/gtkbox.h>

G_BEGIN_DECLS

#define GUI_BINEDITOR(obj)		(GTK_CHECK_CAST((obj), GUI_TYPE_BINEDITOR, GuiBineditor))
#define GUI_BINEDITOR_CLASS(klass)	(GTK_CHECK_CLASS_CAST((klass), GUI_TYPE_BINEDITOR, GuiBineditorClass))
#define GUI_IS_BINEDITOR(obj)		(GTK_CHECK_TYPE((obj), GUI_TYPE_BINEDITOR))
#define GUI_TYPE_BINEDITOR		(gui_bineditor_get_type())

typedef struct _GuiBineditor 		GuiBineditor;
typedef struct _GuiBineditorClass 	GuiBineditorClass;

typedef enum
{
    GUI_BINEDITOR_PROP_TOOLBAR = 1 << 0,
    GUI_BINEDITOR_PROP_CLEAR   = 1 << 1,
    GUI_BINEDITOR_PROP_FIND    = 1 << 2,
    GUI_BINEDITOR_PROP_MJMP    = 1 << 3,
    GUI_BINEDITOR_PROP_RJMP    = 1 << 4, 
    GUI_BINEDITOR_PROP_CHKSUM  = 1 << 5,
    GUI_BINEDITOR_PROP_CALC    = 1 << 6,
    GUI_BINEDITOR_PROP_PRINT   = 1 << 7,
    GUI_BINEDITOR_PROP_EDITABLE = 1 << 8,
} GuiBineditorProperties;

typedef enum
{
    GUI_BINEDITOR_COLOR_TEXT_NORMAL,	/* text */
    GUI_BINEDITOR_COLOR_TEXT_MARKED,	/* text na pozycji markera */
    GUI_BINEDITOR_COLOR_MARKED_BG,	/* tlo tekstu na pozycji markera */
    GUI_BINEDITOR_COLOR_TEXT_DESC,	/* text opisu siatki */
    GUI_BINEDITOR_COLOR_GRID,		/* kolor siatki */
    GUI_BINEDITOR_COLOR_GRID_BG,	/* tlo pod siatka */
    GUI_BINEDITOR_COLOR_DESC_BG,	/* kolor belki opisowej */
    GUI_BINEDITOR_COLOR_UD,		/* kolor prostokatu rysowanego w miejscu znaku spoza zakresu */
    GUI_BINEDITOR_COLOR_HL,		/* kolor podswietlenia */
    GUI_BINEDITOR_COLOR_HL_MRK,		/* kolor podswietlenia na markerze*/
    GUI_BINEDITOR_COLOR_LAST
} GuiBineditorColors;

struct _GuiBineditor
{
    /* <private> */
    GtkBox container;
    float colors[GUI_BINEDITOR_COLOR_LAST * 3];
    GtkWidget *wmain;
    GtkWidget *drawing_area;
    GtkWidget *tb;
    char *vfind;
    char *vreplace;
    unsigned int sum;
    char rfsh;     /* jesli 0 to przerysowanie calosci */
    int cell_width;
    int cell_height;
    int grid_start;
    int grid_end;
    int grid_cols;
    int grid_rows;
    int grid_top;
    int address_mark;
    int address_mark_redo;
    int address_old_hint;    
    int address_hl_start;
    int address_hl_end;
    int properties;
    GtkWidget *clear;
    GtkWidget *print;
    GtkWidget *find;
    GtkWidget *mjmp;
    GtkWidget *rjmp;
    GtkWidget *chksum;
    GtkWidget *calc;
    GtkWidget *statusbar;
    int statusbar_id;

    /* <public> */
    GtkAdjustment  *adj;
    int buffer_size;
    unsigned char *buffer;
    void *user_ptr1;
    void *user_ptr2;    
};

struct _GuiBineditorClass
{
    GtkBoxClass parent_class;

    /* Padding for future expansion */
    void (*bineditor) (GuiBineditor *be);
};

GtkWidget *gui_bineditor_new(GtkWindow *parent);
void gui_bineditor_set_buffer(GuiBineditor *be, int bfsize, unsigned char *buffer);
void gui_bineditor_set_properties(GuiBineditor *be, GuiBineditorProperties prop);
void gui_bineditor_set_colors(GuiBineditor *be, GuiBineditorColors color, float r, float g, float b);
void gui_bineditor_redraw(GuiBineditor *be);
void gui_bineditor_connect_statusbar(GuiBineditor *be, GtkWidget *sb);
GtkType gui_bineditor_get_type(void);

G_END_DECLS

#endif 

