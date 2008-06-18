/* --------------------------------- gettimer.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read and print the status of the PC timer 0. It should be in mode 3.
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
	int	status;
	short	flags;

	flags = disable ();
	outp (COMMAND_REG, READ_SPECIAL);	/* read back timer 0 */
	status = inp (CHANNEL_0);
	(void)inp (CHANNEL_0);
	(void)inp (CHANNEL_0);
	enable (flags);

	printf ("Timer 0 status is %x (mode %x)\n",
		status, (status & TIMER_MODES)/TIMER_MODE);

	exit (0);
	return (0);
}
