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

static geepro *gep=NULL;

#define MODULE_TRANSMIT_ACTION MODULE_WRITE_ACTION
#define MODULE_RESET_ACTION MODULE_VERIFY_ACTION
#define MODULE_SETTINGS_ACTION MODULE_READ_ACTION

MODULE_IMPLEMENTATION

#define SIZE_2716	(2048 * 1)
#define SIZE_2732       (2048 * 2)
#define SIZE_2764       (2048 * 4)
#define SIZE_27128      (2048 * 8)
#define SIZE_27256      (2048 * 16)
#define SIZE_27512      (2048 * 32)
#define SIZE_271024     (2048 * 64)
#define SIZE_272048     (2048 * 128)

static void send_data(int size)
{
    int addr;
    
    hw_set_pgm( 0 );
    hw_set_addr_range( (size - 1) >> 10 );
    hw_sw_dpsw( 1 );        
    progress_loop(addr, size, "Transmission..."){
	hw_set_addr( addr );
	hw_set_data( get_buffer( addr ) );
    }
    hw_sw_dpsw( 0 );
    finish_action();
    hw_set_pgm( 1 );
}

static void send_rst()
{
    hw_rst_addr();
}

static void rst_polarity(int val, void *ptr, int id)
{
    hw_set_cs( val );    
}

static void rst_time(int val, void *ptr, int id)
{
    hw_set_we( val );
}

static void settings_y()
{
    dialog_start("Reset pin settings", 300, 200);
	spin_add("Polarity", 0, 1, hw_get_do(), rst_polarity, 0, NULL);
	slider_add("Time [ms]", 0, 2000, hw_get_data(), rst_time, 0, NULL);
    dialog_end();
}

REGISTER_FUNCTION( send, 16, data, SIZE_2716);
REGISTER_FUNCTION( send, 32, data, SIZE_2732);
REGISTER_FUNCTION( send, 64, data, SIZE_2764);
REGISTER_FUNCTION( send, 128, data, SIZE_27128);
REGISTER_FUNCTION( send, 256, data, SIZE_27256);
REGISTER_FUNCTION( send, 512, data, SIZE_27512);
REGISTER_FUNCTION( send, 1024, data, SIZE_271024);
REGISTER_FUNCTION( send, 2048, data, SIZE_272048);
REGISTER_FUNCTION( send, reset, rst);
REGISTER_FUNCTION( settings, x, y);

REGISTER_MODULE_BEGIN( PROM )

//    rst_polarity( 1, 0,0 );
//    rst_time( 100, 0, 0 );
    
    register_chip_begin("/simulators/momik/memsim 8bit","2x16", "2x16");
	add_buffer("Buffer", SIZE_2716);
	add_action(MODULE_TRANSMIT_ACTION, send_16);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x32", "2x32");
	add_buffer("Buffer", SIZE_2732);
	add_action(MODULE_TRANSMIT_ACTION, send_32);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x64", "2x64");
	add_buffer("Buffer", SIZE_2764);
	add_action(MODULE_TRANSMIT_ACTION, send_64);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x128", "2x128");
	add_buffer("Buffer", SIZE_27128);
	add_action(MODULE_TRANSMIT_ACTION, send_128);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x256", "2x256");
	add_buffer("Buffer", SIZE_27256);
	add_action(MODULE_TRANSMIT_ACTION, send_256);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x512", "2x512");
	add_buffer("Buffer", SIZE_27512);
	add_action(MODULE_TRANSMIT_ACTION, send_512);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x1024", "2x1024");
	add_buffer("Buffer", SIZE_271024);
	add_action(MODULE_TRANSMIT_ACTION, send_1024);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

    register_chip_begin("/simulators/momik/memsim 8bit","2x2048", "2x2048");
	add_buffer("Buffer", SIZE_272048);
	add_action(MODULE_TRANSMIT_ACTION, send_2048);
	add_action(MODULE_RESET_ACTION, send_reset);
	add_action(MODULE_SETTINGS_ACTION, settings_x);
    register_chip_end;

REGISTER_MODULE_END
