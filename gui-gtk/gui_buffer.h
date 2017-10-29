/* 
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

#ifndef __GUI_BUFFER_H__
#define __GUI_BUFFER_H__

#include "bineditor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_gui_buffer_list_ s_gui_buffer_list;
typedef struct s_gui_buffer_ s_gui_buffer;

struct s_gui_buffer_list_
{
    GtkWindow		*wmain;
    GtkWidget		*device_entry;
    GtkWidget		*buffer_entry;
    GtkWidget		*crc_entry;
    GtkWidget		*file_entry;
    GuiBineditor	*be;
    s_gui_buffer	*parent;
    s_buffer_list	*buffer;
    s_gui_buffer_list	*next;
};

struct s_gui_buffer_
{
    GtkNotebook		*np;
    store_str		*store;
    s_gui_buffer_list	*list;    
};

void gui_refresh_button(GtkWidget *, geepro *); // function defined in gui.c

/*
    Create GUI buffer structure, and initialize it.
    Input:
	bf - return initialized s_gui_buffer
	st - storings variables
    Return:
	0 - success
*/
char gui_buffer_init(s_gui_buffer **bf, store_str *st, GtkNotebook *np);

/*
    Destroys all gui element for buffers. (not free buffer structure)
    Free all allocated memory for s_gui_buffer.
    Input:
	bf - s_gui_buffer structure
*/
void gui_buffer_exit(s_gui_buffer *bf);

/*
    Creates new buffer.
    Input:
	bf - return initialized s_gui_buffer	
	buffer - pointer to elelemnt of buffer list
    Return:
	GtkWidget of bineditor on success. Else NULL.
*/
GtkWidget *gui_buffer_new(s_gui_buffer *bf, s_buffer_list *buffer, GtkWindow *wmain);

/*
    Check if buffer content is valid with file.
    (file may change during program run )    
    If file changed and is out of sync with buffer content, asks for reload.
*/
void gui_buffer_check_valid(s_gui_buffer *bf);

/*
    Delete all Gui buffers entries from s_gui_buffer.
*/
void gui_buffer_delete( s_gui_buffer *bf);

/*
    Refresh display.
*/
void gui_buffer_refresh(s_gui_buffer *bf);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

