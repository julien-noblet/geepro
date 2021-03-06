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

#include "programmer.h"

char pgm_select_driver( geepro *gep, const char *drv_name)
{
    char err;

    hw_destroy( gep );
    err = iface_pgm_select(gep->ifc, drv_name );
    hw_gui_init( gep );

    return err;
}

char pgm_select_chip( geepro *gep, const char *chip_name)
{
    char err;

    if((err = iface_chip_select(gep->ifc->chp, chip_name))) return err;
    return hw_set_chip( gep );
}

/****************************************/

char pgm_buffer_write(geepro *gep, unsigned int addr, unsigned char data)
{
    return 0;
}


int pgm_buffer_read(geepro *gep, unsigned int addr)
{
    return 0;
}

void pgm_buffer_checksum(geepro *gep )
{
    
}

void pgm_autostart(geepro *gep)
{
//    if(chp->autostart) chp->autostart( gep );
}


