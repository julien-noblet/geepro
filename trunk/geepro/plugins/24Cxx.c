/* $Revision: 1.5 $ */
/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2007 Bartłomiej Zimoń
 * Email: uzi18 (at) o2 (dot) pl
 *
 * I2C part is from i2csw.c Pascal Stang (avrlib)
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

#define uint unsigned int
#define uchar unsigned char

MODULE_IMPLEMENTATION

#define C01_SIZE 128
#define C02_SIZE 256
#define C04_SIZE 512
#define C08_SIZE 1024
#define C16_SIZE 2048

#define C01_BLOCK 8
#define C02_BLOCK 8
#define C04_BLOCK 16
#define C08_BLOCK 16
#define C16_BLOCK 16

#define C01_ADDR_BYTES 0
#define C02_ADDR_BYTES 0
#define C04_ADDR_BYTES 1
#define C08_ADDR_BYTES 1
#define C16_ADDR_BYTES 1

#define TI 16

#define QDEL  hw_delay(TI/2)
#define HDEL  hw_delay(TI)

#define I2C_SDA_LO      	hw_set_sda(0)
#define I2C_SDA_HI      	hw_set_sda(1)

#define I2C_SCL_LO      	hw_set_scl(0)
#define I2C_SCL_HI      	hw_set_scl(1)

#define I2C_SCL_TOGGLE  	i2c_t();
#define I2C_START       	i2c_start();
#define I2C_STOP        	i2c_stop();  
#define MEMO_24CXX_DEV_ADDR	0xa0

void i2c_t(void)
{
    HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
}

void i2c_init(void)
{
    hw_sw_vcc(1);
    I2C_SDA_HI;
    I2C_SCL_HI;
    hw_set_hold(0);
    hw_delay( 5 * TI );
}

void i2c_start(void)
{
    I2C_SDA_LO; 
    QDEL; 
    I2C_SCL_LO; 
    QDEL; 
}

void i2c_stop(void)
{
    HDEL; 
    I2C_SCL_HI; 
    QDEL; 
    I2C_SDA_HI; 
    HDEL;
}

char i2c_putbyte(uchar b)
{
    int i;

    for( i = 0x80; i; i >>= 1 )
    {
         hw_set_sda( (b & i) != 0 );
         I2C_SCL_TOGGLE;     // clock HI, delay, then LO
    }
    I2C_SDA_HI;                 // leave SDA HI
    HDEL;

    I2C_SCL_HI;                 // clock back up
    b = hw_get_sda();           // get the ACK bit
    HDEL;
    I2C_SCL_LO;                 // not really ??
    HDEL;
    return (b != 0);            // return ACK value
}

uchar i2c_getbyte(char last)
{
    int i;
    uchar b = 0;

    I2C_SDA_HI;                 // make sure pullups are ativated

    for(i = 0x80; i; i >>= 1)
    {
         HDEL;
         I2C_SCL_HI;             // clock HI
         b |= hw_get_sda() ? i : 0;
         HDEL;
         I2C_SCL_LO;             // clock LO
    }

    hw_set_sda( last ? 1:0); 	// NAK/ACK
    I2C_SCL_TOGGLE;             // clock pulse

    I2C_SDA_HI;                 // leave with SDA HI
    HDEL;
    return b;                   // return received byte
}

//! Send a byte sequence on the I2C bus
char i2c_send(uchar device, uint subaddr, uchar length, uchar addr_bytes)
{
    I2C_START;                  // do start transition
    if(i2c_putbyte(device & 0xfe)) return 1; // send DEVICE address, do it until ACK or timeout

    if( addr_bytes)
	if( i2c_putbyte((subaddr >> 8) & 0xff ) ) return 2;
    if( i2c_putbyte(subaddr & 0xff ) ) return 3;

     // send the data
    while (length--)
         if( i2c_putbyte( get_buffer( subaddr++ ) ) ) return 4;

    I2C_SDA_LO;                 // clear data line and
    I2C_STOP;                   // send STOP transition

    return 0;
}

//! Retrieve a byte sequence on the I2C bus
char i2c_receive(uchar device, uint subaddr, uchar length, uchar addr_bytes)
{
    I2C_START;                  		// do start transition
    if(i2c_putbyte(device)) return 1;          // send DEVICE address

    if( addr_bytes)
	if( i2c_putbyte((subaddr >> 8) & 0xff ) ) return 2;
    if( i2c_putbyte(subaddr & 0xff ) ) return 3;

    HDEL;
    I2C_SCL_HI;                 // do a repeated START
    I2C_START;                  // transition

    if( i2c_putbyte(device | 0x01)) return 4;  // resend DEVICE, with READ bit set

     // receive data bytes
    while (length--)
         put_buffer( subaddr++ , i2c_getbyte(length == 0) );

    I2C_SDA_LO;                 // clear data line and
    I2C_STOP;                   // send STOP transition
    return 0;
}

/**************************************************************************************************/

void init_i2c()
{
    hw_set_sda(1);
    hw_set_scl(1);
    hw_delay( TI );
    hw_sw_vcc(1);
    hw_delay( 10 * TI );  // time for internal reset
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
    hw_set_sda(0);
    hw_delay( TI / 2 );
    hw_set_scl(0);
    hw_delay( TI / 2 );
}

void stop_i2c()
{
    hw_set_sda(0);
    hw_set_scl(0);
    hw_delay( TI / 2 );
    hw_set_scl(1);
    hw_delay( TI / 2 );
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
    hw_delay( TI / 2 );    
    b = hw_get_sda();
    hw_delay( TI / 2 );    
    hw_set_scl(0);
    
    return b;
}

void send_byte_i2c( char byte )
{
    int i;
    for( i = 0x80; i; i >>= 1 ) send_byte_i2c( (byte & i) ? 1:0 );
}

char recv_byte_i2c()
{
    int i;
    char b = 0;

    for( i = 8; i; i--, b <<= 1 ) b |= get_bit_i2c();

    return b;
}

void devsel_24Cxx( char rw )
{
    send_byte_i2c( 0xa0 | (rw & 1) );
}

void read_24Cxx(uint dev_size, uchar block_size, uchar addr_bytes)
{
    uint i, j;

    TEST_CONNECTION( VOID );
    i2c_init();
    progress_loop(i, dev_size / block_size, "Reading ..."){
        break_if( i2c_receive(MEMO_24CXX_DEV_ADDR, i * block_size ,block_size, addr_bytes) );
    }
    finish_action();
/*
    init_i2c();
    start_i2c();
    devsel_24Cxx( 0 );
    get_bit_i2c();
    send_byte_i2c( 0 );
    get_bit_i2c();

    start_i2c();
    devsel_24Cxx( 1 );
    get_bit_i2c();
        
    progress_loop(i, dev_size, "Reading ..."){
	put_buffer( i, recv_byte_i2c() );
	send_bit_i2c( i == (dev_size - 1) ? 1 : 0); // ACK/ noack
    }
    finish_action();
*/
}

void write_24Cxx(uint dev_size, uchar block_size, uchar addr_bytes)
{
    uint i;

    TEST_CONNECTION( VOID );

    i2c_init();
    progress_loop(i, dev_size / block_size, "Writing ..."){
        break_if( i2c_send(MEMO_24CXX_DEV_ADDR, i * block_size , block_size, addr_bytes) );
    }
    finish_action();
}

/**********************************************************************************************/

REGISTER_FUNCTION( read,  24C01, 24Cxx, C01_SIZE, C01_BLOCK, C01_ADDR_BYTES);
REGISTER_FUNCTION( write, 24C01, 24Cxx, C01_SIZE, C01_BLOCK, C01_ADDR_BYTES);

REGISTER_FUNCTION( read,  24C02, 24Cxx, C02_SIZE, C02_BLOCK, C02_ADDR_BYTES);
REGISTER_FUNCTION( write, 24C02, 24Cxx, C02_SIZE, C02_BLOCK, C02_ADDR_BYTES);

REGISTER_FUNCTION( read,  24C04, 24Cxx, C04_SIZE, C04_BLOCK, C04_ADDR_BYTES);
REGISTER_FUNCTION( write, 24C04, 24Cxx, C04_SIZE, C04_BLOCK, C04_ADDR_BYTES);

REGISTER_FUNCTION( read,  24C08, 24Cxx, C08_SIZE, C08_BLOCK, C08_ADDR_BYTES);
REGISTER_FUNCTION( write, 24C08, 24Cxx, C08_SIZE, C08_BLOCK, C08_ADDR_BYTES);

REGISTER_FUNCTION( read,  24C16, 24Cxx, C16_SIZE, C16_BLOCK, C16_ADDR_BYTES);
REGISTER_FUNCTION( write, 24C16, 24Cxx, C16_SIZE, C16_BLOCK, C16_ADDR_BYTES);

REGISTER_MODULE_BEGIN(24Cxx)

    register_chip_begin("/Serial EEPROM/24Cxx", "24C01A", "24Cxx", C01_SIZE);
	add_action(MODULE_READ_ACTION, read_24C01);
	add_action(MODULE_PROG_ACTION, write_24C01);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C02, PCF8582", "24Cxx", C02_SIZE);
	add_action(MODULE_READ_ACTION, read_24C02);
	add_action(MODULE_PROG_ACTION, write_24C02);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C04", "24Cxx", C04_SIZE);
	add_action(MODULE_READ_ACTION, read_24C04);
	add_action(MODULE_PROG_ACTION, write_24C04);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C08A", "24Cxx", C08_SIZE);
	add_action(MODULE_READ_ACTION, read_24C08);
	add_action(MODULE_PROG_ACTION, write_24C08);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C16A", "24Cxx", C16_SIZE);
	add_action(MODULE_READ_ACTION, read_24C16);
	add_action(MODULE_PROG_ACTION, write_24C16);
    register_chip_end;

REGISTER_MODULE_END
