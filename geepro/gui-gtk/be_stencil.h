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

#ifndef __BE_STENCIL_H__
#define __BE_STENCIL_H__

#include "bineditor.h"

extern char gui_bineditor_stencil_generate_index_file(GuiBineditor *be, const char *fname);
extern void gui_bineditor_stencil_sheet(GuiBineditor *be, const char *device, const char *stc_fname);
extern char gui_bineditor_stencil_operation(GuiBineditor *be, int id, const char *device, char *path, int operation, char has_child);
#endif // __BE_STENCIL_H__
