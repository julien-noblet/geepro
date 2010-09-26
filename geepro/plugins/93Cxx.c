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

MODULE_IMPLEMENTATION

#define C46_SIZE 128
#define C56_SIZE 256
#define C66_SIZE 512

#define OP_READ  0x18

// TI = 4 ok!
#define TI 6

#define EWEN_93		0x03
#define ERAL_93		0x02
#define WRAL_93		0x01
#define EWDS_93		0x00

#define TCR 100
#define TCW 1000

#define PARM_9346	128,8,7
#define PARM_9356	256,8,8
#define PARM_9366	512,8,9


void reset_93_socket()
{
    hw_set_cs(0);
    hw_set_clk(0);
    hw_set_di(0);    
}

char conn_test()
{
    return 0;
}

void tic_93_out(char bool)
{
    hw_set_clk(0);
    hw_set_di(bool);
    hw_delay(TI);
    hw_set_clk(1);
    hw_delay(TI);
    hw_set_clk(0);
}

char tic_93_in()
{
    char bit;

    hw_set_clk(1);
    hw_delay(TI/2);
    bit=hw_get_do() ? 1 : 0;
    hw_delay(TI/2);
    hw_set_clk(0);
    hw_delay(TI);
    return bit;
}

void start_93_seq()
{
    hw_set_cs(0);
    hw_set_clk(0);
    hw_set_di(0);    

    hw_delay(TI);
    hw_set_di(1);
    hw_delay(TI/2);
    hw_set_cs(1);        
    hw_delay(TI/2);
    hw_set_clk(1);
    hw_delay(TI);
    hw_set_clk(0);
}

void stop_93_seq()
{
    hw_set_cs(0);
    hw_set_clk(0);
    hw_set_di(0);    
    hw_delay(TI);
}

void send_instruction(unsigned int instr, unsigned char bytes)
{
    for(;bytes; bytes--) tic_93_out( (instr >> (bytes-1)) & 0x01);
}

unsigned int recv_93_data(char bites)
{
    unsigned int data=0;
    
    hw_set_di(0);
    for(; bites; bites--) data |= tic_93_in() << (bites-1);
    return data;
}

void send_op_code(unsigned int opcode, char bits)
{
    start_93_seq();
    send_instruction(opcode << (bits - 2), 2 + bits);
    stop_93_seq();
}

/**************************************************************************************************/
void read_9346(int dev_size, char data_bits, char addr_bits)
{
    int i;
    unsigned int d;
    
    if(conn_test()) return;
    gui_progress_bar_init(___geep___,"Odczyt pamieci",dev_size);

    reset_93_socket();
    hw_sw_vcc(1);
    for(i = 0; i < dev_size; ){
	start_93_seq();
	send_instruction(2,2);
	send_instruction(i,addr_bits);

	d=recv_93_data(data_bits);
	if(data_bits==16)
	    buffer_write(___geep___,i++,(d >> 8) & 0xff);
	buffer_write(___geep___,i++,d & 0xff);
	stop_93_seq();
	hw_delay(TCR);
	gui_progress_bar_set(___geep___, i, dev_size);
    }
    gui_progress_bar_free(___geep___);
    finish_action();
}


void erase_9346(int dev_size, char data_bits, char addr_bits)
{
    if(conn_test()) return;

    reset_93_socket();
    hw_sw_vcc(1);
    send_op_code(EWEN_93, addr_bits);
    send_op_code(ERAL_93, addr_bits);
    send_op_code(EWDS_93, addr_bits);
    hw_sw_vcc(0);

    read_9346(dev_size, data_bits, addr_bits);
}

void prog_9346(int dev_size, char data_bits, char addr_bits)
{
    int i;

    if(conn_test()) return;
    gui_progress_bar_init(___geep___,"Programowanie pamieci",dev_size);

    reset_93_socket();
    hw_sw_vcc(1);

    send_op_code(EWEN_93, addr_bits);

    for(i = 0; i < dev_size;){
	start_93_seq();
	send_instruction(1,2);
	send_instruction(i,addr_bits);
	send_instruction(buffer_read(___geep___,i++),8);
if(addr_bits==16)
	send_instruction(buffer_read(___geep___,i++),8);
	stop_93_seq();
	hw_delay(TCW);
	gui_progress_bar_set(___geep___,i, dev_size);
    }
    send_op_code(EWDS_93, addr_bits);
    hw_sw_vcc(0);
    gui_progress_bar_free(___geep___);
}

/**************************************************************************************************
*
* Bezposrednia obsluga funkcji zwrotnych, które wraz z wlasciwymi parametrami wykonaja wlasciwe akcje
*
*/

REG_FUNC_BEGIN(erase_9346A)
    erase_9346(PARM_9346);
REG_FUNC_END

REG_FUNC_BEGIN(prog_9346A)
    prog_9346(PARM_9346);
REG_FUNC_END

REG_FUNC_BEGIN(read_9346A)
    read_9346(PARM_9346);
REG_FUNC_END

REG_FUNC_BEGIN(erase_9356)
    erase_9346(PARM_9356);
REG_FUNC_END

REG_FUNC_BEGIN(prog_9356)
    prog_9346(PARM_9356);
REG_FUNC_END

REG_FUNC_BEGIN(read_9356)
    read_9346(PARM_9356);
REG_FUNC_END

REG_FUNC_BEGIN(erase_9366)
    erase_9346(PARM_9366);
REG_FUNC_END

REG_FUNC_BEGIN(prog_9366)
    prog_9346(PARM_9366);
REG_FUNC_END

REG_FUNC_BEGIN(read_9366)
    read_9346(PARM_9366);
REG_FUNC_END


/**************************************************************************************************
*
* Rejestracja pluginu
*
*/

REGISTER_MODULE_BEGIN( 93Cxx )

    register_chip_begin("/Serial EEPROM/93Cxx", "93C46", "93Cxx", C46_SIZE);
	add_action(MODULE_READ_ACTION, read_9346A);
	add_action(MODULE_PROG_ACTION, prog_9346A);
    	add_action(MODULE_ERASE_ACTION, erase_9346A);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/93Cxx", "93LC46", "93Cxx", C46_SIZE);
	add_action(MODULE_READ_ACTION, read_9346A);
	add_action(MODULE_PROG_ACTION, prog_9346A);
	add_action(MODULE_ERASE_ACTION, erase_9346A);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/93Cxx", "93C56", "93Cxx", C56_SIZE);
	add_action(MODULE_READ_ACTION, read_9356);
	add_action(MODULE_PROG_ACTION, prog_9356);
	add_action(MODULE_ERASE_ACTION, erase_9356);
    register_chip_end;

    register_chip_begin("/Serial EEPROM/93Cxx", "93C66", "93Cxx", C66_SIZE);
	add_action(MODULE_READ_ACTION, read_9366);
	add_action(MODULE_PROG_ACTION, prog_9366);
	add_action(MODULE_ERASE_ACTION, erase_9366);
    register_chip_end;

REGISTER_MODULE_END

