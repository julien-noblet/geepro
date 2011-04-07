/* $Revision: 1.4 $ */
#ifndef __GEEPRO_H_
#define __GEEPRO_H_
/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2007 Krzysztof Komarnicki
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

#include "iface.h"
#include "chip.h"

/* Informacje o wersji */
#define EPROGRAM_NAME "Geepro"
#define ERELDATE "1 Feb 2020"
#define EVERSION "0.0.2 pre"
#define EAUTHORS "Krzysztof Komarnicki krzkomar@wp.pl"
#define ELICENSE "GPL version 2"

/* glówna struktura programu */
typedef struct
{
    int   argc;		
    char  forbid; // blokuje akcje 
    char  **argv;
    iface *ifc;	/* struktura zawierająca kolejki pluginów */
    void  *gui;
    chip_desc  *chp;
} geepro;

#endif

