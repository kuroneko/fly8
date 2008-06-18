/* --------------------------------- oground.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Object description: ground.
*/

#include "fly.h"


/* This object is a simple x-y reference grid that follows the plane.
*/

#define GUNIT	4000
#define HORIZON	(0x07fffL*VONE)

static SHAPE shape_ground = {
	0,
	0,
	0,
	1L,		/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
init_ground (BODY *b)
{
	int	i, last, g, h;
	VERTEX	*p, *vx_ground;

	i = (int)(HORIZON/VONE/GUNIT);
	last = 2*i+1;

	if (F(vx_ground = (VERTEX *)memory_calloc (sizeof (*vx_ground),
								4*last+1)))
		return (1);

	shape_ground.v = vx_ground;
	shape_ground.flags |= SH_DYNVERTEX;

	h = GUNIT*i;
	for (g = -h, p = vx_ground, i = 0; i < last; ++i, g += GUNIT) {
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
create_ground (OBJECT *p)
{
	p->color = ST_GROUND;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);

	return (0);
}

LOCAL_FUNC void FAR
delete_ground (OBJECT *p)
{}

LOCAL_FUNC void FAR
dynamics_ground (OBJECT *p)
{
	p->R[X] = (((CV->R[X]/VONE+GUNIT/2)/GUNIT)*GUNIT)*VONE;
	p->R[Y] = (((CV->R[Y]/VONE+GUNIT/2)/GUNIT)*GUNIT)*VONE;
}

BODY FAR BoGround = {
	0,
	0,
	"GROUND",
	&shape_ground,
	init_ground,
	gen_term,
	create_ground,
	delete_ground,
	dynamics_ground,
	gen_hit
};

#undef GUNIT
#undef HORIZON
