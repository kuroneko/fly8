/* --------------------------------- stick.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common joystick definitions.
*/

#ifndef FLY8_STICK_H
#define FLY8_STICK_H

#define HAT		0x0001		/* only on A */
#define THROTTLE	0x0002		/* only on A */
#define RUDDER		0x0004		/* only on A */
#define FOURBUTTONS	0x0008		/* only on A: 4 buttons */
#define ZEROBUTTONS	0x0010		/* only on A: no buttons */
#define CHPRO		0x0020		/* CH Pro */
#define NBUTTONS	0x0040		/* only on A: */
#define READA2		0x0080		/* only on A */
#define READA3		0x0100		/* only on A */
#define READA4		0x0200		/* only on A */
#define READA5		0x0400		/* only on A */
#define USETIMER	0x0800		/* msdos/mswin: read h'ware timer */
#define USEGAME		0x1000		/* Linux: use <game.h> */
#define USEGP		0x2000		/* unix: use GP */
#define CALIBRATED	0x4000

#define JS_A0		0x01		/* readstick() request mask */
#define JS_A1		0x02
#define JS_A2		0x04
#define JS_A3		0x08
#define JS_A4		0x10
#define JS_A5		0x20

struct stick {
	Ushort		a[6];
	int		nbuttons;
	char		b[NBTNS];
};
typedef struct stick	STICK;

extern int	FAR initstick (int which, char *options, int opts);
extern int	FAR termstick (int which, int opts);
extern Uint	FAR readstick (int which, STICK *s, int mask, int opts);

#endif
