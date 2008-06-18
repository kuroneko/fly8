/* --------------------------------- version.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* version number and last date/time.
*/

#include "fly.h"


#define	VERSION		"1.12"
#define	LAST_CHANGE	"20Feb98"

extern char * FAR
welcome (int where)
{
	static char     ver[1 + sizeof (VERSION) + sizeof (LAST_CHANGE)];

	sprintf (ver, "v%s %s", VERSION, LAST_CHANGE);

	switch (where) {
	case 0:						/* to screen */
		MsgPrintf (100, "Fly8 %s by Eyal Lebedinsky", ver);
		MsgPrintf (100, "Build %s %s eyal@eyal.emu.id.au",
			__DATE__, __TIME__);
		break;
	case 2:						/* to stderr */
#ifndef NOSTDERR
		fprintf (stderr, "Fly8 %s by Eyal Lebedinsky\n", ver);
		fprintf (stderr, "Build %s %s eyal@eyal.emu.id.au\n",
			__DATE__, __TIME__);
		break;
#endif
	case 1:						/* to log */
		LogPrintf ("Fly8 %s by Eyal Lebedinsky\n", ver);
		LogPrintf ("Build %s %s eyal@eyal.emu.id.au\n",
			__DATE__, __TIME__);
		break;
	default:					/* just get it */
		break;
	}

	return (ver);
}
#undef VERSION
