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
#include "dummy.h"
#include "parport.h"
#include "error.h"

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

#include "../intl/lang.h"

iface *iface_init( store_str *st )
{
    iface *ifc;
    if(!(ifc = (iface *)malloc(sizeof(iface)))) return NULL;
    memset(ifc, 0, sizeof(iface));
    if(!(ifc->plugins = (chip_plugins *)malloc(sizeof(chip_plugins)))){
	free( ifc );
	return NULL;
    }
    memset(ifc->plugins, 0, sizeof(chip_plugins));
    ifc->hwd = dummy_hardware_driver;
    chip_init_qe(ifc->plugins);
    iface_device_init( &ifc->dev, st);
    return ifc;
}

void iface_rmv_prg(iface *ifc)
{
    iface_prg *ti;

    while(ifc->prg){
	ti = ifc->prg->next;
	free(ifc->prg);
	ifc->prg =ti;
    }
}

/* dopisać detekcję nazw, aby sie nie powtarzały */
int iface_prg_add(iface *ifc, iface_prg_api api, char on)
{
    iface_prg *new_tie, *tmp;
    char *d_name = NULL;
    if(!api) return -1;

    api(ifc->gep, HW_NAME, 0, &d_name);
    if(!d_name) {
	printf("{iface.h} iface_add() -> driver rejected due to missing name !\n");
	return -1; /* brak nazwy */
    }

    if(!(new_tie = (iface_prg*) malloc(sizeof(iface_prg)))){
	printf("{iface.h} iface_add() -> memory allocation error (1) \n");
	return -1;
    }

    new_tie->next = NULL;
    api(ifc->gep, HW_NAME, 0, &new_tie->name);
    new_tie->api = api;
    new_tie->on = on;

    printf(" *  Adding driver: %s\n", new_tie->name);

    if(!ifc->prg){
	ifc->prg = new_tie;
	return 0;
    }

    for(tmp = ifc->prg; tmp->next; tmp = tmp->next);
    tmp->next = new_tie;

    return 0;
}

void iface_list_prg(iface *ifc, iface_prg_func fc, void *ptr)
{
    iface_prg *tmp;
    
    for(tmp = ifc->prg; tmp; tmp = tmp->next) 
		    if(fc) fc(ifc, tmp->name, ptr);

}

iface_prg_api iface_get_func(iface *ifc, char *name)
{
    iface_prg *tmp;
    
    for(tmp = ifc->prg; tmp; tmp = tmp->next) 
		    if(!strcmp(tmp->name, name)) return tmp->api;
    
    return NULL;
}

/*****************************************************************************************/

int iface_add_driver(iface *ifc, void *phandler, iface_regf fc)
{
    iface_driver *new_tie, *tmp;

    if(!(phandler && fc)) return -1;

    if(!(new_tie = (iface_driver*) malloc(sizeof(iface_driver)))){
	printf("{iface.h} iface_add_driver() -> memory allocation error (1) \n");
	return -1;
    }

    new_tie->next = NULL;
    new_tie->phandler = phandler;
    new_tie->fc = fc;

    if(!ifc->plg){
	ifc->plg = new_tie;
	return 0;
    }

    for(tmp = ifc->plg; tmp->next; tmp = tmp->next);
    tmp->next = new_tie;

    return 0;
}

void iface_rmv_driver(iface *ifc)
{
    iface_driver *tmp;

    while(ifc->plg){
	tmp = ifc->plg->next;
	dlclose(ifc->plg->phandler);
	free(ifc->plg);
	ifc->plg =tmp;
    }
}

int iface_dir_fltr(iface *ifc, const char *lst, const char *path, const char *ext, iface_cb_fltr cb)
{
    char *old_path, *tmp, *tx, *n;
    DIR *dir;
    struct dirent *dr;
    char ex;
    int i;

    for(i = 1, old_path = (char *)malloc(16 * sizeof(char)); !(getcwd(old_path, i * 16)); ){
	if(errno != ERANGE){
	    printf("{iface.h} iface_dir_fltr() -> error getting cwd, errno=%i\n", errno);
	    free(old_path);
	    return -1;
	}
	i++;
	old_path = (char *)realloc(old_path, i * 16 * sizeof(char));
    }

    if(chdir(path) != 0){
	printf("{iface.h} iface_dir_fltr() -> chdir('%s') error 1 \n", path);
	free(old_path);
	return -1;
    }        
    if(!(dir = opendir("./"))){
	printf("{iface.h} iface_dir_fltr() -> error opening current directory \n");
	free(old_path);
	return -1;
    }
    
    while((dr = readdir(dir))){
	for(tmp = dr->d_name + strlen(dr->d_name); tmp != dr->d_name && *tmp != '.'; tmp--);
	if( strcmp(tmp, ext) ) continue;

	for(n = (char *)lst; *n; ){
	
	    for(ex = 0, tx = dr->d_name; *tx && (tx != tmp); tx++, n++ ) 
						    if(*tx != *n){ ex = 1; break; };

	    if(!ex)
	    	if(cb(ifc, path, dr->d_name, old_path) == -1){
	    	    free(old_path);
	    	    return -1;
	    	}
	    
	    for(; *n && (*n != ':'); n++); /* dojechanie do końca nazwy, która nie jest zgodna */
	    if(*n == ':') n++;
	}		
    }

    closedir(dir);
    if(chdir(old_path) != 0){
	printf("{iface.h} iface_dir_fltr() -> chdir('%s') error 2 \n", old_path);
	free(old_path);
	return -1;
    }        

    free(old_path);
    return 0;
} 

static int iface_add_plug_file(iface *ifc, const char *pth, const char *name, const char *cwd)
{
    void *pf;
    iface_regf init;
    char tx[256];
    char *tmp = tx, *path = (char*)pth;
    int len, z, n=0;

    if(*path != '/' ){ /* nie jest ścieżką absolutną */
	path = (char*)malloc(sizeof(char) * (strlen(pth) + strlen(cwd) + 2));
	sprintf(path, "%s/%s", cwd, pth);
	n = 1;
    }

    len = strlen(path) + strlen(name) + 2;
    if( len > 256) 
	tmp = (char *) malloc(len * sizeof(char));
    else
	len = 0;    

    /* jesli brakuje '/' na końcu ścieżki to dodaj '/' */
    strcpy(tmp, path);
    z = strlen(path) - 1;
    if(z < 0) z = 0;
    if( *(tmp + z ) != '/') strcat(tmp, "/");
    strcat(tmp, name);
    printf("[MSG] Adding driver file '%s' ... ", name);
    /* czy ścieżka absoltna, jeśli nie to dodaj cwd */
    if(n) free(path);

    // otwarcie pluginu
    dlerror(); // wyzerowanie błędów 
    if(!(pf = dlopen(tmp, RTLD_LAZY))){
	printf("Error: dlopen() --> %s\n", dlerror());
	if(len) free(tmp);
	return -2;
    }
    if(len) free(tmp); // ściezka do pliku jest już niepotrzebna 

    if(!(init = (iface_regf)dlsym(pf, IFACE_DRIVER_INIT_FUNC_NAME))){
	printf("Error: dlsym() --> %s\n", dlerror());
	dlclose(pf);
	return -2;
    }
    printf("OK\n *Init drivers:\n");
    // wywołanie funkcji rejestrującej plugin 
  if(init(ifc)){
	printf("Error: " IFACE_DRIVER_INIT_FUNC_NAME "()\n");
	dlclose(pf);
	return -2;
    }

    // dodanie pluginu do kolejki
    if(iface_add_driver(ifc, pf, init)){
	printf("Error: " IFACE_DRIVER_INIT_FUNC_NAME "()\n");
	dlclose(pf);
	return -2;
    };

    return 0;
}

int iface_make_driver_list(iface *ifc, const char *path, const char *ext)
{
    return iface_dir_fltr(ifc, ifc->plg_list, path, ext, iface_add_plug_file);
}

void iface_rmv_modules(iface *ifc)
{
    modules *m;
    mod_list *tmp;

    if( !ifc ) return;
    if( !ifc->plugins ) return;
    if( !ifc->plugins->mdl ) return;

    m = ifc->plugins->mdl;
    tmp = m->first_modl;
    while( tmp ){
	m->modl = (mod_list *)tmp->next;
	if( tmp->handler ) dlclose(tmp->handler);	
	free( tmp );
	tmp = m->modl;
    }
    m->modl = NULL;
    m->first_modl = NULL;
}

void iface_destroy(iface *ifc)
{
    if(!ifc) return;
    iface_rmv_prg(ifc);
    iface_rmv_driver(ifc);
    if(ifc->plugins){
	if(ifc->plugins->mdl) iface_rmv_modules(ifc);
	chip_rmv_qe(ifc->plugins);
        free(ifc->plugins);
    }
    iface_device_destroy( ifc->dev );
    free(ifc);
}

void iface_driver_allow(iface *ifc, const char *list)
{
    ifc->plg_list = (char*)list;
}

/******************************************************************************************************************/

void iface_module_allow(iface *ifc, const char *list)
{
    ifc->mod_list = (char*)list;
}

/* kod poniżej do poprawy */
static int iface_add_module(iface *ifc, void *pf, void *init)
{
    static char first_run=1;
    mod_list *tmp;

    if(!(tmp = (mod_list *) malloc(sizeof(mod_list)))){
	printf("{iface.c} iface_add_module() --> memory allocation error !\n");
	return -2;
    }
    tmp->handler = pf;
    tmp->init_module = (int (*)(chip_plugins*))init;
    tmp->next = NULL;
    if(first_run){
	first_run = 0;
	ifc->plugins->mdl->modl = ifc->plugins->mdl->first_modl = tmp;
	return 0;
    }
    
    for(ifc->plugins->mdl->modl = ifc->plugins->mdl->first_modl; 
	ifc->plugins->mdl->modl->next; ifc->plugins->mdl->modl = (mod_list *)ifc->plugins->mdl->modl->next);

    ifc->plugins->mdl->modl->next = tmp;
    return 0;
}

static int iface_add_mod_file(iface *ifc, const char *pth, const char *name, const char *cwd)
{
    void *pf;
    iface_regf init;
    char tx[256];
    char *tmp = tx, *path = (char*)pth;
    int len, z, n=0;

    if(*path != '/' ){ /* nie jest ścieżką absolutną */
	path = (char*)malloc(sizeof(char) * (strlen(pth) + strlen(cwd) + 2));
	sprintf(path, "%s/%s", cwd, pth);
	n = 1;
    }

    len = strlen(path) + strlen(name) + 2;
    if( len > 256) 
	tmp = (char *) malloc(len * sizeof(char));
    else
	len = 0;    

    /* jesli brakuje '/' na końcu ścieżki to dodaj '/' */
    strcpy(tmp, path);
    z = strlen(path) - 1;
    if(z < 0) z = 0;
    if( *(tmp + z ) != '/') strcat(tmp, "/");
    strcat(tmp, name);
    printf("[MSG] Adding module file %s --> ", name);

    /* czy ścieżka absoltna, jeśli nie to dodaj cwd */
    if(n) free(path);

    /* otwarcie pluginu */
    dlerror(); /* wyzerowanie błędów */
    if(!(pf = dlopen(tmp, RTLD_LAZY))){
	printf("Error: dlopen() --> %s\n", dlerror());
	if(len) free(tmp);
	return -2;
    }
    if(len) free(tmp); /* ściezka do pliku jest już niepotrzebna */

    if(!(init = (iface_regf)dlsym(pf, IFACE_MODULE_INIT_FUNC_NAME))){
	printf("Error: dlsym() --> %s\n", dlerror());
	dlclose(pf);
	return -2;
    }


    /* wywołanie funkcji rejestrującej plugin */
    if(init(ifc->gep)){
	printf("Error: " IFACE_MODULE_INIT_FUNC_NAME "()\n");
	dlclose(pf);
	return -2;
    }

    /* dodanie pluginu do kolejki */
    if(iface_add_module(ifc, pf, (void *)init)){
	printf("Error: " IFACE_MODULE_INIT_FUNC_NAME "()\n");
	dlclose(pf);
	return -2;
    };

    return 0;
}

void iface_make_modules_list(iface *ifc, const char *path, const char *ext)
{
    iface_dir_fltr(ifc, ifc->mod_list, path, ext, iface_add_mod_file);
}

void iface_renew(iface *ifc)
{
    geepro *gep;
    char *tmp = NULL;
        
    if( ifc->hwd && ifc->gep){
	gep = GEEPRO(ifc->gep);
	hw_get_name( tmp );
        iface_device_set_programmer( ifc->dev, hw_get_iface(), tmp);
    }
}

/*******************************************************************************************************************************************************/
/* NEW API */

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
    MSG("Parallel ports:");
    parport_get_list( ifc->lpt, PARPORT_CALLBACK(iface_device_parport_set_alias), &tmp, PARPORT_FILTER_NOALIAS );
}

static void iface_device_parport_add(s_parport *pp, s_parport_list *it, s_iface_device *ifc )
{
    iface_device_add_list(ifc, it->alias, IFACE_LPT, IFACE_LPT, it);	
}

static void iface_device_scan_parport( s_iface_device *ifc )
{
    parport_get_list( ifc->lpt, PARPORT_CALLBACK(iface_device_parport_add), ifc, PARPORT_FILTER_ALIAS );
}

/*
static void iface_device_config_com(s_iface_device *ifc, s_cfp *cfg){}
static void iface_device_scan_com( s_iface_device *ifc ){}
*/

/******************************************************************/

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
//    serial_init( &((*ifc)->com) );
    // USB devices
    gusb_init( &((*ifc)->usb) );
    return 0;
}

void iface_device_destroy( s_iface_device *ifc )
{
    iface_device_delete_list( ifc );
    parport_exit( ifc->lpt );
//    serial_exit( ifc->com );
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
//    iface_device_scan_serial( ifc );
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

void iface_prog_select_store(iface *ifc) // improvisation
{
    char *tmp = NULL;
    ifc->prog_sel = 0;
    if( GEEPRO(ifc->gep)->store ){
	if(store_get(  GEEPRO(ifc->gep)->store, IFACE_LAST_SELECTED_PRG_KEY, &tmp) == 0){
	    if( tmp ){
		ifc->prog_sel = strtol(tmp, NULL, 0);    		
		free( tmp );
	    }
	}
    }
}

void iface_device_configure( s_iface_device *ifc, s_cfp *cfg)
{
    iface_device_config_parport( ifc, cfg);
    iface_device_scan_parport( ifc ); // to add interfaces to list
//    iface_device_config_serial( ifc, cfg);
//    iface_device_scan_serial( ifc, cfg);
    iface_device_config_usb( ifc, cfg);
    gusb_set_callback( ifc->usb, GUSB_CALLBACK( ifc_device_usb_callback ), ifc);
    gusb_scan_connected( ifc->usb );
//    iface_device_select_stored( ifc );
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

