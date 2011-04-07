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

#define SIZE_2816	2048
#define SIZE_2832	4096
#define SIZE_2864	8192
#define SIZE_28128	16384
#define SIZE_28256	32768
#define SIZE_28512	65536
#define SIZE_28F010	131072
#define SIZE_28F020	262144
#define SIZE_28F040	524288
#define SIZE_28F080	1048576

char read_byte_2816( int addr )
{
    char data;
    set_address( addr );
    oe(0, 1);
    data = hw_get_data();
    oe(1, 1);
    return data;
}

void read_2816_(int size, char ce)
{
    int addr;
    unsigned char data;
    TEST_CONNECTION( VOID )
    start_action(0, ce);
    progress_loop(addr, size, "Reading data"){
	data = read_byte_2816( addr );
	put_buffer(addr, data);
    }    
    finish_action();
}

char test_2816_(int size, char ce, char silent)
{
    int addr;
    unsigned char rdata;
    char text[256];
    
    TEST_CONNECTION( 1 )
    start_action(0, ce);
    progress_loop(addr, size, "Test blank"){
	rdata = read_byte_2816( addr );
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

void verify_2816_(int size, char ce)
{
    int addr;
    unsigned char rdata, wdata;
    char text[256];
    
    start_action(0, ce);
    progress_loop(addr, size, "Verifying data"){
	rdata = read_byte_2816( addr );
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

/**********************************************************************************************/
void read_signature( char *man, char *dev)
{

}

void signat_read()
{
    char manufact = 0, dev_code = 0; 
    char text[256];
    
    TEST_CONNECTION( VOID )

    read_signature( &manufact, &dev_code );

    sprintf(text, "[IF][TEXT] Manufacturer code = 0x%X%X\nDevice code = 0x%X%X\n%s[/TEXT][BR]OK",
	to_hex( manufact, 1), to_hex( manufact, 0),
	to_hex( dev_code, 1), to_hex( dev_code, 0),
	take_signature_name( manufact * 256 + dev_code)
    );
    show_message( 0, text, NULL, NULL );        
}

char read_eprom(int addr, char oe_vpp)
{
    char data;

    TEST_CONNECTION( 0 )

    set_address(addr);
    hw_delay(5); // tPHQX + tQXGL
    oe(0, 5);
    if(oe_vpp){
        hw_sw_vpp(0);
        ce(0, 1);
    }
    data = hw_get_data();
    if(oe_vpp){
      ce(1, 1);
      hw_sw_vpp(1);
    }
    oe(1, 2);
    return data;
}

void erase_erase(int size, char ce_pgm, char oe_vpp)
{
    
}

void prog_eprom(int size, char ce_pgm, char oe_vpp)
{
/*
    int addr, x;
    unsigned char rdata, wdata;
    unsigned long *lb;
    char text[256];
    char toggle;
    
    TEST_CONNECTION( VOID )
    lb = checkbox(
	"[TITLE]Chip burning[/TITLE][TYPE:QS]"
	"[CB:1:0:Automatic programming]"
	"[CB:8:1: Test blank]"
	"[CB:16:1: Verify]"
    );
    if( !lb ) return;
    x = *lb & 0x07;
    if( x != 1 ) return; // missing algorithm choice
    if( *lb & 0x08 )	// test blank
	if(test_2816_(size, !ce_pgm, 1)) return;
    if( !ce_pgm ) hw_pragma( PRAGMA_CE_EQ_PGM ); // if set, ignoring ce() command for programmers like willem, PROG() will be CE
    hw_sw_vcc(1);    
    hw_sw_vpp(1);
    ce(ce_pgm, 1);
    toggle = 0;
    progress_loop(addr, size, "Writing"){
	if(!toggle){
	    wdata = get_buffer( addr );
	    write_command( 0x40 );
	    write_byte(addr, data);
	    toggle = 1;
	}
	if(toggle_bit_chk()) toggle = 0; else addr--;
	break_if(verify_byte() != wdata);
    }
    hw_sw_vpp(0);
    hw_sw_vcc(0);    

    if( *lb & 0x10 )
	verify_2816_( size, !ce_pgm );
    hw_pragma( 0 );
*/
}

/*********************************************************************************************************/

/* 2816 */
//REGISTER_FUNCTION( read,   2816,  2816_, SIZE_2816, 1 );
//REGISTER_FUNCTION( verify, 2816,  2816_, SIZE_2816, 1 );
//REGISTER_FUNCTION( test,   2816,  2816_, SIZE_2816, 1, 0 );
//REGISTER_FUNCTION( prog,   2816,  eprom, SIZE_2816, 0, 0 );
//REGISTER_FUNCTION( erase,  2816,  erase, SIZE_2816, 0, 0 );
/* 2864 */
//REGISTER_FUNCTION( read,   2864,  2816_, SIZE_2864, 1 );
//REGISTER_FUNCTION( verify, 2864,  2816_, SIZE_2864, 1 );
//REGISTER_FUNCTION( test,   2864,  2816_, SIZE_2864, 1, 0 );
//REGISTER_FUNCTION( prog,   2864,  eprom, SIZE_2864, 0, 0 );
//REGISTER_FUNCTION( erase,  2864,  erase, SIZE_2864, 0, 0 );
/* 28128 */
//REGISTER_FUNCTION( read,   28128, 2816_, SIZE_28128, 1 );
//REGISTER_FUNCTION( verify, 28128, 2816_, SIZE_28128, 1 );
//REGISTER_FUNCTION( test,   28128, 2816_, SIZE_28128, 1, 0 );
//REGISTER_FUNCTION( prog,   28128, eprom, SIZE_28128, 0, 0 );
//REGISTER_FUNCTION( erase,  28128, erase, SIZE_28128, 0, 0 );
/* 28256 */
//REGISTER_FUNCTION( read,   28256, 2816_, SIZE_28256, 0 );
//REGISTER_FUNCTION( verify, 28256, 2816_, SIZE_28256, 0 );
//REGISTER_FUNCTION( test,   28256, 2816_, SIZE_28256, 0, 0 );
//REGISTER_FUNCTION( prog,   28256, eprom, SIZE_28256, 1, 0 );
//REGISTER_FUNCTION( erase,  28256, erase, SIZE_28256, 0, 0 );
/* 28512 */
REGISTER_FUNCTION( read,   28512, 2816_, SIZE_28512, 0 );
REGISTER_FUNCTION( verify, 28512, 2816_, SIZE_28512, 0 );
REGISTER_FUNCTION( test,   28512, 2816_, SIZE_28512, 0, 0 );
REGISTER_FUNCTION( prog,   28512, eprom, SIZE_28512, 1, 1 );
REGISTER_FUNCTION( erase,  28512, erase, SIZE_28512, 0, 0 );
REGISTER_FUNCTION( signat,  28512, read, SIZE_28512, 0, 0 );
/* 28F010 */
//REGISTER_FUNCTION( read,   28F010, 2816_, SIZE_28F010, 0 );
//REGISTER_FUNCTION( verify, 28F010, 2816_, SIZE_28F010, 0 );
//REGISTER_FUNCTION( test,   28F010, 2816_, SIZE_28F010, 1, 0 );
//REGISTER_FUNCTION( prog,   28F010, eprom, SIZE_28F010, 0, 0 );
//REGISTER_FUNCTION( erase,  28F010, erase, SIZE_28F010, 0, 0 );
/* 28F020 */
//REGISTER_FUNCTION( read,   28F020, 2816_, SIZE_28F020, 0 );
//REGISTER_FUNCTION( verify, 28F020, 2816_, SIZE_28F020, 0 );
//REGISTER_FUNCTION( test,   28F020, 2816_, SIZE_28F020, 1, 0 );
//REGISTER_FUNCTION( prog,   28F020, eprom, SIZE_28F020, 0, 0 );
//REGISTER_FUNCTION( erase,  28F020, erase, SIZE_28F020, 0, 0 );
/* 28F040 */
//REGISTER_FUNCTION( read,   28F040, 2816_, SIZE_28F040, 0 );
//REGISTER_FUNCTION( verify, 28F040, 2816_, SIZE_28F040, 0 );
//REGISTER_FUNCTION( test,   28F040, 2816_, SIZE_28F040, 0, 0 );
//REGISTER_FUNCTION( prog,   28F040, eprom, SIZE_28F040, 1, 0 );
//REGISTER_FUNCTION( erase,  28F040, erase, SIZE_28F040, 0, 0 );
/* 28F080 */
//REGISTER_FUNCTION( read,   28F080, 2816_, SIZE_28F080, 0 );
//REGISTER_FUNCTION( verify, 28F080, 2816_, SIZE_28F080, 0 );
//REGISTER_FUNCTION( test,   28F080, 2816_, SIZE_28F080, 0, 0 );
//REGISTER_FUNCTION( prog,   28F080, eprom, SIZE_28F080, 1, 1 );
//REGISTER_FUNCTION( erase,  28F080, erase, SIZE_28F080, 0, 0 );
/********************************************************************************************/
REGISTER_MODULE_BEGIN( 28xx )
/* 24 PIN EEPROM */
//    register_chip_begin("/EEPROM 28xx/24 pin", "AT28C16", "28168", SIZE_2816);
//	add_action(MODULE_READ_ACTION, read_2816);
//	add_action(MODULE_PROG_ACTION, prog_2816);
//	add_action(MODULE_ERASE_ACTION, erase_2816);
//	add_action(MODULE_VERIFY_ACTION, verify_2816);        
//	add_action(MODULE_TEST_ACTION, test_2816);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/24 pin", "2832", "2832", SIZE_2832);
//	add_action(MODULE_READ_ACTION, read_2864);
//	add_action(MODULE_PROG_ACTION, prog_2864);
//	add_action(MODULE_ERASE_ACTION, erase_2864);
//	add_action(MODULE_VERIFY_ACTION, verify_2864);
//	add_action(MODULE_TEST_ACTION, test_2864);
//    register_chip_end;
/* 28 PIN EPROM */
//    register_chip_begin("/EEPROM 28xx/28 pin", "2864", "2864_128", SIZE_2864);
//	add_action(MODULE_READ_ACTION, read_2864);
//	add_action(MODULE_PROG_ACTION, prog_2864);
//	add_action(MODULE_ERASE_ACTION, erase_2864);
//	add_action(MODULE_VERIFY_ACTION, verify_2864);
//	add_action(MODULE_TEST_ACTION, test_2864);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/28 pin", "28128", "2864_128", SIZE_28128);
//	add_action(MODULE_READ_ACTION, read_28128);
//	add_action(MODULE_PROG_ACTION, prog_28128);
//	add_action(MODULE_ERASE_ACTION, erase_28128);
//	add_action(MODULE_VERIFY_ACTION, verify_28128);
//	add_action(MODULE_TEST_ACTION, test_28128);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/28 pin", "28256", "28256", SIZE_28256);
//	add_action(MODULE_READ_ACTION, read_28256);
//	add_action(MODULE_PROG_ACTION, prog_28256);
//	add_action(MODULE_ERASE_ACTION, erase_28256);
//	add_action(MODULE_VERIFY_ACTION, verify_28256);
//	add_action(MODULE_TEST_ACTION, test_28256);
//    register_chip_end;
/* 32 PIN EPROM */
    register_chip_begin("/EEPROM 28xx/32 pin", "28F512", "28512", SIZE_28512);
	add_action(MODULE_READ_ACTION, read_28512);
	add_action(MODULE_PROG_ACTION, prog_28512);
	add_action(MODULE_ERASE_ACTION, erase_28512);
	add_action(MODULE_VERIFY_ACTION, verify_28512);
	add_action(MODULE_TEST_ACTION, test_28512);
	add_action(MODULE_SIGN_ACTION, signat_28512);
    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/32 pin", "28F010,28F1000,28F1001", "28F010", SIZE_28F010);
//	add_action(MODULE_READ_ACTION, read_28F010);
//	add_action(MODULE_PROG_ACTION, prog_28F010);
//	add_action(MODULE_ERASE_ACTION, erase_28F010);
//	add_action(MODULE_VERIFY_ACTION, verify_28F010);
//	add_action(MODULE_TEST_ACTION, test_28F010);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/32 pin", "28F020,28F2000,28F2001", "28F010", SIZE_28F020);
//	add_action(MODULE_READ_ACTION, read_28F020);
//	add_action(MODULE_PROG_ACTION, prog_28F020);
//	add_action(MODULE_ERASE_ACTION, erase_28F020);
//	add_action(MODULE_VERIFY_ACTION, verify_28F020);
//	add_action(MODULE_TEST_ACTION, test_28F020);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/32 pin", "28F040,28F4000,28F4001", "28F040", SIZE_28F040);
//	add_action(MODULE_READ_ACTION, read_28F040);
//	add_action(MODULE_PROG_ACTION, prog_28F040);
//	add_action(MODULE_ERASE_ACTION, erase_28F040);
//	add_action(MODULE_VERIFY_ACTION, verify_28F040);
//	add_action(MODULE_TEST_ACTION, test_28F040);
//    register_chip_end;
//    register_chip_begin("/EEPROM 28xx/32 pin", "28F080,28F8000,28F8001", "28F080", SIZE_28F080);
//	add_action(MODULE_READ_ACTION, read_28F080);
//	add_action(MODULE_PROG_ACTION, prog_28F080);
//	add_action(MODULE_ERASE_ACTION, erase_28F080);
//	add_action(MODULE_VERIFY_ACTION, verify_28F080);
//	add_action(MODULE_TEST_ACTION, test_28F080);
//    register_chip_end;
/* 40 PIN 16bit EEPROM */
/* 42 PIN 16bit EEPROM */
REGISTER_MODULE_END
