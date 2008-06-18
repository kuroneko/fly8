/* --------------------------------- dosstick.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Low level joystick reading.
*/

#include "fly.h"

#if HAVE_JOYSTICK

#include "stick.h"


#if SYS_DJGPP
#include <pc.h>
#define inp(p) 		inportb(p)
#define outp(p,b)	outportb(p,b)
#else
#include <dos.h>
#include <conio.h>
#endif


/* First (lowest) level PC joystick reading. Acquires hardware dependent
 * data as is.
*/

#define GAME_PORT	0x0201
#define GAME_TIMEOUT	0xffffU		/* counter!!! */
#define GAME_START	(outp (GAME_PORT, 0), iefbr14 ())
#define GAME_READ	inp (GAME_PORT)
#define GAME_READING	(Ushort)(mode ? Tm->Hires () : GAME_TIMEOUT-i)

/* Bit mapping for the PC game port status byte.
*/
#define GAME_X1		0x001
#define GAME_Y1		0x002
#define GAME_X2		0x004
#define GAME_Y2		0x008
#define GAME_B1		0x010
#define GAME_B2		0x020
#define GAME_B3		0x040
#define GAME_B4		0x080

extern int FAR
initstick (int which, char *options, int opts)
{return (0);}

extern int FAR
termstick (int which, int opts)
{return (0);}

extern Uint FAR
readstick (int which, STICK *s, int mask, int opts)
{
	register Ushort	i;
	register Uint	m;
	Ushort		t, tt, x1, y1, x2, y2;
	int		js, mode;

	mode = opts & USETIMER;

	i = GAME_TIMEOUT;
	t = GAME_READING;
	x1 = y1 = x2 = y2 = t;

	m = 0;
	if (which) {
		if (mask & JS_A0)
			m |= GAME_X2;
		if (mask & JS_A1)
			m |= GAME_Y2;
	} else {
		if (mask & JS_A0)
			m |= GAME_X1;
		if (mask & JS_A1)
			m |= GAME_Y1;
		if (mask & JS_A2)
			m |= GAME_X2;
		if (mask & JS_A3)
			m |= GAME_Y2;
	}
	GAME_START;			/* set trigger */
	while (m) {
		while (!(~GAME_READ & m) && --i)
			;
		if (!i)
			break;
		tt = GAME_READING;
		js = ~GAME_READ & m;
		if (js & 0x01) {
			x1 = tt;
			m &= ~GAME_X1;
		}
		if (js & 0x02) {
			y1 = tt;
			m &= ~GAME_Y1;
		}
		if (js & 0x04) {
			x2 = tt;
			m &= ~GAME_X2;
		}
		if (js & 0x08) {
			y2 = tt;
			m &= ~GAME_Y2;
		}
	}

/* Set analog values.
*/
	if (which) {
		s->a[0] = (Ushort)(x2 - t);
		s->a[1] = (Ushort)(y2 - t);
	} else {
		s->a[0] = (Ushort)(x1 - t);
		s->a[1] = (Ushort)(y1 - t);
		s->a[2] = (Ushort)(x2 - t);
		s->a[3] = (Ushort)(y2 - t);
	}

/* Set button values.
*/
	js = ~GAME_READ;
	if (which) {
		s->b[0] = (char)T(js & GAME_B3);
		s->b[1] = (char)T(js & GAME_B4);
		s->nbuttons = 2;
	} else {
		s->b[0] = (char)T(js & GAME_B1);
		s->b[1] = (char)T(js & GAME_B2);
		s->b[2] = (char)T(js & GAME_B3);
		s->b[3] = (char)T(js & GAME_B4);
		s->nbuttons = 4;
	}

/* Indicate analog failure.
*/
	js = 0;
	if (which) {
		if (m & GAME_X2)
			js |= JS_A0;
		if (m & GAME_Y2)
			js |= JS_A1;
	} else {
		if (m & GAME_X1)
			js |= JS_A0;
		if (m & GAME_Y1)
			js |= JS_A1;
		if (m & GAME_X2)
			js |= JS_A2;
		if (m & GAME_Y2)
			js |= JS_A3;
	}

	return (js);
}
#undef GAME_PORT
#undef GAME_TIMEOUT
#undef GAME_START
#undef GAME_READ
#undef GAME_READING
#undef GAME_X1
#undef GAME_Y1
#undef GAME_X2
#undef GAME_Y2
#undef GAME_B1
#undef GAME_B2
#undef GAME_B3
#undef GAME_B4

#endif /* if HAVE_JOYSTICK */
