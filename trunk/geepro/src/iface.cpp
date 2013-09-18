/* $Revision: 1.4 $ */

/* 
 * Copyright (C) Krzysztof Komarnicki
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
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <signal.h>
#include "error.h"
#include "../intl/lang.h"

#define MAX_PATH_LENGTH	4096

extern "C" {
    #include "iface.h"
    #include "../drivers/hwdriver.h"
    #include "chip.h"
    #include "main.h"
    #include "files.h"
}

typedef struct
{
    s_cfp *cfg;
    s_iface_device *ifc;
} s_iface_tmp;


static void iface_driver_list(s_iface_driver *ifc, const char *actual_path);
static char iface_driver_configure(s_iface_driver *ifc, s_cfp *cfg);
static char iface_driver_select_driver(s_iface_driver *ifc, const char *driver_name);
static char iface_chip_select_last( s_iface_chip *chip);
static char iface_chip_test_allowable(s_iface_chip *chip, const char *name, int len);

// candidates for static 
char iface_driver_init(s_iface_driver **ifc, store_str *str, s_cfp *cfg);
char iface_device_init( s_iface_device **ifc, store_str *st);
void iface_device_configure( s_iface_device *ifc, s_cfp *cfg);
void iface_device_destroy( s_iface_device *ifc );
void iface_driver_exit( s_iface_driver *ifc);
const char *iface_device_get_key_stored(s_iface_device *ifc);


iface *iface_init( store_str *st, s_cfp *cfg )
{
    char *tmp;
    iface *ifc;
    MALLOC(ifc, iface, 1){
	ERR_MALLOC_MSG;
	return NULL;
    }
    memset(ifc, 0, sizeof(iface));
    // driver configuration
    iface_driver_init( &ifc->drv, st, cfg);
    if( ifc->drv ){
	ifc->drv->store = st;
    }
    if( st ){
	if(store_get( st, IFACE_LAST_SELECTED_PRG_KEY, &tmp) == 0){
	    if(tmp){
		MSG("Set last selected programmer to '%s'", tmp);
		iface_driver_select_driver( ifc->drv, tmp );
	        free( tmp );
	    }
	}
    }
    // interface configuration
    iface_device_init( &ifc->dev, st);
    iface_renew( ifc );
    iface_device_configure(ifc->dev, cfg);
    // chip
    iface_chip_init( &ifc->chp, st, cfg );
    return ifc;
}

void iface_destroy(iface *ifc)
{
    if(!ifc) return;
    iface_chip_exit( ifc->chp );
    iface_device_destroy( ifc->dev );
    iface_driver_exit( ifc->drv );
    free(ifc);
}
void iface_renew(iface *ifc)
{
    char *tmp = NULL;
        
    if( !ifc->drv ) return;
    if( !ifc->drv->selected) return;
    if( !ifc->drv->selected->api) return;

    ifc->drv->selected->api(NULL, HW_NAME, 0, &tmp);
    iface_device_set_programmer( ifc->dev, ifc->drv->selected->api(NULL, HW_IFACE, 0, NULL), tmp);
}

static char iface_device_add_list( s_iface_device *ifc, const char *name, int cl, int group, void *handle )
{
    s_iface_devlist *tmp, *i;

    if( !ifc ) return 0;
    MALLOC( tmp, s_iface_devlist, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset( tmp, 0, sizeof(s_iface_devlist) );
    if( name ){
	MALLOC( tmp->name, char, strlen( name ) + 1){
	    ERR_MALLOC_MSG;
	    return ERR_MALLOC_CODE;    
	}    
	strcpy( tmp->name, name);
    }
    tmp->handler = handle;        
    tmp->cl = cl;
    tmp->group = group;
    tmp->next = NULL;
    if( !ifc->list ){
	ifc->list = tmp;
	return 0;
    }
    for( i = ifc->list; i->next; i = i->next );
    i->next = tmp;
    return 0;
}

static void iface_device_delete_list( s_iface_device *ifc)
{
    s_iface_devlist *it, *tmp;
    if( !ifc ) return;        
    for(it = ifc->list; it;){
	tmp = it->next;
	if(it->name) free(it->name);
	free( it );    
	it = tmp;
    }    
    ifc->list = NULL;
}

static void iface_device_config_usb(s_iface_device *ifc, s_cfp *cfg)
{
    char *tmp= NULL;
    s_usb_device_id id;    
    char path[256];
    int i, k;
    long val;

    MSG("Registering USB devices.");

    k = cfp_tree_count_element(cfg, "/usb_devices/device", "device");
    for(i = 0; i < k; i++){
	sprintf( path, "/usb_devices/device:%i/vendor_id", i);    
	cfp_get_long( cfg, path, &val);
	id.vendor_id = val;

	sprintf( path, "/usb_devices/device:%i/product_id", i);    
	cfp_get_long( cfg, path, &val);
	id.product_id = val;

	sprintf( path, "/usb_devices/device:%i/class", i);    
	cfp_get_long( cfg, path, &val);
	id.dev_class = val;

	sprintf( path, "/usb_devices/device:%i/class_id", i);    
	id.class_id = (cfp_get_long( cfg, path, &val)) ? -1 : val;

	sprintf( path, "/usb_devices/device:%i/option", i);    
	id.option = (cfp_get_long( cfg, path, &val)) ? 0 : val;

	sprintf( path, "/usb_devices/device:%i/bus", i);    
	id.bus = (cfp_get_long( cfg, path, &val)) ? -1 : val;

	sprintf( path, "/usb_devices/device:%i/address", i);    
	id.addr = (cfp_get_long( cfg, path, &val)) ? -1 : val;

	tmp = NULL;
	sprintf( path, "/usb_devices/device:%i/serial", i);    
	cfp_get_string( cfg, path, &tmp);
	id.serial = tmp;

	tmp = NULL;
	sprintf( path, "/usb_devices/device:%i/device", i);    
	cfp_get_string( cfg, path, &tmp);
	id.dev_name = tmp;

	tmp = NULL;
	sprintf( path, "/usb_devices/device:%i/vendor", i);    
	cfp_get_string( cfg, path, &tmp);
	id.vend_name = tmp;

	tmp = NULL;
	sprintf( path, "/usb_devices/device:%i/drivers", i);    
	cfp_get_string( cfg, path, &tmp);
	id.drivers = tmp;

	tmp = NULL;
	sprintf( path, "/usb_devices/device:%i/alias", i);    
	cfp_get_string( cfg, path, &tmp);
	id.alias_name = tmp;

	gusb_add_dev_item( ifc->usb, &id);    
	if(id.dev_name) free(id.dev_name);
	if(id.vend_name) free(id.vend_name);
	if(id.alias_name) free(id.alias_name);
	if(id.drivers) free(id.drivers);
	if(id.serial) free(id.serial);
    }
}

static void iface_device_scan_usb( s_iface_device *ifc, int device_class, const char *driver_name )
{
    s_usb_devlist *it;
    char *devlist, *x;    
    int len;
    
    if( !ifc ) return;
    if( !ifc->usb ) return;
    // Scan all compatible plugged USB devices
    for(it = ifc->usb->usb; it; it = it->next ){
	if( !it->plugged ) continue;
	if( it->dev_id.dev_class != device_class) continue;
	devlist = it->dev_id.drivers;
	while( devlist ){
	    if(!*devlist) return;
	    if(*devlist == ',') devlist++;
	    x = strchr( (char *)devlist, ',');
	    if( x )
		len = x - devlist;
	    else
		len = strlen( devlist );
	    if(!strncmp( driver_name, devlist, len))
		iface_device_add_list(ifc, it->dev_id.alias_name, IFACE_USB, it->dev_id.dev_class, it);
	    devlist = x;
	}        	
    }
}

static void ifc_device_usb_callback(s_usb_devlist *device, char flag, s_iface_device *ifc )
{
    if(! ifc->prog_name ) return;
    iface_device_rescan(ifc);
}

static void iface_device_parport_set_alias(s_parport *pp, s_parport_list *it, s_iface_tmp *ctx )
{
    char *tmp= NULL;
    char path[256];
    long val;
    int k,i;

    if(!it || !ctx || !pp ) return;
    if( !ctx->cfg || !ctx->ifc ) return;
    k = cfp_tree_count_element(ctx->cfg, "/parport_devices/device", "device");
    for(i = 0; i < k; i++){
	tmp = NULL;
	sprintf( path, "/parport_devices/device:%i/path", i);    
	cfp_get_string( ctx->cfg, path, &tmp);
        if( !tmp ) continue;
        if( !it->device_path ) continue;
        if( strcmp(tmp, it->device_path) ) continue; // check if device path match
	free( tmp );
	sprintf( path, "/parport_devices/device:%i/flags", i);    
	cfp_get_long( ctx->cfg, path, &val);
	it->flags = val;
	tmp = NULL;
	sprintf( path, "/parport_devices/device:%i/alias", i);    
	cfp_get_string( ctx->cfg, path, &tmp);
	if(it->alias) free(it->alias);
	it->alias = tmp;
	PRN(" * found parallel port device:'%s', alias name:'%s' flags: 0x%x\n", it->device_path, it->alias, it->flags);
    }
}

static void iface_device_config_parport(s_iface_device *ifc, s_cfp *cfg)
{
    s_iface_tmp tmp;
    tmp.cfg = cfg;
    tmp.ifc = ifc;
    MSG("Registering parallel ports:");
    parport_get_list( ifc->lpt, PARPORT_CALLBACK(iface_device_parport_set_alias), &tmp, PARPORT_FILTER_NOALIAS );
}

static void iface_device_parport_add(s_parport *pp, s_parport_list *it, s_iface_device *ifc )
{
    iface_device_add_list(ifc, it->alias, IFACE_LPT, IFACE_LPT, it);	
}

static void iface_device_scan_parport( s_iface_device *ifc )
{
    if( ifc->prog_class != IFACE_LPT) return;
    parport_get_list( ifc->lpt, PARPORT_CALLBACK(iface_device_parport_add), ifc, PARPORT_FILTER_ALIAS );
}

/********************************************************************************************/

static void iface_device_serial_set_alias(s_serial *pp, s_serial_list *it, s_iface_tmp *ctx )
{
    char *tmp= NULL;
    char path[256];
    int k,i;
    if(!it || !ctx || !pp ) return;
    if( !ctx->cfg || !ctx->ifc ) return;
    
    k = cfp_tree_count_element(ctx->cfg, "/serial_devices/device", "device");
    for(i = 0; i < k; i++){
	tmp = NULL;
	sprintf( path, "/serial_devices/device:%i/path", i);    
	cfp_get_string( ctx->cfg, path, &tmp);
        if( !tmp ) continue;
        if( !it->device_path ) continue;
        if( strcmp(tmp, it->device_path) ) continue; // check if device path match
	free( tmp );
	tmp = NULL;
	sprintf( path, "/serial_devices/device:%i/alias", i);    
	cfp_get_string( ctx->cfg, path, &tmp);
	if(it->alias) free(it->alias);
	it->alias = tmp;
	PRN(" * found serial port device:'%s', alias name:'%s'\n", it->device_path, it->alias);
    }
}

static void iface_device_config_serial(s_iface_device *ifc, s_cfp *cfg)
{
    s_iface_tmp tmp;
    tmp.cfg = cfg;
    tmp.ifc = ifc;
    MSG("Registering serial ports:");
    serial_get_list( ifc->com, SERIAL_CALLBACK(iface_device_serial_set_alias), &tmp);
}

static void iface_device_serial_add(s_serial *pp, s_serial_list *it, s_iface_device *ifc )
{
    if(it->alias)
	iface_device_add_list(ifc, it->alias, IFACE_RS232, IFACE_RS232, it);
}


static void iface_device_scan_serial( s_iface_device *ifc )
{
    if( ifc->prog_class != IFACE_RS232) return;
    serial_get_list( ifc->com, SERIAL_CALLBACK(iface_device_serial_add), ifc );
}

char iface_device_init( s_iface_device **ifc, store_str *st)
{
    if( !ifc ) return -2;
    if( *ifc ) {
	ERR("*s_iface_device != NULL");
	return -2;
    }
    MALLOC( *ifc, s_iface_device, 1 ){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset( *ifc, 0, sizeof( s_iface_device ) );
    (*ifc)->store = st;
    // LPT ports
    parport_init( &((*ifc)->lpt), *ifc );
    // RS232 ports
    serial_init( &((*ifc)->com) );
    // USB devices
    gusb_init( &((*ifc)->usb) );
    return 0;
}

void iface_device_destroy( s_iface_device *ifc )
{
    iface_device_delete_list( ifc );
    parport_exit( ifc->lpt );
    serial_exit( ifc->com );
    gusb_exit( ifc->usb );
}

char iface_device_rescan( s_iface_device *ifc)
{ 
    char tmp[4096];

    if( !ifc ) return 0;
    tmp[0] = 0;
    if(ifc->selected){
	if(ifc->selected->name)
	    strcpy(tmp, ifc->selected->name); // store selected iface name
    }

    iface_device_delete_list( ifc );
    iface_device_scan_parport( ifc );
    iface_device_scan_serial( ifc );
    iface_device_scan_usb( ifc, ifc->prog_class, ifc->prog_name );
    if(tmp[0]){
	iface_device_select(ifc, tmp); // choose previously selected iface
    }
    // notify to refresh display list of devices
    if( ifc->notify )
	ifc->notify(ifc, ifc->selected, ifc->notify_ptr );
    return 0;
}

static void iface_device_set_device( s_iface_device *ifc )
{
    const char *key;
    if(!ifc->selected) return;
    if( ifc->store ){
	key = iface_device_get_key_stored( ifc );
	if( store_set( ifc->store, key,  ifc->selected->name) ){
	    MSG("Cannot store '%s' variable to '%s'", key, ifc->selected->name); 
	}
    }
    switch(ifc->selected->group){
	case IFACE_LPT: parport_set_device( ifc->lpt, ifc->selected->name); 
			parport_open( ifc->lpt ); 
			break;
	case IFACE_USB:;
	case IFACE_RS232:;
    }
}

char iface_device_select( s_iface_device *ifc, const char *device_name )
{    
    s_iface_devlist *it;

    if( !ifc ) return 0;

    if(ifc->selected){
	if( ifc->selected->name ){
	    if(!strcmp(ifc->selected->name, device_name)) return 0; // already selected, so ignore
	}
    }
    for(it = ifc->list; it; it = it->next){
	if( !strcmp( it->name, device_name ) ){
	    ifc->selected = it;
	    iface_device_set_device( ifc );
	    return 0;
	}
    }    
    return 1;
}

char iface_device_get_list( s_iface_device *ifc, f_iface_device dev, void *ptr)
{    
    s_iface_devlist *it;
    int iter = 0;
    
    if( !ifc ) return 0;
    if( !dev ) return -2;
    for(it = ifc->list; it; it = it->next) dev( ifc, it, ptr, iter++);
    return 0;
}

char iface_device_connect_notify( s_iface_device *ifc, f_iface_device_notify notify, void *ptr)
{    
    if( !ifc ) return 0;
    if( !notify ) return -2;
    ifc->notify = notify;
    ifc->notify_ptr = ptr;
    return 0;
}

const char *iface_device_get_key_stored(s_iface_device *ifc)
{
    static char key[256];

    if( !ifc ) return NULL;
    if( !ifc->prog_name ) return NULL;
    if( strlen(ifc->prog_name ) + strlen(IFACE_LAST_SELECTED_IFC_KEY) + 6 >= 256 ){
	ERR("Key name for storing variable exceed 250 characters !");
	return NULL;
    }
    sprintf(key, "%s[%s]", IFACE_LAST_SELECTED_IFC_KEY, ifc->prog_name);
    return key;
}

static char iface_device_match_iface(s_iface_device *ifc)
{
    s_iface_devlist *it;
    if( !ifc ) return -2;
    if( !ifc->list ) return -2;
    for(it = ifc->list; it; it = it->next){
	if(!iface_device_select(ifc, it->name)){
	    MSG("Set interface to '%s'", it->name);
	    return 0;
	}
    }    
    return -1;
}

char iface_device_select_stored(s_iface_device *ifc)
{
    char *tmp = NULL;
    char match = 0;
    char err = -3; // no entry in storings

    // select default
    tmp = NULL;
    if( !ifc ) return err;
    if( ifc->store ){
	if(store_get( ifc->store, iface_device_get_key_stored(ifc), &tmp) == 0){
	    if( tmp ){
		err = -2; // no matched iface
		if(iface_device_select(ifc, tmp)){
		    MSG("Cannot set device to '%s' for driver '%s'", tmp, ifc->prog_name);
		    match = 1;
		} else {
		    MSG("Selected device '%s' (last selected)", tmp);
		    err = 0; // success
		}
		free( tmp );
	    } else match = 1;
	} else match = 1;
	if( match ) {
	    if( iface_device_match_iface( ifc ) ){
		MSG("There is no matched device for programmer '%s'", ifc->prog_name);
	    } else err = -1;
	}
    }
    return err;
}

void iface_device_configure( s_iface_device *ifc, s_cfp *cfg)
{
    iface_device_config_parport( ifc, cfg);
    iface_device_scan_parport( ifc ); // to add interfaces to list
    iface_device_config_serial( ifc, cfg);
    iface_device_scan_serial( ifc );
    iface_device_config_usb( ifc, cfg);
    gusb_set_callback( ifc->usb, GUSB_CALLBACK( ifc_device_usb_callback ), ifc);
    gusb_scan_connected( ifc->usb );
}

void iface_device_event( s_iface_device *ifc)
{
    gusb_events( ifc->usb );
}

void iface_device_set_programmer(s_iface_device *ifc, int device_class, const char *prog_name)
{
    if( !ifc || !prog_name ) return;
    ifc->prog_name = prog_name;
    ifc->prog_class = device_class;
    iface_device_rescan( ifc );
}

/*
**************************
         DRIVER
**************************
*/

int iface_driver_add(s_iface_driver_list *il, iface_prg_api api, char flag)
{
    char *name = NULL;
    s_iface_driver_list *tmp;

    if( !il ) return 0;        
    if(!api) return 0;
    if( il->parent ){	// set api function
	api(NULL, HW_NAME, 0, &name);	
	if( strcmp(il->name, name) ) return 0;
	il->api = api;
	return 0;
    }
    api(NULL, HW_NAME, 0, &name);
    PRN(" * '%s'\n", name);
    tmp = il;
    if(il->name){ // il->name exist, so create next link
	for(;il->next; il = il->next); // go to last link
	MALLOC(tmp, s_iface_driver_list, 1){
	    ERR_MALLOC_MSG;    
	    return -1; // memory allocation error	
	}
	memset(tmp, 0, sizeof( s_iface_driver_list ));
	// copy path to driver
	MALLOC(tmp->driver_path, char, strlen(il->driver_path) + 1){
	    ERR_MALLOC_MSG;    
	    free( tmp );
	    return -1; // memory allocation error	
	}
	strcpy(tmp->driver_path, il->driver_path);
	il->next = tmp;
    }
    MALLOC(tmp->name, char, strlen(name) + 1){
        ERR_MALLOC_MSG;    
        return -1; // memory allocation error	
    }
    strcpy(tmp->name, name);
    tmp->iface_class = api(NULL, HW_IFACE, 0, NULL);
    return 0;
}

static void iface_driver_destroy(s_iface_driver *ifc)
{
    s_iface_driver_list *it, *tmp;
    if( !ifc ) return;        
    if( ifc->selected ){
	if( ifc->dl_handle ) dlclose( ifc->dl_handle );
	ifc->selected = NULL;
    }
    for(it = ifc->list; it; ){
	tmp = it->next;
	if(it->driver_path) free( it->driver_path );
	if(it->name) free( it->name );
	if(it->xml) free( it->xml );
	it = tmp;
    }
    ifc->list = NULL;
}

void iface_driver_scan(s_iface_driver *ifc)
{
    char *tmp = NULL;
    char  buff[4096], *cwd;
    if( !ifc ) return;    
    if( ifc->list ){
	if( ifc->selected ){ // store selected driver name
		MALLOC( tmp, char, sizeof( strlen(ifc->selected->name) + 1)){
		ERR_MALLOC_MSG;
		return;
	    }
	    strcpy(tmp, ifc->selected->name);
	}
	iface_driver_destroy( ifc );	
	ifc->list = NULL;
	ifc->selected = NULL;
    }
    if(!(cwd = getcwd( buff, 4096 ))){
	ERR("Cannot get current directory\n");    
    } 
    iface_driver_list( ifc, ifc->drv_file_path );
    if( cwd )
	if( chdir( cwd ) ){
	    ERR("chdir to directory %s\n", cwd);
	}        
    if( tmp ){
	iface_driver_select( ifc, tmp);
	free( tmp );
    }    
}

char iface_driver_init(s_iface_driver **ifc, store_str *str, s_cfp *cfg)
{ 
    char *tmp;
    MALLOC( *ifc, s_iface_driver, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset(*ifc, 0, sizeof( s_iface_driver ));
    tmp = NULL;

    cfp_get_string( cfg, "/drivers/drv_path", &tmp);
    if(!tmp){
	ERR("Missing /drivers/drv_path variable in config file.");
	return -1;
    }
    (*ifc)->drv_file_path = tmp;

    tmp = NULL;
    cfp_get_string( cfg, "/drivers/xml_path", &tmp);
    if(!tmp){
	ERR("Missing /drivers/xml_path variable in config file.");
	return -1;
    }
    (*ifc)->xml_file_path = tmp;

    iface_driver_scan( *ifc );
    iface_driver_configure( *ifc, cfg);
    
    return 0; 
}

void iface_driver_exit( s_iface_driver *ifc)
{
    if( !ifc ) return;
    iface_driver_destroy( ifc );	    
    if(ifc->drv_file_path) free(ifc->drv_file_path);
    if(ifc->xml_file_path) free(ifc->xml_file_path);
    free( ifc );
}

static void iface_driver_list(s_iface_driver *ifc, const char *actual_path)
{
    void *pf = NULL;
    DIR *dir;
    struct dirent *dr;
    char drv_path[PATH_MAX + 1], *pt, *full_path;
    int  size_fp, len;
    iface_regf init;
    s_iface_driver_list *tmp, *it;

    if( !ifc || !actual_path ) return;
    if( !realpath( actual_path, drv_path)){
	ERR("unresolved path %s Error code:%i", drv_path, errno);
	return;
    }    

    if( ifc->list ) iface_driver_destroy( ifc );
    if( chdir( drv_path ) ){
	ERR("chdir to directory %s\n", drv_path);
	return;
    }        
    if(!(dir = opendir("./"))){
	ERR("opening current directory! %s\n", drv_path);
	return;
    }
    // ls dir    
    size_fp = 4096;
    MALLOC(full_path, char, 4096){
	ERR_MALLOC_MSG;    
	return;
    }
    MSG("Registering drivers:");
    while((dr = readdir(dir))){
	if(!(pt = strchr( dr->d_name, '.'))) continue;		// looking for extension dot
	if(strcmp(pt + 1, DRIVER_FILE_EXTENSION)) continue;	// check file extension
	len = strlen( drv_path ) + strlen(dr->d_name) + 2;
	if( len >= size_fp){
	    pt = (char *)realloc(full_path, len);
	    if(!pt){
		ERR_MALLOC_MSG;    
		free( full_path );
		return; // memory allocation error
	    }
	    full_path = pt;
	}
	sprintf(full_path, "%s/%s", drv_path, dr->d_name);
	dlerror(); // clear dynamic link library errors
	if(!(pf = dlopen(full_path, RTLD_LAZY))){
	    ERR("Error: dlopen() --> %s", dlerror());
	    continue; // try next link
	}
	if(!(init = (iface_regf)dlsym(pf, IFACE_DRIVER_INIT_FUNC_NAME))){
	    ERR("Error: dlsym() --> %s", dlerror());
	    dlclose(pf);
	    continue;
	}
//	PRN(" * File:'%s'\n", full_path);
	// add programmer driver to list
	MALLOC(tmp, s_iface_driver_list, 1){
	    ERR_MALLOC_MSG;    
	    dlclose(pf);
	    free( full_path );
	    return; // memory allocation error	
	}
	memset(tmp, 0, sizeof( s_iface_driver_list ));
	// copy path to driver
	MALLOC(tmp->driver_path, char, strlen(full_path) + 1){
	    ERR_MALLOC_MSG;    
	    dlclose(pf);
	    free( tmp );
	    free( full_path );
	    return; // memory allocation error	
	}
	strcpy(tmp->driver_path, full_path);
	if(init( tmp )){
	    ERR(IFACE_DRIVER_INIT_FUNC_NAME"()");
	    dlclose(pf);
	    free( tmp );
	    free( full_path );
	    return;
	}
	if( !ifc->list ){
	    ifc->list = tmp;
	} else {
            for(it = ifc->list; it->next; it = it->next);
            it->next = tmp;
	}
	dlclose(pf);
    }
    free( full_path );
    closedir(dir);
}

void iface_driver_get_list(s_iface_driver *ifc, f_iface_driver_callback callback, void *ptr)
{
    s_iface_driver_list *it;

    if( !ifc || !callback) return;     
    for(it= ifc->list; it; it = it->next) 
				callback(ifc, it, ptr);
}

static char iface_driver_configure(s_iface_driver *ifc, s_cfp *cfg)
{ 
    char *tmp;
    char path[MAX_PATH_LENGTH];
    int k, i;
    s_iface_driver_list *it;

    if( !ifc || !cfg ) return 0;
    MSG("Configuring drivers.");
    k = cfp_tree_count_element(cfg, "/drivers/driver", "driver");            
    for(i = 0; i < k; i++){
	tmp = NULL;
	sprintf( path, "/drivers/driver:%i/name", i);    
	cfp_get_string( cfg, path, &tmp);
	for(it = ifc->list; it; it = it->next) if(!strcmp(it->name, tmp)) break;
	free( tmp );
	if( !it ) continue; // not found
	// xml file path
	tmp = NULL;
	sprintf( path, "/drivers/driver:%i/xml", i);    
	cfp_get_string( cfg, path, &tmp);
	if( !tmp ){
	    WRN("XML file name not set for '%s' - skipping", it->name);
	    continue;
	}
	if( !ifc->xml_file_path ){
	    free( tmp );
	    return -2;
	}
	MALLOC(it->xml, char, strlen(tmp) + strlen(ifc->xml_file_path) + 10){
	    ERR_MALLOC_MSG;    	
	    free( tmp );
	    return -1;
	}
	sprintf(it->xml, "file://%s/%s", ifc->xml_file_path, tmp);
	free(tmp);
	// chips
	sprintf( path, "/drivers/driver:%i/chips", i);    
	cfp_get_string( cfg, path, &it->chips);
	// used flag
	sprintf( path, "/drivers/driver:%i/flags", i);    
	cfp_get_long( cfg, path, &it->flags);
    }
    return 0; 
}

static char iface_driver_select_driver(s_iface_driver *ifc, const char *driver_name)
{ 
    s_iface_driver_list *it;
    iface_regf init;
    
    if( !ifc || !driver_name ) return 0;    
    for(it = ifc->list; it; it = it->next ){
	if( !strcmp(it->name, driver_name) && it->flags ){
	    ifc->selected = it;
	    break;
	}
    }
    if( !it ){
	WRN("There is no '%s' driver on list", driver_name);
	return -2;
    }
    dlerror(); // clear dynamic link library errors
    if(!(ifc->dl_handle = dlopen(it->driver_path, RTLD_LAZY))){
        ERR("Error: dlopen() --> %s", dlerror());
	return -3;
    }
    if(!(init = (iface_regf)dlsym(ifc->dl_handle, IFACE_DRIVER_INIT_FUNC_NAME))){
        ERR("Error: dlsym() --> %s", dlerror());
        dlclose(ifc->dl_handle);
	return -4;
    }
    it->parent = ifc;
    if(init( it )){
        ERR(IFACE_DRIVER_INIT_FUNC_NAME"()");
        dlclose(ifc->dl_handle);
        return -5;
    }
    return 0; 
}

char iface_driver_select(s_iface_driver *ifc, const char *driver_name)
{
    char err;
    err = iface_driver_select_driver( ifc, driver_name );
    if(err == 0){
	store_set(ifc->store, IFACE_LAST_SELECTED_PRG_KEY, driver_name);
    } else {
	ERR("Cannot select '%s' driver.", driver_name);
    }
    return err;
}

char iface_pgm_select(iface *ifc, const char *driver_name)
{
    char err;

    if( !driver_name ) return 0;
    err = iface_driver_select( ifc->drv, driver_name );
    if( err ) return err;
    iface_renew( ifc );    
    iface_device_select_stored( ifc->dev );
    return err;    
}

static int iface_driver_dummy(void *root, en_hw_api api, int arg, void *ptr){ return 0; }

hw_driver_type iface_driver_call(s_iface_driver *ifc)
{ 
    if(!ifc) return iface_driver_dummy; 
    if(!ifc->selected) return iface_driver_dummy; 
    if(!ifc->selected->api) return iface_driver_dummy; 
    return ifc->selected->api; 
}

const char *iface_get_xml_path(iface *ifc)
{
    if( !ifc ) return NULL;
    if( !ifc->drv ) return NULL;
    if( !ifc->drv->selected ) return NULL;
    if( !ifc->drv->selected->xml) return NULL;
    return ifc->drv->selected->xml;
}

/*********************************************************************************************************/
/*
	dlerror(); // clear dynamic link library errors
	if(!(pf = dlopen(full_path, RTLD_LAZY))){
	    ERR("Error: dlopen() --> %s", dlerror());
	    continue; // try next link
	}
	if(!(init = (iface_regf)dlsym(pf, IFACE_DRIVER_INIT_FUNC_NAME))){
	    ERR("Error: dlsym() --> %s", dlerror());
	    dlclose(pf);
	    continue;
	}
//	PRN(" * File:'%s'\n", full_path);
	// add programmer driver to list
	MALLOC(tmp, s_iface_driver_list, 1){
	    ERR_MALLOC_MSG;    
	    dlclose(pf);
	    free( full_path );
	    return; // memory allocation error	
	}
	memset(tmp, 0, sizeof( s_iface_driver_list ));
	// copy path to driver
	MALLOC(tmp->driver_path, char, strlen(full_path) + 1){
	    ERR_MALLOC_MSG;    
	    dlclose(pf);
	    free( tmp );
	    free( full_path );
	    return; // memory allocation error	
	}
	strcpy(tmp->driver_path, full_path);
	if(init( tmp )){
	    ERR(IFACE_DRIVER_INIT_FUNC_NAME"()");
	    dlclose(pf);
	    free( tmp );
	    free( full_path );
	    return;
	}
	if( !ifc->list ){
	    ifc->list = tmp;
	} else {
            for(it = ifc->list; it->next; it = it->next);
            it->next = tmp;
	}
	dlclose(pf);
*/

static char iface_chip_register( s_iface_chip *chip, const char *fname, char *drv_path)
{
    void *pf = NULL;
    iface_regf init_module;

    if( !realpath( fname, drv_path)){
	ERR("unresolved path %s Error code:%i", fname, errno);
	return -3;
    }    
    dlerror(); // clear dynamic link library errors
    if(!(pf = dlopen(drv_path, RTLD_LAZY))){
        ERR("Error: dlopen() --> %s", dlerror());
	return -4;
    }
    // get initialize plugin function
    if(!(init_module = (iface_regf)dlsym(pf, IFACE_MODULE_INIT_FUNC_NAME))){
        ERR("Error: dlsym() --> %s", dlerror());
        dlclose(pf);
        return -5;
    }
    
    chip->relay = pf;
    if(init_module( chip ))
		     dlclose( pf );

    return 0;
}

static char iface_chip_scan_dir( s_iface_chip *chip)
{
    DIR *dir;
    char drv_path[PATH_MAX + 1];
    struct dirent *dr;
    char *pt;

    MSG("Load chip plugins:");
    if(!(dir = opendir("./"))){
	ERR("opening current directory!");
	return -2;
    }

    while((dr = readdir(dir))){
	if(!(pt = strchr( dr->d_name, '.'))) continue;		 // looking for extension dot
	if(strcmp(pt + 1, CHIP_PLUGIN_FILE_EXTENSION)) continue; // check file extension	
	if(iface_chip_test_allowable( chip, dr->d_name, pt - dr->d_name)) continue; // skip if not allowable
	iface_chip_register( chip, dr->d_name, drv_path ); // ignore error for now --
    }
    closedir( dir );

    return 0;
}

static char iface_chip_build( s_iface_chip *chip)
{
    char  buff[4096], *cwd;
    char *tmp;
    tmp = NULL;


    if(!(cwd = getcwd( buff, 4096 ))){
	ERR("Cannot get current directory\n");    
	return -2;
    } 
    if(cfp_get_string( chip->cfg, "/chip_plugins/path", &tmp)){
	ERR("Missing path to chip plugins '/chip_plugins/path' in config file. ");
	return -3;
    }
    if( chdir( tmp ) ){
	ERR("chdir to directory %s\n", tmp);
	free( tmp );
	return -4;
    }        
    free( tmp );
    if(cfp_get_string( chip->cfg, "/chip_plugins/plugins", &tmp)){
	ERR("Missing allowable plugins list  '/chip_plugins/plugins' in config file. ");
	return -3;
    }
    chip->allowable = tmp;
    if( iface_chip_scan_dir( chip ) ){
	return -5;
    }    
    if( cwd )
	if( chdir( cwd ) ){
	    ERR("chdir to directory %s\n", cwd);
	}        
    return 0;
}

char iface_chip_init(s_iface_chip **chip, store_str *st, s_cfp *cfg)
{
    if( *chip ) return 0; // *chip have to be NULL
    MALLOC( *chip, s_iface_chip, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset(*chip, 0, sizeof( s_iface_chip ));        
    (*chip)->store = st;
    (*chip)->cfg = cfg;

    if(iface_chip_build( *chip )) return -2;
    if(iface_chip_select_last( *chip )) return -3;
    return 0;
}

static void iface_chip_list_destroy_action(s_iface_chip_action *ac)
{
    s_iface_chip_action *tmp, *it;

    for(it = ac; it;){
	tmp = it->next;
	free( it );
	it = tmp;
    }
}

static void iface_chip_destroy( s_iface_chip *chip)
{
    s_iface_chip_list *tmp, *it;

    for(it = chip->list; it;){
	tmp = it->next;
	if(it->action) iface_chip_list_destroy_action( it->action );
	if(it->handler) dlclose( it->handler );
	free( it );
	it = tmp;
    }
}


void iface_chip_exit(s_iface_chip *chip)
{
    if( !chip ) return;
    iface_chip_destroy( chip );
    free( chip );
}

// invoked during init module
void iface_chip_register(s_iface_chip *chip, s_iface_chip_list *cl)
{
    s_iface_chip_list *tmp, *it;
    
    if( !chip || !cl ) return;
    MALLOC( tmp, s_iface_chip_list, 1){
	ERR_MALLOC_MSG;
	// ++ destroy list
	return;
    }
    memcpy( tmp, cl, sizeof( s_iface_chip_list) );
    tmp->next = NULL;    
    tmp->handler = chip->relay;

    if(!chip->list){
	chip->list = tmp;
	return;
    }
    for(it = chip->list; it->next; it = it->next);    
    it->next = tmp;
}

static char iface_chip_select_last( s_iface_chip *chip )
{
    char *tmp = NULL;
    char err = 0;

    if( !chip ) return 0;
    if( !chip->store ) return 0;
    
    if(!store_get( chip->store, IFACE_LAST_SELECTED_CHIP_KEY, &tmp) == 0) return -2;
    if(tmp){
	MSG("Set last selected chip to '%s'", tmp);
	err = iface_chip_select( chip, tmp);
	free( tmp );
    }
    return err;
}

char iface_chip_select( s_iface_chip *chip, const char *chip_name)
{
    s_iface_chip_list *it;
    if( !chip_name || !chip) return 0;
    
    for(it = chip->list; it; it = it->next){
	if(!strcmp(it->name, chip_name)){
	    chip->selected = it;
	    if(!store_set( chip->store, IFACE_LAST_SELECTED_CHIP_KEY, chip_name) == 0){
		ERR("Cannot store variable '%s'", IFACE_LAST_SELECTED_CHIP_KEY);
		return -3;
	    }
	    return 0;
	}
    }
    return -2;
}

void iface_chip_get_list(s_iface_chip *chip, f_iface_chip fc, void *ptr)
{
    s_iface_chip_list *it;

    if( !fc || !chip) return;
    for(it = chip->list; it; it = it->next) fc(chip, it, ptr);
}

void iface_chip_get_actions(s_iface_chip *chip, f_iface_action fc, void *ptr)
{
    s_iface_chip_action *it;
    if( !fc || !chip) return;
    if( !chip->selected ) return;
    for(it = chip->selected->action; it; it = it->next) fc(chip, it, ptr);
}

/**********************************************************************************************************/
void iface_chip_list_init(s_iface_chip_list *cl, const char *path, const char *name, const char *family)
{
    if( !cl ) return;
    memset(cl, 0, sizeof( s_iface_chip_list ) );
    cl->path = path;
    cl->name = name;
    cl->family = family;
}

void iface_chip_list_add_action(s_iface_chip_list *cl, const char *bt_name, const char *tip, f_iface_chip_action cb)
{
    s_iface_chip_action *tmp, *it;
    
    if( !cl ) return;
    MALLOC( tmp, s_iface_chip_action, 1){
	ERR_MALLOC_MSG;
	// ++ destroy list
	return;
    }
    memset( tmp, 0, sizeof( s_iface_chip_action) );
    tmp->name = bt_name;
    tmp->tip = tip;
    tmp->action  = cb;    

    if( !cl->action ){
	cl->action = tmp;
        return;
    }
    for(it = cl->action; it->next; it = it->next);    
    it->next = tmp;
}

// temporary implementation
void iface_chip_list_add_buffer(s_iface_chip_list *cl, const char *name, int size)
{
    cl->buffer = NULL;
    cl->dev_size = size;
}

static char iface_chip_test_allowable(s_iface_chip *chip, const char *name, int len)
{
    if( len < 0 ) return -2;
// test allowable to write
    return 0;
}

const char *iface_get_chip_name( iface *ifc)
{
    if( !ifc ) return NULL;
    if( !ifc->chp ) return NULL;    
    if( !ifc->chp->selected ) return NULL;    
    return ifc->chp->selected->name;
}

const char *iface_get_chip_family( iface *ifc)
{
    if( !ifc ) return NULL;
    if( !ifc->chp ) return NULL;    
    if( !ifc->chp->selected ) return NULL;    
    return ifc->chp->selected->family;
}

const char *iface_get_chip_path( iface *ifc)
{
    if( !ifc ) return NULL;
    if( !ifc->chp ) return NULL;    
    if( !ifc->chp->selected ) return NULL;    
    return ifc->chp->selected->path;
}

