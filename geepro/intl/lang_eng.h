/* $Revision: 0.1 $ */
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


#define TEXT(x)			TEXT_##x

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

#define CONFIG_TIP		"Configuration"
#define HELP_TIP		"Help"
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
#define MB_LOAD_FILE		"Open file"
#define MB_LOAD_FILE_AT		"Insert file"
#define MB_SAVE_FILE		"Write file"
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
#define MB_HELP			"Help"
#define MB_DOCUMENTATION	"Documentation"

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

#define TIP_BE_CLEAR_BUFFER	"Clear buffer"
#define TIP_BE_FIND_STRING	"Find string"
#define TIP_BE_JUMP_TO_MARKER	"Jump to marker"
#define TIP_BE_REDO_JUMP	"Redo jump" 
#define TIP_BE_PRINT		"Print"
#define TIP_BE_MANIPULATOR	"Bit manipulator"
#define TIP_BE_EXEC		"Execute script"
#define TIP_BE_stencil		"stencil"
#define TIP_BE_AUX		"Auxiliary buffer"
#define TIP_BE_BINED		"Binary bitmap editor"
#define TIP_BE_ORGANIZER	"Byte organizer"
#define TIP_BE_RESIZE		"Buffer resize"
#define TIP_BE_CHECKSUM		"Checksum calculator"
#define TIP_BE_ASMVIEW		"Assembler viewer"
#define TIP_BE_CUT		"Cut to clipboard"
#define TIP_BE_COPY		"Copy from clipboard"
#define TIP_BE_TEXTED		"Text editor"
#define TIP_BE_OPEN		"Load binary file"
#define TIP_BE_WRITE		"Save binary file"
#define TIP_BE_UNDO		"Undo changes"
#define TIP_BE_REDO		"Redo changes"

#define TXT_BE_FIND_AND_REPLACE "Find and replace"
#define TXT_BE_MANIPULATOR	"Bit manipulator"
#define TXT_BE_stencil		"stencil"
#define TXT_BE_EXECUTE		"Script execute"
#define TXT_BE_AUX		"Auxiliary buffer"
#define TXT_BE_BINED		"Binary bitmap editor"
#define TXT_BE_CLEAR		"Clear buffer"



#define TXT_BE_WHOLE_BUFFER	"Whole buffer"
#define TXT_BE_MARKED_AREA	"Highlighted area"

#define TXT_BE_ADDRESS_RANGE	"Address range"
#define TXT_BE_ADDRESS_FROM	"From:"
#define TXT_BE_ADDRESS_TO	"to:"
#define TXT_BE_PATTERN		"Pattern"
#define TXT_BE_FIND_ENTRY	"Find string"
#define TXT_BE_REPLACE_ENTRY	"Replace string"

#define TXT_BE_FIND_ST_LABEL	"Settings"
#define TXT_BE_FIND_ST_STRING	"Input string"
#define TXT_BE_FIND_ST_REGEXP	"Input regexp (sed)"
#define TXT_BE_FIND_ST_HEX	"Input hexadecimal"
#define TXT_BE_FIND_ST_BIN	"Input binary"
#define TXT_BE_FIND_ST_CI	"Case insensitive"
#define TXT_BE_FIND_ST_BEGIN	"Search from begining"
#define TXT_BE_FIND_ST_CURSOR	"Search from cursor"
#define TXT_BE_FIND_ST_MARKED	"Search marked area"

#define TXT_BE_TEXT_START	"Start address"
#define TXT_BE_TEXT_WIDTH	"Columns"
#define TXT_BE_TEXT_HEIGHT	"Rows"

#define TXT_ORGANIZER_EVEN	"Organizer function can be done only on even count data"
#define TXT_MANIPULATOR_DIVBYZERO	"Argument cannot be 0 for division"
#define TXT_BE_FIND_FIND_PATTERN	"Find syntax error. Enter string like this: 0x12, 123, \"foo\" 45"
#define TXT_BE_FIND_REPLACE_PATTERN	"Replace syntax error. Enter string like this: 0x12, 123, \"foo\" 45"

#define TXT_BE_BM_START_ADDRESS	"Start address:"
#define TXT_BE_BM_COUNT		"Count        :"
#define TXT_BE_BM_ARGUMENT	"Argument     :"

#define TXT_BE_BM_FUNCTIONS	"Functions"
#define TXT_BE_BM_ARITHM	"Arithmetic:"
#define TXT_BE_BM_LOGIC		"Logic:"
#define TXT_BE_BM_SHIFT		"Shift:"
#define TXT_BE_BM_ROTATE	"Rotate:"

#define TXT_BE_BM_SUB		"sub"
#define TXT_BE_BM_ADD		"add"
#define TXT_BE_BM_MUL		"mul"
#define TXT_BE_BM_DIV		"div"
#define TXT_BE_BM_OR		"or"
#define TXT_BE_BM_AND		"and"
#define TXT_BE_BM_XOR		"xor"
#define TXT_BE_BM_SHL		"sll"
#define TXT_BE_BM_SAL		"sal"
#define TXT_BE_BM_SHR		"slr"
#define TXT_BE_BM_SAR		"sar"
#define TXT_BE_BM_ROL		"rol"
#define TXT_BE_BM_ROR		"ror"
#define TXT_BE_BM_BX		"Bit copy"

#define TXT_BE_ORG_FUNCTIONS	"Byte operations"
#define TXT_BE_ORG_SPLIT	"Split odd/even"
#define TXT_BE_ORG_MERGE	"Merge odd/even"
#define TXT_BE_ORG_XCHG		"Exchange odd/even"
#define TXT_BE_ORG_REORG	"Byte reorganization"
#define TXT_BE_ORG_WHOLE	"Whole area"
#define TXT_BE_ORG_REORG_ADDRS	"Address bits"
#define TXT_BE_BMP_WIDTH	"Width pixels"
#define TXT_BE_BMP_HEIGHT	"Height pixels"
#define TXT_BE_BMP_REV		"Bits reverse order"
#define TXT_BE_BMP_BS		"Bit selection"

#define TXT_BE_CUT_START	"Start address:"
#define TXT_BE_CUT_COUNT	"Bytes count:"
#define TXT_BE_CUT_STOP		"Stop address:"
#define TXT_BE_COPY_ADDRESS	"Insert at:"
#define TXT_BE_ASM_FSEL		"Core select"
#define TXT_BE_ASM_COUNT	"Instruction count: "
#define TXT_BE_ASM_CORE		"*.brain"
#define TXT_BE_AUX_SIZE		"Buffer size"
#define TXT_BE_WINTIT_AUX	"Auxiliary buffer"
#define TXT_BE_SUM_ALGO_LRC	"Algorithm LRC   "
#define TXT_BE_SUM_ALGO_CRC16	"Algorithm CRC-16"
#define TXT_BE_SUM_ALGO_CRC32	"Algorithm CRC-32"
#define TXT_BE_SUM_RUN		"Compute"
#define TXT_BE_RESIZE_RES	"New size"
#define TXT_BE_TITWIN_SELECT_CORE "Select core definition"
#define TXT_BE_STC_FE		"*.stc"
#define TXT_BE_STC_WINTIT	"Select stencil"
#define TXT_BE_OPEN_WINTIT	"Open binary file"
#define TXT_BE_SAVE_WINTIT	"Write binary file"
#define TXT_BE_OPEN_FOFFS	"File byte ofset"
#define TXT_BE_OPEN_START	"Insertion address"
#define TXT_BE_OPEN_COUNT	"Bytes count"
#define TXT_BE_SAVE_START	"Insertion address"
#define TXT_BE_SAVE_COUNT	"Bytes count"
#define TXT_BE_FIND_NO_MATCH	"There is no more match in selected area."
#define TXT_BE_FIND_MATCH	"Found string. What to do ?"
#define TXT_BE_FIND_ALL_BT	"Replace all"
#define TXT_BE_FIND_REPL_BT	"Replace"
#define TXT_BE_FIND_ERR_FIND	"Find string syntax error."
#define TXT_BE_FIND_ERR_REPL	"Replace string syntax error."
#define TXT_BE_FIND_ERR_NEQ	"Find string and replace have different length."
#define TXT_BE_GRID_LBL_ADDRESS "Address"
#define TXT_BE_GRID_LBL_ASCII   "ASCII"


#define TEXT_BE_WIN_TIT_CLEAR			"Clear buffer"
#define TEXT_BE_WIN_TIT_FIND_AND_REPLACE	"Find and replace"
#define TEXT_BE_WIN_TIT_MANIPULATOR		"Bit manipulator"
#define TEXT_BE_WIN_TIT_BMPEDIT			"Bitmap editor"
#define TEXT_BE_WIN_TIT_CHECKSUM		"Checksum calculator"
#define TEXT_BE_WIN_TIT_ORGANIZER		"Byte organizer"
#define TEXT_BE_WIN_TIT_TEXT			"Text editor"
#define TEXT_BE_WIN_TIT_AUX			"Create auxiliary buffer"
#define TEXT_BE_WIN_TIT_SUM			"Checksum calculator"
#define TEXT_BE_WIN_TIT_RESIZE			"Resize buffer"
#define TEXT_BE_WIN_TIT_OPEN			"Open file"
#define TEXT_BE_WIN_TIT_SAVE			"Save to file"

#define TEXT_BE_WIN_TIT_CUT		"Cut"
#define TEXT_BE_WIN_TIT_COPY		"Copy"
#define TEXT_BE_WIN_TIT_ASMVIEWER	"Assembler viewer"

#define DLG_INS_FILE_BUFFER_OFFSET	"Insert to buffer at:"
#define DLG_INS_FILE_SIZE		"Bytes count:"
#define DLG_INS_FILE_OFFSET		"File offset:"

#define TXT_BE_BMP_TB_SHRINK	"Zoom out"
#define TXT_BE_BMP_TB_EXPAND	"Zoom in"
#define TXT_BE_BMP_TB_AUTO	"Zoom fit"
