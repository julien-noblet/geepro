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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "files.h"

extern "C" {
#include "buffer.h"
}

char buffer_init( s_buffer **bf)
{ 
    if(*bf != NULL ) return -2;
    MALLOC(*bf, s_buffer, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset(*bf, 0, sizeof( s_buffer ));
    
    
    return 0; 
}

void buffer_exit(s_buffer *bf)
{
    if( !bf ) return;
    buffer_delete_all( bf );
    free( bf );    
}

char buffer_new( s_buffer *bf, const char *name, int size, int offset)
{ 
    s_buffer_list *it, *tmp;

    if( !bf || !name ) return -2;
    if( size == 0 ) return 0;
    MALLOC(tmp, s_buffer_list, 1){
	ERR_MALLOC_MSG;
	return ERR_MALLOC_CODE;
    }
    memset(tmp, 0, sizeof( s_buffer_list ));
    MALLOC(tmp->name, char, strlen( name ) + 1){
	ERR_MALLOC_MSG;
	free( tmp );
	return ERR_MALLOC_CODE;
    }
    strcpy(tmp->name, name );
    tmp->size = size;
    tmp->offset = offset;
    MALLOC(tmp->data, char, size){
	ERR_MALLOC_MSG;
	free( tmp->name );
	free( tmp );
	return ERR_MALLOC_CODE;
    }
    memset(tmp->data, 0, size);
    if(!bf->list){
	bf->list = tmp;
	bf->selected = tmp;	// default
	return 0;
    }
    for( it = bf->list; it->next; it = it->next);
    it->next = tmp;
    return 0;
}

void buffer_delete_all( s_buffer *bf)
{
    s_buffer_list *it, *tmp;
    if( !bf ) return;
    for(it = bf->list; it;){
	tmp = it->next;
	if( it->name ) free( it->name );
	if( it->data ) free( it->data );
	if(it->last_loaded) free(it->last_loaded);
	if(it->last_loaded_at) free(it->last_loaded_at);
	if(it->last_saved) free(it->last_saved);
	if(it->last_saved_at) free(it->last_saved_at);
	it = tmp;
    }
    bf->list = NULL;
}

char buffer_select( s_buffer *bf, const char *name )
{ 
    s_buffer_list *it;
    if( !bf || !name ) return -2;
    for(it = bf->list; it; it=it->next){
	if(!strcmp( it->name, name)){
	    bf->selected = it;
	    return 0;
	}
    }
    return 1;
}

char buffer_reset_modified(s_buffer *bf)
{
    if( !bf ) return -2;
    if( !bf->selected ) return 1;   
    bf->selected->modified = 0;
    return 0;
}

char buffer_get_modified(s_buffer *bf)
{
    if( !bf ) return -2;
    if( !bf->selected ) return 0;               
    return bf->selected->modified;
}

char *buffer_get_pointer(const s_buffer *bf )
{
    if( !bf ) return NULL;
    if( !bf->selected ) return NULL;
    return bf->selected->data;
}

int buffer_get_size(const s_buffer *bf )
{
    if( !bf ) return 0;
    if( !bf->selected ) return 0;
    return bf->selected->size;
}

char buffer_poke( s_buffer *bf, unsigned int address, char data)
{
    if( !bf ) return -2;
    if( !bf->selected ) return 1;               
    address -= bf->selected->offset;
    if( address >= bf->selected->size) return 2; // address over or underrange
    bf->selected->modified = 1;
    bf->selected->data[address] = data;
    return 0;
}

char buffer_poke_block( s_buffer *bf, unsigned int address, char *data, unsigned int count)
{
    if( !bf ) return -2;
    if( !bf->selected ) return 1;               
    address -= bf->selected->offset;
    if( (address < 0) || (address >= bf->selected->size)) return 2; // address over or underrange
    if( (address + count) >= bf->selected->size ) return 3; // data exceed buffer
    
    bf->selected->modified = 1;
    memcpy(bf->selected->data, data, count);

    return 0;
}

int buffer_peek( s_buffer *bf, unsigned int address)
{
    if( !bf ) return -2;
    if( !bf->selected ) return -3;
    address -= bf->selected->offset;
    if( address >= bf->selected->size ) return -4; // address over or underrange
    return bf->selected->data[address];
}

char buffer_peek_block( s_buffer *bf, unsigned int address, char *data, unsigned int count)
{
    if( !bf ) return -2;
    if( !bf->selected ) return -3;
    address -= bf->selected->offset;
    if( address >= bf->selected->size) return -4; // address over or underrange
    if( (address + count) >= bf->selected->size ) return -5; // data exceed buffer
    memcpy(data, bf->selected->data, count);
    return 0;
}

void buffer_fill(const s_buffer *bf, char data)
{
    if( !bf ) return;
    if( !bf->selected ) return;               
    bf->selected->modified = 1;
    if(bf->selected->data) memset(bf->selected->data, 0, bf->selected->size);
}

long buffer_checksum( s_buffer *bf, e_checksum_algo algo)
{
    if( !bf ) return -2;
    if( !bf->selected ) return -3;    
    return checksum_calculate( algo, bf->selected->size, (unsigned char *)bf->selected->data, 0, bf->selected->size - 1, 0, 0, 0, 0);
}

long buffer_checksum_count( s_buffer_list *bf, e_checksum_algo algo)
{
    if( !bf ) return -2;
    return checksum_calculate( algo, bf->size, (unsigned char *)bf->data, 0, bf->size - 1, 0, 0, 0, 0);
}

void buffer_get_list( s_buffer *bf, f_buffer cb, void *ptr)
{
    s_buffer_list *it;
    if( !bf || !cb ) return;
    if( !bf->list ) return;    
    
    for(it = bf->list; it; it = it->next) 
	cb(bf, it, ptr);
}

/***********************************************************************************************************************
    LOAD/SAVE to file
*/

static const char buffer_empty_string[] = "";

const char *buffer_get_last_loaded_fname( s_buffer_list *bf, store_str *st )
{
    char *tmp;
    char fmt[256];
    if(!bf->last_loaded){
	sprintf(fmt, BUFFER_KEY_LOADED"[%s]", bf->name);
	tmp = NULL;
	if(!store_get(st, (const char *)fmt, &tmp)){
	    bf->last_loaded = tmp;
	}
    }
    return bf->last_loaded ? bf->last_loaded : buffer_empty_string;
}

const char *buffer_get_last_loaded_at_fname( s_buffer_list *bf, store_str *st )
{
    char *tmp;
    char fmt[256];
    if(!bf->last_loaded_at){
	sprintf(fmt, BUFFER_KEY_LOADED_AT"[%s]", bf->name);
	tmp = NULL;
	if(!store_get(st, (const char *)fmt, &tmp)){
	    bf->last_loaded_at = tmp;
	}
    }
    return bf->last_loaded_at ? bf->last_loaded_at : buffer_empty_string;
}

const char *buffer_get_last_saved_fname( s_buffer_list *bf, store_str *st )
{
    char *tmp;
    char fmt[256];
    if(!bf->last_saved){
	sprintf(fmt, BUFFER_KEY_SAVED"[%s]", bf->name);
	tmp = NULL;
	if(!store_get(st, (const char *)fmt, &tmp)){
	    bf->last_saved = tmp;
	}
    }
    return bf->last_saved ? bf->last_saved : buffer_empty_string;
}

const char *buffer_get_last_saved_at_fname( s_buffer_list *bf, store_str *st )
{
    char *tmp;
    char fmt[256];
    if(!bf->last_saved_at){
	sprintf(fmt, BUFFER_KEY_SAVED_AT"[%s]", bf->name);
	tmp = NULL;
	if(!store_get(st, fmt, &tmp)){
	    bf->last_saved_at = tmp;
	}
    }
    return bf->last_saved_at ? bf->last_saved_at : buffer_empty_string;
}

char buffer_file_check_valid(s_buffer_list *bf)
{ 
    long long time = 0;

    if(!bf) return 0;
    if(!bf->last_loaded) return 0;
    if( file_get_time(&time, bf->last_loaded ) ) return -2;
    if( time != bf->time_loaded) return 1;
    return 0;
}

char buffer_load(s_buffer_list *bf, store_str *st, const char *name)
{ 
    char fmt[256];
    char *err, *tmp;
    
    tmp = bf->last_loaded;
    if( !name ){ // reload
	name = buffer_get_last_loaded_fname( bf, st);
	if( !name ) return 0; // nothing to reload
    }

    if( (err = (char *)file_load(bf, name, -1, -1, -1) )){
	MSG("Loading file '%s' error.", name);	
	return -2;
    }
    
    if( name != tmp ){
	sprintf(fmt, BUFFER_KEY_LOADED"[%s]", bf->name);
	if(store_set(st, fmt, name)){
	    WRN("Cannot set variable for last loaded file.");
	}	    
    }
    return 0;
}

char buffer_load_at(s_buffer_list *bf, store_str *st, const char *name, unsigned int buff_pos, unsigned int filpos, unsigned int count)
{ 
    char *err, *tmp;
    char fmt[256];

    tmp = bf->last_loaded_at;
    if( !name ){ // reload
	name = buffer_get_last_loaded_at_fname( bf, st);
	if( !name ) return 0; // nothing to reload
    }

    if( (err = (char *)file_load(bf, name, filpos, buff_pos, count) )){
	MSG("Loading file '%s' error.", name);	
	return -2;
    }

    if( name != tmp ){
	sprintf(fmt, BUFFER_KEY_LOADED_AT"[%s]", bf->name);
	if(store_set(st, fmt, name)){
	    WRN("Cannot set variable for last loaded file.");
	}	    
    }
    return 0;
}

char buffer_save(s_buffer_list *bf, store_str *st, const char *name, char flags)
{ 
    char *err, *tmp;
    char fmt[256];

    tmp = bf->last_saved;
    if( !name ){ // reload
	name = buffer_get_last_saved_fname( bf, st);
	if( !name ) return 0; // nothing to reload
    }

    if( (err = (char *)file_save(bf, name) )){
	MSG("Saving file '%s' error.", name);	
	return -2;
    }

    if( name != tmp ){
	sprintf(fmt, BUFFER_KEY_SAVED"[%s]", bf->name);
	if(store_set(st, fmt, name)){
	    WRN("Cannot set variable for last saved file.");
	}	    
    }

    return 0;
}

/*
char buffer_save_at(s_buffer_list *bf, store_str *st, const char *name, char flags, unsigned int buff_pos, unsigned int count)
{ 
    printf("---- Save AT ----\n");
    return 0;
}
*/


