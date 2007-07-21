/* $Revision: 1.2 $ */
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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include  "../intl/lang.h"
#include "files.h"


#define SAVE_HEXRECSIZE 32
#define DEBUG printf

int file_load_bin(FILE *f, int size, char *buffer)
{
    fread(buffer, 512, size / 512, f);
    return 0;
}

int file_write_bin(FILE *f, int size, char *buffer)
{
    fwrite(buffer, 512, size / 512, f);
    return 0;
}

const char *file_err_msg(int err)
{
    if(!err) return NULL;
    const char *x="jakis blad";
    
    return x;
}

const char *file_save(geepro *gep, char *fname)
{
    FILE *f;
    int err;
    
    if(!gep->chp) return -1;    
    if(!(f = fopen(fname , "w"))) return -1;

    err = file_write_bin(f, gep->chp->dev_size, gep->chp->buffer);

    fclose(f);    
    return file_err_msg(err);
}

const char *file_load(geepro *gep, char *fname)
{
    FILE *f;
    int err;
    if(!gep->chp) return -1;    
    if(!(f = fopen(fname , "r-"))) return -1;

    err = file_load_bin(f, gep->chp->dev_size, gep->chp->buffer);

    fclose(f);    

    return file_err_msg(err);
}

