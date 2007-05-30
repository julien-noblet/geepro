/* $Revision: 1.2 $ */
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

#define C01_SIZE 1024
#define C02_SIZE 2048
#define C04_SIZE 4096
#define C08_SIZE 8192
#define C16_SIZE 16384

#define C01_BLOCK 8
#define C02_BLOCK 8
#define C04_BLOCK 16
#define C08_BLOCK 16
#define C16_BLOCK 16

#define TI 16

#define QDEL  hw_delay(TI/2)
#define HDEL  hw_delay(TI)

#define I2C_SDL_LO      hw_set_sda(0)
#define I2C_SDL_HI      hw_set_sda(1)

#define I2C_SCL_LO      hw_set_scl(0)
#define I2C_SCL_HI      hw_set_scl(1)

void i2c_t(void)
{
    HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
}

void i2c_start(void)
{
    I2C_SDL_LO; QDEL; I2C_SCL_LO; 
}

void i2c_stop(void)
{
    HDEL; I2C_SCL_HI; QDEL; I2C_SDL_HI; HDEL;
}

#define I2C_SCL_TOGGLE  i2c_t();
#define I2C_START       i2c_start();
#define I2C_STOP        i2c_stop();  

char i2c_putbyte(uchar b)
{
     int i;

     for (i=7;i>=0;i--)
     {
         if ( b & (1<<i) )
             I2C_SDL_HI;
         else
             I2C_SDL_LO;         // address bit
             I2C_SCL_TOGGLE;     // clock HI, delay, then LO
     }

     I2C_SDL_HI;                 // leave SDL HI

     HDEL;
     I2C_SCL_HI;                 // clock back up
     b = hw_get_sda();           // get the ACK bit

     HDEL;
     I2C_SCL_LO;                 // not really ??
     HDEL;
     return (b == 0);            // return ACK value
}

uchar i2c_getbyte(char last)
 {
     int i;
     uchar c,b = 0;

     I2C_SDL_HI;                 // make sure pullups are ativated

     for(i=7;i>=0;i--)
     {
         HDEL;
         I2C_SCL_HI;             // clock HI
         c = hw_get_sda();
         b <<= 1;
         if(c) b |= 1;
         HDEL;
         I2C_SCL_LO;             // clock LO
     }

     if (last)
         I2C_SDL_HI;             // set NAK
     else
         I2C_SDL_LO;             // set ACK

     I2C_SCL_TOGGLE;             // clock pulse
     I2C_SDL_HI;                 // leave with SDL HI
     return b;                   // return received byte
}

void i2c_init(void)
{
     I2C_SDL_HI;
     I2C_SCL_HI;
     hw_set_hold(0);
}

//! Send a byte sequence on the I2C bus
void i2c_send(uchar device, uint subaddr, uchar length, char *data)
{
     uchar addr_lo;
     uchar addr_hi;

     addr_lo = (uchar) (subaddr & 0x00ff);
     addr_hi = (uchar) (subaddr >> 8);
     printf("read %d %d \n", addr_hi,addr_lo);
     I2C_START;                  // do start transition
     i2c_putbyte(device & 0xfe); // send DEVICE address
     i2c_putbyte(addr_hi);       // and the hiaddress
     i2c_putbyte(addr_lo);       // and the loaddress

     // send the data
     while (length--)
         i2c_putbyte(*data++);

     I2C_SDL_LO;                 // clear data line and
     I2C_STOP;                   // send STOP transition
}

//! Retrieve a byte sequence on the I2C bus
void i2c_receive(uchar device, uint subaddr, uchar length, char *data)
{
     uint j = length;
     //int i;
     char *p = data;
     uchar addr_lo;
     uchar addr_hi;
     addr_lo= (uchar) (subaddr & 0x00ff);
     addr_hi= (uchar) (subaddr >> 8);
     printf("read %d %d \n", addr_hi,addr_lo);
     //gui_progress_bar_init("Odczyt pamieci",length);


     I2C_START;                  // do start transition
     i2c_putbyte(device);         // send DEVICE address
     i2c_putbyte(addr_hi);        // and the hiaddress
     i2c_putbyte(addr_lo);        // and the loaddress
     HDEL;
     I2C_SCL_HI;                 // do a repeated START
     I2C_START;                  // transition

     i2c_putbyte(device | 0x01);  // resend DEVICE, with READ bit set

     // receive data bytes
     while (j--){
         *p++ = i2c_getbyte(j == 0);
         //gui_progress_bar_set(i++);
     }

     //gui_progress_bar_free();

     I2C_SDL_LO;                 // clear data line and
     I2C_STOP;                   // send STOP transition
}

/**************************************************************************************************/
void read_24xx(uint dev_size, uchar block_size)
{

    uint i;
    //unsigned int d;

    char *buf = buffer_get_buffer_ptr(___geep___);

    hw_sw_vcc(1);
    i2c_init();
    hw_delay(5*TI);
    //for(i = 0; i < dev_size; i+=block_size){
    //i2c_receive(160 ,0 , dev_size, buf);
    for(i = 0; i < dev_size; i+=block_size,buf+=block_size){
        i2c_receive(160, i , block_size, buf);
        //gui_progress_bar_set(i);
    }


    finish_action();
    hw_sw_vcc(0);
}


void erase_24xx(uint dev_size, uchar block_size)
{

	hw_sw_vcc(1);
	hw_set_sda(0);
}

void prog_24xx(uint dev_size, uchar block_size)
{
    uint i;
    char *buf = buffer_get_buffer_ptr(___geep___);

    hw_sw_vcc(1);
    i2c_init();
    hw_delay(5*TI);
    //gui_progress_bar_init("Programowanie pamieci",dev_size);

    for(i = 0; i < dev_size; i+=block_size){
        i2c_send(160,i , block_size, buf);
        //gui_progress_bar_set(i);
    }

    gui_progress_bar_free(___geep___);
    finish_action();
    hw_sw_vcc(0);


}


REG_FUNC_BEGIN(erase_24C01)
	erase_24xx(C01_SIZE,C01_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(prog_24C01)
	prog_24xx(C01_SIZE,C01_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(read_24C01)
	read_24xx(C01_SIZE,C01_BLOCK);
REG_FUNC_END


REG_FUNC_BEGIN(erase_24C02)
	erase_24xx(C02_SIZE,C02_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(prog_24C02)
	prog_24xx(C02_SIZE,C02_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(read_24C02)
	read_24xx(C02_SIZE,C02_BLOCK);
REG_FUNC_END


REG_FUNC_BEGIN(erase_24C04)
	erase_24xx(C04_SIZE,C04_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(prog_24C04)
	prog_24xx(C04_SIZE,C04_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(read_24C04)
	read_24xx(C04_SIZE,C04_BLOCK);
REG_FUNC_END


REG_FUNC_BEGIN(erase_24C08)
	erase_24xx(C08_SIZE,C08_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(prog_24C08)
	prog_24xx(C08_SIZE,C08_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(read_24C08)
	read_24xx(C08_SIZE,C08_BLOCK);
REG_FUNC_END


REG_FUNC_BEGIN(erase_24C16)
	erase_24xx(C16_SIZE,C16_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(prog_24C16)
	prog_24xx(C16_SIZE,C16_BLOCK);
REG_FUNC_END

REG_FUNC_BEGIN(read_24C16)
	read_24xx(C16_SIZE,C16_BLOCK);
REG_FUNC_END


REGISTER_MODULE_BEGIN(24Cxx)

    register_chip_begin("/Serial EEPROM/24Cxx", "24C01A", "24Cxx", C01_SIZE);
	add_action(MODULE_READ_ACTION, read_24C01);
	add_action(MODULE_PROG_ACTION, prog_24C01);
	add_action(MODULE_ERASE_ACTION, erase_24C01);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C02", "24Cxx", C02_SIZE);
	add_action(MODULE_READ_ACTION, read_24C02);
	add_action(MODULE_PROG_ACTION, prog_24C02);
	add_action(MODULE_ERASE_ACTION, erase_24C02);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C04", "24Cxx", C04_SIZE);
	add_action(MODULE_READ_ACTION, read_24C04);
	add_action(MODULE_PROG_ACTION, prog_24C04);
	add_action(MODULE_ERASE_ACTION, erase_24C04);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/24Cxx", "24C08A", "24Cxx", C08_SIZE);
	add_action(MODULE_READ_ACTION, read_24C08);
	add_action(MODULE_PROG_ACTION, prog_24C08);
	add_action(MODULE_ERASE_ACTION, erase_24C08);
    register_chip_end;
    
    register_chip_begin("/Serial EEPROM/24Cxx", "24C16A", "24Cxx", C16_SIZE);
	add_action(MODULE_READ_ACTION, read_24C16);
	add_action(MODULE_PROG_ACTION, prog_24C16);
	add_action(MODULE_ERASE_ACTION, erase_24C16);
    register_chip_end;

REGISTER_MODULE_END
