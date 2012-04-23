#include <stdio.h>
#include <stdlib.h>
#include "../intl/lang.h"
#include "be_bitmap.h"

typedef struct
{
    GtkToolItem *tb_shrink;
    GtkToolItem *tb_expand;
    GtkToolItem *tb_auto;
    GuiBineditor *be;
    GtkWidget *wmain;
    GtkWidget *ctx, *tb;
    GtkWidget *draw_area;
    GtkWidget *vslid;
    GtkWidget *hslid;
    GtkAdjustment *vadj;
    GtkAdjustment *hadj;
    cairo_t *cr;
    unsigned int g_width, g_high;
    unsigned int xx, yy; // width, height of drawing area
    unsigned int x0;
    unsigned int y0;
    unsigned int pixel_size;
    unsigned int ww,hh;  // draw region width and height
    unsigned int rows, cols;
    char first_run;
    unsigned int x1,y1; // window offset

    unsigned int  addr;  // start address
    unsigned char mask; // bit mask
    unsigned char bpb;  // taken bits per byte
    char	  rev;  // reverse flag
    char 	  mask_tb[8]; // masks for bits

    unsigned int mx,my; // ? mouse x, y grid position ?
    gdouble  xp, yp;  // ?
    char mode; // actualize on the fly
    char inverse; // picture negative
    char edit;	// permision for editing
    
} gui_bitmap_bmp_str;

static void gui_bineditor_bmp_set_grid_bit(gui_bitmap_bmp_str *s, int x, int y, char op );
static inline void gui_bineditor_bmp_toolbar_setup( gui_bitmap_bmp_str *s );
static inline void gui_bineditor_bmp_drawing_setup( gui_bitmap_bmp_str *s );
static void gui_bineditor_bmp_draw(gui_bitmap_bmp_str *s);

/********************************************************************************************************/
static void gui_bineditor_bmp_grid_coordinates(gui_bitmap_bmp_str *s, unsigned int x, unsigned int y, char *within)
{
    *within = 1;
    s->mx = s->x1 + ((x - s->x0) / s->pixel_size);
    s->my = s->y1 + ((y - s->y0) / s->pixel_size);
    if((s->mx >= s->g_width) || (s->mx < 0) || (s->my >= s->g_high) || (s->my < 0)) *within = 0;
}

static inline void gui_bineditor_bmp_redraw( gui_bitmap_bmp_str *s )
{
    gtk_widget_queue_draw( s->draw_area );
}

static void gui_bineditor_bmp_destroy(GtkWidget *wg, GuiBineditor *be)
{
    gtk_widget_set_sensitive( be->priv->bined, TRUE );
    free(be->priv->bmp);
    be->priv->bmp = NULL;
}

static void gui_bineditor_bmp_vch(GtkAdjustment *adj, gui_bitmap_bmp_str *s)
{
    gdouble value = gtk_adjustment_get_value( adj );
    s->y1 = value * s->g_high;
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_hch(GtkAdjustment *adj, gui_bitmap_bmp_str *s)
{
    gdouble value = gtk_adjustment_get_value( adj );
    s->x1 = value * s->g_high;
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_draw_compute(gui_bitmap_bmp_str *s)
{
    s->first_run = 0;
    s->cols = s->g_width;
    s->rows = s->g_high;

    if( s->pixel_size != 0){
	// cut off to drawing area
	if( s->cols * s->pixel_size >= s->xx ) s->cols = (s->xx / s->pixel_size) + 1;
	if( s->rows * s->pixel_size >= s->yy ) s->rows = (s->yy / s->pixel_size) + 1;
    } else
	s->pixel_size = 1;

    // compute width and height of grid
    s->ww = s->cols * s->pixel_size;
    s->hh = s->rows * s->pixel_size;
    // center
    s->x0 = ( s->ww < s->xx) ? (s->xx - s->ww) / 2 : 0;
    s->y0 = ( s->hh < s->yy) ? (s->yy - s->hh) / 2 : 0;

}

static gboolean gui_bineditor_bmp_draw_(GtkWidget *wg, cairo_t *cr, gui_bitmap_bmp_str *s)
{
    s->cr = cr;
    s->xx = gtk_widget_get_allocated_width( wg );
    s->yy = gtk_widget_get_allocated_height( wg );
    gui_bineditor_bmp_draw( s );
    return FALSE;
}


static void gui_bineditor_bmp_scrollers_actualization( gui_bitmap_bmp_str *s, int x, int y )
{
    gdouble vv, vs, hv, hs;

    vv = 1.0;
    hv = 1.0;
    gui_bineditor_bmp_draw_compute( s );

    // center at pointing region
    if(( x >= 0)&&(y >= 0)){
// to do
    }   
     
    if((s->pixel_size == 0) || (s->g_high == 0)) 
	vs = 0;    
    else
	vs = (gdouble)s->yy / ( s->pixel_size * s->g_high);

    if((s->pixel_size == 0) || (s->g_width == 0)) 
	hs = 0;        
    else
	hs = (gdouble)s->xx / ( s->pixel_size * s->g_width);    

    gtk_adjustment_configure(s->vadj, vv, 0.0, 1.0, vs,	vs * 8, vs );	
    gtk_adjustment_configure(s->hadj, hv, 0.0, 1.0, hs,	hs * 8, hs );	
}

void gui_bineditor_bmp_zoom_auto( gui_bitmap_bmp_str *s )
{
    unsigned int xmax, ymax;
    
    if( ( s->g_width == 0) || (s->g_high == 0) ){
	s->pixel_size = 1;
	return;
    }
    
    xmax = s->xx / s->g_width;
    ymax = s->yy / s->g_high;

    s->pixel_size = xmax > ymax ? ymax : xmax;
    gui_bineditor_bmp_scrollers_actualization( s , -1, -1);
}

static void gui_bineditor_bmp_zoom_expand( gui_bitmap_bmp_str *s )
{
    s->pixel_size++;
    if( s->pixel_size >= s->xx) s->pixel_size = s->xx;
    if( s->pixel_size >= s->yy) s->pixel_size = s->yy;
    if( s->pixel_size >= s->xx) s->pixel_size = s->xx;	// and again if yy > xx
    gui_bineditor_bmp_scrollers_actualization( s , -1, -1);
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_zoom_shrink( gui_bitmap_bmp_str *s )
{
    s->pixel_size--;
    if( s->pixel_size < 1) s->pixel_size = 1;
    gui_bineditor_bmp_scrollers_actualization( s , -1, -1);
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_scroll(GtkWidget *wg, GdkEventScroll *ev, gui_bitmap_bmp_str *s)
{
    char within = 0;
    gui_bineditor_bmp_grid_coordinates(s, ev->x, ev->y, &within);
    // relative position in grid
    s->xp = (((ev->x - s->x0) / s->pixel_size)) / s->cols;
    s->yp = (((ev->y - s->y0) / s->pixel_size)) / s->rows;

    switch( ev->direction ){
	case GDK_SCROLL_UP: gui_bineditor_bmp_zoom_expand(s); break;
	case GDK_SCROLL_DOWN: gui_bineditor_bmp_zoom_shrink(s); break;
	default:;
    }
    gui_bineditor_bmp_scrollers_actualization( s , ev->x, ev->y);
}

static void gui_bineditor_bmp_zoom_in(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_expand( s );
}

static void gui_bineditor_bmp_zoom_out(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_shrink( s );
}

static void gui_bineditor_bmp_zoom_fit(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_auto( s );
    gui_bineditor_bmp_redraw( s );
}

void gui_bineditor_bitmap(GuiBineditor *be, unsigned int width, unsigned int height, unsigned char mask, unsigned char br)
{
    gui_bitmap_bmp_str *s;
    int i;
    GtkWidget *fr, *wg;

    s = (gui_bitmap_bmp_str *)malloc(sizeof(gui_bitmap_bmp_str));
    if( !s ){
	printf("ERR: gui_bineditor_bitmap() -> memory allocation problem.\n");
	return;    
    }

    s->addr = 0; // to change !!!
    s->inverse = 0; // 0 or 0xff
    s->mask = mask;
    s->mode = 0;
    s->rev  = br;    
    s->bpb  = 0;
    // count '1' in mask
    for(i = 1; i & 0xff; i <<= 1) 
	if( s->mask & i ){
	   s->mask_tb[s->bpb] = i;
	   s->bpb++;
	}

    be->priv->bmp = (void*)s;
    s->edit = 0;
    s->g_width = width;
    s->g_high = height;
    s->be = be;
    s->x1 = 0;
    s->y1 = 0;
    gtk_widget_set_sensitive( be->priv->bined, FALSE );
    s->wmain = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    g_signal_connect(G_OBJECT(s->wmain), "destroy", G_CALLBACK(gui_bineditor_bmp_destroy), be);
    if( be->priv->icon )
	gtk_window_set_icon( GTK_WINDOW( s->wmain ), gdk_pixbuf_new_from_xpm_data( be->priv->icon ));
    gtk_window_set_keep_above(GTK_WINDOW(s->wmain), TRUE);
    gtk_widget_set_size_request( s->wmain, 320, 320); //
    s->ctx = gtk_vbox_new(FALSE, 0);        
    gtk_container_add(GTK_CONTAINER(s->wmain), s->ctx);
    s->tb = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(s->ctx), s->tb, FALSE, FALSE, 0 );
    fr = gtk_frame_new( NULL );    
    gtk_container_add(GTK_CONTAINER(s->ctx), fr);    
    wg = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(fr), wg);    
    fr = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(wg), fr);        
    s->draw_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(fr), s->draw_area);
    s->vadj = gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    s->vslid = gtk_scrollbar_new( GTK_ORIENTATION_VERTICAL, s->vadj );
    gtk_box_pack_end(GTK_BOX(fr), s->vslid, FALSE, FALSE, 0);
    s->hadj = gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    s->hslid = gtk_scrollbar_new( GTK_ORIENTATION_HORIZONTAL, s->hadj );
    gtk_box_pack_end(GTK_BOX(wg), s->hslid, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(s->vadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_vch), s);
    g_signal_connect(G_OBJECT(s->hadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_hch), s);

    gui_bineditor_bmp_toolbar_setup( s );
    gui_bineditor_bmp_drawing_setup( s );
    s->first_run = 1;
    gtk_widget_show_all( s->wmain );
}

static inline void gui_bineditor_bmp_toolbar_setup( gui_bitmap_bmp_str *s )
{
    /* shrink */
    s->tb_shrink = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_OUT );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->tb_shrink ), TXT_BE_BMP_TB_SHRINK);
    g_signal_connect(G_OBJECT(s->tb_shrink), "clicked", G_CALLBACK( gui_bineditor_bmp_zoom_out), s);
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->tb_shrink), -1);
    /* expand */
    s->tb_expand = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_IN );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->tb_expand ), TXT_BE_BMP_TB_EXPAND);
    g_signal_connect(G_OBJECT(s->tb_expand), "clicked", G_CALLBACK( gui_bineditor_bmp_zoom_in), s);
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->tb_expand), -1);
    /* auto */
    s->tb_auto = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_FIT );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->tb_auto ), TXT_BE_BMP_TB_AUTO);
    g_signal_connect(G_OBJECT(s->tb_auto), "clicked", G_CALLBACK( gui_bineditor_bmp_zoom_fit ), s);
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->tb_auto), -1);
}

static void gui_bineditor_bmp_mouse_button( gui_bitmap_bmp_str *s, int button, char state, unsigned int x, unsigned int y )
{
    char within = 0;

    if( !s->edit ) return;

    gui_bineditor_bmp_grid_coordinates(s, x, y, &within);
    if( !within ) return;
    switch( button ){
	case 1 : gui_bineditor_bmp_set_grid_bit(s, x, y, 0 ); break;
	case 3 : gui_bineditor_bmp_set_grid_bit(s, x, y, 1 ); break;
    }
    gui_bineditor_bmp_redraw( s );
    gui_bineditor_redraw( s->be );
}

static void gui_bineditor_bmp_button_press(GtkWidget *wg, GdkEventButton *ev, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_mouse_button( s, ev->button, 1, ev->x, ev->y);
}

static void gui_bineditor_bmp_button_release(GtkWidget *wg, GdkEventButton *ev, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_mouse_button( s, ev->button, 0, ev->x, ev->y);
}

//static void gui_bineditor_bmp_notify(GtkWidget *wg, GdkEventMotion *ev, gui_bitmap_bmp_str *s )
//{
//    unsigned int x = ev->x, y= ev->y;
//}

static inline void gui_bineditor_bmp_drawing_setup( gui_bitmap_bmp_str *s )
{
//    gtk_widget_set_events(s->draw_area, GDK_SCROLL_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK );
    gtk_widget_set_events(s->draw_area, GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK );
    g_signal_connect(G_OBJECT(s->draw_area), "draw", G_CALLBACK(gui_bineditor_bmp_draw_), s);
    g_signal_connect(G_OBJECT(s->draw_area), "scroll_event", G_CALLBACK(gui_bineditor_bmp_scroll), s);
    g_signal_connect(G_OBJECT(s->draw_area), "button_press_event", G_CALLBACK(gui_bineditor_bmp_button_press), s);
    g_signal_connect(G_OBJECT(s->draw_area), "button_release_event", G_CALLBACK(gui_bineditor_bmp_button_release), s);
//    g_signal_connect(G_OBJECT(s->draw_area), "motion_notify_event", G_CALLBACK(gui_bineditor_bmp_notify), s);
    gtk_widget_set_can_focus(s->draw_area, TRUE);
}

/***************************
** Drawing Grid
*/

#define GET_COLOR( idx ) s->be->priv->colors[(idx)*3 + 0],s->be->priv->colors[(idx)*3 + 1],s->be->priv->colors[(idx)*3 + 2]

static inline void gui_bineditor_bmp_draw_background( gui_bitmap_bmp_str *s )
{
    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_BG) ); // background color
    cairo_rectangle( s->cr, s->x0, s->y0, s->ww, s->hh );
    cairo_fill( s->cr );
}

static inline void gui_bineditor_bmp_draw_hline( cairo_t *cr, int x, int y, int len)
{
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x + len, y);
}

static inline void gui_bineditor_bmp_draw_vline( cairo_t *cr, int x, int y, int len)
{
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x, y + len);
}

static inline void gui_bineditor_bmp_draw_grid( gui_bitmap_bmp_str *s)
{
    int i, n;

    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_GRID) ); // background color
    for(i = 0, n = 0; i < s->cols; i++, n += s->pixel_size) 
					gui_bineditor_bmp_draw_vline( s->cr, s->x0 + n, s->y0, s->hh);
    
    for(i = 0, n = 0; i < s->rows; i++, n += s->pixel_size)
					gui_bineditor_bmp_draw_hline( s->cr, s->x0, s->y0 + n, s->ww);

    cairo_stroke( s->cr );        
}

static inline void gui_bineditor_bmp_draw_pixel_part(gui_bitmap_bmp_str *s, int x, int y)
{
    unsigned int x0, y0;

    if(( x >= s->cols) || (y >= s->rows)) return;
    if(( x < 0) || (y < 0)) return;

    x0 = s->x0 + 1 + x * s->pixel_size; 
    y0 = s->y0 + 1 + y * s->pixel_size; 

    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_PIXEL) ); // background color
    cairo_rectangle( s->cr, x0, y0, s->pixel_size - 2, s->pixel_size - 2);
    cairo_fill( s->cr );
}

static inline void gui_bineditor_bmp_draw_ambient( gui_bitmap_bmp_str *s)
{
    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_AMBIENT) ); // background color
    cairo_rectangle( s->cr, 0, 0, s->xx, s->yy );
    cairo_fill( s->cr );
}

static void gui_bineditor_bmp_draw_pixel(gui_bitmap_bmp_str *s, unsigned int x, unsigned int y)
{
    // cutting
//    if((( x > s->x1 + s->pixel_size) || (x < s->x1)) && 
//	(( y > s->y1 + s->pixel_size) || (y < s->y1)))  return;
    // draw pixel with displacement
//    gui_bineditor_bmp_draw_pixel_part( s, x - s->x1, y - s->y1);
    gui_bineditor_bmp_draw_pixel_part( s, x, y);
}

static unsigned int gui_bineditor_bmp_get_grid_addr(gui_bitmap_bmp_str *s, int x, int y, int *bit )
{
    unsigned int  offset = x + y * s->cols;
    
    *bit  = offset % s->bpb;
    return s->addr + ( offset / s->bpb);
}

static unsigned char gui_bineditor_bmp_get_grid_data(gui_bitmap_bmp_str *s, int x, int y, int *bit )
{
    unsigned char data;
    unsigned int addr;

    addr = gui_bineditor_bmp_get_grid_addr( s, x, y, bit);
    if( addr < s->be->priv->buff->size)
	data = s->be->priv->buff->data[addr];
    else
	data = 0;

    return data;
}

static void gui_bineditor_bmp_set_grid_bit(gui_bitmap_bmp_str *s, int x, int y, char op )
{
    unsigned char data, tmp;
    int idx = 0;
    unsigned int addr;

    addr = gui_bineditor_bmp_get_grid_addr( s, x, y, &idx);
    if( addr < s->be->priv->buff->size)
	data = s->be->priv->buff->data[addr];
    else
	return;

    if( s->rev) idx = s->bpb - idx - 1;
    if( idx < 0) return;

    tmp = s->mask_tb[ idx ] ^ s->inverse;

    switch( op ){
	case 0: data |= tmp;  break;	// set pixel
	case 1: data &= ~tmp; break;	// clear bit
	case 2: data ^= tmp;  break;	// change bit to oposite
    }
    s->be->priv->buff->data[addr] = data; 
}

static inline char gui_bineditor_bmp_test_bit(gui_bitmap_bmp_str *s, int x, int y )
{
    unsigned char data;
    int idx = 0;

    data = gui_bineditor_bmp_get_grid_data(s, x, y, &idx);
    if( s->rev) idx = s->bpb - idx - 1;
    
    if( idx < 0) return 0;
    return data & (s->mask_tb[ idx ] ^ s->inverse);
}

static void gui_bineditor_bmp_draw_ctx( gui_bitmap_bmp_str *s )
{
    int i, j;

    for(j = 0; j < s->rows; j++)
	for( i = 0; i < s->cols; i++)
	    if( gui_bineditor_bmp_test_bit(s, i, j)) 
			gui_bineditor_bmp_draw_pixel( s, i, j);
}

static void gui_bineditor_bmp_draw(gui_bitmap_bmp_str *s)
{
    if(s->first_run) gui_bineditor_bmp_zoom_auto( s );
    gui_bineditor_bmp_draw_compute( s );
    gui_bineditor_bmp_draw_ambient( s );
    gui_bineditor_bmp_draw_background( s );
    gui_bineditor_bmp_draw_grid( s );
    gui_bineditor_bmp_draw_ctx( s );
}

void gui_bineditor_bitmap_set_address(GuiBineditor *be, unsigned int address)
{
    if( be->priv->bmp == NULL) return;
    ((gui_bitmap_bmp_str *)be->priv->bmp)->addr = address;
    gui_bineditor_bmp_redraw(  (gui_bitmap_bmp_str *)(be->priv->bmp) );
}

char gui_bineditor_bitmap_get_mode(GuiBineditor *be)
{
    if( be->priv->bmp == NULL) return FALSE;
    return ((gui_bitmap_bmp_str *)be->priv->bmp)->mode;
}

