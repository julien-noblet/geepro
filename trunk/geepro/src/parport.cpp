/* $Revision: 1.4 $ */
/* parport - user space wrapper for LPT port using ppdev v 0.0.2
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
#include <sys/io.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "usb2lpt.h"
#include "parport.h"
#include "error.h"
#include "geepro.h" // for SYSTEM_DEVICE_PATH
#include "files.h"

#define PARPORT_VERSION	"lib parport version 0.2 with Usb2Lpt support\n"
/*
static int parport_usb_init(s_usb2lpt **usb, void *);
static int parport_usb_write_data(s_usb2lpt *usb, char data);
static int parport_usb_write_ctrl(s_usb2lpt *usb, char ctrl);
static char parport_usb_read_stat(s_usb2lpt *usb);
static int parport_usb_close(s_usb2lpt *usb);
*/

static void parport_lookup_devices( s_parport *pp);
static char parport_add_device(s_parport *pp, const char *dev_path, const char *alias, int flags);
static void parport_del_devices(s_parport *pp);

/*
 *
 *     Implementation
 *
 */

char parport_init( s_parport **pp , void *user_ptr )
{
    if( !pp ) return -2;
    if( *pp ){
	ERR("s_parport* have to be NULL !");
	return -2; // *pp have to be NULL
    }
    
    MALLOC( *pp, s_parport, 1 ){
	ERR_MALLOC_MSG;	
	return ERR_MALLOC_CODE;
    }    
    memset( *pp, 0, sizeof( s_parport ) );
    parport_lookup_devices( *pp );
    parport_add_device( *pp, PARPORT_EMULATOR_PATH, PARPORT_EMULATOR_ALIAS, 0 );
    (*pp)->user_ptr = user_ptr;
    return 0;
}

void parport_exit( s_parport *pp )
{
    if( !pp ) return;
    parport_del_devices( pp );
    parport_close( pp );
    free( pp );
}

int parport_open( s_parport *pp )
{
    if( !pp ) return 0;
    if( !pp->selected ) return 0;
    if( !pp->selected->device_path ){
	ERR("Device path is NULL !");
	return 0;
    }

//    if( pp->selected->alias ) // switching to emulation mode
//	pp->em.emulate = !strcmp( pp->selected->alias, PARPORT_EMULATOR_ALIAS );

    pp->pd.allow = 1;

//    if( pp->em.emulate ) 
//	return parport_usb_init( &usb, pp->emul );

    parport_close(pp); // close already opened device

    MSG("Opening device %s", pp->selected->device_path);
    if((pp->pd.handler = open(pp->selected->device_path, O_RDWR | pp->selected->flags)) == -1 ){
	ERR("open(\"%s\", O_RDWR): %s", pp->selected->device_path, strerror(errno));    
	return PP_ERROR;
    }
    pp->pd.init = 1;
    MSG("Device %s opened with handler=%d", pp->selected->device_path, pp->pd.handler);
    if(ioctl(pp->pd.handler, PPCLAIM) == -1){
	MSG("ioctl(%d, PPCLAIM): %s", pp->pd.handler, strerror(errno));
	parport_close(pp);
	return PP_ERROR;
    }
    pp->pd.init = 2;
    pp->pd.mirror[0] = pp->pd.mirror[1] = pp->pd.mirror[2] = 0;      
    parport_reset(pp);
    pp->pd.allow = 0;
    pp->pd.opened = 1;
    return 0;
}

void parport_close(s_parport *pp)
{
    int err = 0;
    if(!pp) return;

    if( !pp->pd.opened ) return;
    
//    if( usb_sw ){ 
//	parport_usb_close( usb );
//	return;
//    }

    if(pp->pd.init == 0) return;
    MSG("Close parport device.");
    if(pp->pd.init > 1)
	if(ioctl(pp->pd.handler, PPRELEASE) == -1){
	    MSG("ioctl(%d, PPRELEASE): %s", pp->pd.handler, strerror(errno));
	    err = PP_ERROR;
	}
    if(pp->pd.init)
	if(close(pp->pd.handler) == -1){
	    MSG("close(%d): %s", pp->pd.handler, strerror(errno));
	    err = PP_ERROR;
	}
    if(!err) pp->pd.init = 0;
    pp->pd.opened = 0;
}

static void parport_del_devices(s_parport *pp)
{
    s_parport_list *tmp, *it;

    for( it = pp->list; it; ){
	tmp = it->next;
	if(it->alias) free( it->alias );
	if(it->device_path) free( it->device_path );
	free( it );
	it = tmp;    
    }
    pp->list = NULL;
    pp->selected = NULL;
}

static char parport_add_device(s_parport *pp, const char *dev_path, const char *alias, int flags)
{
    s_parport_list *tmp, *it;
    
    MALLOC( tmp, s_parport_list, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset( tmp, 0, sizeof(s_parport_list) );
    if( dev_path) {
	MALLOC(tmp->device_path, char, strlen(dev_path) + 1){
	    ERR_MALLOC_MSG;
	    free( tmp );
	    return ERR_MALLOC_CODE;	
	}
	strcpy( tmp->device_path, dev_path );
    }
    if( alias ){
	MALLOC(tmp->alias, char, strlen(alias) + 1){
	    ERR_MALLOC_MSG;
	    if(tmp->device_path) free( tmp->device_path );
	    free( tmp );
	    return ERR_MALLOC_CODE;	
	}
	strcpy( tmp->alias, alias );
    }
    tmp->flags = flags;
    tmp->device = pp;
    tmp->next = NULL;
    if( !pp->list ){
	pp->list = tmp;
	return 0;
    }
    for(it = pp->list; it->next; it = it->next);
    it->next = tmp;
    return 0;
}

static char parport_lookup_callback(const char *fname, const char *error, void *pp)
{
    char tmp[4096];
        
    if( (strlen(fname) + strlen(SYSTEM_DEVICE_PATH)) > 4094) return false;
    sprintf(tmp, "%s%s", SYSTEM_DEVICE_PATH, fname);
    parport_add_device( PARPORT( pp ), tmp, NULL, 0 );    
    return true;
}

static void parport_lookup_devices( s_parport *pp)
{
    char error[256];

    if(!file_ls(SYSTEM_DEVICE_PATH, "^parport[[:digit:]]*$", error, parport_lookup_callback, pp)){
	ERR("%s", error);
    }        
}

void parport_get_list( s_parport *pp, f_parport_callback cb, void *ptr, char filter )
{
    s_parport_list *it;
    if( !pp ) return;

    for( it = pp->list; it; it = it->next ){
	switch( filter ){
	    case PARPORT_FILTER_ALL  : cb( pp, it, ptr ); break;
	    case PARPORT_FILTER_ALIAS: if( !it->alias ) break;
				       if( strcmp( it->alias, "EMULATOR") ) cb( pp, it, ptr );
				       break;
	    case PARPORT_FILTER_NOALIAS: if( !it->alias ) cb( pp, it, ptr ); break;
	}
    }
}

char parport_set_device( s_parport *pp, const char *alias_name )
{
    s_parport_list *it;
    if( !pp ) return -1;
    for( it = pp->list; it; it = it->next ){
	if( !it->alias ) continue;
	if( !strcmp(it->alias, alias_name) ){
	    pp->selected = it;
	    return 0;
	}
    }
    return -1;
}

const s_parport_list *parport_get_device( s_parport *pp )
{
    if( !pp ) return NULL; 
    return pp->selected;
}

/***************************************************************************************************************************/
/* Operacje IO */

int parport_w_data(s_parport *pp, unsigned char data)
{
    if( !pp ) return 0;
    if(pp->pd.allow) return 0;
//    if(usb_sw) return parport_usb_write_data( usb, data);
    if(ioctl(pp->pd.handler, PPWDATA, &data)){
	MSG("ioctl(%d, PPWDATA, %d): %s\n", pp->pd.handler, data, strerror(errno));	
	parport_close(pp);
	return PP_ERROR;
    }    
    return 0;
}

int parport_w_ctrl(s_parport *pp, unsigned char data)
{
    if( !pp ) return 0;
    if(pp->pd.allow) return 0;
//    if(usb_sw) return parport_usb_write_ctrl( usb, data);
    data ^= 0x0b; /* negacja bitów sprzetowo negowanych */
    if(ioctl(pp->pd.handler, PPWCONTROL, &data)){
	MSG("ioctl(%d, PPWCONTROL, %d): %s\n", pp->pd.handler, data, strerror(errno));	
	parport_close(pp);
	return PP_ERROR;
    }    
    return 0;
}

int parport_r_stat(s_parport *pp)
{ 
    if( !pp ) return 0;
    if(pp->pd.allow) return 0;
    unsigned char data;
//    if(usb_sw) return parport_usb_read_stat( usb );
    data = 0;
    if(ioctl(pp->pd.handler, PPRSTATUS, &data)){
	MSG("ioctl(%d, PPRSTATUS): %s\n", pp->pd.handler, strerror(errno));
	parport_close(pp);
	return PP_ERROR;
    }    
    data ^= 0x80;
    return (int)data;
}


int parport_set(s_parport *pp, unsigned int port_idx, unsigned char data)
{
    if( !pp ) return 0;
    if( !pp->selected ) return 0;
    if(pp->pd.allow) return 0;
    if(port_idx > 2) return PP_ERROR;

    pp->pd.mirror[port_idx] = data;
    if(port_idx == PA)
	if(parport_w_data(pp,data) == PP_ERROR) return PP_ERROR;
    if(port_idx == PC)
	if(parport_w_ctrl(pp,data) == PP_ERROR) return PP_ERROR;
    return 0;
}

int parport_get(s_parport *pp, unsigned int port_idx)
{
    if( !pp ) return 0;
    if( !pp->selected ) return 0;
    if(pp->pd.allow) return 0;
    if(port_idx > 2) return 0;
    if(port_idx == PB) pp->pd.mirror[port_idx] = parport_r_stat(pp);
    return pp->pd.mirror[port_idx];
}

int parport_reset(s_parport *pp)
{
    if( !pp ) return 0;
    if(pp->pd.allow) return 0;
    if(parport_set(pp,PA, 0) == PP_ERROR) return PP_ERROR;
    if(parport_set(pp,PB, 0) == PP_ERROR) return PP_ERROR;
    if(parport_set(pp,PC, 0) == PP_ERROR) return PP_ERROR;
    return 0;
}

/***************************************************************************************************************************/
/* operacje bitowe na portach */

int parport_set_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    return parport_set(pp,idx, pp->pd.mirror[idx] | mask);
}

int parport_clr_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    return parport_set(pp,idx, pp->pd.mirror[idx] & ~mask);
}

int parport_get_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    register int x = parport_get(pp,idx) & mask;
    if(x == -1) return PP_ERROR;
    return x ? 1:0;
}

/**************************************************************************************************/


/************************************************************************************************************************
    Emulation layer
*/

/*
void parport_set_emulate(s_parport *pp, char sw)
{
    usb_sw = sw;
}
*/

/*
static int parport_usb_init(s_usb2lpt **usb, void *ptr)
{ 
//    usb_mirror = 0;
//    *usb = usb2lpt_init( ptr );
//    if( usb ) pp->pd.allow = 0;
    return 0;//!usb; 
}

static int parport_usb_write_data(s_usb2lpt *usb, char data)
{
//    usb_mirror &= 0xff00;
//    usb_mirror |= (data & 0x00ff);
//    usb2lpt_output( usb, usb_mirror );
    return 0; 
}

static inline char parport_usb_bit_out_relocate(char  a )
{
    return ((a & 8) >> 3) | ((a & 4) >> 1) | ((a & 2) << 1) | ((a & 1) << 3);
}

static inline char parport_usb_bit_in_relocate(char  a )
{
    a = (( a & 0x1) << 6) |
	(( a & 0x2) << 6) |    
	(( a & 0x4) << 3) |
	(( a & 0x8) << 1) |
	(( a & 0xa) >> 1);
    return a;
}

static int parport_usb_write_ctrl(s_usb2lpt *usb, char ctrl)
{
//    ctrl = parport_usb_bit_out_relocate( ctrl );
//    usb_mirror &= 0x00ff;
//    usb_mirror |= (((int)ctrl) << 8) & 0x0ff00;
//    usb2lpt_output( usb, usb_mirror );
    return 0; 
}

static char parport_usb_read_stat(s_usb2lpt *usb)
{
//    char tmp = usb2lpt_input( usb );
//    tmp = parport_usb_bit_in_relocate( tmp );
    return 0;//tmp; 
}

static int parport_usb_close(s_usb2lpt *usb)
{
//    usb_mirror = 0;
//    usb2lpt_free( usb );
//    usb = NULL;
//    pp->pd.allow = 1;
    return 0; 
}
*/
/********************************************************************************/



