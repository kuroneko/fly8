/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Console keyboard handler: X11 (most code actualy in grX.c).
*/

#include "fly.h"
#include "grx.h"


static int FAR
kread (void)
{
	if (Gr && Gr->extra)
		return (((struct GrxExtra *)(Gr->extra))->Kread ());
	else
		return (-1);
}

static int FAR
kwait (void)
{
	int	esc, c;

	while (-1 == (c = kread ()))
		sys_poll (20);
	for (esc = 0; -1 != c; c = kread ())
		if (K_ESC == c)
			esc = 1;
	return (esc);
}

static int FAR
kgetch (void)
{
	int	c;

	while ((c = kread ()) == -1)
		sys_poll (21);
	return (c);
}

static int FAR
kinit (char *options)
{return (0);}

static void FAR
kterm (void)
{}

struct KbdDriver KbdConsole = {
	"CONSOLE",
	0,
	NULL,	/* extra */
	kinit,
	kterm,
	kread,
	kgetch,
	kwait
};

