#ifndef __BE_BUFFER_H__
#define __BE_BUFFER_H__
#include <stdio.h>

enum
{
    GUI_BE_UNDO,
    GUI_BE_REDO
};

enum
{
    GUI_BE_BUFF_PATT_AUTO,
    GUI_BE_BUFF_PATT_STRING,
    GUI_BE_BUFF_PATT_HEX
};

typedef struct
{
    FILE *ftmp;		// handle to history file
    unsigned char *data; // buffer data
    unsigned int  size;  // buffer size
} gui_be_buffer_str;

#define gui_bineditor_buff_edit_byte(be, from, var) gui_bineditor_buff_edit(be, from, 1, &(var))
extern unsigned char *gui_bineditor_buff_pattern2data(const char *data, unsigned int *data_size, int *error);
extern void gui_bineditor_buff_history_add(gui_be_buffer_str *bf, unsigned int from, unsigned int count, unsigned char *data);
extern char gui_bineditor_buff_edit(gui_be_buffer_str *bf, unsigned int from, unsigned int count, unsigned char *str);
extern void gui_bineditor_buff_constr(gui_be_buffer_str **bf); // create tmp history file
extern void gui_bineditor_buff_destroy(gui_be_buffer_str **bf);  // delete history
extern char gui_bineditor_buff_clr(gui_be_buffer_str *bf, unsigned int from, unsigned int to, const char *pattern);
extern char gui_bineditor_buff_find(gui_be_buffer_str *bf, const char *find, unsigned int size, unsigned int *from, unsigned int to, char ci);
extern void gui_bineditor_buff_bman(gui_be_buffer_str *bf, unsigned int start, unsigned int count, int arg, char func, char *rel);
extern void gui_bineditor_buff_reorg(gui_be_buffer_str *bf, unsigned int start, unsigned int count, char arg, char *rel);
extern void gui_bineditor_buff_cut(gui_be_buffer_str *bf, unsigned int start, unsigned int count, unsigned int stop);
extern void gui_bineditor_buff_copy(gui_be_buffer_str *bf, unsigned int start);
extern void gui_bineditor_buff_asm(gui_be_buffer_str *bf, unsigned int start, unsigned int count);
extern void gui_bineditor_buff_history(gui_be_buffer_str *bf, int operation);
extern void gui_bineditor_buff_file_insert(gui_be_buffer_str *bf, FILE *fh, long offset, int start, int count );
extern void gui_bineditor_buff_file_save(gui_be_buffer_str *bf, int start, int count );
#endif
