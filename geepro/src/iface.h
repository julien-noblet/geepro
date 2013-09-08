/* $Revision: 1.1.1.1 $ */
/** iface.h Lista dostepnych interfejsów do wyboru dla sterownika programatora */
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

#ifndef __iface_h__
#define __iface_h__
#include "chip.h"
#include "../drivers/hwdriver.h"
#include "gep_usb.h"
#include "cfp.h"
#include "parport.h"
#include "storings.h"

#define IFACE_DRIVER_INIT_FUNC_NAME	"driver_init"
#define IFACE_MODULE_INIT_FUNC_NAME	"init_module"

// variable name in program storings
#define IFACE_LAST_SELECTED_IFC_KEY	"LAST_SELECTED_INTERFACE" 
#define IFACE_LAST_SELECTED_PRG_KEY	"LAST_SELECTED_PROGRAMMER" 

#ifdef __cplusplus
extern "C" {
#endif


typedef int (*iface_regf)(void *ifc);

/* Klasy interfejsów  */
enum{
    IFACE_DUMMY = 1,
    IFACE_RS232,
    IFACE_LPT,
    IFACE_USB,
    IFACE_FIREWIRE,
    IFACE_ETH,
    IFACE_PS2,
};

typedef int  (*iface_prg_api)(void *, en_hw_api func, int arg, void *ptr);
typedef struct iface_qe iface_qe;
typedef struct iface_prg iface_prg;
typedef struct iface_driver iface_driver;

struct iface_prg
{
    char *name;		/* nazwa sterownika programatora */
    iface_prg_api api;  /* wskaźnik na funkcję api  */
    char on;		/* właczony driver, czy nie [ pojedynczy plugin może zawierać wiele sterowników ]*/
    iface_prg *next;    
};

struct iface_driver
{
    void *phandler;	/* uchwyt pluginu */    
    iface_regf fc;      /* funkcja rejestrująca */
    iface_driver *next;
};


typedef struct s_iface_devlist_ s_iface_devlist;
struct s_iface_devlist_
{
    void *handler;	// pointer to interface data stucture
    char *name;		// display name
    int  cl;		// interface real class LPT,USB etc
    int  group;		// interface group class LPT, USB, etc (whitch driver should handle it: example - LPT driver can using USB emulation )
    s_iface_devlist *next;
};

typedef struct s_iface_device_ s_iface_device;

typedef void (*f_iface_device)(s_iface_device *, s_iface_devlist *, void *, int iter);
#define IFACE_F_DEVICE( x )	((f_iface_device)x)

typedef void (*f_iface_device_notify)(s_iface_device *, s_iface_devlist *, void *);
#define IFACE_F_DEVICE_NOTIFY( x ) ((f_iface_device_notify)x)
typedef struct iface_ iface;

struct s_iface_device_
{
    s_iface_devlist	*selected;	// currently selected device
    s_iface_devlist	*list;		// actual list of all supported available devices by given programmer
    s_parport		*lpt;		// all lpt ports available in system
//    s_serial *com;			// all RS232 ports available in system
    s_usb		*usb;		// all USB devices supported by geepro
    f_iface_device_notify notify;	// notify callback
    void		*notify_ptr;	// notify callback parameter
    store_str 		*store;
    const char		*prog_name;	// programmer driver name
    int			prog_class;	// programmer driver interface class
};

struct iface_
{
    char *plg_list;	/* lista plików pluginów, które mogą być załadowane */
    char *mod_list;	/* lista plików modułów, które mogą być załadowane */
    chip_plugins *plugins;
    iface_prg *prg;     /* lista driverów */
    iface_driver *plg;  /* lista plików pluginów do obsługi programatora */
/* wybrany programator: */
    int prog_sel;
    int cl;		/* klasa urządzenia */
    void *gep;		/* wskaźnik na główną strukturę danych */
    hw_driver_type hwd;
// new api
    s_iface_device *dev; // lists of interfaces and all connected devices
};


typedef void (*iface_cb)(iface *, int cl, char *name, char *dev, void *ptr);
typedef void (*iface_prg_func)(iface *, char *name, void *ptr);
typedef int  (*iface_cb_fltr)(iface *, const char *path, const char *name, const char *cwd);

/* ogólne */
extern iface *iface_init(store_str *st); /* inicjuje kolejkę interfejsów */
extern void iface_destroy(iface *ifc); /* zwalnia pamięć przydzieloną przez ifc */ 
extern int  iface_load_config(iface *ifc, void *);

/* wybór drivera programatora */
extern int  iface_prg_add(iface *ifc, iface_prg_api, char on); /* dodanie programatora do kolejki */
extern void iface_list_prg(iface *ifc, iface_prg_func , void *ptr);
extern iface_prg_api iface_get_func(iface *ifc, char *name);
extern void iface_rmv_prg(iface *ifc); /* usuwa wszystkie sterowniki z kolejki */

/* lista pluginów */
extern int  iface_make_driver_list(iface *ifc, const char *path, const char *ext); /* tworzy listę pluginów z danej lokalizacji */
extern int  iface_add_driver(iface *ifc, void *phandler, iface_regf); /* dodaje plugin */
extern void iface_rmv_driver(iface *ifc); /* usuwa wszystkie pluginy z kolejki */
extern int  iface_dir_fltr(iface *ifc, const char *lst, const char *path, const char *ext, iface_cb_fltr);
extern void iface_driver_allow(iface *ifc, const char *lst); /* lst jest postaci: "plugin1:plugin2:plugin3 .... "*/

/* lista modułów */
extern void iface_load_drivers(iface *ifc);
extern void iface_make_modules_list( iface *ifc, const char *path, const char *ext);
extern void iface_module_allow(iface *ifc, const char *lst); /* lst jest postaci: "plugin1:plugin2:plugin3 .... "*/
extern void iface_rmv_modules(iface *);

void iface_renew(iface *);

/****************************************************** NEW API ********************************************************************************/
/*
    Functions to manipulate interfaces like LPT, USB, RS232 etc.

*/

/*
    Actualize list of devices supported by choosed programmer.
    Removing old list and creating new one with actually available devices.
    Scans all type of devices lists.
    Added device have to match class type and driver name.
    Invokes notify callback if defined.
*/
char iface_device_rescan( s_iface_device *ifc );

/*
    Select device_name interface for I/O.    
    Return -1 if device_name not found, or 0 on success.
*/
char iface_device_select(s_iface_device *ifc, const char *device_name);

/*
    Invokes f_iface_device with parameter ptr for each device on list.
    Function used to build available devices list in GUI
*/
char iface_device_get_list( s_iface_device *ifc, f_iface_device, void *ptr );

/*
    Invokes f_iface_notify if list of devices changed.
    Invoked when available devices/ports list is changed.
*/
char iface_device_connect_notify( s_iface_device *ifc, f_iface_device_notify, void *ptr);

/*
    Constructor
    ifc -> pointer to s_iface_device struct pointer.
    return 0 on success
*/
char iface_device_init( s_iface_device **ifc, store_str *store);

/*
    Destructor
*/
void iface_device_destroy( s_iface_device *ifc );

/*
    Load configuration of the devices 
*/
void iface_device_configure( s_iface_device *ifc, s_cfp *cfg);

/*
    iface_device event - should be invoked periodicaly
*/
void iface_device_event( s_iface_device *ifc );

/*
    iface_device set programmer driver.
*/
void iface_device_set_programmer(s_iface_device *ifc, int device_class, const char *programmer_name);

/*
    Select interface from stored variable (last choosed interface).
    Input:
	ifc - pointer to s_iface_device structure
    Return:
	0  - success,
	-1 - no matched interface, select first matched from list
	-2 - no matched interface
	-3 - no stored entry
*/
char iface_device_select_stored(s_iface_device *ifc);

/*
    Return key name for storing 
*/
const char *iface_device_get_key_stored(s_iface_device *ifc);


void iface_prog_select_store( iface *);

#ifdef __cplusplus
} // extern "C"
#endif
#endif
 