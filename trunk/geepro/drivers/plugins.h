#ifndef __PLUGINS_H__
#define __PLUGINS_H__
/* $Revision: 1.3 $ */
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
#include <unistd.h>
#include "../src/parport.h"
#include "willem.h"
#include "../src/timer.h"
#include "hwplugin.h"
#include "../src/iface.h"

#include "../src/gepro.h"
#include "../src/geepro.h"

#include "../gui/gui_xml.h"

#define DRIVER_NAME(ptr)	(*(char **)ptr)

#define plugin_register_begin 	int plugin_init(void *___ptr___){
#define plugin_register_end 	return 0; }
#define register_api(api)	iface_prg_add(___ptr___, api, 1)


#endif

