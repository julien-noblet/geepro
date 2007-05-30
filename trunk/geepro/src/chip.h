/* $Revision: 1.2 $ */
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

#ifndef __chip_h__
#define __chip_h__

#ifdef DEBUG_MODE
    #define MSG_2DEBUG(a,b)		printf(a,b)
    #define MSG_1DEBUG(a)		printf(a)
#else
    #define MSG_2DEBUG(a,b)		
    #define MSG_1DEBUG(a)		
#endif

#define CHIP_ERROR		-1

#define ACTION_READ		1
#define ACTION_READ_SIG		2
#define ACTION_WRITE		3
#define ACTION_ERASE		4
#define ACTION_LOCK		5
#define ACTION_UNLOCK		6
#define ACTION_VERIFY		7
#define ACTION_TEST		8
#define ACTION_BLANK_TEST	9


#define chip struct __chip__

typedef struct chip_plugins chip_plugins;

typedef struct{
//    char *mod_name;
    void *handler;
    void *next;
    int (*init_module)(chip_plugins *plg);
} mod_list;

typedef struct{
    mod_list *modl;
    mod_list *first_modl;    
} modules;

typedef struct 
{
    char *name;
    void *wg;
    void *next;
} chip_menu_qe;

struct chip_plugins
{
    chip_menu_qe *menu_qe;  /* wejście do kolejki menu */
    chip *menu_sel;  /* uzywane przez gui, funkcja: device_sel() */
    chip *chip_qe;
    chip *chip_sel;
    modules *mdl;
};

struct __chip__{
    char *chip_path;		/* chip path format (eg "/27xx"), visible later in menu */
    char *chip_name;		/* chip name eg 2716 */
    char *chip_family;
    int  chip_id;		/* ID chip in family */
    int	 dip_switch;		/* dip switch settings for PCB3 - 29xx bit + 12bit dip switch       */
    void *image_willem;		/* pointer to xpm image for willem - the picture at the right 	     */
    void *image_pcb;    	/* as above, but for PCB3 					     */
    int  dev_size;		/* allocation size for buffer					     */
    int  chip_offset;
    char *buffer;
    int  checksum;
    int	 img_will_idx;
    int	 img_pcb3_idx;    
    char vpp;			 /* programing voltage - if fixed, otherway -1			     */
    char vcc;			 /* supply voltage, comments as above				     */
    char vcp;			 /* supply voltage during programming, comments as above		     */
    int  prog_time;		 /* max programing time pulse [us]	*/
    void  **option;		 /* miscalenous option that could be set by user */
    int (*autostart)(void *, void*);	 /* invocated just after module selection */
    int (*read_chip)(void *, void*);    /* read chip content to buffer */
    int (*read_sig_chip)(void *, void*);/* read chip signature (if can't NULL) */
    int (*write_chip)(void *, void*);   /* write content of buffer to chip */
    int (*erase_chip)(void *, void*);	 /* erase chip (EEPROM) */
    int (*lock_chip)(void *, void*);	 /* set lock to avoid later reading content */
    int (*unlock_chip)(void *, void*);  /* unlock locked chip - ok it's just program possibility :) */
    int (*verify_chip)(void *, void*);  /* compare content of chip with content of buffer */
    int (*test_chip)(void *, void*);	 /* Is the chip OK ? */
    chip  *next;		 	 /* pointer to next chip structure */
};

/* akcje jakie mogą być wykonane na ukladzie */
enum
{
    CHIP_READ = 0,
    CHIP_WRITE,
    CHIP_VERIFY,
    CHIP_SIGNATURE,
    CHIP_TESTBLANK,
    CHIP_ERASE,
    CHIP_LOCK,
    CHIP_UNLOCK,
};

/* inicjowanie i usuwanie kolejek */
extern void chip_init_qe(chip_plugins *plg);
extern void chip_rmv_qe(chip_plugins *plg);

/* rejestrowanie/ wyrejestrowywanie ukladów */
extern int chip_register_chip(chip_plugins *plg, chip *new_chip);
extern int chip_unregister_chip(chip_plugins *plg, char *name);
extern chip *chip_lookup_chip(chip_plugins *plg, char *name);
extern void chip_destroy(chip_plugins *plg);

/* pobieranie i ustawianie bieżącego układu */
extern int chip_invoke_action(chip_plugins *plg, int action);
extern chip *chip_get_chip(chip_plugins *plg);

/* struktura menu układów */
extern void chip_rm_path(chip_plugins *plg);
extern int chip_add_path(chip_plugins *plg, char *path, void *wg);
extern void *chip_find_path(chip_plugins *plg, char *path);

/* operacje tekstowe */
extern char chip_cmp(char *name1, char *name2);
extern char *chip_last_pth(char *pth);

extern void chip_menu_create(chip_plugins *plg, void *wg, void *(*submenu)(void *, char *, void *), 
								    void (*item)(chip_plugins *, void *, void*), void*);

#endif


