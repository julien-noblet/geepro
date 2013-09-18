/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
 *
 * RS232 driver for geepro
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

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PACKET_XMODEM,
    PACKET_YMODEM,
    PACKET_ZMODEM,
    PACKET_KERMIT
} e_serial_proto;

// RS232 control lines
enum
{               // 25-pin    9-pin
    LINE_RQS,	//   4        7		- Request to Send
    LINE_CTS,	//   5        8		- Clear To Send
    LINE_DSR,	//   6        6		- Data Set Ready
    LINE_DCD,	//   8        1		- Data Carier Detect
    LINE_DTR,	//   20       4		- Data Terminal Ready
    LINE_RI	//   22       9		- Ring Indicator
};		//   2        3		- (TxD) Transmit data
		//   3        2		- (RxD) Receive Data
		//   7	      5		- (SG) Signal Ground

#define SERIAL_CALLBACK(x)	((f_serial_callback)(x))

typedef struct s_serial_list_ s_serial_list;

typedef struct
{
    s_serial_list *selected;
    s_serial_list *list;
} s_serial;

struct s_serial_list_
{
    s_serial	*main;
    char	*device_path;
    char	*alias;
    int		flags;
    int		bps;
    char	len;
    s_serial_list *next;
};

typedef char (*f_serial_callback)(s_serial *, s_serial_list *, void *);

/********************************************************
*		   System API				*
*********************************************************/
char serial_init(s_serial **);
void serial_exit(s_serial *);
char serial_open(s_serial *);
char serial_close(s_serial *);
char serial_set_device(s_serial *, const char *name);
char *serial_get_device(s_serial *);
void serial_get_list(s_serial *, f_serial_callback, void *);
/********************************************************
*		   I/O operations			*
*********************************************************/
// set transmision parameters
char serial_set_bitrate(s_serial *, int bitrate);
char serial_set_params(s_serial *, char flags);
// set value
char serial_send_byte(s_serial *, char byte);
char serial_send_string(s_serial *, char *bytes, int count);
char serial_set_ctrl_bits(s_serial *, char byte);
char serial_set_ctrl_bit(s_serial *, char state, char mask);
// get value
char serial_get_byte(s_serial *, char *byte);
char serial_get_string(s_serial *, char **byte, int count);
char serial_get_ctrl_bits(s_serial *);
char serial_get_ctrl_bit(s_serial *, char mask);
// send/receive packet
char serial_send_packet(s_serial *, char *bytes, int count, e_serial_proto mode);
char serial_get_packet(s_serial *, char *bytes, int count, e_serial_proto mode);

#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
