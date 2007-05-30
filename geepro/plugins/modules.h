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

#define progress_loop(cn, cnini, rounds, title)		\
	for(cn = 0, gui_progress_bar_init(___geep___,title, rounds);\
	    gui_cmp_pls(___geep___,cn, rounds); cn++, gui_progress_bar_set(___geep___,cn))

#define finish_action()	\
    hw_set_oe(0);\
    hw_set_ce(0);\
    hw_sw_vpp(0);\
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

#define copy_data_from_buffer(addr)	\
    set_data(buffer_read(___geep___,addr))

#define cmp_data_and_buffer_ploop(addr, error) \
	if(hw_get_data() != buffer_read(___geep___,addr)){\
	    error = 0;\
	    gui_progress_bar_free(___geep___);\
	    break;\
	}

#define  show_message(sw,title,msg_1,msg_2)	\
    gui_dialog_box(___geep___, title, sw ? msg_2 : msg_1, "OK", NULL)

#define REGISTER_MODULE_BEGIN(name)	\
    int init_module(geepro *gep___)\
    {\
	int __id__=0, __i__ ;\
	___geep___ = gep___;\
	chip __init_struct__; \
	printf("Init " #name " module.\n");\
	{

#define REGISTER_MODULE_END	\
	} __id__ = 0;\
	__i__ = 0;\
	return 0;\
    }

#define MODULE_IMPLEMENTATION	\
    static geepro *___geep___ = ((void*)0);

#define REG_FUNC_BEGIN(name)	\
    static int name (void *____wg, void *gep___)\
    {\
	___geep___ = (geepro*)gep___;

#define REG_FUNC_END	\
	___geep___ = ((void*)0);\
	return 0;\
    }

#define ERROR	\
    return -1

#define register_chip_begin(path, name, family, size)	\
    memset(&__init_struct__, 0, sizeof(chip));\
    __init_struct__.chip_path = path;\
    __init_struct__.chip_name = name;\
    __init_struct__.chip_family = family;\
    __init_struct__.chip_id = ++__id__;\
    __init_struct__.dev_size = size

#define register_chip_end	\
    chip_register_chip(___geep___->ifc->plugins, &__init_struct__)

#define add_action(bt_name, callback)	\
    gui_add_action(gep___, &__init_struct__, bt_name, callback)

#define MODULE_READ_ACTION	\
    "chip-read", "Read data from chip"

#define MODULE_PROG_ACTION	\
    "chip-prog", "Write data to chip"

#define MODULE_ERASE_ACTION	\
    "chip-erase", "Erase memory"

#define MODULE_TEST_ACTION	\
    "chip-test-blank", "Test blank memory"

#define MODULE_VERIFY_ACTION	\
    "chip-verify", "Verify chip memory with buffer"

#endif

