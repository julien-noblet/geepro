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
#include "serial.h"
#include "error.h"
#include "geepro.h" // for SYSTEM_DEVICE_PATH
#include "files.h"

static inline void serial_add_list(s_serial *ser, const char *fname)
{
    s_serial_list *tmp, *it;
    if( !fname ) return;    
    MALLOC(tmp, s_serial_list, 1){
	ERR_MALLOC_MSG;
	return;
    }        
    memset(tmp, 0, sizeof(s_serial_list) );
    MALLOC(tmp->device_path, char, strlen(fname) + strlen(SYSTEM_DEVICE_PATH) + 2){
	ERR_MALLOC_MSG;
	free( tmp );
	return;
    }        
    sprintf(tmp->device_path, "%s%s", SYSTEM_DEVICE_PATH, fname);
    tmp->main = ser;
    tmp->next = NULL;
    
    if(!ser->list){
	ser->list = tmp;
        return;
    }
    for(it = ser->list; it->next; it = it->next);
    it->next = tmp;
}

static char serial_loopback_add(const char *fname, const char *error, void *ser)
{
    serial_add_list((s_serial*)ser, fname);    
    return true;
}

static void serial_lookup_devices( s_serial *ser)
{
    char error[256];
    if(!file_ls(SYSTEM_DEVICE_PATH, "^ttyS[[:digit:]]*$", error, serial_loopback_add, ser)){
	ERR("%s\n", error);    
    }
}

char serial_init(s_serial **ser)
{ 
    if( *ser ) return 0;
    MALLOC(*ser, s_serial, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }        
    memset(*ser, 0, sizeof(s_serial) );
    serial_lookup_devices( *ser );
    return 0; 
}

void serial_exit(s_serial *ser)
{
    s_serial_list *it, *tmp;
    if(!ser) return;
    serial_close( ser );
    for(it = ser->list; it;){
	tmp = it->next;
	if(it->device_path) free(it->device_path);
	if(it->alias) free(it->alias);
	it = tmp;
    }
}

void serial_get_list(s_serial *ser, f_serial_callback fb, void *ptr)
{
    s_serial_list *it;
    if(!ser) return;
    serial_close( ser );
    for(it = ser->list; it; it = it->next) fb( ser, it, ptr);
}

char serial_set_device(s_serial *, const char *name){ return 0;}
char *serial_get_device(s_serial *){ return NULL;}


char serial_open(s_serial *){ return 0;}
char serial_close(s_serial *){ return 0;}
/*****************/

char serial_set_bitrate(s_serial *, int bitrate){ return 0;}
char serial_set_params(s_serial *, char flags){ return 0;}
char serial_send_byte(s_serial *, char byte){ return 0;}
char serial_send_string(s_serial *, char *bytes, int count){ return 0;}
char serial_set_ctrl_bits(s_serial *, char byte){ return 0;}
char serial_set_ctrl_bit(s_serial *, char state, char mask){ return 0;}
char serial_get_byte(s_serial *, char *byte){ return 0;}
char serial_get_string(s_serial *, char **byte, int count){ return 0;}
char serial_get_ctrl_bits(s_serial *){ return 0;}
char serial_get_ctrl_bit(s_serial *, char mask){ return 0;}
char serial_send_packet(s_serial *, char *bytes, int count, e_serial_proto mode){ return 0;}
char serial_get_packet(s_serial *, char *bytes, int count, e_serial_proto mode){ return 0;}

