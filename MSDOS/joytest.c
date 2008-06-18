/* ------------------------------ joytest.c --------------------------------- */

/* An example of how to read the PC joystick port by polling the hardware
 * port directly.
 * Uses inp()/outp() for byte port access.
 * Will timeout after JS_TIMEOUT reads. On my 486DX2/66, using my joystick port
 * and my joystick, full deflection reads 1600-1800 counts. I repeat the 'my'
 * above to indicate that each joystick card and joystick has different
 * characteristics. This shows as 6000-7000 counts if the READ_TIMER option
 * is used (depends on the mode the timer is set to...).
 *
 * There is no need to optimize this routine since it runs for as long as
 * the joystick circuitry needs. Nevertheless, on slow machines increasing
 * the speed will yield higher resolution. This version is already optimized
 * for speed.
 *
 * About interrupts: these will cause some noise in the reading. The easiest
 * way around it is to read the stick twice and select the SMALLEST reading.
 * You may want to disable interrupts for the process (as the program shows
 * as comments) but watch out for lost serial-port characters and what not.
 * A middle way is to read the system timer which gets around a lot of the
 * noise; just #define USE_TIMER to enable this feature.
 * The enable/disable functions control the interrupts, you can use other
 * functions if your compiler provides them.
 *
 * To compile:
 * 	>cl -O2 joytest.c
 * To Run:
 * 	>joytest
 *
 * Written by Eyal Lebedinsky (eyal@eyal.emu.id.au).
 * This version: January 1994.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

/* #include "pc8254.h" */

/* --------------------------------- pc8254.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Definitions for the timer chip 8254. Used for timekeeping and speaker
 * control.
*/

#define CHANNEL_0	0x0040		/* system tick */
#define	CHANNEL_2	0x0042		/* speaker tone */
#define	COMMAND_REG	0x0043
#define WRITE_CH0	0x0036
#define WRITE_CH2	0x00b6
#define READ_SPECIAL	0x00c2
#define	PORT_B		0x0061		/* speaker on/off control */
#define	XTAL		1193000L
#define TIMER_MODES	0x000e
#define TIMER_MODE	0x0002
#define TIMER_OUT	0x0080
#define TIMER_PERIOD	0x0000ffffL

/* --------------------------------- pc8254.h end---------------------------- */


#define JS_PORT		0x201
#define JS_TIMEOUT	32000
#define JS_READ		inp (JS_PORT)

typedef unsigned short	Ushort;
typedef unsigned int	Uint;
typedef unsigned long	Ulong;

/* These are the timer reading support functions.
*/
static Ushort near
disable (void)
{
	Ushort	flags;

	_asm {
		pushf
		pop	flags
		cli
	}
	return (flags);
}

static void near
enable (Ushort flags)
{
	_asm {
		push	flags
		popf
	}
}

static Uint near
get_timer (void)		/* get fastest timer available */
{
	Ushort		t, status, flags;

	do {
		flags = disable ();
		outp (COMMAND_REG, READ_SPECIAL);
		status = (Ushort) inp (CHANNEL_0);	/* get status */
		t  = (Ushort)inp (CHANNEL_0);		/* low byte */
		t += (Ushort)(inp (CHANNEL_0) << 8);	/* high byte */
		enable (flags);
	} while (0 == t);

	return ((Uint)(Ushort)~((status & TIMER_MODES) == TIMER_MODE*2 ? t
		: ((t>>1) + (Ushort)((status&TIMER_OUT)<<8))));
}

#define READING	(mode ? get_timer () : JS_TIMEOUT-i)

struct stick {
	Ushort	a[4];
	Ushort	b[4];
};
typedef struct stick	STICK;

static int near
readjoy (STICK *s, int mode, int mask, int nread, int delay)
{
	register int	i;
	register Uint	m;
	unsigned int	t, x1, y1, x2, y2, minx1, miny1, minx2, miny2;
	int		js, tt, ntimes;

	minx1 = miny1 = minx2 = miny2 = 0xffffU; /* avoid compiler warning */
	memset (s->a, 0, sizeof (s->a));

	for (ntimes = 0;;) {
		i = JS_TIMEOUT;
		t = READING;
		x1 = y1 = x2 = y2 = t;
		outp (JS_PORT, 0);		/* set trigger */
		for (m = mask; m;) {
			while (!(~JS_READ & m) && --i)
				;
			if (!i)
				break;
			tt = READING;
			js = ~JS_READ & m;
			if (js & 0x01) {
				x1 = tt;
				m &= ~0x01;
			}
			if (js & 0x02) {
				y1 = tt;
				m &= ~0x02;
			}
			if (js & 0x04) {
				x2 = tt;
				m &= ~0x04;
			}
			if (js & 0x08) {
				y2 = tt;
				m &= ~0x08;
			}
		}
		if (minx1 > (x1 -= t))
			minx1 = x1;
		if (miny1 > (y1 -= t))
			miny1 = y1;
		if (minx2 > (x2 -= t))
			minx2 = x2;
		if (miny2 > (y2 -= t))
			miny2 = y2;

		if (++ntimes >= nread)	/* read more? */
			break;

		if (0 != (i = delay)) {		/* delay? */
			tt = 1234;
			for (i *= 10; i-- > 0;)
				tt *= 19;
		}
	}

	js = m | ~mask;
	s->a[0] = (js & 0x01) ? 0 : minx1;	/* analog 1 */
	s->a[1] = (js & 0x02) ? 0 : miny1;	/* analog 2 */
	s->a[2] = (js & 0x04) ? 0 : minx2;	/* analog 3 */
	s->a[3] = (js & 0x08) ? 0 : miny2;	/* analog 4 */

	js = ~JS_READ;
	s->b[0] = !!(js & 0x10);		/* button 1 */
	s->b[1] = !!(js & 0x20);		/* button 2 */
	s->b[2] = !!(js & 0x40);		/* button 3 */
	s->b[3] = !!(js & 0x80);		/* button 4 */

	return (m);
}

static void near
usage (int die)
{
	printf ("Usage:\n");
	printf ("   joytest Mode Mask [q] [Nread [Delay]]\n");
	printf ("Mode is 'count' or 'timer' to indicate the reading method.\n");
	printf ("Mask defines which channels to read. It is one hexadecimal");
	printf (" digit, e.g.:\n");
	printf ("   '3' will read joystock A.\n");
	printf ("   'c' will read joystock B.\n");
	printf ("   'f' will read both.\n");
	printf ("   'b' will read a CH or FCS.\n");
	printf (" Try 'f', if you get st=n with a non-zero n then");
	printf (" use 15-n as your parameter.\n");
	printf ("q will not print the readings, good for timing.\n");
	printf ("Nread will read the stick that many times and return the");
	printf (" minimum as the result.\n");
	printf ("Delay will pause that much between readings.\n");
	printf (" These are useful on fast machines or when");
	printf (" there is interference (network etc.).\n");

	if (die)
		exit (1);
}


/* This main() is for demonstration.
*/

int
main (int argc, char *argv[])
{
	int	i, mode, mask, quiet, nread, delay;
	Ulong	testno;
	STICK	s[1];

	printf ("joystick test,");
	printf (" by Eyal Lebedinsky [eyal@eyal.emu.id.au]\n");

/*	usage (0);*/

	if (argc < 3)
		usage (1);

	if (!strcmp (argv[1], "count"))
		mode = 0;
	else if (!strcmp (argv[1], "timer"))
		mode = 1;
	else
		usage (1);

	if (1 != sscanf (argv[2], "%x", &mask) || mask < 1 || mask > 15)
		usage (1);

	i = 3;
	if (!strcmp (argv[i], "q")) {
		quiet = 1;
		++i;
	} else
		quiet = 0;

	nread = 1;
	delay = 0;
	if (argc > i) {
		if (1 != sscanf (argv[i], "%d", &nread) || nread <= 0)
			usage (1);
		++i;
		if (argc > i
		    && (1 != sscanf (argv[i], "%d", &delay) || delay < 0))
			usage (1);
	}

	printf ("st>0 means some joystick channel is not operational.\n");
	printf ("If the reading is very large");
	printf (" then this channel is not connected.\n");
	printf ("If you do have a stick connected and get st>0");
	printf (" then that port is not functioning.\n");
	printf ("A reading of zero usually indicates a non-operational");
	printf (" channel.\n\n");

	if (mode)
		printf ("Using the PC's timer for timing.\n");
	else
		printf ("Using a loop count for timing.\n");
	printf ("Reading mask %x, %d times with a %d delay.\n",
		mask, nread, delay);

	printf ("\nHit any key to exit\n\n");
	if (quiet)
		printf ("Running in quiet mode.\n");

	for (testno = 1; !kbhit (); ++testno) {
		i = readjoy (s, mode, mask, nread, delay);
		if (!quiet)
printf ("\rst=%x x1=%5u y1=%5u x2=%5u y2=%5u btn=%u%u%u%u n=%lu",
				i, s->a[0], s->a[1], s->a[2], s->a[3],
				s->b[0], s->b[1], s->b[2], s->b[3], testno);
	}
	if (quiet) {
		printf ("Stick was read %lu times.\n", testno);
		printf ("Remember that the time depends on the joystick");
		printf (" position.\n");
	}

	exit (0);
	return (0);
}
