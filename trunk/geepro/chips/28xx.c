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

#define SIZE_2816	KB_SIZE(  2  )
#define SIZE_2832	KB_SIZE(  4  ) 
#define SIZE_2864	KB_SIZE(  8  )
#define SIZE_28128	KB_SIZE( 16  )
#define SIZE_28256	KB_SIZE( 32  )
#define SIZE_28512	KB_SIZE( 64  )
#define SIZE_28F010	KB_SIZE( 128 )
#define SIZE_28F020	KB_SIZE( 256 )
#define SIZE_28F040	KB_SIZE( 512 )
#define SIZE_28F080	MB_SIZE(  1  )

#define SIZE_29F010	KB_SIZE( 128 )
#define SIZE_29F020	KB_SIZE( 256 )
#define SIZE_29F040	KB_SIZE( 512 )
#define SIZE_29F080	MB_SIZE(  1  )

#define SIZE_49F512	KB_SIZE( 64  )
#define SIZE_49F010	KB_SIZE( 128 )
#define SIZE_49F020	KB_SIZE( 256 )
#define SIZE_49F040	KB_SIZE( 512 )

#define TIMEOUT		500
#define TT		1

void read_data_init_flash()
{
//    hw_set_ce( 0 );
    hw_set_oe( 0 );
    hw_set_we( 1 );
}

void write_data_init_flash()
{
//    hw_set_ce( 0 );
    hw_set_oe( 1 );
    hw_set_we( 1 );
    hw_us_delay(1);
}

unsigned char read_data_flash(unsigned int addr)
{
    hw_set_addr( addr );
    hw_us_delay( 1 );
    return hw_get_data();
}

void write_data_flash(unsigned int addr, unsigned char data, unsigned int us)
{
//printf("%x %x\n", addr, data);
    hw_set_oe( 1 );    
    hw_set_addr( addr );
    hw_set_we( 0 );  
    hw_set_data( data );
    hw_us_delay( us );
    hw_set_we( 1 );      
    hw_us_delay( 1 );
}

void init_flash()
{
    hw_set_ce( 1 );
    hw_set_oe( 1 );
    hw_set_we( 1 );
    hw_set_vcc( 500 );
    hw_sw_vcc( 1 );
    hw_ms_delay( 200 );
}

/*********************************************************************************************************/
void read_flash(unsigned int dev_size, unsigned int range)
{
    unsigned int addr;

    init_flash();
    hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop(addr, dev_size, "Reading...")
	put_buffer( addr, read_data_flash( addr ) );
    finish_action();    
}

void verify_flash(unsigned int dev_size, unsigned int range)
{
    char text[256];
    unsigned int addr;
    unsigned char rdata = 0, bdata = 0;

    init_flash();
    hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop(addr, dev_size, "Veryfying..."){
	rdata = read_data_flash( addr );
	bdata = get_buffer( addr );
	break_if( rdata != bdata );
    }
    finish_action();    

    text[0] = 0;
    if( ERROR_VAL )
	sprintf(text, "[WN][TEXT] Memory and buffer differ !!!\n Address = 0x%X\nBuffer=0x%X, Device=0x%X[/TEXT][BR]OK", addr, bdata & 0xff, rdata & 0xff);
    if( rdata >= 0 ){
	show_message(0, ERROR_VAL ? text: "[IF][TEXT] Memory and buffer are consitent[/TEXT][BR]OK", NULL, NULL);    
	ERROR_VAL = 0;
    }
}

void test_blank_flash(unsigned int dev_size, unsigned int range)
{
    char text[256];
    unsigned int addr;
    unsigned char rdata = 0;

    init_flash();
    hw_set_addr_range( range );
    read_data_init_flash();
    progress_loop(addr, dev_size, "Test Blank..."){
	rdata = read_data_flash( addr );
	break_if( rdata != 0xff );
    }
    finish_action();    

    text[0] = 0;
    if( ERROR_VAL )
	sprintf(text, "[WN][TEXT] Memory is dirty !!!\n Address = 0x%X\nDevice=0x%X[/TEXT][BR]OK", addr, rdata & 0xff);
    if( rdata >= 0 ){
	show_message(0, ERROR_VAL ? text: "[IF][TEXT] Memory is clean[/TEXT][BR]OK", NULL, NULL);    
	ERROR_VAL = 0;
    }
}

void sign_flash( char bootlock, int vpp_req )
{
    unsigned char man, id, bl;
    char chip[256], vendor[256], text[1024];
    
    init_flash();
    if( vpp_req ){
	 hw_set_vpp( vpp_req );
	 hw_sw_vpp( 1 );
    }

    write_data_init_flash();
    write_data_flash( 0x5555, 0xaa, 1 );
    write_data_flash( 0x2aaa, 0x55, 1 );
    write_data_flash( 0x5555, 0x90, 1 );

    read_data_init_flash();
    hw_us_delay( 100 );

    man = read_data_flash( 0 );
    id  = read_data_flash( 1 );
    bl  = read_data_flash( 2 );
    finish_action();        

    loockup_signature( "FLASH", man, id, vendor, chip);
    sprintf(text, bootlock ?
	"[IF][TEXT]"
	"Vendor ( 0x%x ): %s\n\n"
	"Chip   ( 0x%x ): %s\n\n"
	"Boot sector protected: %s\n"
	"[/TEXT][BR]OK" :
	"[IF][TEXT]"
	"Vendor ( 0x%x ): %s\n\n"
	"Chip   ( 0x%x ): %s\n\n"
	"[/TEXT][BR]OK",
	man, vendor, 
	id, chip, 
	bl ? "No" : "Yes"
    );

    show_message(0, text, NULL, NULL);
}

/*
void lock_flash_()
{
    unsigned long *lb;
    int i;
    
    lb = checkbox(
	"[TITLE]Boot Block Lock[/TITLE][TYPE:WN]"
	"[CB:2:0: Are you ABSOLUTLY sure ? (Tick if Yes)]"
	"[CB:1:1: Once locked, cannot be unlocked (Untick if Yes)]"
    );
    if( !lb ) return; // resignation by button
    if( !(*lb & 2) ) return; // Not checked
    if( *lb & 1 ) return;

    init_flash();

    // take actual flag
    write_data_flash( 0x5555, 0xaa, 1);
    write_data_flash( 0x2aaa, 0x55, 1);    
    write_data_flash( 0x5555, 0x80, 1);
    write_data_flash( 0x5555, 0xaa, 1);
    write_data_flash( 0x2aaa, 0x55, 1);    
    write_data_flash( 0x5555, 0x40, 1);

    progress_loop(i, 50, "Chip erasing...") hw_ms_delay(20);

    finish_action();        
    hw_ms_delay(200);

    show_dialog("[IF][TEXT]BOOT BLOCK LOCKED[/TEXT][BR]OK","");
}
*/

void clean_28xx_flash(unsigned int vpp_req, unsigned int dev_size, unsigned int range) // PRESTO Algorithm, write 0x00
{
    int addr, n;
    
    init_flash();
    hw_set_addr_range( range );

    if( vpp_req ){
	 hw_set_vpp( vpp_req );
	 hw_sw_vpp( 1 );
    }

    progress_loop(addr, dev_size, "Cleaning chip..."){
	for(n = 0; n < 26; n++){
	    // send command
	    hw_set_oe( 1 );    
	    hw_set_addr( addr );
	    hw_set_we( 0 );  
	    hw_set_data( 0x40 );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 1 );
            // send data
	    hw_set_we( 0 );  
	    hw_set_data( 0x00 );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 10 ); // Time for program
            // veryfication command
	    hw_set_we( 0 );  
	    hw_set_data( 0x0C );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 6 );
	    // read data
	    hw_set_oe( 0 );    
	    if( hw_get_data() == 0) break;	    	    
	}
	break_if( n == 25 ); // Fail after 25 tries
    }
    finish_action();        
    hw_ms_delay(200);
}

void erase_28xx_flash(unsigned int vpp_req, unsigned int dev_size, unsigned int range)
{
    unsigned long *lb;
    int addr, n;
    char k;
    
    lb = checkbox(
	"[TITLE]Erasing chip[/TITLE][TYPE:QS]"
	"[CB:2:0: Are you sure ? (Tick if Yes)]"
	"[CB:1:1: Test blank]"
    );

    if( !lb ) return; // resignation by button
    if( !(*lb & 2) ) return; // Not checked
    // write 0x00 to each cell
    clean_28xx_flash(vpp_req, dev_size, range);

    init_flash();

    if( vpp_req ){
	 hw_set_vpp( vpp_req );
	 hw_sw_vpp( 1 );
    }

    n = 0; k = 1;
    progress_loop(addr, dev_size, "Chip erasing..."){
	for(; n < 26; n++){
	    if( k ){
		hw_set_oe( 1 );    
		// ERASE SET-UP 
		hw_set_we( 0 );  
		hw_set_data( 0x20 );
		hw_us_delay( 1 );
		hw_set_we( 1 );      
		hw_us_delay( 1 );
		hw_set_we( 0 );  
		hw_set_data( 0x20 );
		hw_us_delay( 1 );
		hw_set_we( 1 );      
		hw_ms_delay( 10 );
		k = 0;
	    }
	    // ERASE VERIFY
	    hw_set_addr( addr ); // set address 
	    hw_set_oe( 0 );    
	    hw_us_delay( 6 );
	    if( hw_get_data() == 0xff ) break;
	    k = 1; // repeat erase command
	}
        break_if( n == 25 );
    }
    finish_action();        

    hw_ms_delay(200);
    
    if( (*lb & 1) && !ERROR_VAL) 
	test_blank_flash( dev_size, range );
}

void write_28xx_flash(unsigned int vpp_req, unsigned int dev_size, unsigned int range) // PRESTO Algorithm
{
    unsigned long *lb;
    unsigned char d, data;
    int addr, n;
    
    lb = checkbox(
	"[TITLE]Writing chip[/TITLE][TYPE:QS]"
	"[CB:2:0: Are you sure ? (Tick if Yes)]"
	"[CB:1:1: Verify]"
    );

    if( !lb ) return; // resignation by button
    if( !(*lb & 2) ) return; // Not checked

    init_flash();
    hw_set_addr_range( range );

    if( vpp_req ){
	 hw_set_vpp( vpp_req );
	 hw_sw_vpp( 1 );
    }

    progress_loop(addr, dev_size, "Writing chip..."){
	for(n = 0; n < 26; n++){
	    data = get_buffer( addr );
	    // send command
	    hw_set_oe( 1 );    
	    hw_set_addr( addr );
	    hw_set_we( 0 );  
	    hw_set_data( 0x40 );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 1 );
            // send data
	    hw_set_we( 0 );  
	    hw_set_data( data );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 10 ); // Time for program
            // veryfication command
	    hw_set_we( 0 );  
	    hw_set_data( 0x0C );
	    hw_us_delay( 1 );
	    hw_set_we( 1 );      
	    hw_us_delay( 6 );
	    // read data
	    hw_set_oe( 0 );    
	    d = hw_get_data();
	    if( data == d) break;	    	    
	}
	break_if( n == 25 ); // Fail after 25 tries
    }
    finish_action();        

    hw_ms_delay(200);
    
    if( (*lb & 1) && !ERROR_VAL) 
	verify_flash( dev_size, range );
}




/*********************************************************************************************************/
REGISTER_FUNCTION( read,	28512, flash, SIZE_28512, RANGE_64K );
REGISTER_FUNCTION( verify,	28512, flash, SIZE_28512, RANGE_64K );
REGISTER_FUNCTION( test_blank,	28512, flash, SIZE_28512, RANGE_64K );
REGISTER_FUNCTION( sign,	28512, flash, 0, 1250 );
REGISTER_FUNCTION( erase,	28512, 28xx_flash, 1250, SIZE_28512, RANGE_64K );
REGISTER_FUNCTION( write,	28512, 28xx_flash, 1250, SIZE_28512, RANGE_64K );
/*
REGISTER_FUNCTION( read,	28F010, flash, SIZE_28F010, RANGE_128K );
REGISTER_FUNCTION( verify, 	28F010, flash, SIZE_28F010, RANGE_128K );
REGISTER_FUNCTION( test_blank, 	28F010, flash, SIZE_28F010, RANGE_128K );
REGISTER_FUNCTION( sign,   	28F010, flash, 0, 1250 );
REGISTER_FUNCTION( erase,   	28F010, 28xx_flash, 1250, SIZE_28F010, RANGE_128K );
REGISTER_FUNCTION( write,   	28F010, 28xx_flash, 1250, SIZE_28F010, RANGE_128K );
*/

/*
REGISTER_FUNCTION( read,	28F020, flash, SIZE_28F020, RANGE_256K );
REGISTER_FUNCTION( verify, 	28F020, flash, SIZE_28F020, RANGE_256K );
REGISTER_FUNCTION( test_blank, 	28F020, flash, SIZE_28F020, RANGE_256K );
REGISTER_FUNCTION( sign,   	28F020, flash, 0, 1250 );
REGISTER_FUNCTION( erase,   	28F020, 28xx_flash, 1250, SIZE_28F020, RANGE_256K );
REGISTER_FUNCTION( write,   	28F020, 28xx_flash, 1250, SIZE_28F020, RANGE_256K );
*/

/*
REGISTER_FUNCTION( read,	28F040, flash, SIZE_28F040, RANGE_512K );
REGISTER_FUNCTION( verify, 	28F040, flash, SIZE_28F040, RANGE_512K );
REGISTER_FUNCTION( test_blank, 	28F040, flash, SIZE_28F040, RANGE_512K );
REGISTER_FUNCTION( sign,   	28F040, flash, 0, 1250 );
REGISTER_FUNCTION( erase,   	28F040, 28xx_flash, 1250, SIZE_28F040, RANGE_512K );
REGISTER_FUNCTION( write,   	28F040, 28xx_flash, 1250, SIZE_28F040, RANGE_512K );
*/

//REGISTER_FUNCTION( read,   49F512, flash, SIZE_49F512 );
//REGISTER_FUNCTION( read,   49F010, flash, SIZE_49F010 );
//REGISTER_FUNCTION( read,   49F020, flash, SIZE_49F020 );
//REGISTER_FUNCTION( read,   49F040, flash, SIZE_49F040 );

REGISTER_MODULE_BEGIN( 28xx )

    register_chip_begin("/EEPROM 28Fxx,28Cxx", "28F512", "28512", SIZE_28512);
	add_action(MODULE_READ_ACTION, read_28512);
	add_action(MODULE_VERIFY_ACTION, verify_28512);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_28512);
	add_action(MODULE_SIGN_ACTION, sign_28512);
	add_action(MODULE_ERASE_ACTION, erase_28512);
	add_action(MODULE_WRITE_ACTION, write_28512);
    register_chip_end;
/*
    register_chip_begin("/EEPROM 28Fxx,28Cxx", "28F010", "28512", SIZE_28F010);
	add_action(MODULE_READ_ACTION, read_28F010);
	add_action(MODULE_VERIFY_ACTION, verify_28F010);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_28F010);
	add_action(MODULE_SIGN_ACTION, sign_28F010);
	add_action(MODULE_ERASE_ACTION, erase_28F010);
	add_action(MODULE_WRITE_ACTION, write_28F010);
    register_chip_end;
*/

//    register_chip_begin("/Flash 39SFxx,49Fxx", "39SF512,49F512", "39SF/49F", SIZE_49F512);
//	add_action(MODULE_READ_ACTION, read_49F512);
//	add_action(MODULE_SIGN_ACTION, sign_flash);
//    register_chip_end;

//    register_chip_begin("/Flash 39SFxx,49Fxx", "39SF010,49F010", "39SF/49F", SIZE_49F010);
//	add_action(MODULE_READ_ACTION, read_49F010);
//	add_action(MODULE_SIGN_ACTION, sign_flash);
//    register_chip_end;

//    register_chip_begin("/Flash 39SFxx,49Fxx", "39SF020,49F020", "39SF/49F", SIZE_49F020);
//	add_action(MODULE_READ_ACTION, read_49F020);
//	add_action(MODULE_SIGN_ACTION, sign_flash);
//    register_chip_end;

//    register_chip_begin("/Flash 39SFxx,49Fxx", "39SF040,49F040", "39SF/49F", SIZE_49F040);
//	add_action(MODULE_READ_ACTION, read_49F040);
//	add_action(MODULE_SIGN_ACTION, sign_flash);
//    register_chip_end;

//    register_chip_begin("/EEPROM 28xx/32 pin", "28F080,28F8000,28F8001", "28F080", SIZE_28F080);
//	add_action(MODULE_READ_ACTION, read_28F080);
//	add_action(MODULE_PROG_ACTION, prog_28F080);
//	add_action(MODULE_ERASE_ACTION, erase_28F080);
//	add_action(MODULE_VERIFY_ACTION, verify_28F080);
//	add_action(MODULE_TEST_ACTION, test_28F080);

REGISTER_MODULE_END


