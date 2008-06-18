/* --------------------------------- keymap.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Definition of mapping keys to logical functions.
*/

#ifndef FLY8_KEYMAP_H
#define FLY8_KEYMAP_H

#include "keydef.h"

#define	KF_MENU			K_ESC		/* start menu system */
#define	KF_ESC			K_ESC		/* exit menu system */
#define	KF_INIT			'a' | K_CTRL	/* CTRL-a startup sequence */

#define	KF_UP			'2'
#define	KF_DOWN			'8'
#define	KF_LEFT_TURN		'4'
#define	KF_RIGHT_TURN		'6'
#define	KF_STABLE		'5'
#define	KF_RESET_ROLL		'7'
#define	KF_LEVEL		K_PGUP
#define	KF_ORIGIN		K_PGDN
#define	KF_POWER_UP		'9'
#define	KF_POWER_DOWN		'3'
#define	KF_POWER_0		'0'
#define	KF_POWER_100		'1'
#define	KF_POWER_AB		'.'
#define	KF_FIRE			K_F1
#define	KF_RUDDER_LEFT		K_F2
#define	KF_RUDDER_CENTER	K_F3
#define	KF_RUDDER_RIGHT		K_F4
#define	KF_ZOOMIN		K_F5
#define	KF_ZOOMOUT		K_F6
#define	KF_MACRECORD		K_F7
#define	KF_MACPLAY		K_F8
#define KF_VZOOMIN		K_F9
#define KF_VZOOMOUT		K_F10

#define KF_FLAPS_MORE		']'
#define KF_FLAPS_LESS		'['
#define KF_SPOILERS_MORE	'}'
#define KF_SPOILERS_LESS	'{'
#define KF_BRAKES_MORE		')'
#define KF_BRAKES_LESS		'('
#define KF_BRAKES		'b'
#define KF_SPEED_BRAKES_MORE	'>'
#define KF_SPEED_BRAKES_LESS	'<'
#define KF_SPEED_BRAKES		'+'
#define KF_GEAR			'g'
#define KF_RADAR_SELECT		' '
#define	KF_TRIM_UP		(K_DOWN		| K_CTRL)
#define	KF_TRIM_DOWN		(K_UP		| K_CTRL)
#define	KF_TRIM_LEFT		(K_LEFT		| K_CTRL)
#define	KF_TRIM_RIGHT		(K_RIGHT	| K_CTRL)
#define KF_TRIM_IDLE		'\\'

#define	KF_XUP			K_UP
#define	KF_XDOWN		K_DOWN
#define	KF_XLEFT		K_LEFT
#define	KF_XRIGHT		K_RIGHT

#define	KF_YUP			(K_UP		| K_ALT)
#define	KF_YDOWN		(K_DOWN		| K_ALT)
#define	KF_YLEFT		(K_LEFT		| K_ALT)
#define	KF_YRIGHT		(K_RIGHT	| K_ALT)


#endif
