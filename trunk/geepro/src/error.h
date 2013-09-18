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

/*
    Error message routines
*/

#ifndef __ERROR_H__
#define __ERROR_H__

typedef enum{
    E_MSG,	// message
    E_WRN,	// warning
    E_ERR,	// normal error
    E_CRIT,	// critical error
    E_CRT,	// critical error
} t_error_class;

enum{
    E_OK = 0,
    E_MEM,
    E_NULL,
    E_PAR,
};

enum
{
    _F_ = 0,
    _T_ = 1,
};

typedef char t_error;
typedef char t_bool;

typedef struct
{
    char *stream;
} s_error;

extern void error_printf(s_error *err, const char *fmt, ... ); // prints formatted mesaage to error channel. If err = NULL then output to stdout

// x have to be variable !
#define DEC_8( x )	(x) / 100, ((x) % 100) / 10, ((x) % 10) 
#define HEX_16( x )	((x) >> 12) & 0x0f, ((x) >> 8) & 0x0f, ((x) >> 4) & 0x0f, (x)  & 0x0f

#define E_T_MALLOC			"malloc() == NULL"
#define MALLOC_ERR			"malloc!\n"
#define _ERROR(err, err_class, format, x...)  error_printf(err, "[ERR]{%i-%i}:%s:%s()->"format"\n", err_class, __LINE__, __FILE__,__FUNCTION__, ##x)
#define _ERR( err, fmt, x...)		ERROR( err, E_ERR, fmt, ##x)
#define _MSG( err, fmt, x...)		ERROR( err, E_MSG, fmt, ##x)
#define _WRN( err, fmt, x...)		ERROR( err, E_WRN, fmt, ##x)
#define _CRIT( err, fmt, x...)		ERROR( err, E_CRIT, fmt, ##x)
#define EMALLOC( err )			CRIT( err, E_T_MALLOC);

#define ERROR(err_class, format, x...)  printf("[ERR]{%i-%i}:%s:%s()->"format"\n", err_class, __LINE__, __FILE__,__FUNCTION__, ##x)
#define MESSAGE(format, x...)   printf("[MSG] "format"\n", ##x)
#define ERR( fmt, x...)		ERROR( E_ERR, fmt, ##x)
#define MSG( fmt, x...)		MESSAGE( fmt, ##x)
#define WRN( fmt, x...)		ERROR( E_WRN, fmt, ##x)
#define CRT( fmt, x...)		ERROR( E_CRIT, fmt, ##x)
#define PRN( fmt, x...)		printf( fmt, x)

#define ERR_MALLOC_CODE	-1
#define ERR_MALLOC_MSG	MSG( E_T_MALLOC )
#define MALLOC( var, type, size )	if(!((var) = (type *) malloc( sizeof( type ) * (size) )))
        
#endif // __ERROR_H__

