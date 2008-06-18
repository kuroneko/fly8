/* --------------------------------- plspeak.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for the PC speaker, using plsound.
*/

#include "fly.h"
#include "plsound.h"
#include "pc8254.h"

#if SYS_DJGPP
#include <pc.h>

#define inp(p) 		inportb (p)
#define outp(p,b)	outportb ((p), (b))

#else
#include <conio.h>
#endif

#define INITED		0x8000


LOCAL_FUNC void FAR
SpkTerm (void)
{
	if (Snd->flags & INITED) {
		PlsTerm ();
		Snd->flags &= ~INITED;
		LogPrintf ("%s: term ok\n", Snd->name);
	}
}

LOCAL_FUNC int FAR
SpkInit (char *options)
{
	if (PlsInit (options))
		goto badret;

	Snd->flags |= INITED;

	LogPrintf ("%s: init ok\n", Snd->name);
	return (0);
badret:
	SpkTerm ();
	return (1);
}

LOCAL_FUNC void FAR
SpkStart (int n)
{
	int	i;
	Ulong	flags;

	flags = Sys->Disable ();

	i = inp (PORT_B);	/* get 8255 port B */
	i |= 3;			/* turn on speaker */
	outp (PORT_B, i);	/* set 8255 port B */
	iefbr14 ();

	if (n < (int)(XTAL/0x0ffffL))
		n = 0xffff;
	else
		n = (int)(XTAL / n);

	outp (COMMAND_REG, WRITE_CH2);
	iefbr14 ();
	outp (CHANNEL_2, n & 0x00ff);		/* LSB */
	iefbr14 ();
	outp (CHANNEL_2, (n >> 8) & 0x00ff);	/* MSB */
	iefbr14 ();

	Sys->Enable (flags);
}

LOCAL_FUNC void FAR
SpkStop (void)
{
	int	i;
	Ulong	flags;

	flags = Sys->Disable ();

	i = inp (PORT_B);	/* get 8255 port B */
	i &= ~3;		/* turn off speaker */
	outp (PORT_B, i);	/* set 8255 port B */

	Sys->Enable (flags);
}

static struct plsextra FAR SpkExtra = {
	SpkStart,
	SpkStop,
	NULL,		/* beeps */
	0L,		/* lasttime */
	0L,		/* nexttime */
	0,		/* playing */
	0		/* nbeeps */
};

struct SndDriver NEAR SndPlSpeaker = {
	"PlSpeak",
	0,
	&SpkExtra,
	SpkInit,
	SpkTerm,
	PlsPoll,
	PlsBeep,
	PlsEffect,
	PlsList
};
