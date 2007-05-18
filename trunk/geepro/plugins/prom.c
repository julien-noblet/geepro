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

/*
    Aby m�c odczytywa� zawarto�� PROM serii 74S474 w programatorze WILLEM V 3.1 20-3-2002 
    nale�y przeci�� �cie�k� od wyprowadzenia 28 ZIF i po��czy� j� jumperem pomi�dzy istniej�ce 
    po��czenie, a VCC ZIF.
    To umo�liwi obs�ug� uk�ad�w 24 PIN.
    
    Aby m�c odczytywa� zawarto�� wszystkich 74S475, (wyj�cia OC), nale�y w lini� danych
    13..16,17..21 ZIF wstawi� drabink� 10k do zasilania.
    To ma na celu mo�liwo�� obs�ugi uk�ad�w z wyj�ciami typu Open Collector
*/

#include "modules.h"

MODULE_IMPLEMENTATION

#define ADDR_MASK_474	0x600
#define SIZE_PROM_474	0x200

REG_FUNC_BEGIN(read_474)
    int addr;

    hw_sw_vpp(0);
    hw_sw_vcc(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(1000);
    progress_loop(addr, 0, SIZE_PROM_474, "Odczyt PROM"){
	hw_set_oe(1);
	hw_set_ce(1);
	hw_set_addr(addr | ADDR_MASK_474);	
	hw_delay(200);
	buffer_write(___geep___,addr,hw_get_data());
	hw_delay(200);
    }
    finish_action();
REG_FUNC_END

REG_FUNC_BEGIN(verify_474)
    int addr;

    hw_sw_vpp(0);
    hw_sw_vcc(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(1000);
    progress_loop(addr, 0, SIZE_PROM_474, "Weryfikacja PROM"){
	hw_set_oe(1);
	hw_set_ce(1);
	hw_set_addr(addr | ADDR_MASK_474);	
	hw_delay(200);
	if(buffer_read(___geep___,addr) != hw_get_data() ){
// ?????
	}
	hw_delay(200);
    }
    finish_action();
REG_FUNC_END

REG_FUNC_BEGIN(autostart_474) 
/* autostart dla 474 */
REG_FUNC_END

REG_FUNC_BEGIN(autostart_475)
//    gui_fast_option_add(FO_LABEL,"Warning:\nOpen Collector output", 0, NULL, NULL, 0);
REG_FUNC_END

REGISTER_MODULE_BEGIN( PROM )

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/PROM","74S474", 0, SIZE_PROM_474);
    INIT_IMAGE_SET_IDX(0,0);
    D_FUNC_INIT_SET(read_474, NULL, NULL, verify_474);
    SET_DIPSW(0x30); /* 7 i 8 za��czone */
    SET_AUTOSTART(autostart_474);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/PROM","74S475", 0, SIZE_PROM_474);
    INIT_IMAGE_SET_IDX(0,0);
    D_FUNC_INIT_SET(read_474, NULL, NULL, verify_474);
    SET_DIPSW(0x30); /* 7 i 8 za��czone */
    SET_AUTOSTART(autostart_475);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/PROM","KR556RT5", 0, SIZE_PROM_474);
    INIT_IMAGE_SET_IDX(0,0);
    D_FUNC_INIT_SET(read_474, NULL, NULL, verify_474);
    SET_DIPSW(0x30); /* 7 i 8 za��czone */
    SET_AUTOSTART(autostart_475);
    D_REGISTER;

REGISTER_MODULE_END

