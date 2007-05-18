/* $Revision: 1.1.1.1 $ */
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


#ifndef __buffer_h__
#define __buffer_h__
#include "chip.h"
#include "geepro.h"

extern void buffer_alloc(geepro *gep, unsigned int size);
extern void buffer_clear(geepro *gep);
extern long buffer_checksum(geepro *gep);
extern char buffer_write(geepro *gep, unsigned int addr, unsigned char byte);
extern int  buffer_read(geepro *gep, unsigned int addr);
extern char *buffer_get_buffer_ptr(geepro *gep);
//extern void checksum(void);
#endif

