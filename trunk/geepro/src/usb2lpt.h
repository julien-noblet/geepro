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

#ifndef __USB2LPT_H__
#define __USB2LPT_H__

typedef struct 
{
//    usb_dev_handle *handle;
    void *handle;
    char *buffer;    
    int  bfsize;
} s_usb2lpt;

extern s_usb2lpt *usb2lpt_init();			// initialize driver
extern char usb2lpt_input(s_usb2lpt *usb);		// input data from adapter
extern char *usb2lpt_get_id(s_usb2lpt *usb);		// get identifier -> should be "Geepro"
extern char *usb2lpt_get_rev(s_usb2lpt *usb);		// get revision number -> [0].[1]
extern char usb2lpt_output(s_usb2lpt *usb, int data);	// set pins
//extern char usb2lpt_test( s_usb2lpt *usb ); 		// return TRUE if error
extern void usb2lpt_free(s_usb2lpt *usb );		// destroy driver

#endif /* __USB2LPT_H__ */

