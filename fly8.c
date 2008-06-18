/* --------------------------------- fly8.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* 'fly8' was the name of the last version written back in '74 at the
 * Technion with Danny. It ran on a PDP-15 with a VT15 stroke graphics
 * device at the EE digital lab. Assembley, no color, sound supplied by
 * the mechanical console teletype shaking it's head and ringing it's bell.
 * Those were the days (come to think of it, mostly NIGHTS!).
*/

#include "fly.h"


struct status	NEAR st = {0};

extern int
C_MAIN (int argc, char *argv[])
{
	int	n;

	memset ((char *)&st, 0, sizeof (st));

	check_stack (0);

	initialize (argv);

	sim_set ();
	screen_start ();		/* show fixed screen contents */
	sim_reset ();

	user_command ();		/* let them have the controls */

	n = check_stack (1);
	terminate (n);

	return (0);
}
