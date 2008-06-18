/* --------------------------------- nosound.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* The Sound device driver for when you have none.
*/

#include "fly.h"


LOCAL_FUNC void FAR
NoSndPoll (int force)
{}

LOCAL_FUNC int FAR
NoSndBeep (int f, int milli)
{return (0);}

LOCAL_FUNC int FAR
NoSndList (int *list, int id)
{return (0);}

LOCAL_FUNC int FAR
NoSndEffect (int eff, int command, ...)
{return(0);}

LOCAL_FUNC int FAR
NoSndInit (char *name)
{
	LogPrintf ("%s: init ok\n", Snd->name);
	return (0);
}

LOCAL_FUNC void FAR
NoSndTerm (void)
{
	LogPrintf ("%s: term ok\n", Snd->name);
}

struct SndDriver NEAR SndNone = {
	"NoSound",
	0,
	NULL,
	NoSndInit,
	NoSndTerm,
	NoSndPoll,
	NoSndBeep,
	NoSndEffect,
	NoSndList
};
