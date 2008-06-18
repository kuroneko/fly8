/* --------------------------------- olow.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Object description: ground at close range.
*/

#include "fly.h"


/* This is a small, simple x-y reference grid that follows the plane and
 * only shows when it is very low.
*/

static SHAPE shape_low = {
	0,
	0,
	0,
	1L,		/* weight */
	0		/* drag */
};

#define	GUNIT	100
#define HORIZON	(1000L*VONE)

LOCAL_FUNC int FAR
init_low (BODY *b)
{
	int	i, last, g, h;
	VERTEX	*p, *vx_low;

	i = (int)(HORIZON/VONE/GUNIT);
	last = 2*i+1;

	if (F(vx_low = (VERTEX *)memory_calloc (sizeof (*vx_low), 4*last+1)))
		return (1);

	shape_low.v = vx_low;
	shape_low.flags |= SH_DYNVERTEX;

	h = GUNIT*i;
	for (g = -h, p = vx_low, i = 0; i < last; ++i, g += GUNIT) {
		p->V[X] = g;	p->V[Y] = -h;	p->V[Z] = 0;
		p->flags = V_MOVE;	++p;

		p->V[X] = g;	p->V[Y] = h;	p->V[Z] = 0;
		p->flags = V_DRAW;	++p;

		p->V[X] = -h;	p->V[Y] = g;	p->V[Z] = 0;
		p->flags = V_MOVE;	++p;

		p->V[X] = h;	p->V[Y] = g;	p->V[Z] = 0;
		p->flags = V_DRAW;	++p;
	}
	p->flags = 0;
	return (0);
}

LOCAL_FUNC int FAR
create_low (OBJECT *p)
{
	p->color = CC_LRED;
	p->time = FOREVER;
	Mident (p->T);

	return (0);
}

LOCAL_FUNC void FAR
dynamics_low (OBJECT *p)
{
	p->R[X] = (((CV->R[X]/VONE+GUNIT/2)/GUNIT)*GUNIT)*VONE;
	p->R[Y] = (((CV->R[Y]/VONE+GUNIT/2)/GUNIT)*GUNIT)*VONE;
}

#undef GUNIT
#undef HORIZON

BODY FAR BoLow = {
	0,
	0,
	"LOWLAND",
	&shape_low,
	init_low,
	gen_term,
	create_low,
	gen_delete,
	dynamics_low,
	gen_hit
};
