/* $Revision: 1.5 $ */
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

G_BEGIN_DECLS

#define GUI_TYPE_BINEDITOR		( gui_bineditor_get_type() )
#define GUI_BINEDITOR(obj)		( G_TYPE_CHECK_INSTANCE_CAST((obj), GUI_TYPE_BINEDITOR, GuiBineditor) )
#define GUI_BINEDITOR_CLASS(klass)	( G_TYPE_CHECK_CLASS_CAST((klass), GUI_TYPE_BINEDITOR, GuiBineditorClass) )
#define GUI_IS_BINEDITOR(obj)		( G_TYPE_CHECK_INSTANCE_TYPE((obj), GUI_TYPE_BINEDITOR) )
#define GUI_IS_BINEDITOR_CLASS(klass)	( G_TYPE_CHECK_CLASS_TYPE((klass), GUI_BINEDITOR) )
#define GUI_BINEDITOR_GET_CLASS(obj)	( G_TYPE_INSTANCE_GET_CLASS((obj), GUI_BINEDITOR, GuiBineditorClass) )

typedef struct _GuiBineditor 		GuiBineditor;
typedef struct _GuiBineditorPrivate	GuiBineditorPrivate;
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
    GUI_BINEDITOR_COLOR_TEXT_MARKED,	/* text on marker position */
    GUI_BINEDITOR_COLOR_MARKED_BG,	/* text background on marker position */
    GUI_BINEDITOR_COLOR_TEXT_DESC,	/* grid description */
    GUI_BINEDITOR_COLOR_GRID,		/* grid */
    GUI_BINEDITOR_COLOR_GRID_BG,	/* grid background */
    GUI_BINEDITOR_COLOR_DESC_BG,	/* description background */
    GUI_BINEDITOR_COLOR_UD,		/* color of the solid block drawed on character over range */
    GUI_BINEDITOR_COLOR_HL,		/* highlight color */
    GUI_BINEDITOR_COLOR_HL_MRK,		/* highlight color on marker */
    GUI_BINEDITOR_COLOR_LAST
} GuiBineditorColors;

struct _GuiBineditorPrivate
{
    /* key values */
    int key_left;
    int key_right;
    int key_up;
    int key_down;
    int key_home;
    int key_end;
    int key_pgup;
    int key_pgdn;
    int key_tab;
    /* */
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
    int ascii_start;
    int ascii_end;
    int ascii_space;
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
    GtkWidget *vbox;
    char clear_sens;
    char print_sens;
    char find_sens;
    char mjmp_sens;
    char rjmp_sens;
    char chksum_sens;
    int statusbar_id;
    /* hex editor */
    char edit_hex;		  // flag: bit0 -> hex grid, bit 1 -> ascii grid
    unsigned int edit_hex_start;  // initial address
    int edit_hex_cursor; // cursor position in hex grid
    GtkWidget *info_addr; 
    GtkWidget *info_mark;
// dostep publ dorobic
    GtkAdjustment  *adj;
    int buffer_size;
    unsigned char *buffer;
    void *user_ptr1;
    void *user_ptr2;    
};

struct _GuiBineditor
{
    /*<private>*/
    GtkBox parent;
    GuiBineditorPrivate *priv;
};

struct _GuiBineditorClass
{
    GtkBoxClass parent_class;

    /* Padding for future expansion */
    void (*bineditor) (GuiBineditor *be);
};

GType hui_bineditor_get_type	(void) G_GNUC_CONST;
GtkWidget *gui_bineditor_new(GtkWindow *parent);
void gui_bineditor_set_buffer(GuiBineditor *be, int bfsize, unsigned char *buffer);
void gui_bineditor_set_properties(GuiBineditor *be, GuiBineditorProperties prop);
void gui_bineditor_set_colors(GuiBineditor *be, GuiBineditorColors color, float r, float g, float b);
void gui_bineditor_redraw(GuiBineditor *be);
void gui_bineditor_connect_statusbar(GuiBineditor *be, GtkWidget *sb);
GType gui_bineditor_get_type(void);

G_END_DECLS

#endif 

