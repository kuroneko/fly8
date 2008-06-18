/* --------------------------------- mmsound.c ------------------------------ */
/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for OS/2 multimedia, using plsound.
*/
#include <os2.h>
#include "fly.h"
#include "plsound.h"
#include "os2sound.h"

LOCAL_FUNC int
SpkInit (char *options)
{
      	if (!FInitSnd ())
      		return (-1);
   	return (PlsInit (options));
}

extern void
SpkTerm (void)
{
	PlsTerm ();
   	CloseSnd ();
}

static struct plsextra  SpkExtra = {
   	Fly8PlaySnd,
   	StopSnd,
   	NULL,   /* beeps */             
   	0L,     /* lasttime */
   	0L,     /* nexttime */
   	0,    	/* playing */
   	0     	/* nbeeps */
};

struct SndDriver  PlsMMSound = {    
   	"PLSOUND",
   	0,
   	&SpkExtra,
   	SpkInit,
   	SpkTerm,
   	PlsPoll,
   	PlsBeep,
   	PlsEffect,
   	PlsList
};
