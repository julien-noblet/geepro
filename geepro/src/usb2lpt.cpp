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
#include <libusb-1.0/libusb.h> 
#include "usb_reqs.h"  
#include "usbconfig.h" 
#include "usb2lpt.h"
#include "gep_usb.h"
#include "geepro.h"

static geepro *gep = NULL;

static char usb2lpt_get(s_usb2lpt *usb, char *buff, int buff_len, int req, int rcv)
{
    int cnt;
return 1;
    if( !usb ) return 0;
    cnt = libusb_control_transfer( USB_HANDLER(((s_usb_devlist*)usb->handle)->handler), LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE|LIBUSB_REQUEST_TYPE_CLASS,
	 req, 0, 0, (unsigned char *)buff, buff_len, 5000);
    if(cnt < rcv){
        if(cnt < 0){
            fprintf(stderr, "USB error get: %s\n", libusb_strerror( (enum libusb_error)cnt ));
        }else{
            fprintf(stderr, "only %d of %d bytes received.\n", cnt, rcv);
        }
	return 0;
    }

    return 1;
}        

static char usb2lpt_set(s_usb2lpt *usb, int buff_len, int req)
{
    int cnt;
return 1;
    if( !usb ) return 0;
    cnt = libusb_control_transfer( USB_HANDLER(((s_usb_devlist*)usb->handle)->handler), LIBUSB_ENDPOINT_OUT | LIBUSB_RECIPIENT_INTERFACE|LIBUSB_REQUEST_TYPE_CLASS,
	    req, 0, 0, (unsigned char *)usb->buffer, buff_len, 5000);
    if(cnt != buff_len){
        fprintf(stderr, "USB error set: %s\n", libusb_strerror( (enum libusb_error)cnt) );
        return 0;
    }
    return 1;
}

char usb2lpt_output(s_usb2lpt *usb, int data)
{
    int cnt;
return 1;
    if( !usb ) return 0;
    cnt = libusb_control_transfer( USB_HANDLER(((s_usb_devlist*)usb->handle)->handler), LIBUSB_ENDPOINT_OUT | LIBUSB_RECIPIENT_INTERFACE|LIBUSB_REQUEST_TYPE_CLASS, USB_RQ_OUTPUT,
	    0, data, (unsigned char *)usb->buffer, 0, 5000);
    if(cnt != 0){
        fprintf(stderr, "USB error out: %s\n", libusb_strerror( (enum libusb_error)cnt));
        return 0;
    }
    return 1;
}

/******************************************************************************************************************************************/

char usb2lpt_input(s_usb2lpt *usb)
{
    if( !usb ) return 0;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_INPUT, 1 )) return usb->buffer[0];
    return 0;
}

char *usb2lpt_get_id(s_usb2lpt *usb)
{
    if( !usb ) return NULL;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_ID, 7 )) return usb->buffer;
    return NULL;
}

char *usb2lpt_get_rev(s_usb2lpt *usb)
{
    if( !usb ) return NULL;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_REV, 2 )) return usb->buffer;
    return NULL;
}

char usb2lpt_test( s_usb2lpt *usb ) // return TRUE if error
{
    if( !usb ) return 1;
    usb->buffer[0] = 'E';
    usb->buffer[1] = 'c';
    usb->buffer[2] = 'h';
    usb->buffer[3] = 'o';
    usb2lpt_set(usb, 2, USB_RQ_ECHO );
    return !strncmp(usb->buffer, "Echo", 2);
}

s_usb2lpt *usb2lpt_init( void *ptr )
{
/*
    iface_qe *qe;
    s_usb2lpt *tmp = NULL;
    s_usb_devlist *dl;
    gep = (geepro *)ptr;
    int er = 0;

    if( gep == NULL ) return NULL;
        
    if(!(tmp = (s_usb2lpt *)malloc( sizeof(s_usb2lpt)))){
	printf("Malloc!\n");
	return NULL;
    }
    memset( tmp, 0, sizeof( s_usb2lpt ) );
    tmp->bfsize = 8;
    if(!(tmp->buffer = (char *)malloc( tmp->bfsize ))){
	printf("Malloc!\n");
	free( tmp );
	return NULL;
    }
    qe = iface_get_iface( gep->ifc, (char *)"Usb to LPT adapter");
    if( !qe ){
	if(tmp->buffer) free( tmp->buffer );
	free( tmp );	
	return NULL;
    }
    dl = (s_usb_devlist *)qe->handler;    
    if( !dl ){
	if(tmp->buffer) free( tmp->buffer );
	free( tmp );	
	return NULL;
    }
    if( !dl->plugged ){
	if(tmp->buffer) free( tmp->buffer );
	free( tmp );	
	return NULL;
    }
    if( gusb_open_iface( (s_usb *)gep->usb_cb, dl ) ){
	if(tmp->buffer) free( tmp->buffer );
	free( tmp );	
	return NULL;
    }    
    tmp->handle = dl;
*/
    return NULL;//tmp;
}

void usb2lpt_free(s_usb2lpt *usb )
{
    if( !usb || !gep) return;
    gusb_close_iface( (s_usb *)gep->usb_cb, (s_usb_devlist*)usb->handle );
    if(usb->buffer) free( usb->buffer );
    free( usb );
}

