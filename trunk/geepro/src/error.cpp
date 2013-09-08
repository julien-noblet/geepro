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

#include <stdio.h>
#include <stdarg.h>
#include "error.h"

//static void error_vprintf(s_error *err, const char *fmt, va_list args);

void error_printf(s_error *err, const char *fmt, ...)
{
/*
    va_list args;
    va_start( args, fmt );

    if( err == NULL ){
	vprintf( fmt, args); // default output to stdio if no channel 
    } else
        error_vprintf( err, fmt, args );
    }
    va_end( args );
*/
}

//static void error_vprintf(s_error *err, const char *fmt, va_list args)
//{
//    vprintf( fmt, args); // temporary implementation
//}

