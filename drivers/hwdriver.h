/* $Revision: 1.4 $ */
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

#ifndef __hwdriver_h__
#define __hwdriver_h__

#ifdef __cplusplus
extern "C"{
#endif


// obsolete, will be removed in the future
#define hw_delay(val)		hw_us_delay(val) 

// pragm - some specyfic driver only info
typedef enum{
    PRAGMA_CE_EQ_PGM = 1,
} en_hw_pragm;

// Functions ID
typedef enum{
    HW_INIT_VALUE_0 = 0,
    HW_IFACE,
    HW_GINIT,
    HW_TEST,
    HW_TEST_CONTINUE,
    HW_RESET,
//    HW_OPEN,
//    HW_CLOSE,
    HW_SET_VCC,	
    HW_SET_VPP,
    HW_SW_VCC,
    HW_SW_VPP,
    HW_DELAY,
    HW_LATENCY,
    HW_NAME,
    HW_DESTROY,
    HW_SET_CHIP,
    HW_PRAGMA,	// specyfical parameter for programmer

    HW_INIT_VALUE_1 = 1024,
// specyfical for parallel:
    HW_SET_ADDR_RANGE,
    HW_SET_DATA,
    HW_SET_ADDR,
    HW_INC_ADDR,
    HW_DEC_ADDR,
    HW_RST_ADDR,
    HW_GET_DATA,
    HW_SET_WE,
    HW_SET_OE,
    HW_SET_CE,
    HW_SET_CS,
    HW_SW_DPSW,
// specyfical for serial:
    HW_SET_CLK,
    HW_SET_DI,
    HW_GET_DO,
    HW_SET_HOLD,
    HW_SET_SCL,
    HW_SET_SDA,
    HW_GET_SDA,
    HW_SET_PGM,
    HW_GET_SCL,
// PIC ICSP
    HW_GET_RB7,
    HW_SET_RB6,
// ISP
    HW_SET_ISP_MOSI,
    HW_GET_ISP_MISO,
    HW_SET_ISP_SCK,
    HW_SET_ISP_RST,
    HW_SET_ISP_LED,
    HW_SET_AVR_XTAL,
// JTAG
    HW_SET_JTAG_TDI,
    HW_SET_JTAG_TCK,
    HW_SET_JTAG_TMS,
    HW_GET_JTAG_TDO,
    HW_SET_JTAG_TDO,
    HW_SET_JTAG_TRST,
    HW_GET_JTAG_NSTAT,
// GAL
    HW_GET_GAL_SDOUT,
    HW_SET_GAL_SDIN,
    HW_SET_GAL_SCLK,
    HW_SET_GAL_STB,
    HW_SET_GAL_PV,
    HW_SET_GAL_RA,
} en_hw_api;

/********************************************************************
* errors 
*/

#define HW_ERROR     -1	 // error ocured during function execution
#define HW_ERR_UFUNC -2  // undefined function
#define HW_SUCCESS    0

/********************************************************************
* access function 
*/
typedef int (*hw_driver_type)(void *root, en_hw_api func_id, int arg, void *ptr);


/********************************************************************
* macros for pseudo functions; driver API
*/

#define hwd			iface_driver_call(gep->ifc->drv)

#define hw_get_iface()		hwd(gep, HW_IFACE,  0, NULL)
#define hw_gui_init(geepro)	hwd(gep, HW_GINIT,  0, geepro)
#define hw_test_conn()		hwd(gep, HW_TEST,   0, NULL)
#define hw_test_continue()	hwd(gep, HW_TEST_CONTINUE,   0, NULL)	// returns true or false flag - if true -> allow continue test
#define hw_reset()		hwd(gep, HW_RESET,  0, NULL)
#define hw_set_vcc(val)		hwd(gep, HW_SET_VCC,val, NULL)
#define hw_set_vpp(val)		hwd(gep, HW_SET_VPP,val, NULL)
#define hw_sw_vcc(val)		hwd(gep, HW_SW_VCC, val, NULL)
#define hw_sw_vpp(val)		hwd(gep, HW_SW_VPP, val, NULL)
#define hw_us_delay(val)	hwd(gep, HW_DELAY,  val, NULL)		// delay µs
#define hw_ms_delay(val)	hwd(gep, HW_DELAY,  (val) * 1000, NULL)	// delay ms
#define hw_low_latency(val)	hwd(gep, HW_LATENCY,val, NULL)		// scheduler change if root
#define hw_get_name(val)	hwd(gep, HW_NAME, 0, &(val))
#define hw_destroy(geepro)	hwd(gep, HW_DESTROY, 0, geepro)
#define hw_set_chip(geepro)	hwd(gep, HW_SET_CHIP, 0, geepro)  	// chip information for driver; returns HW_ERROR if not implemented
#define hw_pragma( pragma )	hwd(gep, HW_PRAGMA, pragma, NULL)		// set pragma

// specyfical for parallel
#define hw_set_addr_range(val)	hwd(gep, HW_SET_ADDR_RANGE,val, NULL)
#define hw_set_data(val)	hwd(gep, HW_SET_DATA, val, NULL)
#define hw_get_data()		hwd(gep, HW_GET_DATA, 0, NULL)
#define hw_set_addr(val)	hwd(gep, HW_SET_ADDR, val, NULL)
#define hw_inc_addr()		hwd(gep, HW_INC_ADDR, 0, NULL)
#define hw_dec_addr()		hwd(gep, HW_DEC_ADDR, 0, NULL)
#define hw_rst_addr()		hwd(gep, HW_RST_ADDR, 0, NULL)
#define hw_set_we(val)		hwd(gep, HW_SET_WE,   val, NULL)
#define hw_set_oe(val)		hwd(gep, HW_SET_OE,   val, NULL)
#define hw_set_ce(val)		hwd(gep, HW_SET_CE,   val, NULL)
#define hw_set_pgm(val)		hwd(gep, HW_SET_PGM,   val, NULL)
#define hw_set_cs(val)		hwd(gep, HW_SET_CS,   val, NULL)
#define hw_sw_dpsw(val)		hwd(gep, HW_SW_DPSW,  val, NULL)

// specyfical for serial
#define hw_set_clk(val)		hwd(gep, HW_SET_CLK,  val, NULL)
#define hw_set_di(val)		hwd(gep, HW_SET_DI,   val, NULL)
#define hw_get_do()		hwd(gep, HW_GET_DO,   0, NULL)
#define hw_set_hold(val)	hwd(gep, HW_SET_HOLD, val, NULL)
#define hw_set_scl(val)		hwd(gep, HW_SET_SCL,  val, NULL)
#define hw_set_sda(val)		hwd(gep, HW_SET_SDA,  val, NULL)
#define hw_get_sda()		hwd(gep, HW_GET_SDA,  0, NULL)
#define hw_get_scl()		hwd(gep, HW_GET_SCL,  0, NULL)

// ICSP
#define hw_get_rb7()		hwd(gep, HW_GET_RB7,       0, NULL)
#define hw_set_rb6(val)		hwd(gep, HW_SET_RB6,       val, NULL)

// ISP
#define hw_set_isp_mosi(val)	hwd(gep, HW_SET_ISP_MOSI,  val, NULL)
#define hw_get_isp_miso()	hwd(gep, HW_GET_ISP_MISO,  0, NULL)
#define hw_set_isp_sck(val)	hwd(gep, HW_SET_ISP_SCK,   val, NULL)
#define hw_set_isp_rst(val)	hwd(gep, HW_SET_ISP_RST,   val, NULL)
#define hw_set_isp_led(val)	hwd(gep, HW_SET_ISP_LED,   val, NULL)

#define hw_set_avr_xtal(val)	hwd( HW_SET_AVR_XTAL,   val, NULL)

// JTAG
#define hw_set_jtag_tdi(val)	hwd(gep, HW_SET_JTAG_TDI,  val, NULL)
#define hw_set_jtag_tck(val)	hwd(gep, HW_SET_JTAG_TCK,  val, NULL)
#define hw_set_jtag_tms(val)	hwd(gep, HW_SET_JTAG_TMS,  val, NULL)
#define hw_get_jtag_tdo()	hwd(gep, HW_GET_JTAG_TDO,  0, NULL)
#define hw_set_jtag_tdo()	hwd(gep, HW_SET_JTAG_TDO,  0, NULL)
#define hw_set_jtag_trst()	hwd(gep, HW_SET_JTAG_TRST, 0, NULL)
#define hw_get_jtag_nstat()	hwd(gep, HW_GET_JTAG_NSTAT, 0, NULL)

// GAL 
#define hw_get_gal_sdout()	hwd(gep, HW_GET_GAL_SDOUT,   0, NULL)
#define hw_set_gal_sdin(val)    hwd(gep, HW_SET_GAL_SDIN,  val, NULL)
#define hw_set_gal_sclk(val)    hwd(gep, HW_SET_GAL_SCLK,  val, NULL)
#define hw_set_gal_stb(val)     hwd(gep, HW_SET_GAL_STB,   val, NULL)
#define hw_set_gal_pv(val)      hwd(gep, HW_SET_GAL_PV,    val, NULL)
#define hw_set_gal_ra(val)      hwd(gep, HW_SET_GAL_RA,    val, NULL)

#ifdef __cplusplus
} // extern "C"
#endif

#endif

