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

/* zmienne globalne */
static s_usb2lpt *usb = NULL;
static char usb_sw = 0;
static short int usb_mirror = 0;
unsigned char parport_mirror[3];
static int parport_ppdev_fd=0;
static char allow=0;
void *parport_msgh_ptr = NULL;
static int parport_init_lvl=0;


static int parport_usb_init(s_usb2lpt **usb, void *);
static int parport_usb_write_data(s_usb2lpt *usb, char data);
static int parport_usb_write_ctrl(s_usb2lpt *usb, char ctrl);
static char parport_usb_read_stat(s_usb2lpt *usb);
static int parport_usb_close(s_usb2lpt *usb);
static void parport_lookup_devices( s_parport *pp);

/***************************************************************************************************************************/
/* niskopoziomowe IO */

/* do zmiany */

void parport_close(s_parport *pp)
{
    int err = 0;

    if(!pp) return;
    
//    if( usb_sw ){ 
//	parport_usb_close( usb );
//	return;
//    }
    if(parport_init_lvl == 0) return;
    MSG("Cleanup parport device.");
    if(parport_init_lvl > 1)
	if(ioctl(parport_ppdev_fd, PPRELEASE) == -1){
	    MSG("ioctl(%d, PPRELEASE): %s", parport_ppdev_fd, strerror(errno));
	    err = PP_ERROR;
	}
    if(parport_init_lvl)
	if(close(parport_ppdev_fd) == -1){
	    MSG("close(%d): %s", parport_ppdev_fd, strerror(errno));
	    err = PP_ERROR;
	}
    if(!err) parport_init_lvl = 0;
}

int parport_open( s_parport *pp )
{
    static char first_run=1;

    if( !pp ) return 0;
    if( !pp->selected ) return 0;
    
    if(!strcmp( pp->selected->device_path, "EMULATOR")){
	usb_sw = 1;
    } else 
	usb_sw = 0;
    allow = 1;

//    if( usb_sw ) return parport_usb_init( &usb, pp->emul );

    if(first_run){
	MSG( PARPORT_VERSION );
	first_run=0;
    } else{
	parport_close(pp);
    }

    MSG("Opening device %s\n", pp->selected->device_path);
    if((parport_ppdev_fd = open(pp->selected->device_path, O_RDWR | pp->selected->flags)) == -1 ){
	MSG("open(\"%s\", O_RDWR): %s\n", pp->selected->device_path, strerror(errno));    
	return PP_ERROR;
    }
    parport_init_lvl = 1;
    MSG("Device %s opened with handler=%d\n", pp->selected->device_path, parport_ppdev_fd);
    if(ioctl(parport_ppdev_fd, PPCLAIM) == -1){
	MSG("ioctl(%d, PPCLAIM): %s\n", parport_ppdev_fd, strerror(errno));
	parport_close(pp);
	return PP_ERROR;
    }
    parport_init_lvl = 2;
    parport_mirror[0] = parport_mirror[1] = parport_mirror[2] = 0;      

    parport_reset(pp);
    allow = 0;
    return 0;
}

int parport_w_data(s_parport *pp, unsigned char data)
{
    if( !pp ) return 0;
    if(allow) return 0;
//    if(usb_sw) return parport_usb_write_data( usb, data);
    if(ioctl(parport_ppdev_fd, PPWDATA, &data)){
	MSG("ioctl(%d, PPWDATA, %d): %s\n", parport_ppdev_fd, data, strerror(errno));	
	parport_close(pp);
	return PP_ERROR;
    }    
    return 0;
}

int parport_w_ctrl(s_parport *pp, unsigned char data)
{
    if( !pp ) return 0;
    if(allow) return 0;
//    if(usb_sw) return parport_usb_write_ctrl( usb, data);
    data ^= 0x0b; /* negacja bitów sprzetowo negowanych */
    if(ioctl(parport_ppdev_fd, PPWCONTROL, &data)){
	MSG("ioctl(%d, PPWCONTROL, %d): %s\n", parport_ppdev_fd, data, strerror(errno));	
	parport_close(pp);
	return PP_ERROR;
    }    
    return 0;
}

int parport_r_stat(s_parport *pp)
{ 
    if( !pp ) return 0;
    if(allow) return 0;
    unsigned char data;
//    if(usb_sw) return parport_usb_read_stat( usb );
    data = 0;
    if(ioctl(parport_ppdev_fd, PPRSTATUS, &data)){
	MSG("ioctl(%d, PPRSTATUS): %s\n", parport_ppdev_fd, strerror(errno));
	parport_close(pp);
	return PP_ERROR;
    }    
    data ^= 0x80;
    return (int)data;
}

/***************************************************************************************************************************/
/* Operacje IO */

int parport_set(s_parport *pp, unsigned int port_idx, unsigned char data)
{
    if( !pp ) return 0;
    if( !pp->selected ) return 0;
    if(allow) return 0;
    if(port_idx > 2) return PP_ERROR;

    parport_mirror[port_idx] = data;
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
    if(allow) return 0;
    if(port_idx > 2) return 0;
    if(port_idx == PB) parport_mirror[port_idx] = parport_r_stat(pp);
    return parport_mirror[port_idx];
}

int parport_reset(s_parport *pp)
{
    if( !pp ) return 0;
    if(allow) return 0;
    if(parport_set(pp,PA, 0) == PP_ERROR) return PP_ERROR;
    if(parport_set(pp,PB, 0) == PP_ERROR) return PP_ERROR;
    if(parport_set(pp,PC, 0) == PP_ERROR) return PP_ERROR;
    return 0;
}

/***************************************************************************************************************************/
/* operacje bitowe na portach */

int parport_set_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    return parport_set(pp,idx, parport_mirror[idx] | mask);
}

int parport_clr_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    return parport_set(pp,idx, parport_mirror[idx] & ~mask);
}

int parport_get_bit(s_parport *pp, unsigned int idx, unsigned int mask)
{
    register int x = parport_get(pp,idx) & mask;
    if(x == -1) return PP_ERROR;
    return x ? 1:0;
}

/**************************************************************************************************/

static char parport_add_device(s_parport *pp, char *dev_path, char *alias, int flags)
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
    tmp->next = NULL;
    if( !pp->list ){
	pp->list = tmp;
	return 0;
    }
    for(it = pp->list; it->next; it = it->next);
    it->next = tmp;
    return 0;
}

static void parport_del_devices(s_parport *pp)
{
    s_parport_list *tmp, *it;

    for( it = pp->list; it; ){
	tmp = it->next;
	if(it->device_path){
	    if(it->alias){
		if(strcmp(it->alias, "EMULATOR") ) free(it->device_path);
	    } else
		free(it->device_path);
	}
	if(it->alias){
	    if(strcmp(it->alias, "EMULATOR") ) free(it->alias);
	}
	free( it );
	it = tmp;    
    }
    pp->list = NULL;
    pp->selected = NULL;
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

char parport_init( s_parport **pp , void *emul )
{
    if( !pp ) return -2;
    if( *pp ) return -2; // check if it is not NULL then return.
    
    MALLOC( *pp, s_parport, 1 ){
	ERR_MALLOC_MSG;	
	return ERR_MALLOC_CODE;
    }    
    memset( *pp, 0, sizeof(s_parport) );
    parport_lookup_devices( *pp );
    parport_add_device( *pp, (char *)"USB", (char *)"EMULATOR", 0 );
    (*pp)->emul = emul;
    return 0;
}

void parport_exit( s_parport *pp )
{
    if( !pp ) return;
    parport_del_devices( pp );
    free( pp );
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

void parport_set_emulate(s_parport *pp, char sw)
{
    usb_sw = sw;
}

/************************************************************************************************************************
    USB emulation layer
*/
/*
static int parport_usb_init(s_usb2lpt **usb, void *ptr)
{ 
//    usb_mirror = 0;
//    *usb = usb2lpt_init( ptr );
//    if( usb ) allow = 0;
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
//    allow = 1;
    return 0; 
}
*/
/********************************************************************************/



/*
static const char *iface_get_iface_wildcard( int type)
{
    switch (type) {
	case IFACE_LPT: return "^parport[[:digit:]]*$";
	case IFACE_USB: return "^usb[[:digit:]]*$";
	case IFACE_RS232: return "^ttyS[[:digit:]]*$";
    }
    
    return NULL;
}

typedef struct 
{
    iface *ifc;
    int   type;    
} iface_tmp_arg;

static boolean iface_callback_list(const char *fname, const char *error, void *arg)
{
    char tmp[4096];
    
    if( (strlen(fname) + strlen(SYSTEM_DEVICE_PATH)) > 4094) return false;
    
    iface_tmp_arg *targ = (iface_tmp_arg *)arg;
    
    sprintf(tmp, "%s%s", SYSTEM_DEVICE_PATH, fname);
    
    iface_add(targ->ifc, targ->type, (char *)fname, tmp, NULL);
    return true;
}

static void iface_add_list(iface *ifc, int iface_type)
{
    char error[256];
    const char *regex = iface_get_iface_wildcard( iface_type );
    iface_tmp_arg tmp;
    
    tmp.ifc = ifc;
    tmp.type = iface_type;
    error[0] = 0;
        
    if(!file_ls(SYSTEM_DEVICE_PATH, regex, error, iface_callback_list, &tmp)){
	fprintf(stderr, "[ERROR]%s\n",error);
    }

}



int iface_load_config(iface *ifc, void *cfg)
{
    char *tmp;
    ifc->ifc_sel = 0;
    ifc->cl = IFACE_LPT;

    ifc->prog_sel = 0;    
    tmp = NULL;
    if(!store_get(GEEPRO(ifc->gep)->store, "LAST_SELECTED_PROGRAMMER", &tmp)){
	if( tmp ){
	    ifc->prog_sel = strtol(tmp, NULL, 0);
	    free(tmp);
	}
    }
    tmp = NULL;
    if(!store_get(GEEPRO(ifc->gep)->store, "LAST_SELECTED_IFACE", &tmp)){
	if( tmp ){
	    ifc->ifc_sel = strtol(tmp, NULL, 0);
	    free(tmp);
	}
    }

// interface to choose
    iface_add_list( ifc, IFACE_LPT);
    iface_add_list( ifc, IFACE_USB);
    iface_add_list( ifc, IFACE_RS232);
// usb experimental
//    iface_add(ifc, IFACE_LPT, "Usb2Lpt adapter", "Usb2Lpt");
//exit(0);
    return 0;
}
*/

