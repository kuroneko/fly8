/* --------------------------------- keydef.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* These are the internal names of the keys.
*/

#ifndef FLY8_KEYDEF_H
#define FLY8_KEYDEF_H

/* shift modes:
*/
#define	K_SHIFTS	0xff00
#define	K_MODES		0x0f00
#define	K_RAW		0x00ff

#define	K_SPECIAL	0x0100
#define	K_SHIFT		0x0200
#define	K_CTRL		0x0400
#define	K_ALT		0x0800
#define	K_RLS		0x2000
#define	K_BTN		0x4000
#define	K_QUOTE		0x8000

/* some cotrol keys:
*/
#define K_AESC		0x01b

/* some ASCII codes:
*/
#define K_ESC		('['| K_CTRL)
#define K_BELL		('g'| K_CTRL)
#define K_RUBOUT	('h'| K_CTRL)
#define K_TAB		('i'| K_CTRL)
#define K_NL		('j'| K_CTRL)
#define K_VTAB		('k'| K_CTRL)
#define K_FF		('l'| K_CTRL)
#define K_ENTER		('m'| K_CTRL)
#define K_DEL		0x07f

/* special keys:
*/
#define K_F1		(1  | K_SPECIAL)
#define K_F2		(2  | K_SPECIAL)
#define K_F3		(3  | K_SPECIAL)
#define K_F4		(4  | K_SPECIAL)
#define K_F5		(5  | K_SPECIAL)
#define K_F6		(6  | K_SPECIAL)
#define K_F7		(7  | K_SPECIAL)
#define K_F8		(8  | K_SPECIAL)
#define K_F9		(9  | K_SPECIAL)
#define K_F10		(10 | K_SPECIAL)
#define K_F11		(11 | K_SPECIAL)
#define K_F12		(12 | K_SPECIAL)

#define K_LEFT		(13 | K_SPECIAL)
#define K_RIGHT		(14 | K_SPECIAL)
#define K_UP		(15 | K_SPECIAL)
#define K_DOWN		(16 | K_SPECIAL)
#define K_PGUP		(17 | K_SPECIAL)
#define K_PGDN		(18 | K_SPECIAL)
#define K_HOME		(19 | K_SPECIAL)
#define K_END		(20 | K_SPECIAL)

#define K_INS		(21 | K_SPECIAL)

#define K_CENTER	(22 | K_SPECIAL)

#endif
