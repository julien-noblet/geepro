/* $Revision: 1.9 $ */
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

/* 2716 */
char read_byte_2716( int addr )
{
    char data;
    set_address( addr );
    oe(0, 1);
    data = hw_get_data();
    oe(1, 1);
    return data;
}

void read_2716_(int size, char ce)
{
    int addr;
    unsigned char data;
    TEST_CONNECTION( VOID )
    start_action(0, ce);
    progress_loop(addr, size, "Reading data"){
	data = read_byte_2716( addr );
	put_buffer(addr, data);
    }    
    finish_action();
}

char test_2716_(int size, char ce, char silent)
{
    int addr;
    unsigned char rdata;
    char text[256];
    
    TEST_CONNECTION( 1 )
    start_action(0, ce);
    progress_loop(addr, size, "Test blank"){
	rdata = read_byte_2716( addr );
	break_if( rdata != 0xff );
    }    
    finish_action();
    if( !silent | (rdata != 0xff)){
	sprintf( text,
	    "[WN][TEXT]Chip is not empty !!!\n Address = 0x%X\nByte =0x%X%X [/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0)
	);    
	show_message(0, (rdata == 0xff) ? "[IF][TEXT]Chip is clear.[/TEXT][BR]OK" : text, NULL, NULL);
    }
    return rdata != 0xff;
}

void verify_2716_(int size, char ce)
{
    int addr;
    unsigned char rdata, wdata;
    char text[256];
    
    start_action(0, ce);
    progress_loop(addr, size, "Verifying data"){
	rdata = read_byte_2716( addr );
	wdata = get_buffer( addr );
	break_if( rdata != wdata );
    }    
    finish_action();
    sprintf( text,
	"[WN][TEXT]Inconsistent chip and buffer !!!\n Address = 0x%X\nByte = 0x%X%X, should be 0x%X%X [/TEXT][BR]OK",
	addr,
	to_hex(rdata, 1), to_hex(rdata, 0),
	to_hex(wdata, 1), to_hex(wdata, 0)
    );    
    show_message(0, (rdata == wdata) ? "[IF][TEXT]Chip and buffer are consistent.[/TEXT][BR]OK" : text, NULL, NULL);
}

void write_byte_2716(int addr, char data)
{
    oe(1, 1);
    set_address( addr );
    hw_delay(100);
    set_data(data);    
    hw_delay(100);
    ce(1, 50000); // positive program pulse 50ms
    ce(0, 1000); 
}

void prog_2716_(int size)
{
    int addr, tries;
    unsigned char rdata, wdata;
    char text[256];
    
    TEST_CONNECTION( VOID )
//    if(test_2716_(size, 0, 1)) return;
    start_action(0, 0);
    hw_sw_vpp(1);
    progress_loop(addr, size, "Writing data"){
        tries = 0;
	do{
	    wdata = get_buffer( addr );	
	    if(wdata != 0xff)
		write_byte_2716( addr, wdata );	
	    rdata = read_byte_2716( addr );
	} while( (wdata != rdata) && (++tries < 20));
	break_if( rdata != wdata );
    }    
    finish_action();
    if(rdata != wdata ){
	sprintf( text,
	    "[WN][TEXT]Write error !!!\n Address = 0x%X\nByte = 0x%X%X, should be 0x%X%X [/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0),
	    to_hex(wdata, 1), to_hex(wdata, 0)
	);    
	show_message(0, (rdata == wdata) ? "[IF][TEXT]Chip program OK.[/TEXT][BR]OK" : text, NULL, NULL);
	return;
    }
    verify_2716_( size, 0 );
}

void write_byte_2732(int addr, char data)
{
    oe(0, 1);
    ce(1, 100);
    hw_sw_vpp(1);
    hw_delay( 100 );
    set_address( addr );
    set_data(data);    
    hw_delay( 5 );
    hw_delay( 100 );
    ce(0, 50000);
    ce(1, 5);
    hw_sw_vpp(0);   
    ce(0, 1);
    oe(1, 100); 
}

void prog_2732_(int size)
{
    int addr, tries;
    unsigned char rdata, wdata;
    char text[256];
    
    TEST_CONNECTION( VOID )
//    if(test_2716_(size, 0, 1)) return;
    start_action(0, 0);
    progress_loop(addr, size, "Writing data"){
        tries = 0;
	do{
	    wdata = get_buffer( addr );	
	    if(wdata != 0xff)
		write_byte_2732( addr, wdata );	
	    rdata = read_byte_2716( addr );
	} while( (wdata != rdata) && (++tries < 20));
	break_if( rdata != wdata );
    }    
    finish_action();
    if(rdata != wdata ){
	sprintf( text,
	    "[WN][TEXT]Write error !!!\n Address = 0x%X\nByte = 0x%X%X, should be 0x%X%X [/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0),
	    to_hex(wdata, 1), to_hex(wdata, 0)
	);    
	show_message(0, (rdata == wdata) ? "[IF][TEXT]Chip program OK.[/TEXT][BR]OK" : text, NULL, NULL);
	return;
    }
    verify_2716_( size, 0 );
}

/**********************************************************************************************/
void overprogram_2764(int tpp)
{
    tpp = 3 * tpp * 1000;
    ce(0, tpp);
    ce(1, 100);
}

void write_byte_2764(int addr, char data, int time)
{
    oe(1, 1);
    ce(1, 100);
    hw_delay( 100 );
    set_address( addr );
    set_data(data);    
    hw_delay( 100 ); 	// data and address valid, oe = 1, ce = 0, PGM = 1
    ce(0, time);	// pgm = 0
    ce(1, 100);		// pgm = 1
}

char read_byte_2764( int addr )
{
    oe(0, 1);
    ce(1, 10);
    return read_byte_2716( addr );
}

void prog_2764_(int size)
{
    int addr, x;
    unsigned char rdata, wdata;
    char text[256];
    
    TEST_CONNECTION( VOID )
//    if(test_2716_(size, 1, 1)) return;
    start_action(0, 1);
    hw_sw_vpp(1);
    progress_loop(addr, size, "Writing data"){
	wdata = get_buffer( addr );	
	if(wdata != 0xff){
	    for(x = 1; x < 26; x++){
		write_byte_2764( addr, wdata, 1000 ); // impuls 1ms
		rdata = read_byte_2764( addr );
		if( wdata == rdata ) break;
	    }
	    break_if( wdata != rdata );
	    overprogram_2764( x );
	} else 
	    rdata = read_byte_2764( addr );
	break_if( rdata != wdata );
    }    
    finish_action();
    if(rdata != wdata ){
	sprintf( text,
	    "[WN][TEXT]Write error !!!\n Address = 0x%X\nByte = 0x%X%X, should be 0x%X%X [/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0),
	    to_hex(wdata, 1), to_hex(wdata, 0)
	);    
	show_message(0, (rdata == wdata) ? "[IF][TEXT]Chip program OK.[/TEXT][BR]OK" : text, NULL, NULL);
	return;
    }
    verify_2716_( size, 1 );
}

/*********************************************************************************************************/
char read_byte_27256()
{
    oe(0, 10);
    return hw_get_data();
}

void prog_27256_(int size)
{
    int addr, x;
    unsigned char rdata, wdata;
    char text[256];
    
    TEST_CONNECTION( VOID )
//    if(test_2716_(size, 1, 1)) return;
    start_action(0, 1);
    hw_sw_vpp(1);
    progress_loop(addr, size, "Writing data"){
	wdata = get_buffer( addr );	
	if(wdata != 0xff){
	    for(x = 1; x < 26; x++){
		//write_byte_2764( addr, wdata, 1000 ); // impuls 1ms
		oe(1,1);
		ce(1,10);
		set_address( addr );
		set_data( wdata );
		hw_delay(2);
		ce(0, 1000); // 1ms pulse
		ce(1, 10);
		rdata = read_byte_27256( addr );
		if( wdata == rdata ) break;
	    }
	    break_if( wdata != rdata );
	    overprogram_2764( x );
	}
	break_if( rdata != wdata );
    }    
    finish_action();
    if(rdata != wdata ){
	sprintf( text,
	    "[WN][TEXT]Write error !!!\n Address = 0x%X\nByte = 0x%X%X, should be 0x%X%X [/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0),
	    to_hex(wdata, 1), to_hex(wdata, 0)
	);    
	show_message(0, (rdata == wdata) ? "[IF][TEXT]Chip program OK.[/TEXT][BR]OK" : text, NULL, NULL);
	return;
    }
    verify_2716_( size, 1 );
}

/* 2716 */
REGISTER_FUNCTION( read,   2716, 2716_, SIZE_2716, 0 );
REGISTER_FUNCTION( verify, 2716, 2716_, SIZE_2716, 0 );
REGISTER_FUNCTION( test,   2716, 2716_, SIZE_2716, 0, 0 );
REGISTER_FUNCTION( prog,   2716, 2716_, SIZE_2716 );
/* 2732 */
REGISTER_FUNCTION( read,   2732, 2716_, SIZE_2732, 0 );
REGISTER_FUNCTION( verify, 2732, 2716_, SIZE_2732, 0 );
REGISTER_FUNCTION( test,   2732, 2716_, SIZE_2732, 0, 0 );
REGISTER_FUNCTION( prog,   2732, 2732_, SIZE_2732 );
/* 2764 */
REGISTER_FUNCTION( read,   2764, 2716_, SIZE_2764, 1 );
REGISTER_FUNCTION( verify, 2764, 2716_, SIZE_2764, 1 );
REGISTER_FUNCTION( test,   2764, 2716_, SIZE_2764, 1, 0 );
REGISTER_FUNCTION( prog,   2764, 2764_, SIZE_2764 );
/* 27128 */
REGISTER_FUNCTION( read,   27128, 2716_, SIZE_27128, 1 );
REGISTER_FUNCTION( verify, 27128, 2716_, SIZE_27128, 1 );
REGISTER_FUNCTION( test,   27128, 2716_, SIZE_27128, 1, 0 );
REGISTER_FUNCTION( prog,   27128, 2764_, SIZE_27128 );
/* 27256 */
REGISTER_FUNCTION( read,   27256, 2716_, SIZE_27256,0 );
REGISTER_FUNCTION( verify, 27256, 2716_, SIZE_27256,0 );
REGISTER_FUNCTION( test,   27256, 2716_, SIZE_27256,0, 0 );
REGISTER_FUNCTION( prog,   27256, 27256_, SIZE_27256 );

/********************************************************************************************/
REGISTER_MODULE_BEGIN( 27xx )
/* 24 PIN EPROM */
    register_chip_begin("/EPROM/24 pin", "2716", "2716", SIZE_2716);
	add_action(MODULE_READ_ACTION, read_2716);
	add_action(MODULE_PROG_ACTION, prog_2716);
	add_action(MODULE_VERIFY_ACTION, verify_2716);
	add_action(MODULE_TEST_ACTION, test_2716);
    register_chip_end;
    register_chip_begin("/EPROM/24 pin", "2732", "2732", SIZE_2732);
	add_action(MODULE_READ_ACTION, read_2732);
	add_action(MODULE_PROG_ACTION, prog_2732);
	add_action(MODULE_VERIFY_ACTION, verify_2732);
	add_action(MODULE_TEST_ACTION, test_2732);
    register_chip_end;
/* 28 PIN EPROM */
    register_chip_begin("/EPROM/28 pin", "2764", "2764_128", SIZE_2764);
	add_action(MODULE_READ_ACTION, read_2764);
	add_action(MODULE_PROG_ACTION, prog_2764);
	add_action(MODULE_VERIFY_ACTION, verify_2764);
	add_action(MODULE_TEST_ACTION, test_2764);
    register_chip_end;
    register_chip_begin("/EPROM/28 pin", "27128", "2764_128", SIZE_27128);
	add_action(MODULE_READ_ACTION, read_27128);
	add_action(MODULE_PROG_ACTION, prog_27128);
	add_action(MODULE_VERIFY_ACTION, verify_27128);
	add_action(MODULE_TEST_ACTION, test_27128);
    register_chip_end;
    register_chip_begin("/EPROM/28 pin", "27256", "27256", SIZE_27256);
	add_action(MODULE_READ_ACTION, read_27256);
	add_action(MODULE_PROG_ACTION, prog_27256);
	add_action(MODULE_VERIFY_ACTION, verify_27256);
	add_action(MODULE_TEST_ACTION, test_27256);
    register_chip_end;

REGISTER_MODULE_END
