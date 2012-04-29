#ifndef __BE_BITMAP_H__
#define __BE_BITMAP_H__

#include "bineditor.h"

#define GUI_BINEDITOR_INVERSE_ICON	"geepro-bineditor-inverse"
#define GUI_BINEDITOR_MIRROR_ICON	"geepro-bineditor-mirror"
#define GUI_BINEDITOR_GRID_ICON		"geepro-bineditor-grid"
#define GUI_BINEDITOR_LIVE_ICON		"geepro-bineditor-live"
#define GUI_BINEDITOR_EDIT_ICON		"geepro-bineditor-edit"

extern void gui_bineditor_bitmap(GuiBineditor *be, unsigned int width, unsigned int height, unsigned char mask, unsigned char bit_rev);
extern void gui_bineditor_bitmap_set_address(GuiBineditor *be, unsigned int addr);
extern char gui_bineditor_bitmap_get_mode(GuiBineditor *be);
extern void gui_bineditor_bitmap_redraw(GuiBineditor *be);
#endif // __BE_BITMAP_H__