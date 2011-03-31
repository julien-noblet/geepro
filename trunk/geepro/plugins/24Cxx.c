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

#define uint unsigned int
#define uchar unsigned char

MODULE_IMPLEMENTATION

#define C01_SIZE 128
#define C02_SIZE 256
#define C04_SIZE 512
#define C08_SIZE 1024
#define C16_SIZE 2048
#define C32_SIZE 4096
#define C64_SIZE 8192
#define C128_SIZE 16384
#define C256_SIZE 32768
#define C512_SIZE 65536

#define C01_BLOCK 8
#define C02_BLOCK 8
#define C04_BLOCK 16
#define C08_BLOCK 16
#define C16_BLOCK 16
#define C32_BLOCK 16
#define C64_BLOCK 16
#define C128_BLOCK 16
#define C256_BLOCK 16
#define C512_BLOCK 16

#define TI 16

#define MEMO_24CXX_DEV_ADDR	0xa0

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

#define TIMEOUT	100

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

/***************************************************************************/

char devsel_24Cxx( char rw, char addr )
{
    send_byte_i2c( 0xa0 | (rw & 1) | ((addr << 1) & 0x0e));
    if( wait_ack_i2c() ) return 1;
    return 0;
}

char transm_seq_hdr_24Cxx(int addr, char mode, char addr_mode) // mode = 0 -> write, mode = 1 -> read
{
    unsigned int  msb = (addr >> 8) & 0xff;
    unsigned char page_nb = addr_mode ? 0 : msb & 0x07;
    unsigned char addr_byte = addr & 0xff;
    int timeout;

    // send devsel WRITE
    timeout = TIMEOUT;
    do{
	start_i2c();
	if(--timeout == 0) return 1;
    }while(devsel_24Cxx( 0, page_nb ));

    // send address
    if( addr_mode ){
	send_byte_i2c( msb );    
	if( wait_ack_i2c() ) return 2;
    }
    send_byte_i2c( addr_byte & 0xff);
    if( wait_ack_i2c() ) return 2;

    if( mode ){ // skip for WRITE
	// repeated start + send devsel READ
	start_i2c();
	if(devsel_24Cxx( 1, page_nb )) return 3;
    }
    return 0;
}

void write_24Cxx(int dev_size, char block_size, char addr_mode)
{
    int i;
    char error;
    
    init_i2c();
    progress_loop(i, dev_size, "Writing ..."){
	break_if( (error = transm_seq_hdr_24Cxx( i, 0, addr_mode)) );
	send_byte_i2c( get_buffer( i ) );
	break_if( wait_ack_i2c() );
	stop_i2c();
    }
    finish_action();
}

void write_PCF_8582_(int dev_size, char block_size, char addr_mode)
{
    int i;
//    char error;
    
    init_i2c();
    progress_loop(i, dev_size, "Writing ..."){
//	break_if( (error = transm_seq_hdr_24Cxx( i, 0, addr_mode)) );
//	send_byte_i2c( get_buffer( i ) );
//	break_if( wait_ack_i2c() );
//	stop_i2c();
    }
    finish_action();
}

char rd_block_24Cxx(int addr, int block_size, char addr_mode)
{
    int i;
    char error;

    if( (error = transm_seq_hdr_24Cxx( addr, 1, addr_mode)) ) return error;
    
    for( i = 0; i < block_size; i++){
	put_buffer( addr + i, recv_byte_i2c() );
	send_bit_i2c( i == (block_size - 1) ? 1 : 0); // NOACK / ACK
    }
    stop_i2c();
    return 0;
}

char verify_block_24Cxx(int addr, int block_size, char addr_mode, int *it)
{
    unsigned char error, d0, d1;
    int i;

    if( (error = transm_seq_hdr_24Cxx( addr, 1, addr_mode)) ) return error;
    
    for( i = 0; i < block_size; i++){
	d0 = get_buffer( addr + i);
	d1 = recv_byte_i2c();
	break_if( d0 != d1 );
	send_bit_i2c( i == (block_size - 1) ? 1 : 0); // NOACK / ACK
    }
    stop_i2c();
    *it = i;
    return ERROR_VAL;
}

void read_24Cxx(unsigned int dev_size, unsigned char block_size, char addr_mode)
{
    int i;

    init_i2c();
    progress_loop(i, dev_size / block_size, "Reading ..."){
	break_if( rd_block_24Cxx(i * block_size, block_size, addr_mode));
    }
    finish_action();
}

void verify_24Cxx(unsigned int dev_size, unsigned char block_size, char addr_mode)
{
    int i,j;
    char text[256];

    init_i2c();
    progress_loop(i, dev_size / block_size, "Verify ..."){
	break_if( verify_block_24Cxx(i * block_size, block_size, addr_mode, &j));
    }
    finish_action();

    text[0] = 0;
    if( ERROR_VAL ){
	sprintf(text, "[WN][TEXT] Memory and buffer differ !!!\n Address = 0x%X[/TEXT][BR]OK", j + i * block_size );
    }
    show_message(0, ERROR_VAL ? text: "[IF][TEXT] Memory and buffer are consitent[/TEXT][BR]OK", NULL, NULL);    
    ERROR_VAL = 0;
}

/**********************************************************************************************/

REGISTER_FUNCTION( read,  24C01, 24Cxx, C01_SIZE, C01_BLOCK, 0 );
REGISTER_FUNCTION( write, 24C01, 24Cxx, C01_SIZE, C01_BLOCK, 0 );
REGISTER_FUNCTION( verify, 24C01, 24Cxx, C01_SIZE, C01_BLOCK, 0 );

REGISTER_FUNCTION( read,  24C02, 24Cxx, C02_SIZE, C02_BLOCK, 0 );
REGISTER_FUNCTION( write, 24C02, 24Cxx, C02_SIZE, C02_BLOCK, 0 );
REGISTER_FUNCTION( verify, 24C02, 24Cxx, C02_SIZE, C02_BLOCK, 0 );

REGISTER_FUNCTION( read,  PCF_8582, 24Cxx, C02_SIZE, C02_BLOCK, 0 );
REGISTER_FUNCTION( write, PCF_8582, PCF_8582_, C02_SIZE, C02_BLOCK, 0 );
REGISTER_FUNCTION( verify, PCF_8582, 24Cxx, C02_SIZE, C02_BLOCK, 0 );

REGISTER_FUNCTION( read,  24C04, 24Cxx, C04_SIZE, C04_BLOCK, 0 );
REGISTER_FUNCTION( write, 24C04, 24Cxx, C04_SIZE, C04_BLOCK, 0 );
REGISTER_FUNCTION( verify, 24C04, 24Cxx, C04_SIZE, C04_BLOCK, 0 );

REGISTER_FUNCTION( read,  24C08, 24Cxx, C08_SIZE, C08_BLOCK, 0 );
REGISTER_FUNCTION( write, 24C08, 24Cxx, C08_SIZE, C08_BLOCK, 0 );
REGISTER_FUNCTION( verify, 24C08, 24Cxx, C08_SIZE, C08_BLOCK, 0 );

REGISTER_FUNCTION( read,  24C16, 24Cxx, C16_SIZE, C16_BLOCK, 0 );
REGISTER_FUNCTION( write, 24C16, 24Cxx, C16_SIZE, C16_BLOCK, 0 );
REGISTER_FUNCTION( verify, 24C16, 24Cxx, C16_SIZE, C16_BLOCK, 0 );

REGISTER_FUNCTION( read,  24C32, 24Cxx, C32_SIZE, C32_BLOCK, 1 );
REGISTER_FUNCTION( write, 24C32, 24Cxx, C32_SIZE, C32_BLOCK, 1  );
REGISTER_FUNCTION( verify, 24C32, 24Cxx, C32_SIZE, C32_BLOCK, 1  );

REGISTER_FUNCTION( read,  24C64, 24Cxx, C64_SIZE, C64_BLOCK, 1  );
REGISTER_FUNCTION( write, 24C64, 24Cxx, C64_SIZE, C64_BLOCK, 1  );
REGISTER_FUNCTION( verify, 24C64, 24Cxx, C64_SIZE, C64_BLOCK, 1  );

REGISTER_FUNCTION( read,  24C128, 24Cxx, C128_SIZE, C128_BLOCK, 1  );
REGISTER_FUNCTION( write, 24C128, 24Cxx, C128_SIZE, C128_BLOCK, 1  );
REGISTER_FUNCTION( verify, 24C128, 24Cxx, C128_SIZE, C128_BLOCK, 1  );

REGISTER_FUNCTION( read,  24C256, 24Cxx, C256_SIZE, C256_BLOCK, 1  );
REGISTER_FUNCTION( write, 24C256, 24Cxx, C256_SIZE, C256_BLOCK, 1  );
REGISTER_FUNCTION( verify, 24C256, 24Cxx, C256_SIZE, C256_BLOCK, 1  );

REGISTER_FUNCTION( read,  24C512, 24Cxx, C512_SIZE, C512_BLOCK, 1  );
REGISTER_FUNCTION( write, 24C512, 24Cxx, C512_SIZE, C512_BLOCK, 1  );
REGISTER_FUNCTION( verify, 24C512, 24Cxx, C512_SIZE, C512_BLOCK, 1  );

REGISTER_MODULE_BEGIN(24Cxx)

    register_chip_begin("/Serial EEPROM/24Cxx", "24C01", "24Cxx", C01_SIZE);
	add_action(MODULE_READ_ACTION, read_24C01);
	add_action(MODULE_PROG_ACTION, write_24C01);
	add_action(MODULE_VERIFY_ACTION, verify_24C01);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C02", "24Cxx", C02_SIZE);
	add_action(MODULE_READ_ACTION, read_24C02);
	add_action(MODULE_PROG_ACTION, write_24C02);
	add_action(MODULE_VERIFY_ACTION, verify_24C02);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C04", "24Cxx", C04_SIZE);
	add_action(MODULE_READ_ACTION, read_24C04);
	add_action(MODULE_PROG_ACTION, write_24C04);
	add_action(MODULE_VERIFY_ACTION, verify_24C04);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C08", "24Cxx", C08_SIZE);
	add_action(MODULE_READ_ACTION, read_24C08);
	add_action(MODULE_PROG_ACTION, write_24C08);
	add_action(MODULE_VERIFY_ACTION, verify_24C08);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C16", "24Cxx", C16_SIZE);
	add_action(MODULE_READ_ACTION, read_24C16);
	add_action(MODULE_PROG_ACTION, write_24C16);
	add_action(MODULE_VERIFY_ACTION, verify_24C16);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C32", "24Cxx", C32_SIZE);
	add_action(MODULE_READ_ACTION, read_24C32);
	add_action(MODULE_PROG_ACTION, write_24C32);
	add_action(MODULE_VERIFY_ACTION, verify_24C32);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C64", "24Cxx", C64_SIZE);
	add_action(MODULE_READ_ACTION, read_24C64);
	add_action(MODULE_PROG_ACTION, write_24C64);
	add_action(MODULE_VERIFY_ACTION, verify_24C64);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C128", "24Cxx", C128_SIZE);
	add_action(MODULE_READ_ACTION, read_24C128);
	add_action(MODULE_PROG_ACTION, write_24C128);
	add_action(MODULE_VERIFY_ACTION, verify_24C128);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C256", "24Cxx", C256_SIZE);
	add_action(MODULE_READ_ACTION, read_24C256);
	add_action(MODULE_PROG_ACTION, write_24C256);
	add_action(MODULE_VERIFY_ACTION, verify_24C256);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C512", "24Cxx", C512_SIZE);
	add_action(MODULE_READ_ACTION, read_24C512);
	add_action(MODULE_PROG_ACTION, write_24C512);
	add_action(MODULE_VERIFY_ACTION, verify_24C512);
    register_chip_end;

//    register_chip_begin("/Serial EEPROM/PCF85xx", "PCF8582", "24Cxx", C02_SIZE);
//	add_action(MODULE_READ_ACTION, read_PCF_8582);
//	add_action(MODULE_PROG_ACTION, write_PCF_8582);
//	add_action(MODULE_VERIFY_ACTION, verify_PCF_8582);
//    register_chip_end;

REGISTER_MODULE_END
