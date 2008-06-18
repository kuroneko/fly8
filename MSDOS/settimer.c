/* --------------------------------- settimer.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sets the PC timer 0 to mode 3.
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include "pc8254.h"

static short near
disable (void)
{
	short	flags;

	_asm {
		pushf
		pop	flags
		cli
	}
	return (flags);
}

static void near
enable (short flags)
{
	_asm {
		push	flags
		popf
	}
}

int
main ()
{
	short	flags;

	flags = disable ();
	outp (COMMAND_REG, WRITE_CH0);	/* write timer 0 */
	outp (CHANNEL_0, 0);
	outp (CHANNEL_0, 0);
	enable (flags);

	printf ("Timer 0 set to mode 3\n");

	exit (0);
	return (0);
}
