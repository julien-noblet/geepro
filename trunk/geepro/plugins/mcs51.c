/* $Revision: 1.7 $ */
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

#define SIZE_AT89C1051	1024
#define SIZE_AT89C2051	2048
#define SIZE_AT89C4051	4096

#define SIZE_AT89C51	4096
#define SIZE_AT89C52	8192

//AT89C5x Mode
#define AT89C5x_READ_MODE	(0x0c << 15)
#define AT89C5x_WRITE_MODE	(0x0e << 15)
#define AT89C5x_LB1_MODE	(0x0f << 15)
#define AT89C5x_LB2_MODE	(0x03 << 15)
#define AT89C5x_LB3_MODE	(0x05 << 15)
#define AT89C5x_ERASE_MODE	(0x01 << 15)
#define AT89C5x_SIGN_MODE	(0x00 << 15)
#define AT89C5x_P27_H_MODE	(0x02 << 15)
#define AT89C5x_MODE_MASK	0x78000

// AT89Cx051 modes
#define AT89Cx051_WR_MODE	(0x0e << 6)
#define AT89Cx051_RD_MODE	(0x0a << 6)
#define AT89Cx051_LB1_MODE	(0x0f << 6)
#define AT89Cx051_LB2_MODE	(0x05 << 6)
#define AT89Cx051_ERA_MODE	(0x01 << 6)
#define AT89Cx051_SIGN_MODE	(0x00 << 6)
#define AT89Cx051_MODE_MASK	0x003c0

// AT89Cx051 adapter mux 
#define AT89Cx051_X1_MUX	(0x03 << 15)
#define AT89Cx051_X1_OFF_MUX	(0x00 << 15)
#define AT89Cx051_PROG_MUX	(0x02 << 17)
#define AT89Cx051_RST_MUX	(0x01 << 17)
#define AT90Sx_INT1_MUX		(0x03 << 17)
#define AT89Cx051_MUX_MASK	0x78000

// stan linii RESET
#define AT89Cx051_RST_LOW	0
#define AT89Cx051_RST_HIGH	1
#define AT89Cx051_RST_VPP	2

char at89cXX_lb = 0;	/* default mode for LB1, LB2 */
int addr_state = 0;

// Low level access to adapter
//-----------------------------------------------------------------------------------------------------
void set_AT89Cx051_mode(int mode) // Sets mode pins: P3.3, P3.4, P3.5, P3.7
{
    addr_state &= ~AT89Cx051_MODE_MASK;
    addr_state |= mode & AT89Cx051_MODE_MASK;
    set_address(addr_state);  /* ustawienie linii selekcji stanu: A6-P3.3, A7-P3.5, A8-P3.4, A9-P3.7*/
}

void AT89Cx051_pulse(int duration) // generate programing/address pulse at S4
{
    ce(0, duration);
    ce(1,1);
}

void AT89Cx051_mux(int mode) // Sets A15,A16,A17,A18
{
    addr_state &= ~AT89Cx051_MUX_MASK;
    addr_state |= mode & AT89Cx051_MUX_MASK;
    set_address(addr_state);
}

void AT89Cx051_RST(char state )
{
    switch(state){
	case AT89Cx051_RST_LOW:	 hw_sw_vpp(0); ce(0, 1); AT89Cx051_mux(AT89Cx051_RST_MUX);  break;
	case AT89Cx051_RST_HIGH: hw_sw_vpp(0); ce(1, 1); AT89Cx051_mux(AT89Cx051_RST_MUX);  break;
	case AT89Cx051_RST_VPP:  ce(1, 1); AT89Cx051_mux(AT89Cx051_PROG_MUX); hw_sw_vpp(1); break;
    }        
}

unsigned char AT89Cx051_transcode0(unsigned char data)
{
    char tmp;
    tmp  = (data << 4) & 0xc0;
    tmp |= (data >> 7) & 0x01;
    tmp |= (data >> 5) & 0x02;
    tmp |= (data >> 2) & 0x08;
    tmp |= (data << 1) & 0x20;
    tmp |= (data << 3) & 0x10;
    tmp |= (data << 2) & 0x04;
    return tmp;
}

unsigned char AT89Cx051_transcode1(unsigned char data)
{
    char tmp;
    tmp  = (data >> 4) & 0x0c;
    tmp |= (data >> 1) & 0x10;
    tmp |= (data >> 3) & 0x02;
    tmp |= (data << 2) & 0x20;
    tmp |= (data >> 2) & 0x01;
    tmp |= (data << 5) & 0x40;
    tmp |= (data << 7) & 0x80;
    return tmp;
}

unsigned char AT89Cx051_get_data()
{
    return AT89Cx051_transcode0(hw_get_data());
}

void AT89Cx051_put_data(char data)
{
    set_data( AT89Cx051_transcode1( data & 0xff ));
}

//----------------------------------------------------------------------------------------------------------

int test_blank_AT89Cx051(int size, char mode)
{
    int addr = 0;
    int tmp;
    TEST_CONNECTION( 1 )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    set_AT89Cx051_mode(AT89Cx051_RD_MODE); // set mode
    hw_delay(1000); // 1ms    
    AT89Cx051_mux(AT89Cx051_X1_MUX); // X1 as pulse
    progress_loop(addr, size, "Checking blank"){
	hw_delay(100);
	tmp = AT89Cx051_get_data();
	if( tmp != 0xff){
	   progressbar_free();
	   break;
	}
	AT89Cx051_pulse( 150 );
    }
    if(!mode) show_message(0,(tmp == 0xff) ? "[IF][TEXT]Flash is empty[/TEXT][BR]OK" : "[WN][TEXT]Flash is dirty !!![/TEXT][BR]OK",NULL, NULL);
    set_address(0);
    finish_action();
    return tmp != 0xff;
}

void read_AT89Cx051(int size)
{
    int addr = 0;
    TEST_CONNECTION(VOID)
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    set_AT89Cx051_mode(AT89Cx051_RD_MODE); // set mode
    hw_delay(1000); // 1ms    
    AT89Cx051_mux(AT89Cx051_X1_MUX); // X1 as pulse
    progress_loop(addr, size, "Reading"){
	hw_delay(100);
	put_buffer( addr, AT89Cx051_get_data());
	AT89Cx051_pulse( 150 );
    }
    set_address(0);
    finish_action();
}

void sign_AT89Cx051(int size)
{
    int addr = 0;
    int signature = 0;
    char text[256];
    TEST_CONNECTION(VOID)    
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilisation
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    set_AT89Cx051_mode(AT89Cx051_SIGN_MODE); // set mode
    hw_delay(1000); // 1ms    
    AT89Cx051_mux(AT89Cx051_X1_MUX); // X1 as pulse
    progress_loop(addr, 3, "Reading"){
	hw_delay(100);
	signature |= AT89Cx051_get_data() << (addr * 8);
	AT89Cx051_pulse( 150 );
    }
    set_address(0);
    finish_action();
    sprintf(
	text, "[IF][TEXT]Chip signature: 0x%X%X%X%X%X%X\n%s[/TEXT][BR]OK", 
	to_hex(signature, 5),to_hex(signature, 4),  
	to_hex(signature, 3),to_hex(signature, 2),  
	to_hex(signature, 1),to_hex(signature, 0), 
	take_signature_name( signature )
    );
    show_message(0, text,NULL,NULL);
}

void verify_AT89Cx051(int size, char silent)
{
    int addr = 0;
    char rdata, wdata;
    char text[256];
    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    set_AT89Cx051_mode(AT89Cx051_RD_MODE); // set mode
    hw_delay(1000); // 1ms    
    AT89Cx051_mux(AT89Cx051_X1_MUX); // X1 as pulse
    progress_loop(addr, size, "Veryfying"){
	hw_delay(100);
	rdata = AT89Cx051_get_data();
	wdata = get_buffer(addr);
	if( rdata != wdata){
	   progressbar_free();
	   break;
	}
	AT89Cx051_pulse( 150 );
    }
    if(!silent | (wdata != rdata)){
	 sprintf(text, "[WN][TEXT]Flash and buffer differ !!!\nAddress = 0x%X \nBuffer = 0x%X%X\nChip = 0x%X%X[/TEXT][BR]OK",
	    addr,
	    to_hex(wdata, 1), to_hex(wdata, 0),
	    to_hex(rdata, 1), to_hex(rdata, 0) 
	 );	    
	 show_message(0,(rdata == wdata) ? "[IF][TEXT]Flash and buffer are consistent[/TEXT][BR]OK" : text, NULL, NULL);
     }
    set_address(0);
    finish_action();
}

void erase_AT89Cx051(int size)
{
    TEST_CONNECTION(VOID)
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    set_AT89Cx051_mode(AT89Cx051_ERA_MODE); // set mode to erase
    hw_delay(1000); // 1ms    
    AT89Cx051_mux(AT89Cx051_PROG_MUX); // X1 as pulse
    hw_sw_vpp(1);	// set VPP    
    // erase pulse
    AT89Cx051_pulse( 11000 ); // 11ms
    set_address(0);
    finish_action();
    test_blank_AT89Cx051(size, 0);
}

void write_AT89Cx051(int size)
{
    int addr = 0;
    char wdata, rdata;
    char text[256];
    TEST_CONNECTION(VOID)
    if(test_blank_AT89Cx051(size, 1)){
       SET_ERROR;
       return;
    }
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
    AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
    hw_delay(1000); // 1ms
    AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
    hw_delay(1000); // 1ms
    progress_loop(addr, size, "Writing"){
	set_AT89Cx051_mode(AT89Cx051_WR_MODE); // set mode to write
	hw_delay(100); // 100µs    
	AT89Cx051_mux(AT89Cx051_PROG_MUX); // PROG as pulse
	wdata = get_buffer(addr);	   // get data from buffer and store it for veryfication
	AT89Cx051_put_data(wdata);	   // out data
	hw_delay(10);
	hw_sw_vpp(1);			   // VPP ON
	hw_delay(20);
	AT89Cx051_pulse( 1250 );           // 1.25 ms program pulse
	hw_delay(20);
	hw_sw_vpp(0);			   // VPP OFF
	hw_delay(500); 			   // 100µs    
	set_AT89Cx051_mode(AT89Cx051_RD_MODE); // set mode to read for veryfication
	hw_delay(500); 			   // 500µs    
	rdata = AT89Cx051_get_data();
	if(rdata != wdata){                // veryfication
	    progressbar_free();
	    break;	   
	}
	AT89Cx051_mux(AT89Cx051_X1_MUX);   // X1 as pulse
	hw_delay(20);
	AT89Cx051_pulse( 150 );		   // address increment
	hw_delay(20);
    }
    if(rdata != wdata){
	sprintf(
	    text, "[ER][TEXT]Veryfication error: 0x%X%X do not match 0x%X%X at address 0x%X[/TEXT][BR]OK", 
	    to_hex(rdata, 1), to_hex(rdata, 0), 
	    to_hex(wdata, 1), to_hex(wdata, 0), 
	    addr
	);
	show_message(0,text, NULL, NULL);
	SET_ERROR;
    }
    set_address(0);
    finish_action();
    verify_AT89Cx051(size, 1);		// verificate whole at end
}

void lock_bit_AT89Cx051(int size)
{
    char lb = 0;
    lb = checkbox(
	"[TITLE]LOCKBITS[/TITLE][TYPE:QS]"
	"[CB:1:0:LB1 (further programing of the flash is disabled)]"
	"[CB:2:0:LB2 (same as LB1, also verify is disabled)]"
    );
    TEST_CONNECTION(VOID)
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    if(lb & 1){
	AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
	AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
	hw_delay(1000); // 1ms
	AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
	hw_delay(1000); // 1ms
	set_AT89Cx051_mode(AT89Cx051_LB1_MODE); // set mode to erase
	hw_delay(1000); // 1ms    
	AT89Cx051_mux(AT89Cx051_PROG_MUX); // X1 as pulse
	hw_sw_vpp(1);	// set VPP    
	// erase pulse
	AT89Cx051_pulse( 1250 ); // 1,25ms
	hw_delay(1000);
	hw_sw_vpp(0);
    }
    if(lb & 2){
	AT89Cx051_RST(AT89Cx051_RST_LOW);    // RST to GND
	AT89Cx051_mux(AT89Cx051_X1_OFF_MUX); // X1  to GND
	hw_delay(1000); // 1ms
	AT89Cx051_RST(AT89Cx051_RST_HIGH);    // RST to H, clear internal address counter
	hw_delay(1000); // 1ms
	set_AT89Cx051_mode(AT89Cx051_LB2_MODE); // set mode to erase
	hw_delay(1000); // 1ms    
	AT89Cx051_mux(AT89Cx051_PROG_MUX); // X1 as pulse
	hw_sw_vpp(1);	// set VPP    
	// erase pulse
	AT89Cx051_pulse( 1250 ); // 1,25ms
	hw_delay(1000);
	hw_sw_vpp(0);
    }
    set_address(0);
    finish_action();
}
/*************************************************************************/
void set_AT89C5x_mode(int mode)
{
    addr_state &= ~AT89C5x_MODE_MASK;
    addr_state |= mode & AT89C5x_MODE_MASK;
    set_address(addr_state);  /* ustawienie linii selekcji stanu: A15-P2.6, A16-P2.7, A17-P3.6, A18-P3.7 */
}

void set_AT89C5x_addr(int addr)
{
    addr_state &= AT89C5x_MODE_MASK;
    addr_state |= addr & ~AT89C5x_MODE_MASK;
    set_address(addr_state);  /* ustawienie linii adresowych A0..A14, z pominięciem linii sterujących */
}

#define AT89C5x_prog_pulse	AT89Cx051_pulse	

unsigned char read_byte_AT89C5x(int addr, int mode)
{
    set_AT89C5x_mode( mode | AT89C5x_P27_H_MODE);
    hw_delay(1); // 1us 
    set_AT89C5x_addr(addr);
    hw_delay(1); // 1us 
    set_AT89C5x_mode( mode );	
    hw_delay(1); // 1us 
    return hw_get_data();
}

void read_AT89C5x(int size)
{ 
    int addr = 0;
    int signature;
    addr_state = 0;

    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    signature = 0;
    progress_loop(addr, size, "Reading")
	put_buffer( addr, read_byte_AT89C5x(addr, AT89C5x_READ_MODE) );

    set_address(0);
    set_data(0);
    finish_action();
}

void sign_AT89C5x(int size)
{ 

    int addr = 0;
    int signature;
    char text[256];
    addr_state = 0;
    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    signature = 0;
    progress_loop(addr, 3, "Reading")
	signature |= read_byte_AT89C5x(addr + 0x30, AT89C5x_SIGN_MODE) << (8 * addr);
    set_address(0);
    set_data(0);
    finish_action();
    sprintf(
	text, "[IF][TEXT]Chip signature: 0x%X%X%X%X%X%X\n%s[/TEXT][BR]OK", 
	to_hex(signature, 5),to_hex(signature, 4),  
	to_hex(signature, 3),to_hex(signature, 2),  
	to_hex(signature, 1),to_hex(signature, 0), 
	take_signature_name( signature )
    );
    show_message(0, text,NULL,NULL);
}

void verify_AT89C5x(int size, char silent)
{ 
    int addr = 0;
    char text[256];
    addr_state = 0;
    char rdata, wdata;

    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    progress_loop(addr, size, "Verifying"){
	rdata = read_byte_AT89C5x(addr, AT89C5x_READ_MODE);
	wdata = get_buffer(addr);
	if( rdata != wdata){
	   progressbar_free();
	   break;
	}
    }

    if(!silent | (wdata != rdata)){
	 sprintf(text, "[WN][TEXT]Flash and buffer differ !!!\nAddress = 0x%X \nBuffer = 0x%X%X\nChip = 0x%X%X[/TEXT][BR]OK",
	    addr,
	    to_hex(wdata, 1), to_hex(wdata, 0),
	    to_hex(rdata, 1), to_hex(rdata, 0) 
	 );	    
	 show_message(0,(rdata == wdata) ? "[IF][TEXT]Flash and buffer are consistent[/TEXT][BR]OK" : text, NULL, NULL);
     }
    
    set_address(0);
    set_data(0);
    finish_action();
}

void test_blank_AT89C5x(int size, char silent)
{ 
    int addr = 0;
    char text[256];
    addr_state = 0;
    unsigned char rdata;

    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    progress_loop(addr, size, "Test blank"){
	rdata = read_byte_AT89C5x(addr, AT89C5x_READ_MODE);
	if( rdata != 0xff){
	   progressbar_free();
	   break;
	}
    }
    if(!silent | (rdata != 0xff)){
	 sprintf(text, "[WN][TEXT]Flash is not empty !!!\nAddress = 0x%X\n byte = 0x%X%X[/TEXT][BR]OK",
	    addr,
	    to_hex(rdata, 1), to_hex(rdata, 0) 
	 );	    
	 show_message(0,(rdata == 0xff) ? "[IF][TEXT]Flash is clear[/TEXT][BR]OK" : text, NULL, NULL);
    }
    set_address(0);
    set_data(0);
    finish_action();
}
/*
    set_AT89C5x_mode( mode | AT89C5x_P27_H_MODE);
    hw_delay(1); // 1us 
    set_AT89C5x_addr(addr);
    hw_delay(1); // 1us 
    set_AT89C5x_mode( mode );	
    hw_delay(1); // 1us 
    return hw_get_data();
*/
#define AT89C5x_pulse	AT89Cx051_pulse

void write_AT89C5x(int size)
{ 
    int addr = 0;
    unsigned char wdata, rdata;
    char text[256];
    addr_state = 0;

    TEST_CONNECTION( VOID )
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    progress_loop(addr, size, "Writing"){
        wdata = get_buffer(addr);
        if(wdata == 0xff){
	    rdata = 0xff;
    	    continue; // skip 0xff
        }
	set_AT89C5x_mode( AT89C5x_WRITE_MODE);
	hw_sw_vpp(1);
	hw_delay(1); // 1us 
        set_AT89C5x_addr(addr);
        set_data(wdata);
	hw_delay(1); // 1us 
	AT89C5x_pulse( 1250 );   // 1.25 ms program pulse
	hw_sw_vpp(0);
	hw_delay(400); // 400us 
	set_AT89C5x_mode( AT89C5x_READ_MODE );	
	hw_delay(400); // 400us 
	rdata = hw_get_data();
	if(rdata != wdata){
	   progressbar_free();
	   break;
	}
    }
    if(rdata != wdata){
	sprintf(
	    text, "[ER][TEXT]Veryfication error: 0x%X%X do not match 0x%X%X at address 0x%X[/TEXT][BR]OK", 
	    to_hex(rdata, 1), to_hex(rdata, 0), 
	    to_hex(wdata, 1), to_hex(wdata, 0), 
	    addr
	);
	show_message(0,text, NULL, NULL);
	SET_ERROR;
    }
    set_address(0);
    set_data(0);
    finish_action();
    verify_AT89C5x(size, 1); // verificate whole at end
}

void erase_AT89C5x(int size)
{ 
    TEST_CONNECTION(VOID)
    hw_sw_vpp(0);   // VPP OFF
    hw_sw_vcc(1);   // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    ce(1,100);
    set_AT89C5x_mode( AT89C5x_ERASE_MODE);
    hw_sw_vpp(1);	// set VPP    
    // erase pulse
    AT89C5x_pulse( 11000 ); // 11ms
    set_address(0);
    finish_action();
    test_blank_AT89C5x(size, 0);
}

void lock_bit_AT89C5x(int size)
{ 
    char lb = 0;
    lb = checkbox(
	"[TITLE]LOCKBITS[/TITLE][TYPE:QS]"
	"[CB:1:0:LB1 (further programing of the flash is disabled, lock on MOVC)]"
	"[CB:2:0:LB2 (same as LB1, also verify is disabled)]"
	"[CB:4:0:LB3 (same as LB2, also external execution is disabled)]"
    );
    TEST_CONNECTION(VOID)
    hw_sw_vpp(0);    // VPP OFF
    hw_sw_vcc(1);    // VCC ON
    hw_delay(10000); // 10ms for power stabilise
    if(lb & 1){
	ce(1,100);
	set_AT89C5x_mode( AT89C5x_LB1_MODE);
	hw_sw_vpp(1);	// set VPP    
	AT89C5x_pulse( 11000 ); // 11ms
	set_address(0);
	hw_sw_vpp(0);	// set VPP    
    }
    hw_delay(1000); // 1ms
    if(lb & 2){
	ce(1,100);
	set_AT89C5x_mode( AT89C5x_LB2_MODE);
	hw_sw_vpp(1);	// set VPP    
	AT89C5x_pulse( 11000 ); // 11ms
	set_address(0);
	hw_sw_vpp(0);	// set VPP    
    }
    hw_delay(1000); // 1ms
    if(lb & 4){
	ce(1,100);
	set_AT89C5x_mode( AT89C5x_LB3_MODE);
	hw_sw_vpp(1);	// set VPP    
	AT89C5x_pulse( 11000 ); // 11ms
	set_address(0);
	hw_sw_vpp(0);	// set VPP    
    }
    finish_action();
}

/************************************************************************
* AT89Cx051
*/
REGISTER_FUNCTION( read,       AT89C1051, AT89Cx051, SIZE_AT89C1051 );
REGISTER_FUNCTION( read,       AT89C2051, AT89Cx051, SIZE_AT89C2051 );
REGISTER_FUNCTION( read,       AT89C4051, AT89Cx051, SIZE_AT89C4051 );
REGISTER_FUNCTION( verify,     AT89C1051, AT89Cx051, SIZE_AT89C1051, 0 );
REGISTER_FUNCTION( verify,     AT89C2051, AT89Cx051, SIZE_AT89C2051, 0 );
REGISTER_FUNCTION( verify,     AT89C4051, AT89Cx051, SIZE_AT89C4051, 0 );
REGISTER_FUNCTION( write,      AT89C1051, AT89Cx051, SIZE_AT89C1051 );
REGISTER_FUNCTION( write,      AT89C2051, AT89Cx051, SIZE_AT89C2051 );
REGISTER_FUNCTION( write,      AT89C4051, AT89Cx051, SIZE_AT89C4051 );
REGISTER_FUNCTION( erase,      AT89C1051, AT89Cx051, SIZE_AT89C1051 );
REGISTER_FUNCTION( erase,      AT89C2051, AT89Cx051, SIZE_AT89C2051 );
REGISTER_FUNCTION( erase,      AT89C4051, AT89Cx051, SIZE_AT89C4051 );
REGISTER_FUNCTION( sign,       AT89C1051, AT89Cx051, SIZE_AT89C1051 );
REGISTER_FUNCTION( sign,       AT89C2051, AT89Cx051, SIZE_AT89C2051 );
REGISTER_FUNCTION( sign,       AT89C4051, AT89Cx051, SIZE_AT89C4051 );
REGISTER_FUNCTION( lock_bit,   AT89C1051, AT89Cx051, SIZE_AT89C1051 );
REGISTER_FUNCTION( lock_bit,   AT89C2051, AT89Cx051, SIZE_AT89C2051 );
REGISTER_FUNCTION( lock_bit,   AT89C4051, AT89Cx051, SIZE_AT89C4051 );
REGISTER_FUNCTION( test_blank, AT89C1051, AT89Cx051, SIZE_AT89C1051, 0 );
REGISTER_FUNCTION( test_blank, AT89C2051, AT89Cx051, SIZE_AT89C2051, 0 );
REGISTER_FUNCTION( test_blank, AT89C4051, AT89Cx051, SIZE_AT89C4051, 0 );

/************************************************************************
* AT89C5x
*/
REGISTER_FUNCTION( read,       AT89C51, AT89C5x, SIZE_AT89C51 );
REGISTER_FUNCTION( read,       AT89C52, AT89C5x, SIZE_AT89C52 );
REGISTER_FUNCTION( write,      AT89C51, AT89C5x, SIZE_AT89C51 );
REGISTER_FUNCTION( write,      AT89C52, AT89C5x, SIZE_AT89C52 );
REGISTER_FUNCTION( verify,     AT89C51, AT89C5x, SIZE_AT89C51, 0 );
REGISTER_FUNCTION( verify,     AT89C52, AT89C5x, SIZE_AT89C52, 0 );
REGISTER_FUNCTION( erase,      AT89C51, AT89C5x, SIZE_AT89C51 );
REGISTER_FUNCTION( erase,      AT89C52, AT89C5x, SIZE_AT89C52 );
REGISTER_FUNCTION( sign,       AT89C51, AT89C5x, SIZE_AT89C51 );
REGISTER_FUNCTION( sign,       AT89C52, AT89C5x, SIZE_AT89C52 );
REGISTER_FUNCTION( lock_bit,   AT89C51, AT89C5x, SIZE_AT89C51 );
REGISTER_FUNCTION( lock_bit,   AT89C52, AT89C5x, SIZE_AT89C52 );
REGISTER_FUNCTION( test_blank, AT89C51, AT89C5x, SIZE_AT89C51, 0 );
REGISTER_FUNCTION( test_blank, AT89C52, AT89C5x, SIZE_AT89C52, 0 );

/*************************************************************************/

REGISTER_MODULE_BEGIN( MCS-51 )
// INTEL i8751, i8742
//    register_chip_begin("/uk/MCS-51/INTEL","i8751", "i8751", 1);
//	add_action(MODULE_READ_ACTION, dummy);
//	add_action(MODULE_PROG_ACTION, dummy);
//	add_action(MODULE_VERIFY_ACTION, dummy);
//	add_action(MODULE_LOCKBIT_ACTION, dummy);
//    register_chip_end;
//    register_chip_begin("/uk/MCS-51/INTEL","i8742", "i8742", 1);
//	add_action(MODULE_READ_ACTION, dummy);
//	add_action(MODULE_PROG_ACTION, dummy);
//	add_action(MODULE_VERIFY_ACTION, dummy);
//	add_action(MODULE_LOCKBIT_ACTION, dummy);
//    register_chip_end;
// ATMEL AT89Cx051
    register_chip_begin("/uk/MCS-51/ATMEL/AT89Cx051","AT89C1051", "AT89Cxx51", SIZE_AT89C1051);
	add_action(MODULE_READ_ACTION, read_AT89C1051);
	add_action(MODULE_PROG_ACTION, write_AT89C1051);
	add_action(MODULE_VERIFY_ACTION, verify_AT89C1051);
	add_action(MODULE_LOCKBIT_ACTION, lock_bit_AT89C1051);
	add_action(MODULE_ERASE_ACTION, erase_AT89C1051);
	add_action(MODULE_SIGN_ACTION, sign_AT89C1051);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_AT89C1051);
    register_chip_end;
    register_chip_begin("/uk/MCS-51/ATMEL/AT89Cx051","AT89C2051", "AT89Cxx51", SIZE_AT89C2051);
	add_action(MODULE_READ_ACTION, read_AT89C2051);
	add_action(MODULE_PROG_ACTION, write_AT89C2051);
	add_action(MODULE_VERIFY_ACTION, verify_AT89C2051);
	add_action(MODULE_LOCKBIT_ACTION, lock_bit_AT89C2051);
	add_action(MODULE_ERASE_ACTION, erase_AT89C2051);
	add_action(MODULE_SIGN_ACTION, sign_AT89C2051);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_AT89C2051);
    register_chip_end;
    register_chip_begin("/uk/MCS-51/ATMEL/AT89Cx051","AT89C4051", "AT89Cxx51", SIZE_AT89C4051);
	add_action(MODULE_READ_ACTION, read_AT89C4051);
	add_action(MODULE_PROG_ACTION, write_AT89C4051);
	add_action(MODULE_VERIFY_ACTION, verify_AT89C4051);
	add_action(MODULE_LOCKBIT_ACTION, lock_bit_AT89C4051);
	add_action(MODULE_ERASE_ACTION, erase_AT89C4051);
	add_action(MODULE_SIGN_ACTION, sign_AT89C4051);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_AT89C4051);
    register_chip_end;
// ATMEL AT89C5x
    register_chip_begin("/uk/MCS-51/ATMEL/AT89C5x","AT89C51", "AT89C5x", SIZE_AT89C51);
	add_action(MODULE_READ_ACTION, read_AT89C51);
	add_action(MODULE_PROG_ACTION, write_AT89C51);
	add_action(MODULE_VERIFY_ACTION, verify_AT89C51);
	add_action(MODULE_LOCKBIT_ACTION, lock_bit_AT89C51);
	add_action(MODULE_ERASE_ACTION, erase_AT89C51);
	add_action(MODULE_SIGN_ACTION, sign_AT89C51);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_AT89C51);
    register_chip_end;
    register_chip_begin("/uk/MCS-51/ATMEL/AT89C5x","AT89C52", "AT89C5x", SIZE_AT89C52);
	add_action(MODULE_READ_ACTION, read_AT89C52);
	add_action(MODULE_PROG_ACTION, write_AT89C52);
	add_action(MODULE_VERIFY_ACTION, verify_AT89C52);
	add_action(MODULE_LOCKBIT_ACTION, lock_bit_AT89C52);
	add_action(MODULE_ERASE_ACTION, erase_AT89C52);
	add_action(MODULE_SIGN_ACTION, sign_AT89C52);
	add_action(MODULE_TEST_BLANK_ACTION, test_blank_AT89C52);
    register_chip_end;
REGISTER_MODULE_END

