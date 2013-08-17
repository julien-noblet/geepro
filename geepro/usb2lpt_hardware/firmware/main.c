/* Name: main.c
   Project: Usb2Lpt Converter for geepro
   Author: Krzysztof Komarnicki 
   Creation Date: 17.08.2013
   License: GNU GPL v2 (see License.txt), GNU GPL v3
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "requests.h"       /* The custom request numbers we use */


#define	BIT_LO	(1 << 7)
#define	BIT_HI	(1 << 6)

/*
    Output 12 bit data
    data_lo :
	0..5 -> D0..D5
	6,7  -> D6,D7	pin 2..9
    data_hi :
	0    -> SELIN	pin 17
	1    -> INIT 	pin 16
	2    -> AUTO	pin 14
	3    -> STB	pin 1
*/
inline void set_pins( uchar data_lo, uchar data_hi )
{
    uchar tmp;
    tmp   = data_lo & 0x003f;
    PORTB = tmp; 		  // output MD0..MD5, HI=0, LO = 0
    PORTB = tmp | BIT_LO;         // HI = 0, LO = 1 -> latch IC2
    tmp   = ((data_lo >> 6) & 0x03) | ((data_hi << 2) & 0xfc); 
    PORTB = tmp;    		  // output MD6..MD11, HI=0, LO = 0
    PORTB = tmp | BIT_HI;         // HI = 1, LO = 0 -> latch IC1
    PORTB = tmp;    		  // HI = 0, LO = 0
}

/*
    Input 5 bits of ctrl status LPT
    0 -> ACK	pin 10 
    1 -> BUSY	pin 11
    2 -> PE	pin 12
    3 -> SEL	pin 13
    4 -> ERR	pin 15    
*/
inline uchar get_pins( void )
{
    uchar tmp;
    tmp = PIND;
    return (tmp & 0x03) | ((tmp >> 2) & 0x3c);
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t    *rq = (void *)data;
    static uchar    dataBuffer[7];  // buffer must stay valid when usbFunctionSetup returns

    switch( rq->bRequest ){
	case USB_RQ_ECHO:
		dataBuffer[0] = rq->wValue.bytes[0];
    		dataBuffer[1] = rq->wValue.bytes[1];
    		dataBuffer[2] = rq->wValue.bytes[2];
    		dataBuffer[3] = rq->wValue.bytes[3];
    		usbMsgPtr = dataBuffer;
    		return 4;	// 4 bytes return
	case USB_RQ_OUTPUT:
		set_pins( rq->wIndex.bytes[0] , rq->wIndex.bytes[1] );
		return 0;	// no returns
	case USB_RQ_INPUT:
		dataBuffer[0] = get_pins();
    		usbMsgPtr = dataBuffer;
    		return 1;       // one byte return
	case USB_RQ_ID:
		dataBuffer[0] = 'G';
    		dataBuffer[1] = 'e';
    		dataBuffer[2] = 'e';
    		dataBuffer[3] = 'p';
    		dataBuffer[4] = 'r';
    		dataBuffer[5] = 'o';
    		dataBuffer[6] = 0;    		
    		usbMsgPtr = dataBuffer;
    		return 7;       // one byte return
	case USB_RQ_REV:
		dataBuffer[0] = 1;
		dataBuffer[1] = 0;
    		usbMsgPtr = dataBuffer;
    		return 2;       // one byte return
    }
    return 0;   // default - no return bytes
}

int __attribute__((noreturn)) main(void)
{
    uchar   i;

    DDRB  = 0xff;	// whole port B as output
    DDRD  = 0x00;
    PORTD = 0xf3;  // pullup resistors on
    wdt_enable(WDTO_1S);
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    set_pins( 0, 0 );
    sei();
    for(;;){                /* main event loop */
        wdt_reset();
        usbPoll();
    }
}

