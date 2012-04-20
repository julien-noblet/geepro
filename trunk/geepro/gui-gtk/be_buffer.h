#ifndef __BE_BUFFER_H__
#define __BE_BUFFER_H__
#include <stdio.h>

enum
{
    GUI_BINEDITOR_BM_FUNC_SUB,
    GUI_BINEDITOR_BM_FUNC_ADD,
    GUI_BINEDITOR_BM_FUNC_MUL,
    GUI_BINEDITOR_BM_FUNC_DIV,
    GUI_BINEDITOR_BM_FUNC_OR,
    GUI_BINEDITOR_BM_FUNC_AND,
    GUI_BINEDITOR_BM_FUNC_XOR,
    GUI_BINEDITOR_BM_FUNC_SHL,
    GUI_BINEDITOR_BM_FUNC_SAL,
    GUI_BINEDITOR_BM_FUNC_SHR,
    GUI_BINEDITOR_BM_FUNC_SAR,
    GUI_BINEDITOR_BM_FUNC_ROL,
    GUI_BINEDITOR_BM_FUNC_ROR,
    GUI_BINEDITOR_BM_FUNC_BIT
};

enum
{
    GUI_BINEDITOR_ORG_SPLIT,
    GUI_BINEDITOR_ORG_MERGE,
    GUI_BINEDITOR_ORG_XCHG,
    GUI_BINEDITOR_ORG_REORG
};

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
extern unsigned char *gui_bineditor_buff_pattern2data(const char *data, unsigned int *data_size, int *error); // allocate memory, that must be freed
extern void gui_bineditor_buff_history_add(gui_be_buffer_str *bf, unsigned int from, unsigned int to);
extern char gui_bineditor_buff_edit(gui_be_buffer_str *bf, unsigned int from, unsigned int count, unsigned char *str);
extern void gui_bineditor_buff_constr(gui_be_buffer_str **bf); // create tmp history file
extern void gui_bineditor_buff_destroy(gui_be_buffer_str **bf);  // delete history
extern char gui_bineditor_buff_clr(gui_be_buffer_str *bf, unsigned int from, unsigned int to, const char *pattern);
extern char gui_bineditor_buff_find(gui_be_buffer_str *bf, const char *find, unsigned int size, unsigned int *from, unsigned int to, char ci);
extern void gui_bineditor_buff_bman(gui_be_buffer_str *bf, unsigned int start, unsigned int count, int arg, char func, char *rel);
extern void gui_bineditor_buff_reorg(gui_be_buffer_str *bf, unsigned int start, unsigned int count, char arg, char *rel, char bits);
extern void gui_bineditor_buff_asm(gui_be_buffer_str *bf, unsigned int start, unsigned int count);
extern void gui_bineditor_buff_history(gui_be_buffer_str *bf, int operation);
extern void gui_bineditor_buff_file_insert(gui_be_buffer_str *bf, FILE *fh, long offset, int start, int count );
extern void gui_bineditor_buff_file_save(gui_be_buffer_str *bf, int start, int count );
#endif
