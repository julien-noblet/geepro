/* $Revision: 1.2 $ */
/* parport - user space wrapper for LPT port using ppdev  v 0.0.3
 * Copyright (C) 2007 Krzysztof Komarnicki
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

#ifndef __parport_h__
#define __parport_h__

#ifdef __cplusplus
extern "C" {
#endif

#define PA	0	// to write only
#define PB	1	// to read only
#define PC	2	// to write only (read previously wrote data)
#define PP_ERROR	-1

/***************************************/
/*          Signal names               */
/***************************************/

// port PC 
#define PP_STB		(1  | (PC << 8))
#define PP_ALF		(2  | (PC << 8))
#define PP_INI		(4  | (PC << 8))
#define PP_DSL		(8  | (PC << 8))
#define PP_IRQ		(16 | (PC << 8))

// port PB 
#define PP_ERR		(8   | (PB << 8))
#define PP_ONOF		(16  | (PB << 8))
#define PP_PAP		(32  | (PB << 8))
#define PP_ACK		(64  | (PB << 8))
#define PP_BUSY		(128 | (PB << 8))

// port PA
#define PP_D0		(1   | (PA << 8))
#define PP_D1		(2   | (PA << 8))
#define PP_D2		(4   | (PA << 8))
#define PP_D3		(8   | (PA << 8))
#define PP_D4		(16  | (PA << 8))
#define PP_D5		(32  | (PA << 8))
#define PP_D6		(64  | (PA << 8))
#define PP_D7		(128 | (PA << 8))

// port pins definitions in DB25 socket
#define PP_01		PP_STB	/* PC */
#define PP_02		PP_D0	/* PA */
#define PP_03		PP_D1	/* PA */
#define PP_04		PP_D2	/* PA */
#define PP_05		PP_D3	/* PA */
#define PP_06		PP_D4	/* PA */
#define PP_07		PP_D5	/* PA */
#define PP_08		PP_D6	/* PA */
#define PP_09		PP_D7	/* PA */
#define PP_10		PP_ACK	/* PB */
#define PP_11		PP_BUSY	/* PB */
#define PP_12		PP_PAP	/* PB */
#define PP_13		PP_ONOF	/* PB */
#define PP_14		PP_ALF	/* PC */
#define PP_15		PP_ERR	/* PB */
#define PP_16		PP_INI	/* PC */
#define PP_17		PP_DSL	/* PC */

// change of single bits for functions parport_set_bit() and parport_clr_bit()
#define SPP_01		PC,PP_STB
#define SPP_02		PA,PP_D0
#define SPP_03		PA,PP_D1
#define SPP_04		PA,PP_D2
#define SPP_05		PA,PP_D3
#define SPP_06		PA,PP_D4
#define SPP_07		PA,PP_D5
#define SPP_08		PA,PP_D6
#define SPP_09		PA,PP_D7
#define SPP_10		PB,PP_ACK
#define SPP_11		PB,PP_BUSY
#define SPP_12		PB,PP_PAP
#define SPP_13		PB,PP_ONOF
#define SPP_14		PC,PP_ALF
#define SPP_15		PB,PP_ERR
#define SPP_16		PC,PP_INI
#define SPP_17		PC,PP_DSL

#define PARPORT( x )	((s_parport *)(x))

typedef struct s_parport_list_ s_parport_list;
struct s_parport_list_
{
    char *device_path;
    char *alias;
    int  flags;    
    s_parport_list *next;
};

typedef struct
{
    s_parport_list *selected;
    s_parport_list *list;    
    void *emul;			// pointer to iface structure
} s_parport;

#define PARPORT_FILTER_ALL	0
#define PARPORT_FILTER_NOALIAS  1
#define PARPORT_FILTER_ALIAS	2

#define PARPORT_CALLBACK( x )	((f_parport_callback)(x))
typedef void (*f_parport_callback)(s_parport *, s_parport_list *, void *);

/****************************************************/
/*                  Control                         */
/****************************************************/

/*
    Create parallel port driver.
    Looking for parports in system and adding them to list.
    Input:
	pp - pointer to pointer s_parport structure of NULL value
    Return:
	0 - success, -1 - error
*/
extern char parport_init(s_parport **pp, void *emul);

/*
    Free all resources allocated by parport_init() and parport_open().
    Destroy parallel driver.
    Input:
	pp - pointer to s_parport structure
*/
extern void parport_exit(s_parport *pp);

/*
    Open device for I/O
    Input:
	pp - pointer to s_parport structure
    Return:
	0 - success, -1 - error
*/
extern int parport_open( s_parport * ); 

/*
    Close device
    Input:
	pp - pointer to s_parport structure
*/
extern void parport_close( s_parport * ); 

/*
    Redirects all parport calls to emulator adapter.
    Input:
	pp - pointer to s_parport structure
	sw - switch value: 0 - ppdev, 1 - redirect
    Return:
	none
*/
extern void parport_set_emulate(s_parport *, char );

/*
    Invokes f_parport_callback for each element from device list.
    Input:
	pp - pointer to s_parport structure
	f  - callback function
	ptr - callback function parameter
	filter_mode - PARPORT_FILTER_ALL, PARPORT_FILTER_ALIAS, PARPORT_FILTER_NOALIAS
    Return:
	none
    Note:
	Skipping "EMULATE" device.
*/
extern void parport_get_list(s_parport *, f_parport_callback f, void *ptr, char filter_mode);

/*
    Set current interface for I/O.
    Input:
	pp - pointer to s_parport structure
	alias_name  - interface alias name
    Return:
	0 - success, -1 - error
    Note:
	If alias_name = "EMULATE" then parport switch to emulate
*/
extern char parport_set_device(s_parport *, const char *alias_name);

/*
    Return current selected device info.
    Input:
	pp - pointer to s_parport structure
    Return:
	Selected device info or NULL when error (or not set).        
*/
extern const s_parport_list *parport_get_current(s_parport *);

/****************************************************/
/*            I/O operations                        */
/****************************************************/

// set/read of ports, idx = PA,PB,PC 
extern int parport_set(s_parport *, unsigned int port_idx, unsigned char data);
extern int parport_get(s_parport *, unsigned int port_idx);
extern int parport_reset(s_parport *);

/* 
    set/read of bits, idx = PA,PB,PC 
    return PP_ERROR on error
*/
extern int parport_set_bit(s_parport *, unsigned int idx, unsigned int mask);
extern int parport_clr_bit(s_parport *, unsigned int idx, unsigned int mask);
/*
    return 0 or 1 or PP_ERROR
*/
extern int parport_get_bit(s_parport * ,unsigned int idx, unsigned int mask);

#ifdef __cplusplus
}
#endif

#endif

