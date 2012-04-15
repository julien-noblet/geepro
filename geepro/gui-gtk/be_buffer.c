#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "be_buffer.h"

unsigned char *gui_bineditor_buff_pattern2data(const char *data, unsigned int *data_size, int *error)
{
    unsigned char *tmp;
    int len;
    
    len = strlen(data);
    *data_size = len;
    *error = 0;
    tmp = (unsigned char *)malloc( len );
// temporary
    memcpy(tmp, data, len);

    return tmp;
}

void gui_bineditor_buff_history_add(gui_be_buffer_str *bf, unsigned int from, unsigned int count, unsigned char *data)
{
    printf("history not implemented yet\n");
}

void gui_bineditor_buff_history(gui_be_buffer_str *be, int op)
{
    printf("history: %i\n", op);
}

/*******************************************************************************************************************************************/

void gui_bineditor_buff_constr(gui_be_buffer_str **bf)
{
    *bf = (gui_be_buffer_str *)malloc( sizeof( gui_be_buffer_str ) );
    if(*bf == NULL){
	printf("Critical error: memory allocation in gui_bineditor_buff_constr()\n");
	exit(-1);
    }
    (*bf)->ftmp = tmpfile();
    if((*bf)->ftmp == NULL ){
	printf("Error: temporary file creation for history in gui_bineditor_buff_constr() can not be created!\n");
    }
    (*bf)->size = 0;
    (*bf)->data = NULL;
    printf("Temporary file for history created.\n");
}

void gui_bineditor_buff_destroy(gui_be_buffer_str **bf)
{
    if((*bf)->ftmp){
	 fclose((*bf)->ftmp);
	 printf("Temporary file cleanup.\n");
    }
    if(*bf) free(*bf);
    *bf = NULL;
}

char gui_bineditor_buff_edit(gui_be_buffer_str *bf, unsigned int from, unsigned int count, unsigned char *data)
{
    char error;
    
    error = 0;
    
    if(bf == NULL){
	printf("Error: gui_bineditor_buff_edit() buffer == NULL -> ignoring function.\n");
	return 1;
    }        
    if(from >= bf->size){
	printf("Warn: gui_bineditor_buff_edit() insert address is greater than buffor -> ignoring function.\n");    
	return 2;
    }    
    if(from + count >= bf->size){
	count = bf->size - count - 1;
	error = 128;
    }    

    gui_bineditor_buff_history_add(bf, from, count, data);
    memcpy(bf->data + from, data, count);

    return error;
}

char gui_bineditor_buff_clr(gui_be_buffer_str *bf, unsigned int from, unsigned int to, const char *pattern)
{
    unsigned char *data;    
    unsigned int  data_size = 0, i;
    int error = 0;
    
    if( from >= bf->size) error = 1;
    if( to >= bf->size) error = 2;

    if( error ){
	printf("Warning: gui_bineditor_buff_clr() -> Address exceed buffer. Ignoring.\n");
	return error;
    }
    
    data = gui_bineditor_buff_pattern2data( pattern, &data_size, &error);
    
    if( error ){
	printf("Warning: gui_bineditor_buff_clr() -> Pattern error. Ignoring.\n");
	return error;
    }

    // fill buffer
//    for( i = from; (i <= to) && !error; i += data_size)
//			    error = gui_bineditor_buff_edit(bf, from + i, data_size, data);
    
    if( data ) free( data );
    return 0;
}

char gui_bineditor_buff_find(gui_be_buffer_str *bf, const char *find, unsigned int size, unsigned int *from, unsigned int to, char ci)
{
    unsigned int i;

    if( bf->data == NULL ) return 0;
    if( find == NULL ) return 0;

    for(; (*from < bf->size) && (*from <= to); (*from)++){    
	if( *from + size >= bf->size) return 0;
	if(!ci){
	    if(memcmp( bf->data + *from, find, size ) == 0) return 1;
	} else {
	    for(i = 0; i < size; i++){
		if( tolower( bf->data[*from + i] ) == tolower( find[ i ] )) return 1;
	    }
	}
    }
    return 0;
}

void gui_bineditor_buff_bman(gui_be_buffer_str *be, unsigned int start, unsigned int count, int arg, char func, char *rel)
{
    printf("bit manager: %i %i %i %i\n", start, count, arg, func);
}

void gui_bineditor_buff_reorg(gui_be_buffer_str *be, unsigned int start, unsigned int count, char op, char *rel)
{
    printf("byte reorg: %i %i %i\n", start, count, op);
}

void gui_bineditor_buff_cut(gui_be_buffer_str *be, unsigned int start, unsigned int count, unsigned int stop)
{
    printf("byte cut\n");
}

void gui_bineditor_buff_copy(gui_be_buffer_str *be, unsigned int start)
{
    printf("byte copy\n");
}

void gui_bineditor_buff_asm(gui_be_buffer_str *be, unsigned int start, unsigned int count)
{
    printf("byte asm\n");
}


void gui_bineditor_buff_file_insert(gui_be_buffer_str *be, FILE *fh, long offs, int start, int count)
{
    printf("open \n");
}

void gui_bineditor_buff_file_save(gui_be_buffer_str *be, int start, int count)
{
    printf("save \n");
}


