/* $Revision: 1.1 $ */
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

#define ABOUT EPROGRAM_NAME" - integrated circuit programmer\n"\
	    "ver: " EVERSION "\n"\
	    "release date: " ERELDATE \
	    "\nAuthors: \n "\
	    EAUTHORS "\n"\
	    "License:\n"\
	    ELICENSE

#define MEMORY_ALLOCATION_ERROR_CHIP_QUEUE "Memory allocation error in function register_chip()\n"

#define STS_CHIP_DUMP		"Memory chip read complete"
#define TXT_READING_CHIP_PB	"Memory chip reading"
#define ALLOCATION_ERROR	"memory allocation error\n"
#define STS_EMPTY		"Buffer is empty"
#define DEVICE_ENTRY_LB		"Chip name: "
#define INIT_PORT_ERROR		"Initialization port I/O error\n"
#define STATUS_BAR_START_TXT	"Status"
#define LAB_NOTE_1		"Chip"
#define LAB_NOTE_1		"Chip"
#define LAB_NB_BUFFER		"Buffer"
#define LAB_NOTE_3		"Device test"
#define FR_NB_01_TITLE		NULL
#define FR_NB_02_TITLE		NULL
#define FR_NB_03_TITLE		"Algorythm"
#define FR_NB_04_TITLE		"Programmer"
#define SIZE_HEX_LB		"Memory size (hex)"
#define CHECKSUM_LB		"Control sum (LRC)"
#define SHIFT_LB		"Address offset"
#define ALGO_NORMAL_LB		"Normal"
#define ALGO_FAST_LB		"Fast"
#define ALGO_EXPRESS_LB		"Express"
#define SKIP_FF_LB		"Skip 0xFF bytes"

#define PCB_WILLEM_LB		"PCB3 / Willem"
#define LP0_LB			"/dev/lp0"
#define LP1_LB			"/dev/lp1"
#define LP2_LB			"/dev/lp2"
#define FILE_LB			"Current file"
#define TEST_BUTTON_LB		"Connecting test"

#define OPEN_FILE_TIP		"Open file"
#define SAVE_FILE_TIP		"Write to file"
#define CLEAR_BUFFER_TIP	"Clear buffer"
#define READ_CHIP_TIP		"Read chips memory"
#define VERIFY_CHIP_TIP		"Compare with buffer"
#define ID_CHIP_TIP		"Read signature"
#define LB_CHIP_TIP		"Fuse lock bits"
#define ULB_CHIP_TIP		"Break lock bits"
#define BLANK_CHIP_TIP		"Test blank"
#define PROGRAM_CHIP_TIP	"Write chip memory"
#define ERASE_CHIP_TIP    	"Erase chip memory"
#define ADD_PATH_QE_ERR		"Cannot locate pointer to main menu\n"
#define ADD_CHIP_WARN_BUG	"Warning: Cannot find path for chip %s; chip is deleted from list (bug?)\n"
#define ADD_CHIP_MSG		"Message: Chip added %s\n"
#define ADD_MODULE_ERROR	"Error module insertion\n"
#define LD_MODULE_ERROR_LD	"Error: %s\n"
#define LD_MODULE_ERROR_SYM	"Error: %s\n"
#define CH_DIR_ERROR		"Change directory error\n"
#define BAD_CHIP_NAME		"Chip name or its path error\n"
#define MALLOC_ERROR		"Memory allocation error\n"
#define LB_FR_FAST_OPTION	"Fast option"
#define NO_FO_MSG		"Option missing"
#define FO_MAX_ITEM_MSG		"Reached max quantity elements of fast option\n"
#define TST_LB_ADDRESS		"Address:"
#define TST_LB_DATA		"Data:"
#define TST_LB_RD_DATA		"Read-out D0..D7"
#define TST_LB_ZIF		"ZIF-32"
#define TST_LB_SET_AD		"Set Address/Data "
#define TST_LB_SER_FR		"Memory 93Cxx"
#define TST_LB_CS		"Line CS Pin 1"
#define TST_LB_CLK		"Line CLOCK Pin 2"
#define TST_LB_DI		"Line DI Pin 3"
#define TST_LB_SER_RD		"read-out DO Pin 4"
#define TST_LB_DIP		"DIP SW"
#define PROPERTIES_TIP		"Program settings"


#define MB_FILE			"File"
#define MB_LOAD_BIN_FILE	"Open BIN"
#define MB_SAVE_BIN_FILE	"Write BIN"
#define MB_LOAD_HEX_FILE	"Open HEX"
#define MB_SAVE_HEX_FILE	"Write HEX"
#define MB_ABOUT_FILE		"About program"
#define MB_EXIT_FILE		"Exit"
#define MB_ACTION		"Action"
#define MB_CLR_BUFF_ACTION	"Clear buffer"
#define MB_ERASE_ACTION		"Chip erase"
#define MB_VERIFY_ACTION	"Verification"
#define MB_BL_TST_ACTION	"Is chip blank ?"
#define MB_WRITE_ACTION		"Chip write"
#define MB_READ_ACTION		"Read-out chip"
#define MB_OPTIONS		"Options"
#define MB_DEVICE		"Chip"

#define ABOUT_TITLE		"About program"
#define BUTTON_OK		"OK"
#define BUTTON_NO		"Cancel"
#define NO_CHIP_PLUGIN		"Missing plugin for chip"
#define QUIT_TITLE		"Message"
#define QUIT_MSG		"Are you realy want to quit?"

#define FSEL_LOAD		"Read file"
#define FSEL_SAVE		"Write file"
#define MISSING_PROG_PLUGIN	"Missing programmer plugin\n"
#define CHIP_DESCRIPTION	"Chip description"
#define TXT_MISSING		"missing"
#define TXT_PROGRAMMER		"Programmer:"
#define TXT_INTERFACE		"Device:"
#define TXT_BUFFER		"Buffer"
#define TXT_EXIT		"bye bye.\n"

#define RELOAD_QUESTION		"Buffer file and disk file are differ.\nReload ?"
