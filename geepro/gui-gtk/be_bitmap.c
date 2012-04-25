#include <stdio.h>
#include <stdlib.h>
#include "../intl/lang.h"
#include "be_bitmap.h"

#define TEXT_TIP_TB_ZOOM_OUT	"zoom out"
#define TEXT_TIP_TB_ZOOM_IN	"zoom in"
#define TEXT_TIP_TB_ZOOM_FIT	"zoom fit"
#define TEXT_TIP_TB_INVERSE	"negate bit viewing"
#define TEXT_TIP_TB_MIRROR	"inverse bit order"
#define TEXT_TIP_TB_GRID	"grid off"
#define TEXT_TIP_TB_LIVE	"refresh on the fly"
#define TEXT_TIP_TB_EDIT	"permision to edit"
#define TEXT_LBL_SB_MULT	"Mult:"
#define TEXT_LBL_SB_WIDTH	"Width:"
#define TEXT_LBL_SB_HEIGHT	"Height:"
#define TEXT_LBL_SB_OFFSET	"Offset:"

// temporary used icons - to change in the future
#define GUI_BINEDITOR_ICON_INVERSE GTK_STOCK_CLOSE
#define GUI_BINEDITOR_ICON_MIRROR  GTK_STOCK_REFRESH
#define GUI_BINEDITOR_ICON_GRID    GTK_STOCK_ADD
#define GUI_BINEDITOR_ICON_LIVE    GTK_STOCK_EXECUTE
#define GUI_BINEDITOR_ICON_EDIT    GTK_STOCK_NO

#define GET_COLOR( idx ) s->be->priv->colors[(idx)*3 + 0],s->be->priv->colors[(idx)*3 + 1],s->be->priv->colors[(idx)*3 + 2]

typedef struct
{
    GuiBineditor *be;	// parent
    // Widgets
    GtkWidget *wmain;
    GtkToolItem *w_zoom_out, *w_zoom_in, *w_zoom_fit;
    GtkToolItem *w_inverse, *w_mirror, *w_grid;
    GtkToolItem *w_live, *w_edit;
    GtkWidget *ctx, *tb, *draw_area, *param_hbox, *info_hbox, *tb_line, *lbl_addr;
    GtkWidget *vslid, *hslid, *w_width, *w_height, *w_offset, *w_multip; // scrollbars
    GtkAdjustment *vadj, *hadj; // scrollbars adjustments

    /* input parameters */
    unsigned int width, height, addr;

    /* graphic */
    cairo_t *cr;
    char first_run;
    unsigned int da_width, da_height; // drawing area size in pixels
    unsigned int wx0,wy0, wxx, wyy;   // position and size of 'window' in pixels 
    unsigned int pixel;		      // 'pixel' size in pixels

    /* grid */
    unsigned int cell_xx, cell_yy;   // cols and rows of displayed grid


    /* flags */    
    char 	grid:1;	// display grid

} gui_bitmap_bmp_str;

/*
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
    s->edit = 1;
    s->g_width = width;
    s->g_high = height;

    s->x1 = 0;
    s->y1 = 0;
*/

static inline void gui_bineditor_bmp_draw_ambient( gui_bitmap_bmp_str *);
static inline void gui_bineditor_bmp_toolbar_setup(gui_bitmap_bmp_str *);
static void gui_bineditor_bmp_destroy(GtkWidget *, GuiBineditor *);
static void gui_bineditor_bmp_parameter_setup(gui_bitmap_bmp_str *);
static void gui_bineditor_bmp_view_addr( gui_bitmap_bmp_str *);
static gboolean gui_bineditor_bmp_draw_event(GtkWidget *, cairo_t *, gui_bitmap_bmp_str *);
static inline void gui_bineditor_bmp_drawing_setup( gui_bitmap_bmp_str *);
static void gui_bineditor_bmp_draw(gui_bitmap_bmp_str *);
static inline void gui_bineditor_bmp_draw_background( gui_bitmap_bmp_str * );
static void gui_bineditor_bmp_draw_compute(gui_bitmap_bmp_str *);
void gui_bineditor_bmp_zoom_fit( gui_bitmap_bmp_str * );
static inline void gui_bineditor_bmp_draw_grid( gui_bitmap_bmp_str *);
static void gui_bineditor_bmp_tb_zoom_fit(GtkWidget *, gui_bitmap_bmp_str *);
static void gui_bineditor_bmp_tb_zoom_in(GtkWidget *, gui_bitmap_bmp_str * );
static void gui_bineditor_bmp_tb_zoom_out(GtkWidget *, gui_bitmap_bmp_str * );
static void gui_bineditor_bmp_scroll(GtkWidget *, GdkEventScroll *, gui_bitmap_bmp_str *);


//static void gui_bineditor_bmp_tb_inverse();
//static void gui_bineditor_bmp_tb_mirror();
//static void gui_bineditor_bmp_tb_grid();
//static void gui_bineditor_bmp_tb_live();
//static void gui_bineditor_bmp_tb_edit();

/***********************************************
*            Public functions                  *
************************************************/
void gui_bineditor_bitmap(GuiBineditor *be, unsigned int width, unsigned int height, unsigned char mask, unsigned char br)
{
    gui_bitmap_bmp_str *s;
    GtkWidget *fr, *wg;

    s = (gui_bitmap_bmp_str *)malloc(sizeof(gui_bitmap_bmp_str));
    if( !s ){
	printf("ERR: gui_bineditor_bitmap() -> memory allocation problem.\n");
	return;    
    }
    s->addr = 0;
    s->be = be;
    s->width = width;
    s->height = height;
    s->first_run = 1;
    s->grid = 1;
    gtk_widget_set_sensitive( be->priv->bined, FALSE );
    s->wmain = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    g_signal_connect(G_OBJECT(s->wmain), "destroy", G_CALLBACK(gui_bineditor_bmp_destroy), be);
    if( be->priv->icon )
	gtk_window_set_icon( GTK_WINDOW( s->wmain ), gdk_pixbuf_new_from_xpm_data( be->priv->icon ));
    gtk_window_set_keep_above(GTK_WINDOW(s->wmain), TRUE);
    gtk_widget_set_size_request( s->wmain, 320, 320); //
    s->ctx = gtk_vbox_new(FALSE, 0);        
    gtk_container_add(GTK_CONTAINER(s->wmain), s->ctx);
    s->tb_line = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(s->ctx), s->tb_line, FALSE, FALSE, 0 );
    s->param_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(s->ctx), s->param_hbox, FALSE, FALSE, 0);
    s->info_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(s->ctx), s->info_hbox, FALSE, FALSE, 0);
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

//    g_signal_connect(G_OBJECT(s->vadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_vch), s);
//    g_signal_connect(G_OBJECT(s->hadj), "value_changed", G_CALLBACK(gui_bineditor_bmp_hch), s);

    gui_bineditor_bmp_toolbar_setup( s );
    gui_bineditor_bmp_parameter_setup( s );
    gui_bineditor_bmp_drawing_setup( s );
    gtk_widget_show_all( s->wmain );


}

void gui_bineditor_bitmap_set_address(GuiBineditor *be, unsigned int address)
{
//    if( be->priv->bmp == NULL) return;
//    ((gui_bitmap_bmp_str *)be->priv->bmp)->addr = address;
//    gui_bineditor_bmp_redraw(  (gui_bitmap_bmp_str *)(be->priv->bmp) );
}

char gui_bineditor_bitmap_get_mode(GuiBineditor *be)
{
//    if( be->priv->bmp == NULL) return FALSE;
//    return ((gui_bitmap_bmp_str *)be->priv->bmp)->mode;
return 0;
}

/***********************************************
*            Private functions                 *
************************************************/
/*=======================================================================================================================================*/
static void gui_bineditor_bmp_destroy(GtkWidget *wg, GuiBineditor *be)
{
    gtk_widget_set_sensitive( be->priv->bined, TRUE );
    free(be->priv->bmp);
    be->priv->bmp = NULL;
}


static inline void gui_bineditor_bmp_toolbar_setup( gui_bitmap_bmp_str *s )
{
    GtkWidget *wg;
    
    // Address info
    wg = gtk_frame_new("");
    s->lbl_addr = gtk_label_new("--");
    gtk_container_add(GTK_CONTAINER( wg ), s->lbl_addr);
    gtk_box_pack_start(GTK_BOX(s->tb_line), wg, FALSE, FALSE, 3);

    gui_bineditor_bmp_view_addr( s );

    // ===> Toolbar <===
    s->tb = gtk_toolbar_new();
    gtk_orientable_set_orientation( GTK_ORIENTABLE(s->tb), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style(GTK_TOOLBAR(s->tb), GTK_TOOLBAR_ICONS);
    gtk_container_add(GTK_CONTAINER(s->tb_line), s->tb);
    // Zoom out 
    s->w_zoom_out = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_OUT );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_zoom_out ), TEXT(TIP_TB_ZOOM_OUT));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_zoom_out), -1);
    // Zoom in
    s->w_zoom_in = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_IN );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_zoom_in ), TEXT(TIP_TB_ZOOM_IN));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_zoom_in), -1);
    // Zoom fit
    s->w_zoom_fit = gtk_tool_button_new_from_stock( GTK_STOCK_ZOOM_FIT );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_zoom_fit ), TEXT(TIP_TB_ZOOM_FIT));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_zoom_fit), -1);
    // Separator
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), gtk_separator_tool_item_new(), -1);
    // Inverse on/off
    s->w_inverse = gtk_tool_button_new_from_stock( GUI_BINEDITOR_ICON_INVERSE );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_inverse ), TEXT(TIP_TB_INVERSE));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_inverse), -1);
    // Mirror  on/off
    s->w_mirror = gtk_tool_button_new_from_stock( GUI_BINEDITOR_ICON_MIRROR );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_mirror ), TEXT(TIP_TB_MIRROR));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_mirror), -1);
    // Grid    on/off
    s->w_grid = gtk_tool_button_new_from_stock( GUI_BINEDITOR_ICON_GRID );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_grid ), TEXT(TIP_TB_GRID));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_grid), -1);
    // Separator
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), gtk_separator_tool_item_new(), -1);
    // Live    on/off
    s->w_live = gtk_tool_button_new_from_stock( GUI_BINEDITOR_ICON_LIVE );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_live ), TEXT(TIP_TB_LIVE));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_live), -1);
    // edit    on/off
    s->w_edit = gtk_tool_button_new_from_stock( GUI_BINEDITOR_ICON_EDIT );
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM( s->w_edit ), TEXT(TIP_TB_EDIT));
    gtk_toolbar_insert(GTK_TOOLBAR( s->tb ), GTK_TOOL_ITEM(s->w_edit), -1);

    g_signal_connect(G_OBJECT(s->w_zoom_out), "clicked", G_CALLBACK( gui_bineditor_bmp_tb_zoom_out), s);
    g_signal_connect(G_OBJECT(s->w_zoom_in), "clicked", G_CALLBACK( gui_bineditor_bmp_tb_zoom_in), s);
    g_signal_connect(G_OBJECT(s->w_zoom_fit), "clicked", G_CALLBACK( gui_bineditor_bmp_tb_zoom_fit ), s);
//    g_signal_connect(G_OBJECT(s->w_inverse, "clicked", G_CALLBACK( gui_bineditor_bmp_tb_inverse ), s);
//    g_signal_connect(G_OBJECT(s->w_mirror, "clicked", G_CALLBACK( gui_bineditor_bmp_tb_mirror ), s);
//    g_signal_connect(G_OBJECT(s->w_grid, "clicked", G_CALLBACK( gui_bineditor_bmp_tb_grid ), s);
//    g_signal_connect(G_OBJECT(s->w_live, "clicked", G_CALLBACK( gui_bineditor_bmp_tb_live ), s);
//    g_signal_connect(G_OBJECT(s->w_edit, "clicked", G_CALLBACK( gui_bineditor_bmp_tb_edit ), s);

}

static void gui_bineditor_bmp_parameter_setup(gui_bitmap_bmp_str *s)
{
    s->w_multip = gtk_spin_button_new_with_range(0, s->be->priv->buff->size, 1);
    s->w_width  = gtk_spin_button_new_with_range(0, s->be->priv->buff->size, 1);
    s->w_height = gtk_spin_button_new_with_range(0, s->be->priv->buff->size, 1);
    s->w_offset = gtk_spin_button_new_with_range(0, s->be->priv->buff->size, 1);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), gtk_label_new(TEXT(LBL_SB_MULT)), FALSE, FALSE, 10);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), s->w_multip, FALSE, FALSE, 0);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), gtk_label_new(TEXT(LBL_SB_WIDTH)), FALSE, FALSE, 10);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), s->w_width,  FALSE, FALSE, 0);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), gtk_label_new(TEXT(LBL_SB_HEIGHT)), FALSE, FALSE, 10);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), s->w_height, FALSE, FALSE, 0);
    gtk_box_pack_start( GTK_BOX(s->param_hbox), gtk_label_new(TEXT(LBL_SB_OFFSET)), FALSE, FALSE, 10);    
    gtk_box_pack_start( GTK_BOX(s->param_hbox), s->w_offset, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->w_multip), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->w_width),  s->width);    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->w_height), s->height);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->w_offset), 0);

//    g_signal_connect(G_OBJECT(s->w_multip, "value_changed", G_CALLBACK( gui_bineditor_bmp_sb_multip ), s);    
//    g_signal_connect(G_OBJECT(s->w_width,  "value_changed", G_CALLBACK( gui_bineditor_bmp_sb_width ),  s);    
//    g_signal_connect(G_OBJECT(s->w_height, "value_changed", G_CALLBACK( gui_bineditor_bmp_sb_height ), s);    
//    g_signal_connect(G_OBJECT(s->w_offset, "value_changed", G_CALLBACK( gui_bineditor_bmp_sb_offset ), s);    
}

static inline void gui_bineditor_bmp_drawing_setup( gui_bitmap_bmp_str *s )
{
    gtk_widget_set_events(s->draw_area, GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK );
    gtk_widget_set_can_focus(s->draw_area, TRUE);
    g_signal_connect(G_OBJECT(s->draw_area), "draw", G_CALLBACK(gui_bineditor_bmp_draw_event), s);
    g_signal_connect(G_OBJECT(s->draw_area), "scroll_event", G_CALLBACK(gui_bineditor_bmp_scroll), s);
//    g_signal_connect(G_OBJECT(s->draw_area), "button_press_event", G_CALLBACK(gui_bineditor_bmp_button_press), s);
//    g_signal_connect(G_OBJECT(s->draw_area), "button_release_event", G_CALLBACK(gui_bineditor_bmp_button_release), s);
}


static void gui_bineditor_bmp_view_addr( gui_bitmap_bmp_str *s )
{
    char tmp[32];
    
    sprintf( tmp, "ADDR: 0x%X%X%X%X%X%X%X%X", 
	(s->addr >> (4*7)) & 0xf,
	(s->addr >> (4*6)) & 0xf,    
	(s->addr >> (4*5)) & 0xf,
	(s->addr >> (4*4)) & 0xf,
	(s->addr >> (4*3)) & 0xf,
	(s->addr >> (4*2)) & 0xf,
	(s->addr >> (4*1)) & 0xf,
	(s->addr >> (4*0)) & 0xf
    );
    gtk_label_set_text(GTK_LABEL( s->lbl_addr), tmp);    
}

static gboolean gui_bineditor_bmp_draw_event(GtkWidget *wg, cairo_t *cr, gui_bitmap_bmp_str *s)
{
    s->cr = cr;
    s->da_width  = gtk_widget_get_allocated_width( wg );
    s->da_height = gtk_widget_get_allocated_height( wg );
    gui_bineditor_bmp_draw( s );
    return FALSE;
}

/*
    ================================> DRAWING <===============================
*/

static void gui_bineditor_bmp_scrollers_actualization( gui_bitmap_bmp_str *s)
{
/*
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
*/
}

static inline void gui_bineditor_bmp_redraw( gui_bitmap_bmp_str *s )
{
    gtk_widget_queue_draw( s->draw_area );
}

static void gui_bineditor_bmp_draw(gui_bitmap_bmp_str *s)
{
    if(s->first_run) gui_bineditor_bmp_zoom_fit( s );
    gui_bineditor_bmp_draw_compute( s );
    gui_bineditor_bmp_draw_ambient( s );
    gui_bineditor_bmp_draw_background( s );

    gui_bineditor_bmp_draw_grid( s );
//    gui_bineditor_bmp_draw_ctx( s );
}

static inline void gui_bineditor_bmp_draw_ambient( gui_bitmap_bmp_str *s)
{
    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_AMBIENT) ); // background color
    cairo_rectangle( s->cr, 0, 0, s->da_width, s->da_height );
    cairo_fill( s->cr );
}

static inline void gui_bineditor_bmp_draw_background( gui_bitmap_bmp_str *s )
{
    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_BG) ); // background color
    cairo_rectangle( s->cr, s->wx0, s->wy0, s->wxx, s->wyy );
    cairo_fill( s->cr );
}

void gui_bineditor_bmp_zoom_fit( gui_bitmap_bmp_str *s )
{
    unsigned int a, b;

    a = s->width  ? s->da_width / s->width : 1;
    b = s->height ? s->da_height / s->height : 1;
    s->pixel = (a > b) ? b : a;
    s->cell_xx = s->width;
    s->cell_yy = s->height;
    s->wxx = s->pixel * s->cell_xx;
    s->wyy = s->pixel * s->cell_yy;
    // Center 
    s->wx0 = (s->da_width - s->wxx) / 2;
    s->wy0 = (s->da_height - s->wyy) / 2;
    
    gui_bineditor_bmp_scrollers_actualization( s );
    s->first_run = 0;
}

static void gui_bineditor_bmp_tb_zoom_fit(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_fit( s );
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_zoom_in( gui_bitmap_bmp_str *s )
{
    s->pixel++;
    if( s->pixel >= s->da_width)  s->pixel = s->da_width;
    if( s->pixel >= s->da_height) s->pixel = s->da_height;
    if( s->pixel >= s->da_width)  s->pixel = s->da_width;
    gui_bineditor_bmp_scrollers_actualization( s );
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_zoom_out( gui_bitmap_bmp_str *s )
{
    s->pixel--;
    if( s->pixel < 1) s->pixel = 1;
    gui_bineditor_bmp_scrollers_actualization( s );
    gui_bineditor_bmp_redraw( s );
}

static void gui_bineditor_bmp_draw_compute(gui_bitmap_bmp_str *s)
{
    if(!s->pixel) return;
//    s->cell_xx = s->wxx / s->pixel;
//    s->cell_yy = s->wyy / s->pixel;

//    s->wxx = s->pixel * s->cell_xx;
//    s->wyy = s->pixel * s->cell_yy;

    // Center 
//    s->wx0 = (s->da_width - s->wxx) / 2;
//    s->wy0 = (s->da_height - s->wyy) / 2;

//    s->first_run = 0;
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

    if( !s->grid ) return;
    cairo_set_source_rgb( s->cr, GET_COLOR(GUI_BINEDITOR_COLOR_BMP_GRID) ); 
    for(i = 0, n = s->wx0; i < s->cell_xx; i++, n += s->pixel) 
	gui_bineditor_bmp_draw_vline( s->cr, n, s->wy0, s->wyy);
    
    for(i = 0, n = s->wy0; i < s->cell_yy; i++, n += s->pixel)
	gui_bineditor_bmp_draw_hline( s->cr, s->wx0, n, s->wxx);

    cairo_stroke( s->cr );        
}

static void gui_bineditor_bmp_tb_zoom_in(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_in( s );
}

static void gui_bineditor_bmp_tb_zoom_out(GtkWidget *wg, gui_bitmap_bmp_str *s )
{
    gui_bineditor_bmp_zoom_out( s );
}

static void gui_bineditor_bmp_scroll(GtkWidget *wg, GdkEventScroll *ev, gui_bitmap_bmp_str *s)
{
    char within = 0;
//    gui_bineditor_bmp_grid_coordinates(s, ev->x, ev->y, &within);
    // relative position in grid
//    s->xp = (((ev->x - s->x0) / s->pixel_size)) / s->cols;
//    s->yp = (((ev->y - s->y0) / s->pixel_size)) / s->rows;

    switch( ev->direction ){
	case GDK_SCROLL_UP: gui_bineditor_bmp_zoom_in(s); break;
	case GDK_SCROLL_DOWN: gui_bineditor_bmp_zoom_out(s); break;
	default:;
    }
    gui_bineditor_bmp_scrollers_actualization( s );
}

/*
static void gui_bineditor_bmp_grid_coordinates(gui_bitmap_bmp_str *s, unsigned int x, unsigned int y, char *within)
{
    *within = 1;
    s->mx = s->x1 + ((x - s->x0) / s->pixel_size);
    s->my = s->y1 + ((y - s->y0) / s->pixel_size);
    if((s->mx >= s->g_width) || (s->mx < 0) || (s->my >= s->g_high) || (s->my < 0)) *within = 0;
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



static void gui_bineditor_bmp_mouse_button( gui_bitmap_bmp_str *s, int button, char state, unsigned int x, unsigned int y )
{
    char within = 0;

    if( !s->edit ) return;

    gui_bineditor_bmp_grid_coordinates(s, x, y, &within);
    if( !within ) return;
    switch( button ){
	case 1 : gui_bineditor_bmp_set_grid_bit(s, s->mx + s->x1, s->my + s->y1, 0 ); break;
	case 3 : gui_bineditor_bmp_set_grid_bit(s, s->mx + s->x1, s->my + s->y1, 1 ); break;
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
	    if( gui_bineditor_bmp_test_bit(s, i + s->x1, j + s->y1)) 
			gui_bineditor_bmp_draw_pixel( s, i - s->x1, j - s->y1);
}

*/
