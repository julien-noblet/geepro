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

#ifndef __PROTOCOLS_H__
#define __PROTOCOLS_H__

/* I2C protocol */
extern void init_i2c();
extern void scl_tik_i2c();
extern void start_i2c();
extern void stop_i2c();
extern void send_bit_i2c( char bit );
extern char get_bit_i2c();
extern void send_byte_i2c( char byte );
extern char recv_byte_i2c();
extern char wait_ack_i2c();

#endif