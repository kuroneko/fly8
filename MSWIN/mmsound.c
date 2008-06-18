/* --------------------------------- mmsound.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for MS Windows multimedia, using plsound.
*/

#include <windows.h>

#include "fly.h"

#if HAVE_WAVE
#include "plsound.h"
#include "wave.h"


LOCAL_FUNC int FAR
SpkInit (char *options)
{
    	if (!FInitSnd ())
		return (-1);

	return (PlsInit (options));
}

extern void FAR
SpkTerm (void)
{
        PlsTerm ();
	CloseSnd ();
}

static struct plsextra FAR SpkExtra = {
	Fly8PlaySnd,
	StopSnd,
	NULL,		/* beeps */
	0L,		/* lasttime */
	0L,		/* nexttime */
	0,		/* playing */
	0		/* nbeeps */
};

struct SndDriver NEAR SndPlWave = {
	"PlWave",
	0,
	&SpkExtra,
	SpkInit,
	SpkTerm,
	PlsPoll,
	PlsBeep,
	PlsEffect,
	PlsList
};
#endif /* if HAVE_WAVE */
