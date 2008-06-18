/* --------------------------------- keyname.h ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* key names, used by mac2max and max2mac.
*/

#ifndef FLY8_KEYNAME_H
#define FLY8_KEYNAME_H

#include "keymap.h"

#define	VK_DEF	0x00010000L
#define	VK_EOF	0x00020000L
#define	VK_COM	0x00040000L
#define	VK_STR	0x00080000L

struct names {
	char	*name;
	long	value;
};

static struct names f_name[] = {		/* function names */
	{"Menu",		KF_MENU},

	{"MacroRecord",		KF_MACRECORD},
	{"MacroPlay",		KF_MACPLAY},

	{"PitchUp",		KF_UP},
	{"PitchDown",		KF_DOWN},
	{"RollLeft",		KF_LEFT_TURN},
	{"RollRight",		KF_RIGHT_TURN},
	{"RollReset",		KF_RESET_ROLL},
	{"RollLevel",		KF_LEVEL},
	{"RudderLeft",		KF_RUDDER_LEFT},
	{"RudderIdle",		KF_RUDDER_CENTER},
	{"RuddderRight",	KF_RUDDER_RIGHT},
	{"Stable",		KF_STABLE},
	{"Origin",		KF_ORIGIN},

	{"TrimUp",		KF_TRIM_UP},
	{"TrimDown",		KF_TRIM_DOWN},
	{"TrimLeft",		KF_TRIM_LEFT},
	{"TrimRight",		KF_TRIM_RIGHT},
	{"TrimIdle",		KF_TRIM_IDLE},

	{"PowerUp",		KF_POWER_UP},
	{"PowerDown",		KF_POWER_DOWN},
	{"Power0",		KF_POWER_0},
	{"Power100",		KF_POWER_100},
	{"PowerAB",		KF_POWER_AB},

	{"FlapsMore",		KF_FLAPS_MORE},
	{"FlapsLess",		KF_FLAPS_LESS},
	{"SpoilersMore",	KF_SPOILERS_MORE},
	{"SpoilersLess",	KF_SPOILERS_LESS},
	{"BrakesMore",		KF_BRAKES_MORE},
	{"BrakesLess",		KF_BRAKES_LESS},
	{"Brakes",		KF_BRAKES},
	{"SpeedBrakesMore",	KF_SPEED_BRAKES_MORE},
	{"SpeedBrakesLess",	KF_SPEED_BRAKES_LESS},
	{"SpeedBrakes",		KF_SPEED_BRAKES},
	{"Gear",		KF_GEAR},
	{"RadarSelect",		KF_RADAR_SELECT},

	{"Fire",		KF_FIRE},

	{"ZoomIn",		KF_ZOOMIN},
	{"ZoomOut",		KF_ZOOMOUT},
	{"VZoomIn",		KF_VZOOMIN},
	{"VZoomOut",		KF_VZOOMOUT},

	{"Xup",			KF_XUP},
	{"Xdown",		KF_XDOWN},
	{"Xleft",		KF_XLEFT},
	{"Xright",		KF_XRIGHT},

	{"Yup",			KF_YUP},
	{"Ydown",		KF_YDOWN},
	{"Yleft",		KF_YLEFT},
	{"Yright",		KF_YRIGHT},

{0,0}};

static struct names k_name[] = {		/* key names */
	{"",			0},
	{"F1",			K_F1},
	{"F2",			K_F2},
	{"F3",			K_F3},
	{"F4",			K_F4},
	{"F5",			K_F5},
	{"F6",			K_F6},
	{"F7",			K_F7},
	{"F8",			K_F8},
	{"F9",			K_F9},
	{"F10",			K_F10},
	{"F11",			K_F11},
	{"F12",			K_F12},
	{"Left",		K_LEFT},
	{"Right",		K_RIGHT},
	{"Up",			K_UP},
	{"Dn",			K_DOWN},
	{"Down",		K_DOWN},
	{"PgUp",		K_PGUP},
	{"PageUp",		K_PGUP},
	{"PgDn",		K_PGDN},
	{"PageDown",		K_PGDN},
	{"Home",		K_HOME},
	{"End",			K_END},
	{"Ins",			K_INS},
	{"Insert",		K_INS},
	{"Ctr",			K_CENTER},
	{"Center",		K_CENTER},

	{"Del",			K_DEL},
	{"Delete",		K_DEL},
	{"Sp",			' '},
	{"Space",		' '},
	{"Bell",		K_BELL},
	{"Bs",			K_RUBOUT},
	{"Ro",			K_RUBOUT},
	{"Rubout",		K_RUBOUT},
	{"Esc",			K_ESC},
	{"Escape",		K_ESC},
	{"Ff",			K_FF},
	{"Cr",			K_ENTER},
	{"Enter",		K_ENTER},
	{"Ent",			K_ENTER},
	{"Ret",			K_ENTER},
	{"Nl",			K_NL},
	{"Tab",			K_TAB},
	{"Vt",			K_VTAB},

	{"Brl",			K_BTN|K_RLS},

	{"Spc",			K_SPECIAL},
	{"Special",		K_SPECIAL},
	{"Sh",			K_SHIFT},
	{"Shift",		K_SHIFT},
	{"Ctrl",		K_CTRL},
	{"Alt",			K_ALT},
	{"Rls",			K_RLS},
	{"Btn",			K_BTN},
	{"Qte",			K_QUOTE},
	{"Quote",		K_QUOTE},
	{"Def",			VK_DEF},
{0,0}};

#endif
