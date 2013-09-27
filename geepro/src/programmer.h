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

#ifndef __PROGRAMMER_H
#define __PROGRAMMER_H

#include "geepro.h"
#include "iface.h"

#ifdef __cplusplus
extern "C" {
#endif

char pgm_select_driver(geepro *gep, const char *driver_name);
char pgm_select_chip( geepro *gep, const char *chip_name);
void pgm_autostart(geepro *gep);

/*
    Buffer operations
*/
char pgm_buffer_write(geepro *gep, unsigned int addr, unsigned char data);
int  pgm_buffer_read(geepro *gep, unsigned int addr);
void pgm_buffer_checksum(geepro *gep );


#ifdef __cplusplus
}
#endif

#endif /* __PROGRAMMER_H */

