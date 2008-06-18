/* --------------------------------- otarget.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: target.
*/

#include "fly.h"


static SHAPE shape_target = {
	0,
	0,
	SH_BEHIT,
	100*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_target (OBJECT *p)
{
	p->color = CC_GREEN;
	p->time = FOREVER;
	p->flags |= F_VISIBLE|F_EXPORTED;
	p->damage = 1;
	p->damaging = 1;
	p->R[X] = (Frand () % 6000 - 3000)*(long)VONE;
	p->R[Y] = (Frand () % 6000 - 3000)*(long)VONE;
	p->R[Z] = (Frand () % 3000       )*(long)VONE + 500*VONE;
	p->da[Z] = (Frand () % VD90  - VD90)*4;
	Mident (p->T);
	return (0);
}

BODY FAR BoTarget = {
	0,
	0,
	"TARGET",
	&shape_target,
	gen_read,
	gen_term,
	create_target,
	gen_delete,
	gen_dynamics,
	gen_hit
};
