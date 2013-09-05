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

#ifndef __GEP_USB_H__
#define __GEP_USB_H__
#include <libusb-1.0/libusb.h>

#define GUSB_ATTACH	1
#define GUSB_DETACH	0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    /* identification data, if value is 0, or NULL - ignored check */
    int	 vendor_id;
    int  product_id;
    int  class_id;
    char  *serial;
    char *dev_name;
    char *vend_name;
    /* list */    
    char *alias_name;   // name for listing
    int  dev_class;	// interface class USB, or emulating LPT, RS232 etc 
    int  bus;           // bus number, if -1 -> any
    int  addr;		// device addres on bus
    int  option;	// flags for driver
    char *drivers;	// comma separated string of drivers supported by device
} s_usb_device_id;

/*
    List of ALL compatible USB devices
*/
typedef struct s_usb_devlist_ s_usb_devlist;
struct s_usb_devlist_
{
    s_usb_device_id dev_id;
    /* status */    
    char plugged;	// '1' if device is connected to USB, otherwise '0'
    /* handler data */
    void *handler;	// USB handler aka libusb_device_handle, NULL if device is unplugged
    void *hp_hdl;	// Hotplug handlers aka libusb_hotplug_callback_handle[2]    
    void *device;	// libusb_device*
    /* next structure in queue, or NULL if last */
    s_usb_devlist *next;
};

#define GUSB_CALLBACK( x )	((f_usb_callback)x)

typedef void (*f_usb_callback)(s_usb_devlist*, char, void *);

typedef struct
{
    struct timeval *ht;	// hot plug timeout
    libusb_context *ctx;
    s_usb_devlist  *usb;
    char 	   notify;
    f_usb_callback callback;
    void	   *parameter;
} s_usb;

#define S_USB(x)	((s_usb *)(x))
#define SS_USB(x)	((s_usb **)(x))

/*
    Constructor of library
*/
int  gusb_init( s_usb ** ); // return 0 at success

/*
    Adds USB device to be awayting for plugged/unplugged status. Content of a stucture s_usb_dev_list is copying to queue.
    Structure s_usb_device_id should be filled prior function call.
    Mandatory settings:
	int	 vendor_id	-> Vendor ID key
	int  product_id		-> Product ID key
	int  class_id		-> Category class ( 0xff for vendor specific )
	char *alias_name	-> System visible alias name of device
	int  dev_class		-> System category
    Additional settings:
	int  serial		-> serial number or 0 for any
	char *dev_name		-> Device Name or NULL for any
	char *vend_name		-> Vendor Name or NULL for any
	int  bus    		-> bus number or -1 for any
*/
int  gusb_add_dev_item(s_usb *, s_usb_device_id *);

/*
    polling USB events, have to be call periodicaly. Using non blocking mode.
*/
void gusb_events(s_usb * );

/*
    Destructor of library
*/
void gusb_exit( s_usb * );

/*
    Enable or disable handle of incoming events
*/
void gusb_notify_allow(s_usb *, char flag);

/*
    Check and actualize list of connected devices. Should be called after device item add
*/
void gusb_scan_connected( s_usb *);

/*
    sets callback function to notify attach/detach devices. Should be called before gusb_scan_connected()
*/
void gusb_set_callback(s_usb *, f_usb_callback, void *ptr);

/*
    Removes lists of compatible devices
*/
void gusb_remove_list( s_usb * );

/*********************************************************************************************************/
/*
    open interface dev for I/O
*/
char gusb_open_iface(s_usb *, s_usb_devlist *dev);

/*
    close interface dev for I/O
*/
void gusb_close_iface( s_usb *, s_usb_devlist *dev);


#ifdef __cplusplus
}
#endif

#endif
