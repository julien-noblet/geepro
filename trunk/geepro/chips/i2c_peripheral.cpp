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

#define M52749FP_SIZE	10
#define M52749FP_ADDR	0x88

static void send_m52749(char func, char val)
{
    hw_ms_delay( 5 );
    start_i2c();    
    send_byte_i2c( M52749FP_ADDR );
    if( wait_ack_i2c() ) throw "Chip select address";
    send_byte_i2c( func ); // function select
    if( wait_ack_i2c() ) throw "Function select";
    send_byte_i2c( val ); // value
    if( wait_ack_i2c() ) throw "Value set";
    stop_i2c();        
    hw_ms_delay( 5 );
}

static void m52749_value_set(int val, void *ptr, int ipar)
{
    try{
    init_i2c();
    switch( ipar ){
	case 0: put_buffer(0, val); send_m52749( 0,  get_buffer(0) ); break; // Main contrast
	case 1: put_buffer(1, val); send_m52749( 1,  get_buffer(1) ); break; // Sub contrast R
	case 2: put_buffer(2, val); send_m52749( 2,  get_buffer(2) ); break; // Sub contrast G
	case 3: put_buffer(3, val); send_m52749( 3,  get_buffer(3) ); break; // Sub contrast B
	case 4: put_buffer(4, val); send_m52749( 4,  get_buffer(4) ); break; // OSD level (4bit)
	// 5 not existed
	case 6: put_buffer(5, val); send_m52749( 6,  get_buffer(5) ); break; // D/A out 1
	case 7: put_buffer(6, val); send_m52749( 7,  get_buffer(6) ); break; // D/A out 2
	case 8: put_buffer(7, val); send_m52749( 8,  get_buffer(7) ); break; // D/A out 3
	case 9: put_buffer(8, val); send_m52749( 9,  get_buffer(8) ); break; // D/A out 4
	case 10: put_buffer(9, val); send_m52749( 10, get_buffer(9) ); break;// D/A out 5
    }
    finish_action();
    } catch (const char *x){
	message("[WN][TEXT]-= TIMEOUT! =-\n %s \n No ACK bit received [/TEXT][BR]OK", x);    
    }
}

void write_m52749_()
{
    // send settings to chip    
    dialog_start("M52749 Control", 300, 200);
    	    frame_start( "Main control" );
    	    slider_add("Contrast", 0, 255, get_buffer(0), m52749_value_set, 0, NULL);
	    slider_add("OSD level", 0, 15, get_buffer(4), m52749_value_set, 4, NULL);
	frame_end();
	frame_start("Sub Contrast");
	    slider_add("R", 0, 255, get_buffer(1), m52749_value_set, 1, NULL);    
	    slider_add("G", 0, 255, get_buffer(2), m52749_value_set, 2, NULL);    
	    slider_add("B", 0, 255, get_buffer(3), m52749_value_set, 3, NULL);    
	frame_end();
	frame_start("DAC");
	    slider_add("D/A out 1", 0, 255, get_buffer(5), m52749_value_set, 6, NULL);    
	    slider_add("D/A out 2", 0, 255, get_buffer(6), m52749_value_set, 7, NULL);    
	    slider_add("D/A out 3", 0, 255, get_buffer(7), m52749_value_set, 8, NULL);    
	    slider_add("D/A out 4", 0, 255, get_buffer(8), m52749_value_set, 9, NULL);    
	    slider_add("D/A out 5", 0, 255, get_buffer(9), m52749_value_set, 10, NULL);
	frame_end();
    dialog_end();
}

REGISTER_FUNCTION( write, m52749fp, m52749_);

REGISTER_MODULE_BEGIN(I2c_peripheral)

    register_chip_begin("/I2c peripheral/Video", "M52749FP", "I2C_PERIPHERAL", M52749FP_SIZE);
	add_action(MODULE_PROG_ACTION, write_m52749fp);
    register_chip_end;

REGISTER_MODULE_END
