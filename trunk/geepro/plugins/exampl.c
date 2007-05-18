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

#include "modules.h"

MODULE_IMPLEMENTATION

REG_FUNC_BEGIN(ex_read) REG_FUNC_END
REG_FUNC_BEGIN(ex_verify) REG_FUNC_END
REG_FUNC_BEGIN(ex_prog) REG_FUNC_END
REG_FUNC_BEGIN(ex_erase) REG_FUNC_END
REG_FUNC_BEGIN(signature) REG_FUNC_END
REG_FUNC_BEGIN(lock_bits) REG_FUNC_END
REG_FUNC_BEGIN(unlock_bits) REG_FUNC_END
REG_FUNC_BEGIN(test_chip) REG_FUNC_END


REG_FUNC_BEGIN(autostart_module)
/*
    gui_adv_add_page(___geep___,"Konf_1");
    gui_adv_option_add(___geep___,FO_RB_FIRST,"aaa", 0, NULL, NULL,FO_H_FIRST);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,FO_H_NEXT);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"aaa", 0, NULL, NULL,FO_H_FIRST);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,FO_H_NEXT);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"aaa", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_ENTRY,"zulu:", 0xaa, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_ENTRY,"zulu:", 0xaa, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_ENTRY,"zulu:", 0xaa, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);

    gui_adv_add_page(___geep___,"Konf_2");
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_CHECK,"eee", 0, NULL, NULL,0);
    gui_adv_option_add(___geep___,FO_RB_FIRST,"aaa", 0, NULL, NULL,FO_H_FIRST);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,FO_H_NEXT);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"aaa", 0, NULL, NULL,FO_H_FIRST);
    gui_adv_option_add(___geep___,FO_RB_NEXT,"bbb", 0, NULL, NULL,FO_H_NEXT);
*/
REG_FUNC_END


REGISTER_MODULE_BEGIN( Example dummy )

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/Example/example 1","chip 1", 0, 1024);
//    INIT_IMAGE_SET_IDX(D_93XX_WILLEM_IMG,D_93XX_PCB3_IMG);
    D_FUNC_INIT_SET(ex_read, ex_prog, ex_erase, ex_verify);
    D_XFUNC_INIT_SET(signature,lock_bits,unlock_bits,test_chip);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/Example/example 2","chip 1", 1, 2048);
//    INIT_IMAGE_SET_IDX(D_93XX_WILLEM_IMG,D_93XX_PCB3_IMG);
    D_FUNC_INIT_SET(ex_read, NULL, ex_erase, NULL);
    SET_AUTOSTART(autostart_module);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/Example/example 1","chip2", 2, 65536);
//    INIT_IMAGE_SET_IDX(D_2764_WILLEM_IMG,D_2764_PCB3_IMG);
    D_FUNC_INIT_SET(ex_read, ex_prog, ex_erase, ex_verify);
    SET_AUTOSTART(autostart_module);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/Example","chip", 0, 256);
//    INIT_IMAGE_SET_IDX(D_93XX_WILLEM_IMG,D_93XX_PCB3_IMG);
    D_FUNC_INIT_SET(ex_read, ex_prog, ex_erase, ex_verify);
    SET_AUTOSTART(autostart_module);
    D_REGISTER;

REGISTER_MODULE_END
