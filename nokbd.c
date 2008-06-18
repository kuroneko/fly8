/* --------------------------------- nokbd.c   ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* The Keyboard device driver for when you have none.
*/

#include "fly.h"


LOCAL_FUNC int FAR
nkinit (char *options)
{return (0);}

LOCAL_FUNC void FAR
nkterm (void)
{}

LOCAL_FUNC int FAR
nkread (void)
{return (-1);}

LOCAL_FUNC int FAR
nkgetch (void)
{return (0);}

LOCAL_FUNC int FAR
nkwait (void)
{return (0);}

struct KbdDriver NEAR KbdNone = {
	"NoKbd",
	0,
	NULL,
	nkinit,
	nkterm,
	nkread,
	nkgetch,
	nkwait
};
