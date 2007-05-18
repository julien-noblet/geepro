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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gepro.h"
#include "buffer.h"
#include "../intl/lang.h"
#include "main.h"
#include "../gui/gui.h"

/*
void buffer_alloc(geepro *gep, unsigned int size)
{
    if(gep->chp->buffer) free(gep->chp->buffer);
    if(!(SET_BUFFER((char *)malloc(sizeof(char)*size)))){
	printf(ALLOCATION_ERROR);
	gui_kill_me(gep);
    }
    memset( GET_BUFFER, 0, GET_DEV_SIZE );
    SET_BF_SH(0);
    SET_CHECKSUM(0);
//    gui_set_viewer_size(GET_DEV_SIZE);
}
*/

void buffer_clear(geepro *gep)
{
    if(!gep->chp) return;
    memset( gep->chp->buffer, 0, gep->chp->dev_size );
//    SET_STATUS(STS_EMPTY);
//    gui_stat_rfsh(plg);
}

long buffer_checksum(geepro *gep){
    int i;
    long tmp = 0;
    
    for( i = 0; i < gep->chp->dev_size; i++ ) tmp += gep->chp->buffer[i];
//    SET_CHECKSUM(tmp);
    return tmp;
}


char buffer_write(geepro *gep, unsigned int addr, unsigned char byte)
{
    if(addr >= gep->chp->dev_size) return -1;
    gep->chp->buffer[addr] = byte;
    return 0;
}

int buffer_read(geepro *gep, unsigned int addr)
{
    if(addr >= gep->chp->dev_size) return -1;
    return gep->chp->buffer[addr];
}

char *buffer_get_buffer_ptr(geepro *gep)
{
	return gep->chp->buffer;
}

