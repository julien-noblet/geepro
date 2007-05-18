/* $Revision: 1.1.1.1 $ */
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

#ifndef _gepro_h
#define _gepro_h	1

#define EPROGRAM_NAME "Geepro"
#define ERELDATE "1 Feb 2020"
#define EVERSION "0.0.2 pre"
#define EAUTHORS "Krzysztof Komarnicki krzkomar@wp.pl"
#define ELICENSE "GPL version 2"

typedef struct{
    char 	willem_pcb;
    int		chip_erase_id;
    int  	dip_switch;
    int		port_addr;
    void  	*image_pcb3;
    void  	*image_willem;    
    void  	*image;    
    char	*dev_name;	
    char	*fname;
    int 	dev_size;    /* rozmiar pamiêci do zaprogramowania */
    int		dev_checksum;
    int		eep;
    int		epr64;
    char	*buffer;
    int		view_shift; /* buffer shift */
    char	*status_bar;
    int		chip_family;
    int		chip_subid;
    char	start; /* start aplikacji */
} conf_state;

extern void *p_wgts[];


/* chip family */
#define C24XX_FAM	0
#define C25XX_FAM	1
#define C27XX_FAM	2
#define C28XX_FAM	3
#define F28XX_FAM	4
#define F29XX_FAM	5
#define MCS51_FAM	6
#define PIC_FAM		7
#define C93_FAM		8


/* chip */
#define A46_ID		0
#define B46_ID		1
#define C46_ID		2
#define B56_ID		3
#define C56_ID		4
#define C66_ID  	5


#define ERASE_F28	0
#define ERASE_F29	1
#define ERASE_PIC	2
#define ERASE_93	3

extern conf_state program_state;


#define PROGRAM_STATE		program_state
#define SET_FNAME(value)	program_state.fname=value
#define GET_FNAME		program_state.fname

#define SET_CHIP_SUBID(value)	program_state.chip_subid=value
#define GET_CHIP_SUBID		program_state.chip_subid

#define SET_CHIP_FAM(value)	program_state.chip_family=value
#define GET_CHIP_FAM		program_state.chip_family


#define SET_STATUS(value)	program_state.status_bar=value
#define GET_STATUS		program_state.status_bar
#define SET_BF_SH(value)	program_state.view_shift=value
#define GET_BF_SH		program_state.view_shift
#define SET_BUFFER(value)	program_state.buffer=value
#define GET_BUFFER		program_state.buffer
#define BUFFER(adr)		(*(GET_BUFFER+adr))
#define SET_ERASE(value)	program_state.chip_erase_id=value
#define GET_ERASE		program_state.chip_erase_id
#define SET_CHECKSUM(value)	program_state.dev_checksum=value
#define GET_CHECKSUM		program_state.dev_checksum
#define SET_EPR64(value)	program_state.epr64=value
#define GET_EPR64		program_state.epr64
#define SET_DEV_NAME(value)	program_state.dev_name=value
#define GET_DEV_NAME		program_state.dev_name
#define SET_EEP_FLAG(value)	program_state.eep=value
#define GET_EEP_FLAG		program_state.eep
#define SET_DEV_SIZE(value)	program_state.dev_size=value
#define GET_DEV_SIZE		program_state.dev_size
#define SET_PCB_WILLEM(bool)	program_state.willem_pcb = bool
#define GET_PCB_WILLEM		program_state.willem_pcb
#define SET_PORT_ADDR(value)	program_state.port_addr=value
#define GET_PORT_ADDR		program_state.port_addr


#define LP0_ADDR	0x378
#define LP1_ADDR	0x278
#define LP2_ADDR	0x3B8

#define SW_FONT_DSC	"Luxi Mono 8"
#define SW_DESCRIPTION	"29x","","1","2","3","4","5","6","7","8","9","A","B","C"
#define M_DEBUG(msg) 	g_print(msg)
#define T_NULL(tst)	tst ? "->Not NULL\n":"->Is NULL\n"
#define DIP_SWITCH_SIZE	14 /* dip switch + 2 */

//#define SET_BT(name,state)  gtk_toggle_button_set_state(p_wgts[name],state)
//#define STORE_WG(name, widget) p_wgts[name]=(void *)widget
//#define GET_WG(name)  GTK_WIDGET(p_wgts[name])
//#define GET_FONT(name)  (GdkFont *)p_wgts[name]

//#define STORE_PIX(name, widget) p_wgts[name]=(void *)widget
//#define STORE_FONT(name, widget) p_wgts[name]=(void *)widget
//#define STORE_ADJ(name, adj) p_wgts[name]=(void *)adj

#define GET_PIX(name)  GDK_DRAWABLE(p_wgts[name])
#define GET_ADJ(name)  GTK_ADJUSTMENT(p_wgts[name])

#define SW_WIDTH	10
#define SW_HEIGHT	24
#define FLAG_29XX_SW	0x1000


#define CH_BT_PCB		1
#define RD_BUTTON_LP0		2
#define RD_BUTTON_LP1		3
#define RD_BUTTON_LP2		4
#define EN_SIZE_HEX		5
#define EN_CHECKSUM		6
#define EN_SHIFT		7
#define LB_STATUS_VAL		8
#define CL_BUFFER		9
#define TB_P_OPEN_FILE		10
#define TB_P_SAVE_FILE		11
#define TB_P_CLEAR_BUFFER	12
#define TB_P_READ_CHIP		13
#define TB_P_VERIFY_CHIP	14
#define TB_P_ID_CHIP		15
#define TB_P_LB_CHIP		16
#define TB_P_BLANK_CHIP		17
#define TB_P_PROGRAM_CHIP	18
#define TB_P_ERASE_CHIP		19
#define PIX_BUF_SW		20
#define PIX_BUF_VW		21
#define WG_SW_ON		22
#define WG_SW_OFF		23
#define WG_SW_DA		24
#define WG_VW_DA		25
#define WG_WMAIN		26
#define FONT_SW			27
#define EN_DEV_NAME		28
#define TB_CONTAINER		29

#define TB_RM_OPEN_FILE		30
#define TB_RM_SAVE_FILE		31
#define TB_RM_CLEAR_BUFFER	32
#define TB_RM_READ_CHIP		33
#define TB_RM_VERIFY_CHIP	34
#define TB_RM_ID_CHIP		35
#define TB_RM_LB_CHIP		36
#define TB_RM_BLANK_CHIP	37
#define TB_RM_PROGRAM_CHIP	38
#define TB_RM_ERASE_CHIP	39

#define TB_RM_ULB_CHIP		40
#define TB_RM_SPACE		41
#define FAST_OPTION_MENU	42
#define MB_P_DEVICE		43
#define MAIN_WINDOW		44
#define ADV_OPTION		45
#define NB_MAIN			46
#define NB_TEST_HW		47
#define SWITCH_TEST_HW		48
#define SWITCH_TEST_HW_BX	49
#define MPROG_BAR		54
#define MPROG_BAR_W		55
#define TB_P_ULB_CHIP		56
#define NB_BUFFER		57

#define TB_READ_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->read_chip), plg
#define TB_VERIFY_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->verify_chip), plg
#define TB_ID_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->read_sig_chip), plg
#define TB_LB_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->lock_chip), plg
#define TB_ULB_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->unlock_chip), plg
#define TB_BLANK_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->test_chip), plg
#define TB_PROGRAM_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->write_chip), plg
#define TB_ERASE_CHIP		GTK_SIGNAL_FUNC(chip_get_chip(plg)->erase_chip), plg

#define OPEN_FILE_XPM		open_file_xpm
#define SAVE_FILE_XPM		save_file_xpm
#define CLEAR_BUFFER_XPM	clear_buffer_xpm
#define READ_CHIP_XPM		read_chip_xpm
#define VERIFY_CHIP_XPM		verify_chip_xpm
#define ID_CHIP_XPM		id_chip_xpm
#define LB_CHIP_XPM		lb_xpm
#define ULB_CHIP_XPM		lb_xpm /* tymczasowo */
#define BLANK_CHIP_XPM		blank_chip_xpm
#define PROGRAM_CHIP_XPM	program_chip_xpm
#define ERASE_CHIP_XPM		erase_chip_xpm
#define SW_ON_XPM		sw_on_xpm
#define SW_OFF_XPM		sw_off_xpm

#define SW_TEST_WILLEM_XPM	sw_test_willem_xpm
#define SW_TEST_PCB3_XPM	sw_test_pcb3_xpm

#define ADAPTER_51_XPM		adapter_51_xpm
#define D_2716_XPM		d_2716_xpm
#define D_2732_XPM		d_2732_xpm
#define D_2764_XPM		d_2764_xpm
#define D_27010_XPM		d_27010_xpm
#define D_24CXX_PCB3_XPM	d_24Cxx_pcb3_xpm
#define D_24CXX_WILLEM_XPM	d_24Cxx_willem_xpm
#define D_25CXX_PCB3_XPM	d_25Cxx_pcb3_xpm
#define D_25CXX_WILLEM_XPM	d_25Cxx_willem_xpm
#define D_93XX_PCB3_XPM		d_93xx_pcb3_xpm
#define D_93XX_WILLEM_XPM	d_93xx_willem_xpm
#define PIC_18_PCB3_XPM		pic_18_pcb3_xpm
#define PIC_18_WILLEM_XPM	pic_18_willem_xpm


/***************************************************************/

#define ERROR_BRK	{ error(); break; }
#define PIC_PRG_MASK	0x3fff
#define PIC_DAT_MASK	0xff
#define PIC_DAT_OFFS	0x2100
#define PIC_PRG_SIZE	510


#define DESTROY_WG(w)	gtk_widget_destroy(GTK_WIDGET(w));
#define MSG_BOX_CANCEL_BUTTON_PRESSED	(msg_cancel_button && 1)
#define warn_beep	;

#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef _SYS_TYPES_H
typedef unsigned int uint;
#endif



#define P_UCHAR(n) (uchar *)n

#endif
