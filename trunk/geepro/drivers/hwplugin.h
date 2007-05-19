/* $Revision: 1.2 $ */
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

#ifndef __hwplugin_h__
#define __hwplugin_h__

/********************************************************************/
/* funkcje v. 0.0.1 */

/* ogólne */
#define HW_IFACE	1
#define HW_GINIT	2
#define HW_TEST		3
#define HW_RESET	4
#define HW_OPEN		5
#define HW_CLOSE	6
#define HW_SET_VCC	7
#define HW_SET_VPP	8
#define HW_SW_VCC	9
#define HW_SW_VPP	10
#define HW_DELAY	11
#define HW_LATENCY      12
#define HW_NAME		13
#define HW_DESTROY	14

/* specyficzne dla programatora eprom */
#define HW_SET_DATA	  1024
#define HW_SET_ADDR	  1025
#define HW_INC_ADDR	  1026
#define HW_DEC_ADDR	  1027
#define HW_RST_ADDR	  1028
#define HW_GET_DATA	  1029
#define HW_SET_WE	  1030
#define HW_SET_OE	  1031
#define HW_SET_CE	  1032
#define HW_SET_CS	  1033
#define HW_SET_CLK	  1034
#define HW_SET_DI	  1035
#define HW_GET_DO	  1036
#define HW_SET_HOLD	  1037
#define HW_SET_SCL	  1038
#define HW_SW_DPSW	  1040
#define HW_SET_SDA	  1041
#define HW_GET_SDA        1042

/********************************************************************/
/* błędy */

#define HW_ERROR     -1	 // wystapił bład wykonania funkcji
#define HW_ERR_UFUNC -2  // nieznana funkcja
#define HW_SUCCESS    0

/********************************************************************/
/* funkcja dostępu */
typedef int (*hw_module_type)(int func_id, int arg, void *ptr);

extern hw_module_type ___hardware_module___; 

/********************************************************************/
/* makra pseudofunkcji */

#define hw_get_iface()		___hardware_module___( HW_IFACE,  0, NULL)
#define hw_gui_init(geepro)	___hardware_module___( HW_GINIT,  0, geepro)
#define hw_test_conn()		___hardware_module___( HW_TEST,   0, NULL)
#define hw_reset()		___hardware_module___( HW_RESET,  0, NULL)
#define hw_open(dev, flags)	___hardware_module___( HW_OPEN,   flags, dev)
#define hw_close()		___hardware_module___( HW_CLOSE,  0, NULL)
#define hw_set_vcc(val)		___hardware_module___( HW_SET_VCC,val, NULL)
#define hw_set_vpp(val)		___hardware_module___( HW_SET_VPP,val, NULL)
#define hw_sw_vcc(val)		___hardware_module___( HW_SW_VCC, val, NULL)
#define hw_sw_vpp(val)		___hardware_module___( HW_SW_VPP, val, NULL)
#define hw_delay(val)		___hardware_module___( HW_DELAY,  val, NULL)	/* opoznienie w us */
#define hw_low_latency(val)	___hardware_module___( HW_LATENCY,val, NULL)	/* jesli root to właczenie/ wyłaczenie zmiany schedulera */
#define hw_get_name(val)	___hardware_module___( HW_NAME, 0, &(val))
#define hw_destroy(geepro)	___hardware_module___( HW_DESTROY, 0, geepro)

/* specyficzne dla programatora eprom */
#define hw_set_data(val)	___hardware_module___( HW_SET_DATA, val, NULL)
#define hw_get_data()		___hardware_module___( HW_GET_DATA, 0, NULL)
#define hw_set_addr(val)	___hardware_module___( HW_SET_ADDR, val, NULL)
#define hw_inc_addr()		___hardware_module___( HW_INC_ADDR, 0, NULL)
#define hw_dec_addr()		___hardware_module___( HW_DEC_ADDR, 0, NULL)
#define hw_rst_addr()		___hardware_module___( HW_RST_ADDR, 0, NULL)
#define hw_set_we(val)		___hardware_module___( HW_SET_WE,   val, NULL)
#define hw_set_oe(val)		___hardware_module___( HW_SET_OE,   val, NULL)
#define hw_set_ce(val)		___hardware_module___( HW_SET_CE,   val, NULL)
#define hw_set_cs(val)		___hardware_module___( HW_SET_CS,   val, NULL)
#define hw_set_clk(val)		___hardware_module___( HW_SET_CLK,  val, NULL)
#define hw_set_di(val)		___hardware_module___( HW_SET_DI,   val, NULL)
#define hw_get_do()		___hardware_module___( HW_GET_DO,   0, NULL)
#define hw_set_hold(val)	___hardware_module___( HW_SET_HOLD, val, NULL)
#define hw_set_scl(val)		___hardware_module___( HW_SET_SCL,  val, NULL)
#define hw_set_sda(val)		___hardware_module___( HW_SET_SDA,  val, NULL)
#define hw_get_sda()		___hardware_module___( HW_GET_SDA,  0, NULL)
#define hw_sw_dpsw(val)		___hardware_module___( HW_SW_DPSW,  val, NULL)

#endif

