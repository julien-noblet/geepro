/* $Revision: 1.7 $ */
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

#ifndef __modules_h__
#define __modules_h__
#include <stdio.h>
#include <string.h>
#include "../drivers/hwplugin.h"
#include "../src/chip.h"
#include "../gui/gui.h"
#include "../src/buffer.h"
#include "../src/geepro.h"


#define ms_delay(ms)	\
    hw_delay((ms)*1000)
    
#define set_data(value)\
    hw_set_data((value) & 0xff)
    
#define set_address(a)	\
    hw_set_addr(a)

#define oe(state, delay)	\
    hw_set_oe(state);\
    hw_delay(delay)
    
#define ce(state, delay)	\
    hw_set_ce(state);\
    hw_delay(delay)

#define progress_loop(cn, rounds, title)		\
	for(cn = 0, gui_progress_bar_init(___geep___,title, rounds);\
	    gui_cmp_pls(___geep___,cn, rounds); cn++, gui_progress_bar_set(___geep___,cn, rounds))

#define finish_action()	\
    hw_sw_vpp(0);\
    hw_set_oe(0);\
    hw_set_ce(0);\
    hw_sw_vcc(0);\
    buffer_checksum(___geep___);\
    gui_stat_rfsh(___geep___)

#define start_action(oe_,ce_)   \
    hw_sw_vpp(0);\
    hw_sw_vcc(1);\
    hw_set_oe(oe_);\
    hw_set_ce(ce_);\
    hw_delay(5000)

#define copy_data_to_buffer(addr)	\
    buffer_write(___geep___,addr, hw_get_data())

#define put_buffer( addr, data) buffer_write(___geep___,addr, data)

#define get_buffer(addr) buffer_read(___geep___,addr)

#define copy_data_from_buffer(addr)  set_data( get_buffer(addr) )

#define progressbar_free() gui_progress_bar_free(___geep___)

#define cmp_data_and_buffer_ploop(addr, error) \
	if(hw_get_data() != buffer_read(___geep___,addr)){\
	    error = 0;\
	    gui_progress_bar_free(___geep___);\
	    break;\
	}

#define  show_message(sw,title,msg_1,msg_2)	\
    gui_dialog_box(___geep___, title, sw ? msg_2 : msg_1, "OK", NULL)

#define show_dialog(title, msg)	\
    gui_dialog_box(___geep___, title, msg, "OK", NULL)

#define REGISTER_MODULE_BEGIN(name)	\
    int init_module(geepro *gep___)\
    {\
	int __id__=0, __i__ ;\
	___geep___ = gep___;\
	chip_desc __init_struct__; \
	printf("Init " #name " module.\n");\
	{

#define REGISTER_MODULE_END	\
	} __id__ = 0;\
	__i__ = 0;\
	return 0;\
    }

#define MODULE_IMPLEMENTATION	\
    static geepro *___geep___ = ((void*)0);\
    static int ___error___ = 0;\
    char test_connection()\
    {                      \
	if(hw_test_conn()) return 0;\
	gui_dialog_box(___geep___, "[ER][TEXT]Programmer unplugged.[/TEXT][BR]OK",NULL, NULL);\
	return -1;\
    }

#define VOID
#define TEST_CONNECTION( ret ) \
    if(test_connection()){\
     SET_ERROR;\
     return ret;\
    }

#define REG_FUNC_BEGIN(name)	\
    static int name (void *gep___)\
    {\
	___geep___ = (geepro*)gep___;\
	___error___ = 0;

#define SET_ERROR	___error___ = -1;	

#define REG_FUNC_END	\
	___geep___ = ((void*)0);\
	return ___error___;\
    }

#define to_hex(value, digit)	((value >> (digit * 4)) & 0x0f)

#define ERROR	\
    return -1

#define register_chip_begin(path, name, family, size)	\
    memset(&__init_struct__, 0, sizeof(chip_desc));\
    __init_struct__.chip_path = path;\
    __init_struct__.chip_name = name;\
    __init_struct__.chip_family = family;\
    __init_struct__.chip_id = ++__id__;\
    __init_struct__.dev_size = size

#define register_chip_end	\
    chip_register_chip(___geep___->ifc->plugins, &__init_struct__)

#define add_action(bt_name, callback)	\
    chip_add_action(&__init_struct__, bt_name, callback)

#define add_autostart(callback)	\
    __init_struct__.autostart = callback

#define MODULE_WRITE_ACTION MODULE_PROG_ACTION
#define MODULE_TEST_BLANK_ACTION MODULE_TEST_ACTION

#define checkbox(fmt)	gui_checkbox(___geep___, fmt)

#define MODULE_READ_ACTION	\
    "geepro-read-action", "Read data from chip"

#define MODULE_SIGN_ACTION	\
    "geepro-sign-action", "Read signature from chip"

#define MODULE_PROG_ACTION	\
    "geepro-write-action", "Write data to chip"

#define MODULE_ERASE_ACTION	\
    "geepro-erase-action", "Erase memory"

#define MODULE_TEST_ACTION	\
    "geepro-testblank-action", "Test blank memory"

#define MODULE_VERIFY_ACTION	\
    "geepro-verify-action", "Verify chip memory with buffer"

#define MODULE_LOCKBIT_ACTION	\
    "geepro-lockbit-action", "Set lock-bits"

#define REGISTER_FUNCTION_( registered_func, exec_func, call_parameters... )	\
    REG_FUNC_BEGIN( registered_func )	\
	exec_func(call_parameters);	\
    REG_FUNC_END

#define REGISTER_FUNCTION( action, registered_name, exec_name, call_parameters... )	\
    REGISTER_FUNCTION_( action##_##registered_name, action##_##exec_name, call_parameters)

#endif





