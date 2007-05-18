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
#include "../pixmaps/mcs51.xpm"

MODULE_IMPLEMENTATION

#define SIZE_AT89C1051	1024
#define SIZE_AT89C2051	2048
#define SIZE_AT89C4051	4096

#define ATX051_WR_PEROM		0x0000e
#define ATX051_RD_PEROM		0x0000a
#define ATX051_WR_LOCK0		0x0000f
#define ATX051_WR_LOCK1		0x00005
#define ATX051_ERA_PEROM	0x00001
#define ATX051_SIGN_CHIP	0x00000

char at89cXX_lb = 0;	/* domy¶lny tryb dla LB1,LB2 */

void set_ATX051_mode(int mode)
{
    set_address(0x20000);  /* P3.2 = 1, P3.3=P3.4=P3.5=P3.7 =0, X1 = 0, RST=0V, CE=0,OE=0 */
    hw_get_data();	   /* putting input to high impedance */
    hw_delay(10000);
    set_address(0x00000);  /* P3.2 = 1, P3.3=P3.4=P3.5=P3.7 =0, X1 = 0, RST=5V, CE=0,OE=0 */
    set_address(mode << 6);
    hw_delay(1000);
}

void read_AT89Cx051(int size)
{
    int addr;

    start_action(0,0); 
    set_ATX051_mode(ATX051_RD_PEROM);
    progress_loop(addr, 1, size, "Odczyt"){
	copy_data_to_buffer(addr);	
	ce(1,500);
	ce(0,500);
    }
    finish_action();
}

void verify_AT89Cx051(int size)
{
    int addr;
    char ok_stat=1;
    
    start_action(0,0); 
    set_ATX051_mode(ATX051_RD_PEROM);
    progress_loop(addr, 1, size, "Weryfikacja"){
	cmp_data_and_buffer_ploop(addr, ok_stat);	
	ce(1,500);ce(0,500);
    }
    finish_action();
    show_message(ok_stat,"Weryfikacja","Niezgodnosc danych","Dane zgodne");
}

void write_AT89Cx051(int size)
{
    int addr;
    char ok_stat=1;

    start_action(0,0); 
    set_ATX051_mode(ATX051_WR_PEROM);
    progress_loop(addr, 0, size, "Zapis"){
	copy_data_from_buffer(addr);
	hw_delay(100);
	hw_sw_vpp(1); /* set RST to 12V */
	hw_delay(2000); /* 2ms wait for internal write (typ 1.2ms )*/
	hw_sw_vpp(0); /* RST to 5V*/
	cmp_data_and_buffer_ploop(addr,ok_stat);
	ce(1,500);ce(0,500);
    }
    finish_action();
    show_message(ok_stat,"Weryfikacja zapisu","!!! Blad zapisu !!!","Uklad zaprogramowano poprawnie");
}

void read_sign_AT89Cx051()
{
    int addr;
    char sig0,sig1,sig2;
    
    start_action(0,0); 
    set_ATX051_mode(ATX051_SIGN_CHIP);

    copy_data_to_buffer(addr);	
    sig0 = hw_get_data();
    ce(1,500);ce(0,500);
    copy_data_to_buffer(addr);	
    sig1 = hw_get_data();
    ce(1,500);ce(0,500);
    copy_data_to_buffer(addr);	
    sig2 = hw_get_data();
    ce(1,500);ce(0,500);
/*  */
    

    finish_action();
}

void set_lock_AT89Cx051(int size)
{
//    int addr;
//    char ok_stat=1;
/*
    if(lb0){
	start_action(0,0); 
	set_ATX051_mode(ATX051_WR_LOCK0);
	vpp_on(); 
	hw_delay(2000); 
	vpp_off();
	finish_action();
    }
    if(lb1){
	start_action(0,0); 
	set_ATX051_mode(ATX051_WR_LOCK1);
	vpp_on(); 
	hw_delay(2000); 
	vpp_off();
	finish_action();
    }
*/
}


/*************************************************************************/

REG_FUNC_BEGIN(read_AT89C1051)
    read_AT89Cx051(SIZE_AT89C1051); 
REG_FUNC_END

REG_FUNC_BEGIN(read_AT89C2051)
    read_AT89Cx051(SIZE_AT89C2051);
REG_FUNC_END

REG_FUNC_BEGIN(read_AT89C4051) 
    read_AT89Cx051(SIZE_AT89C4051); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_AT89C1051)
    verify_AT89Cx051(SIZE_AT89C1051); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_AT89C2051)
    verify_AT89Cx051(SIZE_AT89C2051); 
REG_FUNC_END

REG_FUNC_BEGIN(verify_AT89C4051)
    verify_AT89Cx051(SIZE_AT89C4051);
REG_FUNC_END

REG_FUNC_BEGIN(write_AT89C1051)
    write_AT89Cx051(SIZE_AT89C1051); 
REG_FUNC_END

REG_FUNC_BEGIN(write_AT89C2051)
    write_AT89Cx051(SIZE_AT89C2051); 
REG_FUNC_END

REG_FUNC_BEGIN(write_AT89C4051)
    write_AT89Cx051(SIZE_AT89C4051); 
REG_FUNC_END

/*************************************************************************/
void AT89Cxx_setLB(void *x, char *parm)
{
    switch(*parm){
	case '0': at89cXX_lb = 0; break;
	case '1': at89cXX_lb = 1; break;
	case '2': at89cXX_lb = 2; break;
    }
printf("|->LB = %i\n",at89cXX_lb);    
}

REG_FUNC_BEGIN(AT89Cxx_autostart)
//    gui_fast_option_add(FO_LABEL,"Lock Bits mode",0,NULL,NULL,0);
//    gui_fast_option_add(FO_RB_FIRST,"No lock",0,AT89Cxx_setLB,"0",0);
//    gui_fast_option_add(FO_RB_NEXT, "No further programming",0,AT89Cxx_setLB,"1",0);
//    gui_fast_option_add(FO_RB_NEXT, "No verify, no further programming",0,AT89Cxx_setLB,"2",0);
REG_FUNC_END

REGISTER_MODULE_BEGIN( MCS-51 )

    REGISTER_IMAGE(img, mcs51_xpm);

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-51/AT89Cx051","AT89C1051", 0, SIZE_AT89C1051);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_AT89C1051, write_AT89C1051, NULL, verify_AT89C1051);
    SET_DIPSW(2);
    SET_AUTOSTART(AT89Cxx_autostart);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-51/AT89Cx051","AT89C2051", 0, SIZE_AT89C2051);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_AT89C2051, write_AT89C2051, NULL, verify_AT89C2051);
    SET_DIPSW(0x6d5);
    SET_AUTOSTART(AT89Cxx_autostart);
    D_REGISTER;

    INIT_DEFAULT_SET;
    D_DATA_INIT_SET("/uk/MCS-51/AT89Cx051","AT89C4051", 0, SIZE_AT89C4051);
    INIT_IMAGE_SET_IDX(img,img);
    D_FUNC_INIT_SET(read_AT89C4051, write_AT89C4051, NULL, verify_AT89C4051);
    SET_DIPSW(0x6d5);
    SET_AUTOSTART(AT89Cxx_autostart);
    D_REGISTER;

REGISTER_MODULE_END
