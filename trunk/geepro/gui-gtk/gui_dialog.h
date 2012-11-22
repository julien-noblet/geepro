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

#ifndef __gui_dialog_h__
#define __gui_dialog_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dlg_slider_cb)(int value, void *u_ptr, int i_par);

extern void dialog_start(const char *title, int width, int height);
extern void dialog_end();
extern void dialog_cleanup(); // 
extern void frame_start(const char *name);
extern void frame_end();
extern void slider_add(const char *label, int min, int max, int def, dlg_slider_cb, int i_par, void *u_ptr);


#ifdef __cplusplus
} //extern "C"
#endif

#endif

