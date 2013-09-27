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
#include "buffer.h"
#include "chip.h"
#include "../drivers/hwdriver.h"
#include "gep_usb.h"
#include "cfp.h"
#include "parport.h"
#include "serial.h"
#include "storings.h"

#define IFACE_DRIVER_INIT_FUNC_NAME	"driver_init"
#define IFACE_MODULE_INIT_FUNC_NAME	"init_module"
#define DRIVER_FILE_EXTENSION		"driver"
#define CHIP_PLUGIN_FILE_EXTENSION	"chip"

// variable name in program storings
#define IFACE_LAST_SELECTED_IFC_KEY	"LAST_SELECTED_INTERFACE" 
#define IFACE_LAST_SELECTED_PRG_KEY	"LAST_SELECTED_PROGRAMMER" 
#define IFACE_LAST_SELECTED_CHIP_KEY	"LAST_SELECTED_CHIP"

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

#define IFACE_F_DEVICE_NOTIFY( x ) ((f_iface_device_notify)x)
#define IFACE_F_DEVICE( x )	((f_iface_device)x)
#define IFACE_CHIP( x )		((s_iface_chip *)x)
#define F_IFACE_CHIP( x )	((f_iface_chip)x)
#define F_IFACE_ACTION( x ) 	((f_iface_action)x)

typedef struct iface_ iface;
typedef struct iface_qe iface_qe;
typedef struct iface_prg iface_prg;
typedef struct iface_driver iface_driver;
typedef struct s_iface_device_ s_iface_device;
typedef struct s_iface_chip_action_ s_iface_chip_action;
typedef struct s_iface_desc_buffer_ s_iface_desc_buffer;

typedef struct s_iface_driver_list_ s_iface_driver_list;
typedef struct s_iface_devlist_ s_iface_devlist;
typedef struct s_iface_calls_list_ s_iface_calls_list;
typedef struct s_iface_driver_ s_iface_driver;
typedef struct s_iface_chip_list_ s_iface_chip_list;
typedef struct s_iface_chip_ s_iface_chip;

//typedef void (*iface_cb)(iface *, int cl, char *name, char *dev, void *ptr);
//typedef int  (*iface_cb_fltr)(iface *, const char *path, const char *name, const char *cwd);

typedef void (*iface_prg_func)(iface *, char *name, void *ptr);
typedef void (*f_iface_device_notify)(s_iface_device *, s_iface_devlist *, void *);
typedef int  (*iface_prg_api)(void *, en_hw_api func, int arg, void *ptr);

typedef void (*f_iface_device)(s_iface_device *, s_iface_devlist *, void *, int iter);
typedef void (*f_iface_driver_callback)(s_iface_driver *, s_iface_driver_list *, void *);
typedef void (*f_iface_chip)(s_iface_chip *, s_iface_chip_list *, void *);
typedef void (*f_iface_action)(s_iface_chip *, s_iface_chip_action *, void *);
typedef int (*f_iface_chip_action)(void *);
/*
    Interface to device
*/

struct s_iface_devlist_
{
    void *handler;	// pointer to interface data stucture
    char *name;		// display name
    int  cl;		// interface real class LPT,USB etc
    int  group;		// interface group class LPT, USB, etc (whitch driver should handle it: example - LPT driver can using USB emulation )
    s_iface_devlist *next;
};

struct s_iface_device_
{
    s_iface_devlist	*selected;	// currently selected device
    s_iface_devlist	*list;		// actual list of all supported available devices by given programmer
    s_parport		*lpt;		// all lpt ports available in system
    s_serial 		*com;		// all RS232 ports available in system
    s_usb		*usb;		// all USB devices supported by geepro
    f_iface_device_notify notify;	// notify callback
    void		*notify_ptr;	// notify callback parameter
    store_str 		*store;
    const char		*prog_name;	// programmer driver name
    int			prog_class;	// programmer driver interface class
};

/*
    Driver
*/

struct s_iface_driver_list_ 
{
    char *driver_path;	// driver file path
    char *name;		// driver name
    char *xml;		// full path to xml configuration file
    char *chips;	// supported chip plugins
    char iface_class;	// driver interface class IFACE_LPT | IFACE_RS232 | IFACE_USB
    hw_driver_type api; // callback to driver
    long flags;         // 1 - enabled, 0 - disabled
    s_iface_driver *parent;
    s_iface_driver_list *next; // next link
};
/*
struct s_iface_calls_list_
{
    int id;			// function identifier
    char *func_name;		// function name
    char *description;
    char *format;		// parameter format
    s_iface_calls_list *next;	// next link
};
*/
struct  s_iface_driver_
{    
    char *xml_file_path;		// path to xml files
    char *drv_file_path;		// path to driver plugins files
    void *dl_handle;			// handle to selected driver plugin
    store_str *store;
    s_iface_driver_list *selected;	// currently selected driver
    s_iface_driver_list *list;    	// list of all available drivers
//    s_iface_calls_list  *calls;	// function calls list of driver
};

/*
    Chip
*/

struct s_iface_chip_action_
{
    const char *name;			// Action name
    const char *tip;			// Action tip help
    f_iface_chip_action action;		// callback
    s_iface_chip_action *next;		// next link
};

struct s_iface_desc_buffer_
{
    char *name;				// buffer name
    unsigned int size;			// buffer size in bytes
    unsigned int offset;
    s_iface_desc_buffer *next;		// next link
};

struct s_iface_chip_list_
{
    const char	*name;			// chip name
    const char 	*family;		// chip family
    const char 	*path;			// menu path
    s_iface_desc_buffer *desc_buffer;	// memory buffers description
    void	*handler;		// plugin handler
    s_iface_chip_action *action;	// actions queue
    f_iface_chip_action autostart;	// autostart function
    s_iface_chip_list *next;
};

struct s_iface_chip_ 
{
    void		*relay;		// temporary variable for relay dl_handler
    char		*allowable;	// list of allowable plugins, separated by colon
    s_cfp		*cfg;		// configuration file
    store_str		*store;		// storings 
    s_iface_chip_list	*selected;	// selected and supported by programmer chip from list
    s_buffer		*buffer;	// selected chip buffer 
    s_iface_chip_list	*list;		// list of all chips
};

/*
    Main structure
*/

struct iface_
{	
    s_iface_chip   *chp;	// list of available chip procedures
    s_iface_driver *drv;	// list of available programmer drivers
    s_iface_device *dev;	// list of interfaces and all connected devices
};

/* ============================================> FUNCTIONS <============================================ */

/* wybór drivera programatora - do usuniecia*/
extern void iface_list_prg(iface *ifc, iface_prg_func , void *ptr);
extern iface_prg_api iface_get_func(iface *ifc, char *name);


/*
	Load/Unload drivers like willem, galblaster, etc    
*/
extern iface *iface_init(store_str *st, s_cfp *); /* inicjuje kolejkę interfejsów */
extern void iface_destroy(iface *ifc); /* zwalnia pamięć przydzieloną przez ifc */ 

char iface_pgm_select(iface *, const char *pgm_name); 

void iface_renew(iface *);

const char *iface_get_chip_name(const iface *);   // return selected chip name or NULL
const char *iface_get_chip_family(const iface *); // return selected chip family or NULL
const char *iface_get_chip_path(const iface *); // return selected chip path or NULL
const s_buffer *iface_get_chip_buffer(const iface *); // return selected chip buffer or NULL

const s_iface_chip_list *iface_get_selected_chip(const iface *); // return selected chip or NULL
void iface_unselect_chip(const iface *);

extern int  iface_driver_add(s_iface_driver_list *ifc, iface_prg_api, char on); /* dodanie programatora do kolejki */

/*
    Constructor
      Input:
	ifc - pointer on s_iface_driver pointer structure
	str - pointer to storings 
      Return:
        0 - success
        < 0 - error
*/
//char iface_driver_init(s_iface_driver **ifc, store_str *str, s_cfp *cfg);

/*
    Destroy iface_driver. Frees all allocated resources.
    Input:
	ifc - s_iface_driver structure
    Return:
	None
*/
//void iface_driver_exit( s_iface_driver *ifc);

/*
    Rescan all driver plugins. Renew list.
*/
void iface_driver_scan(s_iface_driver *ifc);

/*
    Invokes callback for each element of driver list.
    Input:
	ifc - s_iface_driver structure    
	callback - callback function
	ptr - user pointer for callback
    Return:
	None
*/
void iface_driver_get_list(s_iface_driver *ifc, f_iface_driver_callback callback, void *ptr);

/*
    Select driver from list.    
	Input:
	ifc - s_iface_driver structure    	    
	driver_name - name of driver
    Return:
	0 - if driver found, otherwise < 0
*/
char iface_driver_select(s_iface_driver *ifc, const char *driver_name);

/*
    Return selected driver callback.
    Input:
	ifc - s_iface_driver structure    	    	
    Return:
	Success: driver's api callback
	Fail   : internal error func
*/
hw_driver_type iface_driver_call(s_iface_driver *ifc);

/*
    
*/
//char iface_driver_configure(s_iface_driver *ifc, s_cfp *cfg);

/******************************************************************
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
//char iface_device_init( s_iface_device **ifc, store_str *store);

/*
    Destructor
*/
//void iface_device_destroy( s_iface_device *ifc );

/*
    Load configuration of the devices 
*/
//void iface_device_configure( s_iface_device *ifc, s_cfp *cfg);

/*
    Checks hotpluuged USB devices. If new device appears or disappears
    actualize list of them (invokes also f_iface_notify if defined).
    New device is tested for compatibility with choosed programmer.
    Function should be called periodicaly. It is non blocking function.
    Input:
	ifc - pointer to s_iface_device structure    
    Return:
	None
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
//const char *iface_device_get_key_stored(s_iface_device *ifc);


//void iface_prog_select_store( iface *);

const char *iface_get_xml_path( iface *);

/************************************************************************************************************************************************/
/*
    Create chip structure.
    Input:
	chip - pointer to pointer of chip structure - pointer structure have to be initialized as NULL
	s - storings structure
	cfg - cfp structure to config file
    Return:
	0 - success
	less than zero - error
*/
char iface_chip_init( s_iface_chip **chip, store_str *s, s_cfp *cfg );

/*
    Free all resources alocated and stored in chip structure
    Input:
	chip - pointer to s_iface_chip structure
    Return:
	None
*/
void iface_chip_exit( s_iface_chip *chip );

/*
    Register chip. cd is internaly copied
    Input:
	chip - pointer to s_iface_chip structure
	cd   - chip descriptor
    Return:
	None
*/
void iface_chip_register(s_iface_chip *chip, s_iface_chip_list *cd);

/*
    Select chip from list.
    Input:
	chip - pointer to s_iface_chip structure
	chip_name - chip name to find on list
    Return:
	0 - success    
	less than 0 - error
*/
char iface_chip_select(s_iface_chip *chip, const char *chip_name);

/*
    List whole content of chip list
    Input:
	chip - pointer to s_iface_chip structure
	f    - callback 
	ptr  - parameter for callback
    Return:	
	None
*/
void iface_chip_get_list(s_iface_chip *chip, f_iface_chip f, void *ptr);

/*
    List all posible actions for selected chip
    Input:
	chip - pointer to s_iface_chip structure
	f    - callback 
	ptr  - parameter for callback
    Return:	
	None
*/
void iface_chip_get_actions(s_iface_chip *chip, f_iface_action f, void *ptr);

/******************************************************************************************************************************/

void iface_chip_list_init(s_iface_chip_list *cd, const char *path, const char *chip_name, const char *family);

/*
    Register callback action function
    Input:
	cd   - chip descriptor
	bt_name - button identifier
	tip  - tip text
	f    - callback for action	
    Return:
	None

*/
void iface_chip_list_add_action(s_iface_chip_list *cd, const char *bt_name, const char *tip, f_iface_chip_action f);

/*
    Register buffers for given chip.
    Input:
	cd   - chip descriptor
	buffer_name - name to identify buffer
	size - buffer bytes count    
*/
void iface_chip_register_buffer(s_iface_chip_list *cd, const char *buffer_name, unsigned int size, unsigned int offset);


#ifdef __cplusplus
} // extern "C"
#endif
#endif
 