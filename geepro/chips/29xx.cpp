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

static geepro *gep=NULL;

MODULE_IMPLEMENTATION

static void init_flash5V()
{
    gep->hw_set_ce(1);
    gep->hw_set_oe(1);
    gep->hw_set_we(1);
    gep->hw_set_vcc(500);
    gep->hw_sw_vcc(1);
    gep->hw_ms_delay(200);
}

static void read_data_init_flash()
{
    gep->hw_set_ce(0);
    gep->hw_set_oe(0);
    gep->hw_set_we(1);
}

static char read_data_flash(int addr)
{
    gep->hw_set_addr( addr );
    gep->hw_us_delay(1);
    return gep->hw_get_data();
}

static void read_flash29C(int dev_size, int range)
{
    int addr;
    init_flash5V();
    gep->hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop( addr, dev_size, "Reading...")
	put_buffer( addr, read_data_flash(addr) );
    finish_action();
}

static void verify_flash29C(int dev_size, int range)
{
    char text[256];
    int addr;
    unsigned char rdata = 0, bdata = 0, tries = 0;
    
    init_flash5V();
    gep->hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop( addr, dev_size, "Verify..."){
	rdata = read_data_flash( addr );
	bdata = get_buffer( addr );
	if( rdata != bdata ){
	    if( tries > 8 ) break_if( 1 );
	    if( addr ) addr--; else break_if( 1 );
	    tries++;
	} else {
	    tries = 0;
	}
    }
    finish_action();
    text[0] = 0;
    if( ERROR_VAL )
	sprintf( text, "[WN][TEXT] Memory and buffer differ !!!\n Address=0x%X\nBuffer=0x%X, Device=0x%X[/TEXT][BR]OK", addr, bdata & 0xff, rdata & 0xff);
    show_message( 0, ERROR_VAL ? text : "[IF][TEXT] Memory and buffer are consistent[/TEXT][BR]OK", NULL, NULL );
    ERROR_VAL = 0;
}

static void test_blank_flash29C(int dev_size, int range)
{
    char text[256];
    int addr;
    unsigned char rdata = 0;
    init_flash5V();
    gep->hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop( addr, dev_size, "Reading..."){
	rdata = read_data_flash( addr );
	break_if( rdata != 0xff );
    }
    finish_action();
    if( ERROR_VAL )
	sprintf( text, "[WN][TEXT] Memory and buffer differ !!!\n Address=0x%X\nDevice=0x%X[/TEXT][BR]OK", addr, rdata & 0xff);
    show_message( 0, ERROR_VAL ? text : "[IF][TEXT] Memory and buffer are consistent[/TEXT][BR]OK", NULL, NULL );
    ERROR_VAL = 0;
}

void load_byte( int addr, char data )
{
    gep->hw_set_we(1);    
    gep->hw_set_oe(1); 
    gep->hw_set_ce(0); // ignore in willem
    gep->hw_set_addr( addr );
    gep->hw_set_we(0);  // latch address on falling edge
    gep->hw_set_data( data );
    gep->hw_set_we(1);  // latch data on rising edge
    gep->hw_set_ce(1); // ignore in willem
    gep->hw_us_delay(200);
}


static void send_cmd_flash29C( char cmd )
{
    load_byte( 0x5555, 0xAA );
    load_byte( 0x2AAA, 0x55 );
    load_byte( 0x5555, cmd  );
    gep->hw_ms_delay( 10 );
}

static void sign_flash29C()
{
    char text[256];
    int addr;
    unsigned char rdata = 0, manuf = 0, device = 0;
    
    init_flash5V();

    send_cmd_flash29C( 0x90 );
	manuf  = read_data_flash( 0x00000 );
	device = read_data_flash( 0x00001 );
    send_cmd_flash29C( 0xf0 );
printf("--->%x, %x\n", manuf, device);
    finish_action();
}


REGISTER_FUNCTION( read,	29C256, flash29C, SIZE_32K, RANGE_32K );
REGISTER_FUNCTION( verify,	29C256, flash29C, SIZE_32K, RANGE_32K );
REGISTER_FUNCTION( test_blank,	29C256, flash29C, SIZE_32K, RANGE_32K );
REGISTER_FUNCTION( sign,	29C256, flash29C);

REGISTER_FUNCTION( read,	29EE020, flash29C, SIZE_256K, RANGE_256K );
REGISTER_FUNCTION( verify,	29EE020, flash29C, SIZE_256K, RANGE_256K );
REGISTER_FUNCTION( test_blank,	29EE020, flash29C, SIZE_256K, RANGE_256K );
REGISTER_FUNCTION( sign,	29EE020, flash29C);

/*
    register_chip_begin("/EEPROM 29EExx", "SST29EE010", "29x010", SIZE_29EE010);
	add_action(MODULE_READ_ACTION, read_29EE010);
	add_action(MODULE_VERIFY_ACTION, verify_29EE010);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_29EE010);
	add_action(MODULE_SIGN_ACTION, sign_29EE010);
//	add_action(MODULE_ERASE_ACTION, erase_29EE010);
//	add_action(MODULE_WRITE_ACTION, write_29EE010);
    register_chip_end;
*/

REGISTER_MODULE_BEGIN( 29xx )
    register_chip_begin("/Flash 29Cxxx", "AT29C256", "29Cxx", SIZE_32K);
	add_action(MODULE_READ_ACTION, read_29C256);
	add_action(MODULE_VERIFY_ACTION, verify_29C256);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_29C256);
	add_action(MODULE_SIGN_ACTION, sign_29C256);
    register_chip_end;

    register_chip_begin("/Flash 29EExxx", "SST29EE020", "29x010", SIZE_256K);
	add_action(MODULE_READ_ACTION, read_29EE020);
	add_action(MODULE_VERIFY_ACTION, verify_29EE020);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_29EE020);
	add_action(MODULE_SIGN_ACTION, sign_29EE020);
    register_chip_end;


/*
// 29Fxxx
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F64", "zupa", SIZE_29F64);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F128", "zupa", SIZE_29F128);
    register_chip_end;
//    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F256", "zupa", SIZE_);
//    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F512", "zupa", SIZE_29F512);
    register_chip_end;                                                    
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F010", "zupa", SIZE_29F010);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F020", "zupa", SIZE_29F020);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "29F040", "zupa", SIZE_29F040);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "M29F512B", "zupa", SIZE_M29F512);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "M29F010B", "zupa", SIZE_M29F010);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fxxx", "M29F040B", "zupa", SIZE_M29F040);
    register_chip_end;
// 39/49Fxxx
    register_chip_begin("/Flash (29,39,49)Fxxx/(39,49)Fxxx", "39SF,49F512", "zupa", SIZE_49F512);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(39,49)Fxxx", "39SF,49F010", "zupa", SIZE_49F010);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(39,49)Fxxx", "39SF,49F020", "zupa", SIZE_49F020);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(39,49)Fxxx", "39SF,49F040", "zupa", SIZE_49F040);
    register_chip_end;
// MXIC 29FFxxx
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F001-T", "zupa", SIZE_MX29F001);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F002-T", "zupa", SIZE_MX29F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F004-T", "zupa", SIZE_MX29F004);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F001-B", "zupa", SIZE_MX29F001);
    register_chip_end;                                                                  
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F002-B", "zupa", SIZE_MX29F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F004-B", "zupa", SIZE_MX29F004);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F040", "zupa", SIZE_MX29F040);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/MXIC 29Fxxx", "MX29F1610", "zupa", SIZE_MX29F1610);
    register_chip_end;
// 29/49F00xx
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "29F001T", "zupa", SIZE_29F001);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "29F002", "zupa", SIZE_29F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "29F002NT", "zupa", SIZE_29F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "AT49F001", "zupa", SIZE_AT49F001);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "AT49F001(N)T", "zupa", SIZE_AT49F001);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "AT49F002", "zupa", SIZE_AT49F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "AT49F002(N)T", "zupa", SIZE_AT49F002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/(29,49)F00xx", "AT49F008A", "zupa", SIZE_AT49F008);
    register_chip_end;
// 29Fx00 8/16bit
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "MX29F100", "zupa", SIZE_AM29F100);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "Am29F200", "zupa", SIZE_AM29F200);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "Am29F400", "zupa", SIZE_AM29F400);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "Am29F800", "zupa", SIZE_AM29F800);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "Am29F160", "zupa", SIZE_AM29F160);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/29Fx00 (8,16bit)", "Am29F320", "zupa", SIZE_AM29F320);
    register_chip_end;
// M29F0x0 TSOP40    
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV002", "zupa", SIZE_MBM29LV002);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV004", "zupa", SIZE_MBM29LV004);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV004x", "zupa", SIZE_MBM29LV004);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV008x", "zupa", SIZE_MBM29LV008);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV016x", "zupa", SIZE_MBM29LV016);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "MBM29LV032x", "zupa", SIZE_MBM29LV032);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "HY29F080", "zupa", SIZE_HY29F080);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "M29F080", "zupa", SIZE_M29F080);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "M29F016", "zupa", SIZE_M29F016);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "M29F032", "zupa", SIZE_M29F032);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "Am29F080", "zupa", SIZE_AM29F080);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "Am29F016", "zupa", SIZE_AM29F016);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "Am29F032", "zupa", SIZE_AM29F032);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "Am29F032 (32Mb)", "zupa", SIZE_Am29F032mb);
    register_chip_end;
    register_chip_begin("/Flash (29,39,49)Fxxx/M29F0x0 TSOP40", "AT49F080", "zupa", SIZE_AT49F080);
    register_chip_end;
*/
REGISTER_MODULE_END

