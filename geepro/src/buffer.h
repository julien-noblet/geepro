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


#ifndef __buffer_h__
#define __buffer_h__

#include "checksum.h"
#include "storings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_buffer_list_ s_buffer_list;

struct s_buffer_list_
{
    char *name;		// buffer name
    char *data;		// buffer data
    unsigned int  size; // size of buffer in bytes
    unsigned int  offset; // address offset of buffer
    char modified;	// set if buffer was modified
    s_buffer_list	*next;
};

typedef struct 
{
    s_buffer_list *selected;	// selected buffer from list
    s_buffer_list *list;	// buffers list
} s_buffer;

typedef void (*f_buffer)(s_buffer *, s_buffer_list *, void *);

#define BUFFER_CB(x)	((f_buffer)x)

//---
/*
    Create and initialize buffer structure.
    Input:
	buffer - pointer to pointer buffer structure. Have to be NULL.
    Return
	0 - success
	less than zero - error
*/
char buffer_init( s_buffer **buffer);    

/*
    Frees all allocated resorces by buffer.
    Input:
	buffer - pointer buffer structure.
    Return:
	None
*/
void buffer_exit(s_buffer *);

/*
    Create new named buffer.
    Input:
	buffer - pointer buffer structure.
	name   - buffer name, have to be unique
	size   - buffer capacity
	offset - address offset
    Return:
	0 - success	
	less than zero - error
	1 - buffer with given name already exist. 
*/
char buffer_new( s_buffer *buffer, const char *name, int size, int offset);

/*
    Remove all buffers on list. Free all allocated memory.
    Input:
	buffer - pointer buffer structure.    
    Return:
	None
*/
void buffer_delete_all( s_buffer *buffer);

/*
    Select buffer from list.
    Input:
	buffer - pointer buffer structure.
	name   - buffer name, have to be unique
    Return:
	0 - success	
	1 - not found
*/
char buffer_select( s_buffer *buffer, const char *name );

/*
    Clears modified flag
    modified flag is on writing operations to buffer
*/
char buffer_reset_modified(s_buffer *buffer);

/*
    get modified flag
*/
char buffer_get_modified(s_buffer *buffer);

/* 
   ++++++++++++
    Operations
   ++++++++++++
*/

/*
    Returns pointer to internal data of selected buffer
*/
char *buffer_get_pointer(const s_buffer *buffer );

/*
    Write byte to selected buffer at specified address - offset.
    Input:
	buffer  - pointer buffer structure.
	address - virtual address
	data    - data to write
    Return:
	0 - success	
	less than zero - error
	1 - address over/under range    
*/
char buffer_poke( s_buffer *buffer, unsigned int address, char data);

/*
    Write block of byte to selected buffer at specified address - offset.
    Input:
	buffer  - pointer buffer structure.
	address - virtual address
	data    - data to write
	count   - count of writen data
    Return:
	0 - success	
	less than zero - error
	1 - address over/under range    
*/
char buffer_poke_block( s_buffer *buffer, unsigned int address, char *data, unsigned int count);

/*
    Read byte from specified address in selected buffer.
    Input:
	buffer  - pointer buffer structure.
	address - virtual address
    Return:
	data or error
*/
int buffer_peek( s_buffer *buffer, int address);

/*
    Read byte from specified address in selected buffer.
    Input:
	buffer  - pointer buffer structure.
	address - virtual address
	count   - count of rode data
    Return:
	data
*/
char buffer_peek_block( s_buffer *buffer, unsigned int address, char *data, unsigned int count);

/*
    Clears selected buffer - filling by data
    Input:
	buffer  - pointer buffer structure.
	data    - filling byte
    Return:
	None
*/
void buffer_fill(const s_buffer *buffer, char data);

/*
    Calculate checksum on selected buffer
    Input:
	buffer  - pointer buffer structure.
	algo    - name of checksum algorithm
    Return:
	None
*/
long buffer_checksum( s_buffer *buffer, e_checksum_algo algo);

/*
    Calculate checksum on specified buffer
    Input:
	buffer  - pointer buffer structure.
	algo    - name of checksum algorithm
    Return:
	None
*/
long buffer_checksum_count( s_buffer_list *buffer, e_checksum_algo algo);

/*
    Load file to buffer. Type of file is automaticaly determined.
    Loaded file is cut to buffer size.
    Input:
	buffer - buffer structure
	str    - storing values structure
	name   - file name or NULL if reload already loaded file
    Return:
	0 - success
	value less then zero - error
*/
char buffer_load(s_buffer_list *buffer, store_str *str, const char *name);

/*
    Load binary file to buffer at specified location.
    Loaded file is cut to buffer maximal address.
    Input:
	buffer - buffer structure
	str    - storing values structure
	name   - file name	
	buff_pos - buffer osition to insert file content
	filepos  - offset in file
	count    - bytes count to load
    Return:
	0 - success
	value less then zero - error    
*/
char buffer_load_at(s_buffer_list *buffer, store_str *str, const char *name, unsigned int buff_pos, unsigned int filepos, unsigned int count);

/*
    Save buffer to file. Type of file is automaitacaly determined by extension.
    Input:
	buffer - buffer structure
	str    - storing values structure
	name   - file name
	flags  - 1 - allow overrwrite
    Return:
	0 - success
	value less then zero - error                
	1 - file already exist
*/
char buffer_save(s_buffer_list *buffer, store_str *str, const char *name, char flags);

/*
    Save buffer to binary file from position buff_pos. 
    Input:
	buffer - buffer structure
	str    - storing values structure
	name   - file name
	buff_pos - start position in buffer
	count    - count bytes;
	flags  - 1 - allow overrwrite
    Return:
	0 - success
	value less then zero - error                
*/
char buffer_save_at(s_buffer_list *buffer, store_str *str, const char *name, char flags, unsigned int buff_pos, unsigned int count);

/*
    Check if the last loaded file for specified buffer is valid. 
    Input:
	buffer - buffer structure
    Return:
	0 - file is valid
	1 - file is out of date
	-2 - error on stat on file
*/
char buffer_file_check_valid(s_buffer_list *buffer);

const char *buffer_get_last_loaded_fname(s_buffer_list *buffer);
const char *buffer_get_last_loaded_at_fname(s_buffer_list *buffer);
const char *buffer_get_last_saved_fname(s_buffer_list *buffer);
const char *buffer_get_last_saved_at_fname(s_buffer_list *buffer);

/*
    return selected buffer size or 0
*/
int buffer_get_size( const s_buffer *buffer );


/*
    Lists all buffers. For each buffer call cb.

*/
void buffer_get_list( s_buffer *buffer, f_buffer cb, void *ptr);

//---

//void buffer_push(s_buffer *buffer);
//void buffer_pop(s_buffer *buffer);
//void buffer_get_name(s_buffer *buffer);

#ifdef __cplusplus
};
#endif
#endif

