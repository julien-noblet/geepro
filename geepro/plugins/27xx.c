/* $Revision: 1.3 $ */
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

#define SIZE_2716	2048
#define SIZE_2732	4096
#define SIZE_2764	8192
#define SIZE_27128	16384
#define SIZE_27256	32768
#define SIZE_27512	65536
#define SIZE_27C010	131072
#define SIZE_27C020	262144
#define SIZE_27C040	524288

void fin_message(char error, char *title, char *ok, char *err)
{
    gui_dialog_box(___geep___, title, error ? err : ok, "OK", NULL);
}

unsigned char read_eprom_byte(int addr, int t, char ce_, char oe_)
{
    unsigned char data;
    
    hw_set_oe(ce_); hw_set_ce(oe_);
    hw_set_addr(addr);
    hw_delay(t);
    hw_set_oe(oe_); hw_set_ce(ce_);
    data = hw_get_data();
    hw_delay(t);
    return data;
}

void read_27xx(char *pname, int size, int t1, int t2, char ce, char oe)
{
    int addr;
    
    hw_sw_vcc(0); hw_sw_vpp(0);
    hw_delay(t1);
    hw_sw_vcc(1);
    hw_set_addr(0); hw_set_data(0);
    hw_delay(t1);

    gui_progress_bar_init(___geep___,pname, size);
    for(addr = 0; addr < size; addr++){
	gui_progress_bar_set(___geep___,addr);
	buffer_write(___geep___,addr, read_eprom_byte(addr, t2, ce, oe));
    }
    gui_progress_bar_free(___geep___);
    finish_action();
}


void verify_27xx(char *pname, int size, int t1, int t2, char ce, char oe)
{
    int addr;
    char error;
    int a,b;
    
    hw_sw_vcc(0);
    hw_sw_vpp(0);
    hw_delay(t1);
    hw_sw_vcc(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(t1);
    error=0;

    gui_progress_bar_init(___geep___,pname,size);
    for(addr = 0; addr < size; addr++){
	gui_progress_bar_set(___geep___,addr);
	if( (a = read_eprom_byte(addr, t2,ce,oe)) != (b = buffer_read(___geep___,addr) & 0xff) ){
//printf("%i != %i, adres: %x\n",a,b,addr);
	    error = 1;
	    gui_dialog_box(___geep___,"Weryfikacja","Uklad niezgodny z buforem","OK",NULL);
//	    set_status("Uklad nie jest skasowany");
	    break;    
	}
	hw_delay(t2);
    }
    if(!error){
	    gui_dialog_box(___geep___,"Weryfikacja","Uklad zgodny z buforem","OK",NULL);	    
//	    set_status("Uklad jest skasowany");	    
    }
    gui_progress_bar_free(___geep___);

    finish_action();
}

void test_27xx(char *pname, int size, int t1, int t2, char ce, char oe)
{
    int addr;
    char error;

    hw_sw_vcc(0);
    hw_sw_vpp(0);
    hw_delay(t1);
    gui_progress_bar_init(___geep___,pname,size);
    hw_sw_vcc(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(t1);
    error=0;
    for(addr = 0; addr < size; addr++){
	gui_progress_bar_set(___geep___,addr);
	if( read_eprom_byte(addr, t2,ce,oe) != 0xff ){
	    error = 1;
	    gui_dialog_box(___geep___,"Test","Uklad nie jest w pelni skasowany","OK",NULL);
//	    set_status("Uklad nie jest skasowany");
	    break;    
	}
	hw_delay(t2);
    }
    if(!error){
	    gui_dialog_box(___geep___,"Test","Uklad jest czysty","OK",NULL);	    
//	    set_status("Uklad jest skasowany");	    
    }
    gui_progress_bar_free(___geep___);
    
    finish_action();
}

void write_eprom_byte_2716(int addr, unsigned char data, unsigned int ts, unsigned int time)
{
    hw_set_oe(1); hw_set_ce(0);
    hw_set_addr(addr);
    hw_set_data(data);
    hw_delay(ts);
    hw_set_ce(1);		
    hw_delay(time);	/* 50ms */
    hw_set_ce(0);
}

REG_FUNC_BEGIN(prog_2716)
    int addr;
    int a,b;
    
    hw_sw_vcc(0); hw_sw_vpp(0);
    hw_delay(1000);
    gui_progress_bar_init(___geep___,"Programowanie EPROM 2716", SIZE_2716);
    hw_sw_vcc(1); hw_sw_vpp(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(1000);
    for(addr = 0; addr < SIZE_2716; addr++){
	gui_progress_bar_set(___geep___,addr);
	if((buffer_read(___geep___,addr) & 0xff) != 0xff )
	    write_eprom_byte_2716(addr, buffer_read(___geep___,addr), 20, 50 * 1000);
	if( (a = read_eprom_byte(addr, 20,0,0)) != (b = buffer_read(___geep___,addr) & 0xff) ){
//printf("%i != %i, adres: %x\n",a,b,addr);
	    gui_dialog_box(___geep___,"Blad programowania !!!!","Blad zapisu","OK",NULL);
//	    set_status("Uklad nie jest skasowany");
	    break;    
	}
    }
    gui_progress_bar_free(___geep___);
    finish_action();
REG_FUNC_END

REG_FUNC_BEGIN(read_2716)
    read_27xx("Odczyt ukladu EPROM 2716",SIZE_2716,1000,10,0,0);
REG_FUNC_END

REG_FUNC_BEGIN(verify_2716)
    verify_27xx("Weryfikacja ukladu EPROM 2716",SIZE_2716,1000,10,0,0);
REG_FUNC_END

REG_FUNC_BEGIN(test_2716)
    test_27xx("Test czysto¶ci ukladu EPROM 2716",SIZE_2716,1000,10,0,0);
REG_FUNC_END

void write_eprom_byte_oevpp(int addr, unsigned char data, unsigned int ts, unsigned int time)
{
    hw_set_ce(0); hw_sw_vpp(0);
    hw_set_addr(addr);
    hw_set_data(data);
    hw_delay(ts);
    hw_sw_vpp(1);
    hw_delay(time);	/* 50ms */
    hw_set_ce(0);
    hw_sw_vpp(0);
}


/********************************************************************************************/
REG_FUNC_BEGIN(read_2732)
    read_27xx("Odczyt ukladu EPROM 2732",SIZE_2732,1000,1,0,0);
REG_FUNC_END

REG_FUNC_BEGIN(verify_2732)
    verify_27xx("Weryfikacja ukladu EPROM 2732",SIZE_2732,1000,1,0,0);
REG_FUNC_END

REG_FUNC_BEGIN(test_2732)
    test_27xx("Test czysto¶ci ukladu EPROM 2732",SIZE_2732,1000,1,0,0);
REG_FUNC_END
/*
REG_FUNC_BEGIN(prog_2732)
    int addr;
    int a,b;
    
    hw_sw_vcc(0); hw_sw_vpp(0);
    hw_delay(1000);
    gui_progress_bar_init("Programowanie EPROM 2732", SIZE_2732);
    hw_sw_vcc(1);
    hw_set_addr(0);
    hw_set_data(0);
    hw_delay(1000);
    for(addr = 0; addr < SIZE_2732; addr++){
	gui_progress_bar_set(addr);
	if((read_buffer(addr) & 0xff) != 0xff )
	    write_eprom_byte_oevpp(addr, read_buffer(addr), 20, 50 * 1000);
	if( (a = read_eprom_byte(addr, 20,0,0)) != (b = read_buffer(addr) & 0xff) ){
//printf("%i != %i, adres: %x\n",a,b,addr);
	    dialog_box("Blad programowania !!!!","Blad zapisu","OK",NULL);
//	    set_status("Uklad nie jest skasowany");
	    break;    
	}
    }
    gui_progress_bar_free();
    finish_action();
REG_FUNC_END
*/

/********************************************************************************************/
void pgm_imp(char type,int tp, int ts)
{
    hw_delay(ts);
    if(type==0)
	{ hw_set_ce(0); hw_delay(tp); hw_set_ce(1); }
}

void prog_init(char type, int ts)
{
    hw_set_addr(0); hw_set_data(0);
    if(type == 0)
	{ hw_set_ce(1); hw_set_oe(1); }

    hw_sw_vcc(1); hw_sw_vpp(1);
}

char byte_verify(unsigned int addr, char type, char ce, char oe)
{
    return (buffer_read(___geep___,addr) & 0xff) != (read_eprom_byte(addr, 20,ce,oe) & 0xff);
}


void set_prog_data(unsigned int addr)
{
    hw_set_addr(addr);
    hw_set_data(buffer_read(___geep___,addr));
}

void fast_prog(int size, char type, int tp, int ts, char ce, char oe)
{
    char X, error;
    unsigned int addr;    
    
    hw_sw_vcc(0); hw_sw_vpp(0);
    prog_init(type,ts);

    gui_progress_bar_init(___geep___,"Programowanie algorytmem FAST", size);
    addr  = 0;
    error = 0;
    for(addr=0; addr < size; addr++){
printf("ADDR: %x\n",addr);
	gui_progress_bar_set(___geep___,addr);
	set_prog_data(addr);
	hw_delay(ts);	
        pgm_imp(type,tp,ts);
	X = 0;
	while(byte_verify(addr, type, ce, oe)){
	    set_prog_data(addr);
	    X++;
	    if(X == 25 ) {error=1; break;}
	    pgm_imp(type, 3 * X * tp, ts);	
	}
	if(X == 25) break;
    }
    gui_progress_bar_free(___geep___);
    if(!error){
	gui_progress_bar_init(___geep___,"Weryfikacja zapisu", size);
	for(addr = 0; addr < size; addr++ ){
		gui_progress_bar_set(___geep___,addr);
		if( byte_verify(addr, type, ce, oe) ){ error = 1; break; }
	}
	gui_progress_bar_free(___geep___);
    }
    fin_message(error, "Programowanie zakonczone",
		       "Poprawnie zaprogramowane", "Blad programowania !!!!");
    finish_action();    
}

void normal_prog(int size, char type, int tp, int ts, char ce, char oe)
{
    char error;
    unsigned int addr;    
    
    hw_sw_vcc(0); hw_sw_vpp(0);
    prog_init(type,ts);

    gui_progress_bar_init(___geep___,"Programowanie algorytmem NORMAL", size);
    addr  = 0;
    error = 0;

    do{
	gui_progress_bar_set(___geep___,addr);

	hw_set_addr(addr);
	hw_set_data(buffer_read(___geep___,addr));
	hw_delay(ts);	

        pgm_imp(type,tp,ts);
	if(byte_verify(addr, type, ce, oe)) {error=1; break;}
    }while( ++addr != size);

    gui_progress_bar_free(___geep___);

    gui_progress_bar_init(___geep___,"Weryfikacja zapisu", size);
    for(addr = 0; addr < size; addr++ )	if( byte_verify(addr, type, ce, oe) ){ error = 1; break; }
    gui_progress_bar_free(___geep___);

    fin_message(error, "Programowanie zakonczone",
		       "Poprawnie zaprogramowane", "Blad programowania !!!!");
    finish_action();    
}

/********************************************************************************************/
REG_FUNC_BEGIN(read_2764) read_27xx("Odczyt ukladu EPROM 2764",SIZE_2764,1000,1,1,0); REG_FUNC_END
REG_FUNC_BEGIN(verify_2764) verify_27xx("Weryfikacja ukladu EPROM 2764",SIZE_2764,1000,1,1,0); REG_FUNC_END
REG_FUNC_BEGIN(test_2764) test_27xx("Test czysto¶ci ukladu EPROM 2764",SIZE_2764,1000,1,1,0); REG_FUNC_END
REG_FUNC_BEGIN(prog_2764)
//    normal_prog(SIZE_2764,0,50*1000,10,1,0);
    fast_prog(SIZE_2764,0,1000,1,1,0); 
REG_FUNC_END

/********************************************************************************************/
REG_FUNC_BEGIN(prog_27xx) REG_FUNC_END

REGISTER_MODULE_BEGIN( 27xx )

    register_chip_begin("/EPROM/24 pin", "2716", "27xx", SIZE_2716);
	add_action(MODULE_READ_ACTION, read_2716);
	add_action(MODULE_PROG_ACTION, prog_2716);
	add_action(MODULE_VERIFY_ACTION, verify_2716);
	add_action(MODULE_TEST_ACTION, test_2716);
    register_chip_end;

    register_chip_begin("/EPROM/24 pin", "2732", "27xx", SIZE_2732);
	add_action(MODULE_READ_ACTION, read_2732);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_2732);
	add_action(MODULE_TEST_ACTION, test_2732);
    register_chip_end;

    register_chip_begin("/EPROM/28 pin", "2764", "27xx", SIZE_2764);
	add_action(MODULE_READ_ACTION, read_2764);
	add_action(MODULE_PROG_ACTION, prog_2764);
	add_action(MODULE_VERIFY_ACTION, verify_2764);
	add_action(MODULE_TEST_ACTION, test_2764);
    register_chip_end;

/*
    register_chip_begin("/EPROM/28 pin", "27128", "27xx", SIZE_27128);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;

    register_chip_begin("/EPROM/28 pin", "27256", "27xx", SIZE_27256);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;

    register_chip_begin("/EPROM/28 pin", "27512", "27xx", SIZE_27512);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;

    register_chip_begin("/EPROM/32 pin", "27C010", "27C0xx", SIZE_27C010);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;

    register_chip_begin("/EPROM/32 pin", "27C020", "27C0xx", SIZE_27C020);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;

    register_chip_begin("/EPROM/32 pin", "27C040", "27C0xx", SIZE_27C040);
	add_action(MODULE_READ_ACTION, read_27xx);
	add_action(MODULE_PROG_ACTION, prog_27xx);
	add_action(MODULE_VERIFY_ACTION, verify_27xx);
	add_action(MODULE_TEST_ACTION, test_27xx);
    register_chip_end;
*/
REGISTER_MODULE_END
