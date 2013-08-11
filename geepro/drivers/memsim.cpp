/* $Revision: 1.2 $ */
/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
 *
 * STK200 parallel port driver
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

#include "drivers.h"

#define MEMSIM_TRANSM		(1 << 0)
#define MEMSIM_MUX		(1 << 1)
#define MEMSIM_RST_STATE	(1 << 2)
#define MEMSIM_RST_OUT		(1 << 3)
#define MEMSIM_RDY		(1 << 4)

#define MEMSIM_CTRL	4	// U17
#define MEMSIM_ADDR_0	0	// U4
#define MEMSIM_ADDR_1	1	// U5
#define MEMSIM_ADDR_2	2	// U6
#define MEMSIM_MASK	3	// U7
#define MEMSIM_DATA	5	// /WE

#define BX(d, sh)	((d >> sh) & 0xff)

static geepro *gep = NULL; // temporary

static char memsim_ctrl = 0;
static char reset_state = 0;
static char reset_duration = 1; // ms

static void memsim_set_mux(char state);
static void memsim_set_rst_state(char state);
static void memsim_set_rst_out(char state);

static void memsim_set_mode( char reg)
{
    if( reg & 4 ) parport_set_bit( PC, PP_17); else parport_clr_bit(PC, PP_17);
    if( reg & 2 ) parport_set_bit( PC, PP_14); else parport_clr_bit(PC, PP_14);
    if( reg & 1 ) parport_set_bit( PC, PP_16); else parport_clr_bit(PC, PP_16);
}

static void memsim_pulse()
{
    parport_set_bit( PC, PP_01);
//timer_us_delay(10000);
    parport_clr_bit( PC, PP_01);
//timer_us_delay(10000);
}

static void memsim_set_register(char reg, char data)
{
    parport_set(PA, data);
    memsim_set_mode( reg );
    memsim_pulse();
}

static void memsim_set_address(int addr)
{
    memsim_set_register( MEMSIM_ADDR_0, BX(addr, 0)  );
    memsim_set_register( MEMSIM_ADDR_1, BX(addr, 8)  );
    memsim_set_register( MEMSIM_ADDR_2, BX(addr, 16) );
}

static void memsim_set_mask(int mask)
{
    memsim_set_register( MEMSIM_MASK, mask );
}

static inline void memsim_set_ctrl_reg( char data )
{
    memsim_set_register( MEMSIM_CTRL, data);
//timer_us_delay(10000);
}

static void memsim_wr_data(char data)
{
    parport_set(PA, data);
    memsim_set_mode( MEMSIM_DATA );
    parport_set_bit( PC, PP_01);
    parport_clr_bit( PC, PP_01);
}

static void memsim_write_byte(int addr, char data)
{
    memsim_set_address( addr );
    memsim_wr_data( data );
}

static int memsim_reset()
{
    int err = 0;
    memsim_ctrl = 0;
    err =  parport_reset();
    memsim_set_ctrl_reg( 0 );
    memsim_set_address( 0 );
    memsim_set_mask( 0 );
    memsim_set_mux( 0 );
    memsim_set_rst_state( 0 );
    memsim_set_rst_out( 0 );
    return err;
}

static int memsim_open(const char *ptr, int flags)
{
    if(parport_init(ptr, flags)) return HW_ERROR;
    return memsim_reset();
}

static int memsim_close()
{
    if( memsim_reset() == PP_ERROR ) return HW_ERROR;
    return parport_cleanup();
}

inline static int memsim_set_test(char test)
{
    if( test ){
	if(parport_set_bit(PC, PP_14)) return PP_ERROR;
    } else {
	if(parport_clr_bit(PC, PP_14)) return PP_ERROR;
    }
    return 0;
}

inline static int memsim_get_test()
{    
    return parport_get_bit(PB, PP_15);
}

static int memsim_test_connected()
{
    if( memsim_set_test( 0 ) ) return 0;
    if( memsim_get_test() ) return 0;
    gep->hw_delay( 1000 );
    if( memsim_set_test( 1 ) ) return 0;
    if( !memsim_get_test() ) return 0;
    gep->hw_delay( 1000 );
    if( memsim_set_test( 0 ) ) return 0;
    return 1;
}

static void memsim_set_rdy(char state)
{
    if( state )
	memsim_ctrl |= MEMSIM_RDY;
    else
	memsim_ctrl &= ~MEMSIM_RDY;
    memsim_set_ctrl_reg( memsim_ctrl );
}

static void memsim_set_trans(char state)
{
    if( state )
	memsim_ctrl |= MEMSIM_TRANSM;
    else
	memsim_ctrl &= ~MEMSIM_TRANSM;
    memsim_set_ctrl_reg( memsim_ctrl );
}

static void memsim_set_rst_state(char state)
{
    if( state )
	memsim_ctrl |= MEMSIM_RST_STATE;
    else
	memsim_ctrl &= ~MEMSIM_RST_STATE;
    memsim_set_ctrl_reg( memsim_ctrl );
}

static void memsim_set_rst_out(char state)
{
    if( state )
	memsim_ctrl |= MEMSIM_RST_OUT;
    else
	memsim_ctrl &= ~MEMSIM_RST_OUT;
    memsim_set_ctrl_reg( memsim_ctrl );
}

static void memsim_set_mux(char state)
{
    if( state )
	memsim_ctrl |= MEMSIM_MUX;
    else
	memsim_ctrl &= ~MEMSIM_MUX;
    memsim_set_ctrl_reg( memsim_ctrl );
}

// generate reset output pulse
static void memsim_out_reset()
{
    memsim_set_rst_state( reset_state );
    timer_us_delay(1000);
    memsim_set_rst_out( 1 );    
    timer_us_delay(1000 * reset_duration);
    memsim_set_rst_out( 0 );    
}

/*
    GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI 
*/
static void memsim_event(gui_xml_ev *ev, int value, const char *sval)
{
    gui_xml_val_str gxvs;
    
    gui_xml_get_widget_value((gui_xml *)(ev->root_parent), GUI_XML_CHECK_BUTTON, (char *)"READY", &gxvs);
    memsim_set_rdy(gxvs.ival);
    gui_xml_get_widget_value((gui_xml *)(ev->root_parent), GUI_XML_CHECK_BUTTON, (char *)"TRANS", &gxvs);
    memsim_set_trans(gxvs.ival);
    gui_xml_get_widget_value((gui_xml *)(ev->root_parent), GUI_XML_CHECK_BUTTON, (char *)"RST_ST", &gxvs);
    memsim_set_rst_state(gxvs.ival);
    gui_xml_get_widget_value((gui_xml *)(ev->root_parent), GUI_XML_CHECK_BUTTON, (char *)"RST_OUT", &gxvs);
    memsim_set_rst_out(gxvs.ival);
}

static gui_xml_ifattr	memsim_if_attr[4] = {{ "chip", "none" }, { "programmer", "memsim" }, { "family", "none" }, { NULL, NULL }};

static int memsim_gui(geepro *gep, const char *chip_name, const char *family)
{
    // HW test page
    memsim_if_attr[0].val = chip_name;
    memsim_if_attr[2].val = family;
    gui_xml_build(GUI_XML(GUI(gep->gui)->xml), (char *)"file://./drivers/memsim.xml", (char *)"info,notebook", memsim_if_attr, gep->shared_geepro_dir);
    gui_xml_register_event_func(GUI_XML(GUI(gep->gui)->xml), memsim_event);
    return 0;
}

/*
    API API API API API API API API API API API API API API API API API API API API API API API API API API API API 
*/
int memsim_api(void *g,en_hw_api func, int val, void *ptr)
{
    gep = GEEPRO(g);
    switch(func)
    {
	case HW_IFACE: return IFACE_LPT;
	case HW_NAME : DRIVER_NAME(ptr) = (char *)"memSIM"; return HW_SUCCESS;
	case HW_OPEN : return memsim_open((const char *)ptr, val);
	case HW_CLOSE: return memsim_close();
	case HW_TEST : return memsim_test_connected();
	case HW_TEST_CONTINUE : return 1;
	case HW_SET_DATA: memsim_wr_data( val ); return HW_SUCCESS;
	case HW_SET_ADDR: memsim_set_address( val ); return HW_SUCCESS;
	case HW_SET_PGM: memsim_set_rdy( val ); return HW_SUCCESS;
	case HW_SET_OE: memsim_set_mux( val ); return HW_SUCCESS;
	case HW_RST_ADDR: memsim_out_reset(); return HW_SUCCESS;
	case HW_SET_CS: reset_state = val; return HW_SUCCESS;
	case HW_SET_ADDR_RANGE: memsim_set_mask( val ); return HW_SUCCESS;
	case HW_SET_WE: reset_duration = val; return HW_SUCCESS;
	case HW_GET_DATA: return reset_duration;
	case HW_GET_DO: return reset_state;
	case HW_SW_DPSW: memsim_set_trans(val); return HW_SUCCESS;
	// GUI
	case HW_GINIT: return memsim_gui( GEEPRO(ptr), (const char *)"none", (const char *)"" );
	default: return 0;
    }

    return -2;
}

driver_register_begin
    register_api( memsim_api );
driver_register_end

