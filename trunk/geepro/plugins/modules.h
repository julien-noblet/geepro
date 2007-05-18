/* $Revision: 1.1.1.1 $ */
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
#include "../pixmaps/img_idx.h"
#include "../drivers/hwplugin.h"
#include "../src/chip.h"
#include "../gui/gui.h"
#include "../src/buffer.h"
#include "../src/geepro.h"

#define FO_RB_FIRST	1
#define FO_RB_NEXT	2
#define FO_LABEL	3
#define FO_CHECK	4
#define FO_ENTRY	5
#define FO_CPAGE	6

#define FO_NONE		0
#define FO_H_FIRST	1
#define FO_H_NEXT	2

#define INIT_MODULE(msg) \
    chip __init_struct__; \
    printf(msg); 

#define SET_DIPSW(a) __init_struct__.dip_switch = a;

#define INIT_DEFAULT_SET \
    __init_struct__.chip_name = NULL;\
    __init_struct__.sub_id = __init_struct__.dev_size = __init_struct__.dip_switch = 0; \
    __init_struct__.vpp = __init_struct__.vcc = __init_struct__.vcp = __init_struct__.prog_time = -1; \
    __init_struct__.image_willem = __init_struct__.image_pcb = __init_struct__.option = NULL; \
    __init_struct__.read_sig_chip = __init_struct__.lock_chip = __init_struct__.unlock_chip = NULL; \
    __init_struct__.test_chip  = __init_struct__.read_chip = NULL; \
    __init_struct__.write_chip = __init_struct__.erase_chip = __init_struct__.verify_chip = NULL;\
    __init_struct__.autostart = NULL;
    
#define D_TEST(test)	__init_struct__.test_chip  = test
#define D_REGISTER chip_register_chip(___geep___->ifc->plugins, &__init_struct__)

#define INIT_IMAGE_SET_IDX(willem,pcb3)\
    __init_struct__.img_will_idx = willem;\
    __init_struct__.img_pcb3_idx = pcb3;

#define SET_AUTOSTART(fnc);\
    __init_struct__.autostart = fnc

#define D_DATA_INIT_SET( path, name, id, size) \
    __init_struct__.chip_path = path; __init_struct__.chip_name = name; \
    __init_struct__.sub_id = id; __init_struct__.dev_size = size;

#define D_FUNC_INIT_SET(read,write,erase,verify) \
    __init_struct__.read_chip = read; __init_struct__.write_chip = write; \
    __init_struct__.erase_chip = erase; __init_struct__.verify_chip = verify;

#define D_XFUNC_INIT_SET(sig,test,lock,unlock) \
    __init_struct__.read_chip = sig; __init_struct__.write_chip = test; \
    __init_struct__.erase_chip = lock; __init_struct__.verify_chip = unlock;

#define D_DIP_INIT_SET(dip)     __init_struct__.erase_chip = dip;


#define ms_delay(ms)	hw_delay((ms)*1000)
#define set_data(value)	hw_set_data((value) & 0xff);
#define set_address(a)	hw_set_addr(a);

#define oe(state, delay) hw_set_oe(state); hw_delay(delay);
#define ce(state, delay) hw_set_ce(state); hw_delay(delay);

#define progress_loop(cn, cnini, rounds, title)		\
	for(cn = 0, gui_progress_bar_init(___geep___,title, rounds);\
	gui_cmp_pls(___geep___,cn, rounds); cn++, gui_progress_bar_set(___geep___,cn))

#define finish_action()		hw_set_oe(0); hw_set_ce(0);hw_sw_vpp(0); \
				hw_sw_vcc(0); buffer_checksum(___geep___); gui_stat_rfsh(___geep___);

#define start_action(oe_,ce_)   \
    hw_sw_vpp(0); hw_sw_vcc(1); hw_set_oe(oe_); hw_set_ce(ce_); hw_delay(5000);


#define copy_data_to_buffer(addr)   buffer_write(___geep___,addr, hw_get_data())
#define copy_data_from_buffer(addr) set_data(buffer_read(___geep___,addr))

#define cmp_data_and_buffer_ploop(addr, error) \
	if(hw_get_data() != buffer_read(___geep___,addr)){ error = 0; gui_progress_bar_free(___geep___); break;}

#define  show_message(sw,title,msg_1,msg_2) gui_dialog_box(___geep___,title,sw ? msg_2:msg_1,"OK",NULL);

#define REGISTER_MODULE_BEGIN(name)	int init_module(geepro *gep___)\
					{ int __id__=0, __i__ ; ___geep___ = gep___;\
					 INIT_MODULE("Init " #name " module.\n"); {

#define REGISTER_MODULE_END	} __id__ = 0; __i__ = 0; return 0; }

#define MODULE_IMPLEMENTATION	static geepro *___geep___ = ((void*)0);
#define REG_FUNC_BEGIN(name)	static int name (void *____wg, void *gep___){ ___geep___ = (geepro*)gep___;
#define REG_FUNC_END		___geep___ = ((void*)0); return 0; }
#define ERROR			return -1;
#define REGISTER_IMAGE(var, img) int var = gui_register_image(___geep___, img);


#define register_chip(path, name, dev_size)	INIT_DEFAULT_SET; \
	    D_DATA_INIT_SET(path, name, ++__id__, dev_size);\
			for(__i__ = 1; __i__; __i__ = 0, (D_REGISTER))

#endif
