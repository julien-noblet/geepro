#ifndef __ICONS_XPM_H__
#define __ICONS_XPM_H__

extern char *logo_xpm[];

#define CONST_XPM(x)		(const char **)x

#define LOGO_ICON		CONST_XPM( logo_xpm )
#define READ_ACTION_ICON	CONST_XPM( read_xpm )
#define READ_EEPROM_ACTION_ICON	CONST_XPM( read_eeprom_xpm )
#define WRITE_ACTION_ICON	CONST_XPM( write_xpm )
#define WRITE_EEPROM_ACTION_ICON  CONST_XPM( write_eeprom_xpm )
#define VERIFY_ACTION_ICON	  CONST_XPM( verify_xpm )
#define VERIFY_EEPROM_ACTION_ICON CONST_XPM( verify_eeprom_xpm )
#define SIGN_ACTION_ICON	CONST_XPM( signature_xpm )
#define ERASE_ACTION_ICON	CONST_XPM( erase_xpm )	
#define LOCKBIT_ACTION_ICON	CONST_XPM( lockbit_xpm )
#define LOCKBREAK_ACTION_ICON	CONST_XPM( lockbreak_xpm )
#define TESTBLANK_ACTION_ICON	CONST_XPM( empty_xpm )

#endif
