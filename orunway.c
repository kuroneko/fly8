/* --------------------------------- orunway.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object runway: a standard airfield with two runways.
*/

#include "fly.h"


static SHAPE shape_runway = {
	0,
	0,
	0,
	1L,		/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_runway (OBJECT *p)
{
	p->color = CC_BROWN;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);
	return (0);
}

BODY FAR BoRunway = {
	0,
	0,
	"RUNWAY",
	&shape_runway,
	gen_read,
	gen_term,
	create_runway,
	gen_delete,
	gen_dynamics,
	gen_hit
};
