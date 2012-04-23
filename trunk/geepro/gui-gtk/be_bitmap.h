#ifndef __BE_BITMAP_H__
#define __BE_BITMAP_H__

#include "bineditor.h"

extern void gui_bineditor_bitmap(GuiBineditor *be, unsigned int width, unsigned int height, unsigned char mask, unsigned char bit_rev);
extern void gui_bineditor_bitmap_set_address(GuiBineditor *be, unsigned int addr);
extern char gui_bineditor_bitmap_get_mode(GuiBineditor *be);
#endif // __BE_BITMAP_H__