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

/********************************* LOW LEVEL OPERATIONS ******************************************************/

void set_addr_rc_LPC( int row, int column )
{
    hw_set_addr( row );
    hw_sw_vcc(0);
    hw_us_delay(1);    
    hw_set_addr( column );
    hw_sw_vcc(1);
    hw_us_delay(10);    
}

void set_addr_LPC( int addr )
{
    set_addr_rc_LPC( addr & 0x7ff, (addr >> 11) & 0x7ff );
}

void set_data_LPC( unsigned char data )
{
    hw_set_pgm( 0 );
    hw_set_data( data );
    hw_us_delay( 1 );
    hw_set_pgm( 1 );
    hw_us_delay( 10 );
    hw_set_we( 0 );
}

unsigned char get_data_LPC()
{
    unsigned char data;

    hw_set_oe(0);
    hw_us_delay( 1 );
    data = hw_get_data();
    hw_set_oe(1);
    hw_us_delay( 1 );
    return data;
}

/*********************************************************************************************/

void read_LPC(unsigned int dev_size)
{
    unsigned int addr;
    
    TEST_CONNECTION( VOID )
    hw_set_vcc( 330 );

    hw_set_pgm( 1 );
    hw_set_oe( 1 );
    hw_sw_vcc( 1 );
    
    progress_loop(addr, dev_size, "Reading..."){
	set_addr_LPC( addr );
	put_buffer( addr, get_data_LPC() );
    }
    finish_action();    
}

/*********************************************************************************************/

REGISTER_FUNCTION( read,  LF002, LPC, LF002_SIZE);

REGISTER_MODULE_BEGIN(HUB_LPC)
    register_chip_begin("/HUB LPC/SST49LFxxx", "SST49LF002", "HUB_LPC", LF002_SIZE);
	add_action(MODULE_READ_ACTION, read_LF002);
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