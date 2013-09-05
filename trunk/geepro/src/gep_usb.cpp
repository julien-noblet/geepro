/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "error.h"
#include "gep_usb.h"
#include "usbconfig.h"
#include <string.h>

#define USB_DEBUG

static void gusb_hotplug_notify(s_usb *usb, s_usb_devlist *id, char flag )
{
    if(!usb) return;
    if(!usb->callback) return;
    usb->callback(id, flag, usb->parameter);    
}

static int gusb_get_usb_info(libusb_device *dev, struct libusb_device_descriptor *desc, struct libusb_device_handle * handle, s_usb_device_id *id )
{
    static char dev_name[256], vend_name[256], serial[256];

    dev_name[0] = 0; vend_name[0] = 0; serial[0] = 0;
    libusb_get_string_descriptor_ascii( handle, desc->iManufacturer, (unsigned char *)vend_name, sizeof( vend_name ) );    
    libusb_get_string_descriptor_ascii( handle, desc->iProduct, (unsigned char *)dev_name, sizeof( dev_name ) );    
    libusb_get_string_descriptor_ascii( handle, desc->iSerialNumber, (unsigned char *)serial, sizeof( serial ) );    

    id->vendor_id  = desc->idVendor;
    id->product_id = desc->idProduct;
    id->class_id   = desc->bDeviceClass;
    id->serial     = serial;
    id->dev_name   = dev_name;
    id->vend_name  = vend_name;
    id->bus 	   = libusb_get_bus_number( dev );
    id->addr	   = libusb_get_device_address( dev );
    return 0;
}

static s_usb_devlist *gusb_looking_for_matched(s_usb *usb, s_usb_device_id *id)
{
    s_usb_devlist *it;
    
    for(it = usb->usb; it; it = it->next ){
	if( (it->dev_id.vendor_id != id->vendor_id) || (it->dev_id.product_id != id->product_id) ) continue; // Main key
	// additional optional test	
	if((it->dev_id.class_id > 0) && (it->dev_id.class_id != id->class_id)) continue;
	if((it->dev_id.bus > 0) && (it->dev_id.bus != id->bus)) continue;
	if((it->dev_id.addr > 0) && (it->dev_id.addr != id->addr)) continue;

	if( it->dev_id.serial )
	    if( strcmp(it->dev_id.serial, id->serial) ) continue;
	if( it->dev_id.dev_name )
	    if( strcmp(it->dev_id.dev_name, id->dev_name) ) continue;
	if( it->dev_id.vend_name )
	    if( strcmp(it->dev_id.vend_name, id->vend_name) ) continue;
	return it; 
    }
    return NULL;
}

static int LIBUSB_CALL gusb_hotplug_callback_attach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, s_usb *usb)
{
    libusb_device_descriptor desc;
    libusb_device_handle *handler = NULL;
    s_usb_devlist *found;
    s_usb_device_id id;    
    int rc;

    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "[ERR] Error getting USB device descriptor\n");
        return 1;
    }

    rc = libusb_open (dev, &handler); // open device for gather information
    if( rc < 0){
        fprintf (stderr, "[ERR] Error opening USB device\n");
        return 1;
    }

    if( (rc = gusb_get_usb_info( dev, &desc, handler, &id ) ) < 0 ){
        libusb_close (handler);
        fprintf (stderr, "[ERR] Error fetching USB device information\n");
        return 1;	    
    }
    libusb_close (handler);

    if(!( found = gusb_looking_for_matched( usb, &id))) return 0; // Normal return, it is just unknown device
    if( found->plugged ) return 0; // already connected, so ignore (for cause where few devices on the list has the same vendor:product keys )
    // set device status as plugged
    found->handler = NULL;
    found->plugged = 1;
    found->dev_id.bus = id.bus;
    found->dev_id.addr = id.addr;
    found->device = dev;
    MSG("USB plugged -> Bus %i%i%i Device %i%i%i: ID %x%x%x%x:%x%x%x%x \"%s\"", DEC_8(id.bus), DEC_8(id.addr), HEX_16(id.vendor_id), HEX_16(id.product_id), id.dev_name);
    gusb_hotplug_notify(usb, found, GUSB_ATTACH );
    return 0;
}

static int LIBUSB_CALL gusb_hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, s_usb *usb)
{
    struct libusb_device_descriptor desc;
    s_usb_devlist *it;
    int rc, bus, addr;

    // get descriptor of device
    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "[ERR] Error getting USB device descriptor\n");
        return 1;
    }
    bus   = libusb_get_bus_number( dev );
    addr  = libusb_get_device_address( dev );
    
    // looking for device to unplug

    for(it = usb->usb; it; it = it->next ){
	if( (it->dev_id.vendor_id != desc.idVendor) || (it->dev_id.product_id != desc.idProduct) ) continue;
	if((it->dev_id.bus == bus) && (it->dev_id.addr == addr)) break;
    }
    if( !it ) return 0; // unplugged not registered (plugged) device in geepro 
    if( !it->plugged ) return 0; // already unplugged
    it->plugged = 0;
    if( it->handler ){
	libusb_close((libusb_device_handle*)it->handler);
	it->handler = NULL;
    }
    it->dev_id.bus = -1;
    it->dev_id.addr = -1;
    MSG("USB unplugged -> Bus %i%i%i Device %i%i%i: ID %x%x%x%x:%x%x%x%x \"%s\"", DEC_8(bus), DEC_8(addr), HEX_16(desc.idVendor), HEX_16(desc.idProduct), it->dev_id.dev_name);
    gusb_hotplug_notify(usb, it, GUSB_DETACH );
    return 0;
}

static int gusb_hotplug_register(s_usb *usb, libusb_hotplug_callback_handle *hp, int vendor_id, int product_id, int class_id)
{
    int rc;

    rc = libusb_hotplug_register_callback(usb->ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, (libusb_hotplug_flag)0, vendor_id,
	product_id, class_id, (libusb_hotplug_callback_fn)gusb_hotplug_callback_attach, usb, &hp[0]);
    if (LIBUSB_SUCCESS != rc) {
	fprintf (stderr, "Error registering callback 0\n");
	libusb_exit (usb->ctx);
	return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (usb->ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, (libusb_hotplug_flag)0, vendor_id,
	product_id,class_id, (libusb_hotplug_callback_fn)gusb_hotplug_callback_detach, usb, &hp[1]);
    if (LIBUSB_SUCCESS != rc) {
	fprintf (stderr, "Error registering callback 1\n");
	libusb_exit (usb->ctx);
	return EXIT_FAILURE;
    }

    return 0;
}

int gusb_init(s_usb **usb)
{
    static s_usb usb_cb;
    static struct timeval tv;    

    *usb = &usb_cb;
    memset(*usb, 0, sizeof( s_usb ));
    (*usb)->ht = &tv;
    memset( (*usb)->ht, 0, sizeof(struct timeval));

    libusb_init ( &(*usb)->ctx );
    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
	printf ("[WRN] Hotplug capabilites are not supported on this platform\n");
	libusb_exit ((*usb)->ctx);
	return EXIT_FAILURE;
    }
    (*usb)->notify = 1;
#ifdef USB_DEBUG
    libusb_set_debug((*usb)->ctx, 3 );
#endif
    return 0;
}

void gusb_events(s_usb *usb)
{
    if(!usb) return;
    if(!usb->ctx || !usb->notify ) return;
    libusb_handle_events_timeout_completed( usb->ctx, usb->ht, NULL);
}

void gusb_exit(s_usb *usb)
{
    if(!usb->ctx) return;
    gusb_remove_list( usb );
    libusb_exit (usb->ctx);
}

static s_usb_devlist *gusb_add_dev_item_list(s_usb *usb, s_usb_device_id *item)
{
    s_usb_devlist *tmp, *it;

    if(item->bus >= 0)
	printf(" * [Bus %i: ID %4x:%4x] class: %i Device alias '%s'\n", item->bus , item->vendor_id, item->product_id, item->dev_class, item->alias_name);
    else
	printf(" * [Bus ANY: ID %4x:%4x] class: %i Device alias '%s'\n", item->vendor_id, item->product_id, item->dev_class, item->alias_name);

    if(!(tmp = (s_usb_devlist *)malloc( sizeof(s_usb_devlist) ))){
	printf("malloc!\n");
	return NULL;
    }
    memset( tmp, 0, sizeof(s_usb_devlist) );
    tmp->dev_id.vendor_id  = item->vendor_id;
    tmp->dev_id.product_id = item->product_id;
    tmp->dev_id.class_id   = item->class_id;
    tmp->dev_id.bus 	   = item->bus;
    tmp->dev_id.addr 	   = item->addr;
    tmp->dev_id.dev_class  = item->dev_class;

    if( item->dev_name ){
	if(!(tmp->dev_id.dev_name = (char *)malloc( strlen(item->dev_name) + 1))){
	    printf("malloc!\n");
	    free( tmp );
	    return NULL;
	}
	strcpy(tmp->dev_id.dev_name, item->dev_name);
    }
    if( item->vend_name ){
	if(!(tmp->dev_id.vend_name = (char *)malloc( strlen(item->vend_name) + 1))){
	    printf("malloc!\n");
	    free(tmp->dev_id.dev_name);
	    free( tmp );
	    return NULL;
	}
	strcpy(tmp->dev_id.vend_name, item->vend_name);
    }
    if( item->alias_name ){
	if(!(tmp->dev_id.alias_name = (char *)malloc( strlen(item->alias_name) + 1))){
	    printf("malloc!\n");
	    free(tmp->dev_id.dev_name);
	    free(tmp->dev_id.vend_name);
	    free( tmp );
	    return NULL;
	}
	strcpy(tmp->dev_id.alias_name, item->alias_name);
    }
    if( item->serial ){
	if(!(tmp->dev_id.serial = (char *)malloc( strlen(item->serial) + 1))){
	    printf("malloc!\n");
	    free( tmp->dev_id.alias_name );
	    free(tmp->dev_id.dev_name);
	    free(tmp->dev_id.vend_name);
	    free( tmp );
	    return NULL;
	}
	strcpy(tmp->dev_id.serial, item->serial);
    }

    if( item->drivers ){
	if(!(tmp->dev_id.drivers = (char *)malloc( strlen(item->drivers) + 1))){
	    printf("malloc!\n");
	    free(tmp->dev_id.serial);
	    free( tmp->dev_id.alias_name );
	    free(tmp->dev_id.dev_name);
	    free(tmp->dev_id.vend_name);
	    free( tmp );
	    return NULL;
	}
	strcpy(tmp->dev_id.drivers, item->drivers);
    }
    
    if(!(tmp->hp_hdl = (libusb_hotplug_callback_handle *)malloc( sizeof(libusb_hotplug_callback_handle) ))){
	    printf("malloc!\n");
	    free(tmp->dev_id.drivers);
	    free(tmp->dev_id.serial);
	    free(tmp->dev_id.alias_name);
	    free(tmp->dev_id.dev_name);
	    free(tmp->dev_id.vend_name);
	    free( tmp );
	    return NULL;    
    }

    if( usb->usb == NULL ){
	usb->usb = tmp;
	return tmp;
    }

    for(it = usb->usb; it->next; it = it->next);    
    it->next = tmp;

    return tmp;
}

int gusb_add_dev_item(s_usb *usb, s_usb_device_id *item)
{
    s_usb_devlist *tmp;
    if(!usb) return 0;
    if(!(tmp = gusb_add_dev_item_list(usb, item))) return 1;
    return gusb_hotplug_register(usb, (libusb_hotplug_callback_handle *)tmp->hp_hdl, tmp->dev_id.vendor_id, tmp->dev_id.product_id, tmp->dev_id.class_id);
}

void gusb_remove_list(s_usb *usb)
{
    s_usb_devlist *tmp, *x = usb->usb;
    if(!usb) return;    
    while( x ){
	tmp = x->next;
	if( x->dev_id.dev_name ) free( x->dev_id.dev_name);
	if( x->dev_id.vend_name ) free( x->dev_id.vend_name);
	if( x->dev_id.alias_name ) free( x->dev_id.alias_name);
	if( x->dev_id.serial ) free( x->dev_id.serial);
	if( x->dev_id.drivers ) free( x->dev_id.drivers);
	if( x->hp_hdl) free( x->hp_hdl );
	if( x->handler ) libusb_close( (libusb_device_handle *)x->handler );
	free( x );
	x = tmp;
    }
    usb->usb = NULL;
}

void gusb_notify_allow(s_usb *usb, int flag)
{
    if(!usb) return;
    usb->notify = flag;
}

void gusb_scan_connected(s_usb *usb)
{
    struct libusb_device **list;
    int rc, i;

    if(!usb) return;
    // make list of USB devices currently attached to the system
    rc = libusb_get_device_list( usb->ctx, &list);
    if( rc < 0 ){
	ERR("Getting USB device list");
	return;
    }
    for(i = 0; i < rc; i++)
	gusb_hotplug_callback_attach(usb->ctx, list[i], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, usb);
    libusb_free_device_list( list, 1);
}

void gusb_set_callback(s_usb *usb, f_usb_callback cb, void *ptr)
{
    if(!usb) return;
    usb->callback = cb;
    usb->parameter = ptr;
}

/*****************************************************************************************************************************/

char gusb_open_iface(s_usb *usb, s_usb_devlist *dev)
{
    int er;
    printf("TEST-> Open iface USB for %s\n", dev->dev_id.alias_name);
/*
    dev->handler = NULL;
    er = libusb_open((libusb_device *)dev->device, (libusb_device_handle**)(&dev->handler));
    if( er ){
	printf("open usb error\n");
	return 0;
    }

    er = libusb_claim_interface((libusb_device_handle*)dev->handler, 0);
    if( er ){
	printf("claim usb error\n");
	return 0;
    }

*/

    

    return 0;
}

void gusb_close_iface(s_usb *usb, s_usb_devlist *dev)
{
//    if( !dev->handler ) return;
//    libusb_close( (libusb_device_handle*)dev->handler );
    printf("TEST-> Close iface USB for %s\n", dev->dev_id.alias_name);
}




