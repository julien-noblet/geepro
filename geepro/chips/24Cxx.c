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

#define C01_SIZE	128
#define C21_SIZE	128
#define C02_SIZE	256
#define PCF8582_SIZE	256
#define C04_SIZE	512
#define C08_SIZE	KB_SIZE( 1 )
#define C16_SIZE	KB_SIZE( 2 )
#define C32_SIZE	KB_SIZE( 4 )
#define C64_SIZE	KB_SIZE( 8 )
#define C128_SIZE	KB_SIZE( 16 )
#define C256_SIZE	KB_SIZE( 32 )
#define C512_SIZE	KB_SIZE( 64 )

#define C01_BLOCK	8
#define C21_BLOCK	8
#define C02_BLOCK	8
#define PCF8582_BLOCK	8
#define C04_BLOCK	16
#define C08_BLOCK	16
#define C16_BLOCK	16
#define C32_BLOCK	16
#define C64_BLOCK	16
#define C128_BLOCK	16
#define C256_BLOCK	16
#define C512_BLOCK	16

#define TI 			4
#define TIMEOUT			100
#define MEMO_24CXX_DEV_ADDR	0xa0	// Device internal address for 24Cxx

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

/*
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
*/

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
/*
//REGISTER_FUNCTION( read,  PCF_8582, 24Cxx, C02_SIZE, C02_BLOCK, 0 );
REGISTER_FUNCTION( write, PCF_8582, PCF_8582_, C02_SIZE, C02_BLOCK, 0 );
//REGISTER_FUNCTION( verify, PCF_8582, 24Cxx, C02_SIZE, C02_BLOCK, 0 );
*/
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

    register_chip_begin("/Serial EEPROM/PCF85xx", "PCF8582", "24Cxx", PCF8582_SIZE);
	add_action(MODULE_READ_ACTION, read_24C02);
//	add_action(MODULE_PROG_ACTION, write_PCF_8582);
	add_action(MODULE_VERIFY_ACTION, verify_24C02);
    register_chip_end;

//    register_chip_begin("/Serial EEPROM/24Cxx", "24C21", "24Cxx", C21_SIZE);
//	add_action(MODULE_READ_ACTION, read_24C21);
//	add_action(MODULE_PROG_ACTION, write_24C21);
//	add_action(MODULE_VERIFY_ACTION, verify_24C21);
//    register_chip_end;

REGISTER_MODULE_END
