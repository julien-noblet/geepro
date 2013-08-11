/* $Revision: 1.5 $ */
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

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#include "src/config.h"
#include "buffer.h"
#include "../gui-gtk/gui.h"
#include "main.h"
#include "chip.h"
#include "../intl/lang.h"
#include "../drivers/hwdriver.h"
#include "geepro.h"
#include "storings.h"
#include "error.h"

#define CONFIG_FILE_PATH_LIST	"./cfg/geepro.cfg:~/.geepro/geepro.cfg:/usr/share/geepro/geepro.cfg"
#define CONFIG_FILE_ENV_VAR     "GEEPRO_CFG_PATH"

#ifndef DEFAULT_DRIVERS_PATH
#define DEFAULT_DRIVERS_PATH	"./drivers"
#endif
#ifndef DEFAULT_CHIPS_PATH
#define DEFAULT_CHIPS_PATH	"./chips"
#endif
#ifndef DEFAULT_SHARE_PATH
#define DEFAULT_SHARE_PATH	"./"
#endif

}

static geepro *__geepro_root__ = NULL; // data structure for system signals handling, local for this file

static char load_cfg(geepro *gep, const char *path)
{
    char err;
    if(path == NULL ) return -1;
    if( !gep || !path ) return -1;
    cmt_error(TR("[MSG] Loading config file '%s' ... "), path);
    err = cfp_load( gep->cfg, path );
    if( err )
	cmt_error(TR("FAIL\n"));
    return err;
}

static void load_cfg_file(geepro *gep)
{
    char *cfg_file, err;

    err = 0;
    cfg_file = getenv( CONFIG_FILE_ENV_VAR );
    if( cfg_file )
	err = load_cfg( gep, cfg_file );

    if( !cfg_file || err ){
	cfg_file = cfp_path_selector( CONFIG_FILE_PATH_LIST, F_OK);
	if( cfg_file ){
	    err = load_cfg( gep, cfg_file );
	    free( cfg_file );
	}
    }
    if( !err )
	cmt_error(TR("OK\n"));	
//    else 
//	create_default_cfg_file();
    
}

static char set_path_var(geepro *gep, const char *var_name, const char *default_name, const char *default_val)
{
    char *ptr, *tmp;
    ptr = NULL;
    if( !gep->cfg ) return 1;
    if( cfp_get_string(gep->cfg, var_name, &ptr) ){
	if( default_val )
	    cmt_error(TR("[WRN] No '%s' declared in config file, using default.\n"), var_name);
	else {
	    cmt_error(TR("[ERR] No '%s' declared in config file.\n"), var_name);
	    return 1;
	}	
    };
    if( default_val ) cfp_heap_set(gep->cfg, default_name, default_val);
    if( ptr ){
	if((tmp = cfp_path_selector( ptr, F_OK))) 
	    cfp_heap_set(gep->cfg, var_name + 1, tmp);
	else {
	    cmt_error(TR("[ERR] Can not find any valid path from list:'%s'.\n"), ptr);
	    free( ptr );
	    return 1;
	}
	free( ptr );
	free( tmp );
    }
    return 0;
}

static char set_opt_var(geepro *gep, const char *path, const char *val_default)
{
    char *tmp = NULL;
    char err = 0;

    if( !path || !val_default || !gep ) return 1;
    if(cfp_get_string(gep->cfg, path, &tmp)){
	cmt_error(TR("[WRN] Missing '%s' parameter in config file. Using default.\n"), path + 1);
	err = cfp_heap_set(gep->cfg, path + 1, val_default); // +1 because of '/'
    } else {
	err = cfp_heap_set(gep->cfg, path + 1, tmp);
	free( tmp );    
    }    
    return err;
}

static inline char load_variables(geepro *gep)
{
    // Looking for prefix location of data files
    if( set_path_var(gep, "/drivers_path", "DEFAULT_DRIVERS_PATH", DEFAULT_DRIVERS_PATH) ) return -1;
    if( set_path_var(gep, "/chips_path", "DEFAULT_CHIPS_PATH", DEFAULT_CHIPS_PATH) ) return -1;
    if( set_path_var(gep, "/shared_path", "DEFAULT_SHARE_PATH", DEFAULT_SHARE_PATH) ) return -1;

    if( cfp_heap_set(gep->cfg, "shared_geepro_directory", "$shared_path/") ) return -1;

    if( set_opt_var(gep, "/driver", "willem") ) return -1;
    if( set_opt_var(gep, "/store_vars_dir","~/.geepro") ) return -1;
    if( set_opt_var(gep, "/store_vars_file","geepro.st") ) return -1;
    if( set_opt_var(gep, "/drivers","willem") ) return -1;
    if( set_opt_var(gep, "/chips","27xx") ) return -1;
    if( set_opt_var(gep, "/shared_drv_xml_path","./") ) return -1;

    // unfold stored variables
    cfp_heap_unfold( gep->cfg );
    gep->shared_drivers_xml_file = cfp_heap_get(gep->cfg, "shared_drv_xml_path");
    gep->shared_geepro_dir = cfp_heap_get(gep->cfg, "shared_geepro_directory");
    return 0;
}	

template<class T> T *geep_alloc()
{
    T *tmp;
    if(!(tmp = (T *)malloc( sizeof( T ) ))){
	cmt_error(TR( MALLOC_ERR ));
	return NULL;
    }
    memset(tmp, 0, sizeof( T ));    
    return tmp;
}

static void geep_free(geepro *g)
{
    if( !g ) return;
    if( g->gui ) free( g->gui );
    if( g->store ) free( g->store );
    free( g );
}

static inline geepro *geep_init(int argc, char **argv)
{
    geepro *g;

    if( !(g = geep_alloc< geepro >())) return NULL;
    if( !(g->gui = geep_alloc<gui>())){
	geep_free( g );
	return NULL;
    }
    if( !(g->store = geep_alloc< store_str >())){
	geep_free( g );
	return NULL;
    }
    g->argc = argc;
    g->argv = argv;
    return g;    
}

static void destruct(geepro *geep)
{
    // Destruct
    if( geep->ifc ) iface_destroy(geep->ifc);
    if(geep->cfg) cfp_free( geep->cfg );
    if(geep->store) store_destr( geep->store );

    if( geep ) geep_free( geep );
}

static void kill_me(int signal)
{
//__geepro_root__
    printf("SIG INT -> KILL\n");
}

int main(int argc, char **argv)
{
    geepro *geep = NULL;

    // Init
    __geepro_root__ = geep;
    if(!(geep = geep_init( argc, argv ))) return -1;
    if(!(geep->cfg = cfp_init())){
	destruct( geep );
	return -2;
    };
    geep->uid = getuid();
    load_cfg_file( geep );
    if(load_variables( geep )){
	destruct( geep );
	return -3;
    }
    if(store_constr(geep->store, cfp_heap_get(geep->cfg, "store_vars_dir"), cfp_heap_get(geep->cfg, "store_vars_file"))){
	destruct( geep );
	return -4;
    };
    if(( geep->ifc = iface_init() )){
	geep->ifc->gep = geep;
	iface_driver_allow(geep->ifc, cfp_heap_get(geep->cfg, "drivers"));
	iface_module_allow(geep->ifc, cfp_heap_get(geep->cfg, "chips"));
	iface_load_config(geep->ifc, NULL);
	iface_make_driver_list(geep->ifc, cfp_heap_get(geep->cfg, "drivers_path"), ".driver");
	iface_make_modules_list( geep->ifc, cfp_heap_get(geep->cfg, "chips_path"), ".chip"); 
    }
    signal(SIGINT, kill_me);
    gui_run( geep );
    destruct( geep );
    return 0;
}

