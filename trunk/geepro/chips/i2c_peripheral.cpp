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

static geepro *gep= NULL; // temp

MODULE_IMPLEMENTATION

#define M52749FP_SIZE	10
#define M52749FP_ADDR	0x88
#define M62392FP_SIZE	13

// -------------------------------------------------------- M52749 --------

static void send_m52749(char func, char val)
{
    gep->hw_ms_delay( 5 );
    start_i2c(gep);    
    send_byte_i2c(gep, M52749FP_ADDR );
    if( wait_ack_i2c(gep) ) throw "Chip select address";
    send_byte_i2c(gep, func ); // function select
    if( wait_ack_i2c(gep) ) throw "Function select";
    send_byte_i2c(gep, val ); // value
    if( wait_ack_i2c(gep) ) throw "Value set";
    stop_i2c(gep);        
    gep->hw_ms_delay( 5 );
}

static void m52749_value_set(int val, void *ptr, int ipar)
{
    try{
	init_i2c(gep);
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
	finish_action();
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

// ------------------------------------------------------------- M62392 --------
static void send_m62392(char ch, char val)
{
    gep->hw_ms_delay( 5 );
    start_i2c(gep);    
    send_byte_i2c(gep,  0x90 | ((get_buffer( 0 ) && 0x07) << 1) ); 
    if( wait_ack_i2c(gep) ) throw "Chip select address";
    send_byte_i2c(gep, ch ); // function select
    if( wait_ack_i2c(gep) ) throw "Channel select";
    send_byte_i2c(gep, val ); // value
    if( wait_ack_i2c(gep) ) throw "Value set";
    stop_i2c(gep);        
    gep->hw_ms_delay( 5 );
}

static void m62392_value_set(int val, void *ptr, int ipar)
{
    try{
	if(ipar == 0 )
	    put_buffer(0, val);
	else{
	    init_i2c(gep);
	    put_buffer(ipar, val); send_m62392( ipar,  get_buffer(ipar) ); // Main contrast
	    finish_action();
	}
    } catch (const char *x){
	finish_action();
	message("[WN][TEXT]-= TIMEOUT! =-\n %s \n No ACK bit received [/TEXT][BR]OK", x);    
    }
}

void write_m62392_()
{
    // send settings to chip    
    dialog_start("M62392 Control", 300, 200);
	spin_add("Slave address ", 0, 7, get_buffer(0 ), m62392_value_set, 0 , NULL); 
	frame_start("DAC Channels");
	    slider_add("ch1 ", 0, 255, get_buffer(1 ), m62392_value_set, 1 , NULL); 
	    slider_add("ch2 ", 0, 255, get_buffer(2 ), m62392_value_set, 2 , NULL);    
	    slider_add("ch3 ", 0, 255, get_buffer(3 ), m62392_value_set, 3 , NULL);    
	    slider_add("ch4 ", 0, 255, get_buffer(4 ), m62392_value_set, 4 , NULL);    
	    slider_add("ch5 ", 0, 255, get_buffer(5 ), m62392_value_set, 5 , NULL);    
	    slider_add("ch6 ", 0, 255, get_buffer(6 ), m62392_value_set, 6 , NULL);    
	    slider_add("ch7 ", 0, 255, get_buffer(7 ), m62392_value_set, 7 , NULL);    
	    slider_add("ch8 ", 0, 255, get_buffer(8 ), m62392_value_set, 8 , NULL);    
	    slider_add("ch9 ", 0, 255, get_buffer(9 ), m62392_value_set, 9 , NULL);    
	    slider_add("ch10", 0, 255, get_buffer(10), m62392_value_set, 10, NULL);    
	    slider_add("ch11", 0, 255, get_buffer(11), m62392_value_set, 11, NULL);    
	    slider_add("ch12", 0, 255, get_buffer(12), m62392_value_set, 12, NULL);    
	frame_end();
    dialog_end();
}


REGISTER_FUNCTION( write, m52749fp, m52749_);
REGISTER_FUNCTION( write, m62392fp, m62392_);

REGISTER_MODULE_BEGIN(I2c_peripheral)

    register_chip_begin("/I2c peripheral/Video", "M52749FP", "I2C_PERIPHERAL", M52749FP_SIZE);
	add_action(MODULE_PROG_ACTION, write_m52749fp);
    register_chip_end;

    register_chip_begin("/I2c peripheral/DAC", "M62392FP", "I2C_PERIPHERAL", M62392FP_SIZE);
	add_action(MODULE_PROG_ACTION, write_m62392fp);
    register_chip_end;

REGISTER_MODULE_END
