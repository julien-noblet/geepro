/* $Revision: 1.3 $ */
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

#include "plugins.h"


static unsigned int _addr=0;
extern int ___uid___;


static int willem_set_dip_sw(int dip){ DEBUG("[DM]Set Dip SW: 0x%x\n",dip); return 0; }
static int willem_set_vpp_voltage(int voltage){ DEBUG("[DM]Set VPP value: %f\n",voltage / 100.0); return 0; }
static int willem_set_vcc_voltage(int voltage){ DEBUG("[DM]Set VCC value: %f\n",voltage / 100.0); return 0; }

/* programmer control */
static int willem_vcc_on()
{
    return parport_set_bit(PC,PP_16);
}

static int willem_vcc_off()
{
    return parport_clr_bit(PC,PP_16);
}

static int willem_vpp_on()
{
    return parport_set_bit(PC,PP_01);
}

static int willem_vpp_off()
{
    return parport_clr_bit(PC,PP_01);
}

/* ZIF-32 - test settings */
static int willem_set_par_data_pin(char data)
{
    if(parport_clr_bit(PC,PP_14) == PP_ERROR) return HW_ERROR;
    return parport_set(PA,data);
}

static int willem_set_par_addr_pin(int addr)
{
    int mask, err=0;

    _addr = addr;
    err = parport_set_bit(PC, PP_14); /* przelaczenie multipleksera U7 na D i CLK*/
    mask = 0x800000;    
    err |= parport_clr_bit(PA, PP_03 | PP_02); /* wyzerowanie D0 i D1 -> D i CLK przesuwnika */
    while(mask && !err){
	timer_us_delay(TA_01);
	if(addr & mask) err |= parport_set_bit(PA, PP_03); else err |= parport_clr_bit(PA, PP_03);
	timer_us_delay(TA_02);
	err |= parport_set_bit(PA, PP_02);
	timer_us_delay(TA_03);
	err |= parport_clr_bit(PA, PP_02);
	mask = mask >> 1;
    }
    err |= parport_clr_bit(PC, PP_14); /* przelaczenie multipleksera U7 na dane */ 
    return err;
}

static int willempro2_set_par_addr_pin(int addr)
{
    // pro 2 pozwala przeslac do kazdego rejestru 4015 oddzielnie 
    int mask0,mask1,mask2, err=0;

    _addr = addr;
    err = parport_set_bit(PC, PP_14); /* przelaczenie multipleksera U7 na D i CLK*/
    mask0 = 0x000080;
    mask1 = 0x008000;
    mask2 = 0x800000;
    err |= parport_clr_bit(PA, PP_07 |PP_06 | PP_03 | PP_02); /* wyzerowanie D5,D4,D0 i D1 -> D i CLK przesuwnika */
    while(mask0 && !err){
	timer_us_delay(TA_01);
	if(addr & mask0) err |= parport_set_bit(PA, PP_03); else err |= parport_clr_bit(PA, PP_03);
	if(addr & mask1) err |= parport_set_bit(PA, PP_06); else err |= parport_clr_bit(PA, PP_06);
	if(addr & mask2) err |= parport_set_bit(PA, PP_07); else err |= parport_clr_bit(PA, PP_07);
	timer_us_delay(TA_02);
	err |= parport_set_bit(PA, PP_02);
	timer_us_delay(TA_03);
	err |= parport_clr_bit(PA, PP_02);
	mask0 = mask0 >> 1;
	mask1 = mask1 >> 1;
	mask2 = mask2 >> 1;
    }
    err |= parport_clr_bit(PC, PP_14); /* przelaczenie multipleksera U7 na dane */ 
    return err;
}

static int willem_inc_addr()
{
    if(_addr < -1) _addr++;
    return willem_set_par_addr_pin(_addr);
}

static int willem_dec_addr()
{
    if(_addr > 0) _addr--;
    return willem_set_par_addr_pin(_addr);
}

static int willem_rst_addr()
{
    _addr=0;
    return willem_set_par_addr_pin(_addr);
}

static int willem_set_we_pin(char bool)
{
    if(bool) return parport_set_bit(PC,PP_17); 
    return parport_clr_bit(PC,PP_17);
}

static int willem_set_oe_pin(char bool)
{
    if(bool) return parport_clr_bit(PC,PP_14); 
    return parport_set_bit(PC,PP_14);
}

static int willem_set_ce_pin(char bool)
{
    if(bool) return parport_set_bit(PC,PP_17); 
    return parport_clr_bit(PC,PP_17);
}

static int willem_get_par_data_pin(void)
{
    unsigned char data,i;
    int err =0, x;    
    
    data = 0;
    err = parport_set_bit(PC,PP_14);
    err |= parport_set_bit(PA,PP_04);
    err |= parport_clr_bit(PA,PP_03);    
    timer_us_delay(TD_01);
    err |= parport_set_bit(PA,PP_03);    
    timer_us_delay(TD_02);
    err |= parport_clr_bit(PA,PP_04);        
    timer_us_delay(TD_03);
    err |= parport_set_bit(PA,PP_04);        
    err |= parport_clr_bit(PA,PP_03);        

    for(i = 0x80; i && !err; i >>= 1){
	timer_us_delay(TD_04);
	if((x = parport_get_bit(PB, PP_10))) data |= i; 
	if(x == PP_ERROR) err = HW_ERROR;	
	err |= parport_clr_bit(PA,PP_04);
	timer_us_delay(TD_05);
	err |= parport_set_bit(PA,PP_04);
    }

    return data | err;
}


/* serial 93Cxx */
static int willem_set_cs_pin(char bool)
{
    if(bool) return parport_set_bit(PC,PP_17); 
    return parport_clr_bit(PC,PP_17); 
}

static int willem_set_clk_pin(char bool)
{
    if(parport_clr_bit(PC,PP_14) == PP_ERROR) return HW_ERROR;
    if(bool) return parport_set_bit(PA,PP_03); 
    return parport_clr_bit(PA,PP_03);
}

static int willem_set_di_pin(char bool)
{
    if(bool) return parport_clr_bit(PA,PP_01); 
    return parport_set_bit(PA,PP_01);
}

static int willem_get_do_pin(void)
{
    return parport_get_bit(PB,PP_11);
}

/* pic & 24cxx */
static int willem_set_scl_pin(char val)
{ return willem_set_clk_pin(val); }

static int willem_set_sda_pin(char val)
{ return willem_set_di_pin(!val); }

static int willem_set_test_pin(char val)
{ return willem_set_cs_pin(val); }

static int willem_get_sda_pin(void)
{ return willem_get_do_pin(); }


static int willem_reset(void)
{
    int err=0;
    err = parport_reset();
    err |= willem_vpp_off();
    err |= willem_vcc_off();
    err |= willem_set_vpp_voltage(0);
    err |= willem_set_vcc_voltage(0);
    err |= willem_set_dip_sw(0);
    err |= willem_set_par_data_pin(0);
    err |= willem_set_par_addr_pin(0);
    err |= willem_set_we_pin(0);        
    err |= willem_set_oe_pin(0);
    err |= willem_set_ce_pin(0);
    err |= willem_rst_addr();
    err |= parport_reset();
    return err;
}

#define W20(time, r) \
    willem_set_di_pin(r);\
    usleep(time);\
    if(willem_get_do_pin() == r) { willem_vcc_off(); return 0; }\
    usleep(time);\

static int willem_test_connection()
{
    willem_vcc_on();    
    willem_set_di_pin(0);
    usleep(10000);
    W20(1000, 1);
    W20(1000, 0);
    W20(1000, 1);
    W20(1000, 0);
    W20(1000, 1);
    W20(1000, 1);
    W20(1000, 1);
    W20(1000, 0);
    W20(1000, 0);
    usleep(1000);
    willem_vcc_off();
    return 1;
}

static void willem_set_gui_main(geepro *gep)
{
    gui_xml_build(GUI_XML(GUI(gep->gui)->xml), "file://./drivers/willem.xml", "info,notebook", "");
}

static int willem_gui_init(void *ptr)
{
    willem_set_gui_main((geepro*)ptr);
    return 0;
}

static int willem_open(const char *ptr, int flags)
{
    SET_PCB_WILLEM(1);
    if(parport_init(ptr, flags) == PP_ERROR) return HW_ERROR;
    return willem_reset();
}


static int pcb3_open(const char *ptr, int flags)
{
    SET_PCB_WILLEM(0);
    if(parport_init(ptr, flags) == PP_ERROR) return HW_ERROR;
    return willem_reset();
}

static int willem_close(void)
{
    if(willem_reset() == PP_ERROR) return HW_ERROR;
    return parport_cleanup();
}


/**************************************************************************************************************************/

int willem3_hardware_module(int funct, int val, void *ptr)
{
    switch(funct){
	/* ogólne */
	case HW_NAME:	  DRIVER_NAME(ptr) = "WILLEM 3.0";
	case HW_IFACE:	  return IFACE_LPT;
	case HW_GINIT:    return willem_gui_init(ptr);
	case HW_TEST:	  return willem_test_connection();
	case HW_RESET:    return willem_reset();
	case HW_OPEN:     return willem_open(ptr,val);
	case HW_CLOSE:    return willem_close();
	case HW_SET_VCC:  return willem_set_vcc_voltage(val);
	case HW_SET_VPP:  return willem_set_vpp_voltage(val);
	case HW_SW_VCC:	  if(val == 0) return willem_vcc_off(); else return willem_vcc_on();
	case HW_SW_VPP:	  if(val == 0) return willem_vpp_off(); else return willem_vpp_on();

	case HW_SW_DPSW:  return willem_set_dip_sw(val);
	/* funkcje gniazda eprom */
	case HW_SET_DATA: return willem_set_par_data_pin(val);
	case HW_SET_ADDR: return willem_set_par_addr_pin(val);
	case HW_INC_ADDR: return willem_inc_addr();
	case HW_DEC_ADDR: return willem_dec_addr();
	case HW_RST_ADDR: return willem_rst_addr();
	case HW_GET_DATA: return willem_get_par_data_pin();
	case HW_SET_WE:   return willem_set_we_pin(val);
	case HW_SET_OE:   return willem_set_oe_pin(val);
	case HW_SET_CE:   return willem_set_ce_pin(val);
	/* Serial SPI jak 93Cxx, 25Cxx*/
	case HW_SET_CS:	  return willem_set_cs_pin(val);
	case HW_SET_CLK:  return willem_set_clk_pin(val);
	case HW_SET_DI:	  return willem_set_di_pin(val);
	case HW_GET_DO:	  return willem_get_do_pin();
	/* Serial I2C jak 24Cxx, PIC */
	case HW_SET_HOLD: return willem_set_test_pin(val);
	case HW_SET_SCL:  return willem_set_scl_pin(val);
	case HW_SET_SDA:  return willem_set_sda_pin(val);
	case HW_GET_SDA:  return willem_get_sda_pin();
	case HW_DELAY:	  timer_us_delay(val); break;
	case HW_LATENCY:  timer_latency(val, ___uid___); break;
	default:  	  return HW_ERROR;
    }
    return -2;
}


/**************************************************************************************************************/

int willem4_hardware_module(int funct, int val, void *ptr)
{
    switch(funct){
	/* ogólne */
	case HW_NAME:	  DRIVER_NAME(ptr) = "PCB 3";
	case HW_IFACE:	  return IFACE_LPT;
	case HW_GINIT:    return willem_gui_init(ptr);
	case HW_TEST:	  return willem_test_connection();
	case HW_RESET:    return willem_reset();
	case HW_OPEN:     return pcb3_open(ptr,val);
	case HW_CLOSE:    return willem_close();
	case HW_SET_VCC:  return willem_set_vcc_voltage(val);
	case HW_SET_VPP:  return willem_set_vpp_voltage(val);
	case HW_SW_VCC:	  if(val == 0) return willem_vcc_off(); else return willem_vcc_on();
	case HW_SW_VPP:	  if(val == 0) return willem_vpp_off(); else return willem_vpp_on();

	case HW_SW_DPSW:  return willem_set_dip_sw(val);
	/* funkcje gniazda eprom */
	case HW_SET_DATA: return willem_set_par_data_pin(val);
	case HW_SET_ADDR: return willem_set_par_addr_pin(val);
	case HW_INC_ADDR: return willem_inc_addr();
	case HW_DEC_ADDR: return willem_dec_addr();
	case HW_RST_ADDR: return willem_rst_addr();
	case HW_GET_DATA: return willem_get_par_data_pin();
	case HW_SET_WE:   return willem_set_we_pin(val);
	case HW_SET_OE:   return willem_set_oe_pin(val);
	case HW_SET_CE:   return willem_set_ce_pin(val);
	/* Serial SPI jak 93Cxx, 25Cxx*/
	case HW_SET_CS:	  return willem_set_cs_pin(val);
	case HW_SET_CLK:  return willem_set_clk_pin(val);
	case HW_SET_DI:	  return willem_set_di_pin(val);
	case HW_GET_DO:	  return willem_get_do_pin();
	/* Serial I2C jak 24Cxx, PIC */
	case HW_SET_HOLD: return willem_set_test_pin(val);
	case HW_SET_SCL:  return willem_set_scl_pin(val);
	case HW_SET_SDA:  return willem_set_sda_pin(val);
	case HW_GET_SDA:  return willem_get_sda_pin();
	case HW_DELAY:	  timer_us_delay(val); break;
	case HW_LATENCY:  timer_latency(val, ___uid___); break;
	default:  	  return HW_ERROR;
    }
    return -2;
}

/**************************************************************************************************************/

int willempro2_hardware_module(int funct, int val, void *ptr)
{
    switch(funct){
	/* ogólne */
	case HW_NAME:	  DRIVER_NAME(ptr) = "WILLEM PRO 2";
	case HW_IFACE:	  return IFACE_LPT;
	case HW_GINIT:    return willem_gui_init(ptr);
	case HW_TEST:	  return willem_test_connection();
	case HW_RESET:    return willem_reset();
	case HW_OPEN:     return pcb3_open(ptr,val);
	case HW_CLOSE:    return willem_close();
	case HW_SET_VCC:  return willem_set_vcc_voltage(val);
	case HW_SET_VPP:  return willem_set_vpp_voltage(val);
	case HW_SW_VCC:	  if(val == 0) return willem_vcc_off(); else return willem_vcc_on();
	case HW_SW_VPP:	  if(val == 0) return willem_vpp_off(); else return willem_vpp_on();

	case HW_SW_DPSW:  return willem_set_dip_sw(val);
	/* funkcje gniazda eprom */
	case HW_SET_DATA: return willem_set_par_data_pin(val);
	case HW_SET_ADDR: return willempro2_set_par_addr_pin(val);
	case HW_INC_ADDR: return willem_inc_addr();
	case HW_DEC_ADDR: return willem_dec_addr();
	case HW_RST_ADDR: return willem_rst_addr();
	case HW_GET_DATA: return willem_get_par_data_pin();
	case HW_SET_WE:   return willem_set_we_pin(val);
	case HW_SET_OE:   return willem_set_oe_pin(val);
	case HW_SET_CE:   return willem_set_ce_pin(val);
	/* Serial SPI jak 93Cxx, 25Cxx*/
	case HW_SET_CS:	  return willem_set_cs_pin(val);
	case HW_SET_CLK:  return willem_set_clk_pin(val);
	case HW_SET_DI:	  return willem_set_di_pin(val);
	case HW_GET_DO:	  return willem_get_do_pin();
	/* Serial I2C jak 24Cxx, PIC */
	case HW_SET_HOLD: return willem_set_test_pin(val);
	case HW_SET_SCL:  return willem_set_scl_pin(val);
	case HW_SET_SDA:  return willem_set_sda_pin(val);
	case HW_GET_SDA:  return willem_get_sda_pin();
	case HW_DELAY:	  timer_us_delay(val); break;
	case HW_LATENCY:  timer_latency(val, ___uid___); break;
	default:  	  return HW_ERROR;
    }
    return -2;
}


/*************************************************************************************************************************/
/* Rejestracja driverów */

plugin_register_begin

    register_api( willem3_hardware_module );
    register_api( willem4_hardware_module );
    register_api( willempro2_hardware_module );

plugin_register_end

