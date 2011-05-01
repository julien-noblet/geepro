/* $Revision: 1.5 $ */
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

#define LF002_SIZE	KB_SIZE( 256 )
#define LF003_SIZE	KB_SIZE( 384 )
#define LF004_SIZE	KB_SIZE( 512 )
#define LF008_SIZE	MB_SIZE( 1 )

#define LF002_START	0x00000
#define LF003_START	0x20000
#define LF004_START	0x00000
#define LF008_START	0x00000

#define TT			50	// cycle time

#define WE_HUB( state )		hw_set_ce( state )
#define OE_HUB( state )		hw_set_oe( state )
#define RC_HUB( state )		hw_sw_vcc( state ) // as RC signal in adapter
#define VCC_HUB( state )	hw_sw_vpp( state ) // as VCC in adapter

/********************************* LOW LEVEL OPERATIONS ******************************************************/
void init_HUB()
{
    hw_set_vcc( 500  );
    hw_set_vpp( 1200 );
    WE_HUB( 1 );
    OE_HUB( 1 );    
    RC_HUB( 1 );    
    VCC_HUB( 1 );    
    hw_ms_delay(200); // time for reset
}

void write_data_HUB( unsigned int addr, unsigned char data)
{
    hw_set_addr( addr & 0x7ff );
    hw_us_delay( TT );    
    RC_HUB( 0 );	 // store low 11 bits of address
    OE_HUB( 1 );    
    WE_HUB( 0 );
    hw_us_delay( TT );
    hw_set_addr( (addr >> 11) & 0x7ff );
    RC_HUB( 1 );	 // store high 11 bits of address    
    hw_set_data( data ); // set data
    hw_us_delay( TT );
    WE_HUB( 1 );	// store data
    hw_us_delay( TT );
}

unsigned char read_data_HUB( unsigned int addr)
{
    unsigned char data = 0;

    RC_HUB( 1 );	
    OE_HUB( 1 );    
    WE_HUB( 1 );
    hw_set_addr( addr & 0x7ff ); // low 11 bits
    RC_HUB( 0 );	 	 // store low 11 bits of address
    hw_us_delay( TT );
    hw_set_addr( (addr >> 11) & 0x7ff );
    RC_HUB( 1 );	 // store high 11 bits of address    
    hw_us_delay( TT );
    OE_HUB( 0 );    
    hw_us_delay( TT );
    data = hw_get_data();
    hw_us_delay( TT );
    OE_HUB( 1 );        
    return data;
}

/*********************************************************************************************/

void read_HUB(unsigned int dev_size, unsigned int start)
{
    unsigned int addr;

    TEST_CONNECTION( VOID )

    init_HUB();
    progress_loop(addr, dev_size, "Reading...")
	put_buffer( addr, read_data_HUB( start + addr ) );
    finish_action();    
}

void sign_HUB()
{
    unsigned char man, id, tbl;
    char chip[256], vendor[256], text[1024];

    TEST_CONNECTION( VOID )
    init_HUB();

    write_data_HUB( 0x5555, 0xaa);
    write_data_HUB( 0x2aaa, 0x55);    
    write_data_HUB( 0x5555, 0x90);

    man = read_data_HUB( 0 );
    id  = read_data_HUB( 1 );    
    tbl = read_data_HUB( 2 ) & 1;   // if 0 boot block is locked for writing

    finish_action();        

    loockup_signature( "HUB/LPC", man, id, vendor, chip);

    sprintf(text, "[IF][TEXT]"
	"Vendor ( 0x%x ): %s\n\n"
	"Chip   ( 0x%x ): %s\n\n"
	"Boot sector protected: %s\n"
	"[/TEXT][BR]OK", 
	man, vendor, 
	id, chip, 
	tbl ? "No" : "Yes"
    );
    show_message(0, text, NULL, NULL);
}

/*********************************************************************************************/
REGISTER_FUNCTION( sign,  LF, HUB );

REGISTER_FUNCTION( read,  LF002, HUB, LF002_SIZE, LF002_START);

REGISTER_FUNCTION( read,  LF003, HUB, LF003_SIZE, LF003_START);

REGISTER_FUNCTION( read,  LF004, HUB, LF004_SIZE, LF004_START);

REGISTER_FUNCTION( read,  LF008, HUB, LF008_SIZE, LF008_START);

REGISTER_MODULE_BEGIN(HUB_HUB)

    register_chip_begin("/HUB LPC/SST49LFxxx", "SST49LF002", "HUB_LPC", LF002_SIZE);
	add_action(MODULE_READ_ACTION, read_LF002);
	add_action(MODULE_SIGN_ACTION, sign_LF);
    register_chip_end;

    register_chip_begin("/HUB LPC/SST49LFxxx", "SST49LF003", "HUB_LPC", LF003_SIZE);
	add_action(MODULE_READ_ACTION, read_LF003);
	add_action(MODULE_SIGN_ACTION, sign_LF);
    register_chip_end;

    register_chip_begin("/HUB LPC/SST49LFxxx", "SST49LF004", "HUB_LPC", LF004_SIZE);
	add_action(MODULE_READ_ACTION, read_LF004);
	add_action(MODULE_SIGN_ACTION, sign_LF);
    register_chip_end;

    register_chip_begin("/HUB LPC/SST49LFxxx", "SST49LF008", "HUB_LPC", LF008_SIZE);
	add_action(MODULE_READ_ACTION, read_LF008);
	add_action(MODULE_SIGN_ACTION, sign_LF);
    register_chip_end;


REGISTER_MODULE_END

/*
    lb = checkbox(
	"[TITLE]Writing chip[/TITLE][TYPE:QS]"
	"[CB:2:0: Are you sure ? (Tick if Yes)]"
	"[CB:1:1: Verify after process]"
    );

    if( !lb ) return; // resignation by button
    if( !(*lb & 2) ) return; // Not checked
*/

