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
#include "../pixmaps/mcs48.xpm"

MODULE_IMPLEMENTATION

/* ROM */
#define SIZE_8048AH	1024
#define SIZE_8049AH	2048
#define SIZE_8050AH	4096
/* EPROM */
#define SIZE_8748H	1024
#define SIZE_8749H	2048
#define SIZE_8741	1024
#define SIZE_8742	2048

void read_8048(int size)
{
    unsigned short int addr;    

    start_action(0,0);
    hw_sw_vpp(1);    
    hw_delay(100000);
    progress_loop(addr, 0, size, "Odczyt"){
	oe(0,3000);
	set_address((addr >> 8) & 0x0f);
	set_data(addr);
	oe(1,1500);
//	set_address(((addr >> 8) & 0x0f) | 0x10);
	hw_delay(500);
	copy_data_to_buffer(addr);
    }
    finish_action();
}

void verify_8048(int size)
{
    unsigned short int addr;    
    char ok_status;

    start_action(0,0);
    hw_sw_vpp(1);    
    hw_delay(100000);

    ok_status = 1;
    progress_loop(addr, 0, size, "Weryfikacja"){
	oe(0,3000)
	set_address((addr >> 8) & 0x0f);
	set_data(addr);
	oe(1,3000);
	cmp_data_and_buffer_ploop(addr, ok_status);
    }
    finish_action();
    show_message(ok_status,"Weryfikacja", "Niezgodnosc danych","Dane zgodne z buforem");
}

/*************************************************************************/
REG_FUNC_BEGIN(read_8048AH) 
    read_8048(SIZE_8048AH); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_8048AH) 
    verify_8048(SIZE_8048AH); 
REG_FUNC_END

REG_FUNC_BEGIN(read_8049AH) 
    read_8048(SIZE_8049AH); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_8049AH) 
    verify_8048(SIZE_8049AH); 
REG_FUNC_END

REG_FUNC_BEGIN(read_8050AH) 
    read_8048(SIZE_8050AH); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_8050AH) 
    verify_8048(SIZE_8050AH); 
REG_FUNC_END

REG_FUNC_BEGIN(read_8748H) 
    read_8048(SIZE_8748H); 
REG_FUNC_END

//REG_FUNC_BEGIN(verify_8748AH() 
//    verify_8748(SIZE_8748AH); 
//REG_FUNC_END

REG_FUNC_BEGIN(read_8749H) 
    read_8048(SIZE_8749H); 
REG_FUNC_END

//REG_FUNC_BEGIN(verify_8749AH) 
//    verify_8748(SIZE_8749AH); 
//REG_FUNC_END

/*************************************************************************/

REGISTER_MODULE_BEGIN( MCS-48 )

    REGISTER_IMAGE(img, mcs48_xpm);

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-48","8048AH", 0, SIZE_8048AH);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_8048AH, NULL, NULL, verify_8048AH);
    SET_DIPSW(0x6d5);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-48","8049AH", 0, SIZE_8049AH);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_8049AH, NULL, NULL, verify_8049AH);
    SET_DIPSW(0x6d5);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-48","8050AH", 0, SIZE_8050AH);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_8050AH, NULL, NULL, verify_8050AH);
    SET_DIPSW(0x6d5);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-48","8748H", 0, SIZE_8748H);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_8748H, NULL, NULL, NULL);
    SET_DIPSW(0x6d5);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-48","8749H", 0, SIZE_8749H);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_8749H, NULL, NULL, NULL);
    SET_DIPSW(0x6d5);
    D_REGISTER;

REGISTER_MODULE_END
