/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2011 Krzysztof Komarnicki
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

#include <stdio.h>
#include "../drivers/hwplugin.h"

#define TI 16
#define TIMEOUT	100

void init_i2c()
{
    hw_set_sda(1);
    hw_set_scl(1);
    hw_set_hold(0);
    hw_delay( TI );
    hw_sw_vcc(1);
    hw_delay( 10 * TI );  // time for POR
}

void scl_tik_i2c()
{
    hw_delay( TI / 2 );
    hw_set_scl(1);
    hw_delay( TI );
    hw_set_scl(0);
    hw_delay( TI / 2 );
}

void start_i2c()
{
    hw_set_sda(1);
    hw_delay( TI / 2 );    
    hw_set_scl(1);
    hw_delay( TI / 2 );    

    hw_set_sda(0);
    hw_delay( TI / 2 );
    hw_set_scl(0);
    hw_delay( TI / 2 );
}

void stop_i2c()
{
    hw_set_sda(0);
    hw_delay( TI );
    hw_set_scl(0);
    hw_delay( TI );
    hw_set_scl(1);
    hw_delay( TI );
    hw_set_sda(1);
    hw_delay( TI );
}

void send_bit_i2c( char bit )
{
    hw_set_sda(bit);
    scl_tik_i2c();
}

char get_bit_i2c()
{
    char b;
    
    hw_set_sda( 1 );
    hw_set_scl(1);    
    hw_delay( TI );    

    b = hw_get_sda();
    hw_delay( TI);    
    hw_set_scl(0);
    return b;
}

void send_byte_i2c( char byte )
{
    int i;
    for( i = 0x80; i; i >>= 1 ) send_bit_i2c( (byte & i) ? 1:0 );
}

char recv_byte_i2c()
{
    int i;
    char b = 0;

    for( i = 8; i; i-- ){
	b <<= 1;
	b |= get_bit_i2c();
    }

    return b;
}

char wait_ack_i2c()
{
    int i;
    char b;
    
    hw_set_sda( 1 );		// release SDA
    hw_delay( TI / 2 );    	// wait for stabilize
    hw_set_scl(1);    		// SCL = 1
    for( b = 1, i = 0; (i < TIMEOUT) && b; i++){
	hw_delay( TI / 2 );    	// wait for SDA = 0
	b = hw_get_sda();
    }
    hw_set_scl(0);	
    return (b != 0) ? 1:0;
}

/*************************************************************************************************/


