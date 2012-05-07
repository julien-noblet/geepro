/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2011 Krzysztof Komarnicki
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

#ifndef __BE_TEXTEDIT_H__
#define __BE_TEXTEDIT_H__
#include "bineditor.h"

typedef struct
{
    GuiBineditor *be;
    unsigned int base_addr;
    unsigned int ed_len;
    GtkWidget *wmain, *ctx,*draw_area, *tb_line, *ed_wg;
    GtkTextBuffer *ed_buffer;
    GtkAdjustment *vadj, *hadj;
    cairo_t *cr;

} gui_bineditor_text_str;

extern void gui_bineditor_text_editor(GuiBineditor *be, unsigned int start, unsigned int len);
extern char gui_bineditor_text_rfsh(GuiBineditor *be);

#endif // __BE_TEXTEDIT_H__
