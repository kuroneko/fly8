/* --------------------------------- obox.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object O_BOX: only one side edge makes it more animated.
*/

#include "fly.h"


static SHAPE shape_box = {
	0,
	0,
	SH_G|SH_BEHIT,
	50*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_box (OBJECT *p)
{
	int	i;

	p->da[Z] = VD360/5;
	Mident (p->T);
	if ((i = CD->colors) > 16)
		i = 16;
	p->color = Frand () % (i-1) + 1;
	p->time = 5*60*TIMEPSEC;
	p->flags |= F_VISIBLE;
	p->damage = 3;
	p->damaging = 1;
	p->R[Z] = (Frand()%3000 + 1000)*(long)VONE;
	do {
		p->V[Y] = Frand()%(400*VONE) - 200*VONE;
		p->V[X] = Frand()%(400*VONE) - 200*VONE;
		p->speed = ihypot3d (p->V);
	} while (p->speed < 120*VONE);
	return (0);
}

LOCAL_FUNC void FAR
dynamics_box (OBJECT *p)
{
	if (p->flags & F_HIT) {
		gen_dynamics (p);
		return;
	}

	if (p->V[Z] > -250*VONE)
		p->V[Z] -= TADJ (5*GACC);

	if (p->R[Z] < 0) {
		p->R[Z] = 0;
		p->V[Z] = -p->V[Z];
		p->V[Z] -= p->V[Z]/16;
	}

	if (p->R[X] > 5000L*VONE) {
		p->V[X] = -p->V[X];
		p->R[X] = 5000L*VONE;
	} else if (p->R[X] < -5000L*VONE) {
		p->V[X] = -p->V[X];
		p->R[X] = -5000L*VONE;
	}

	if (p->R[Y] > 5000L*VONE) {
		p->V[Y] = -p->V[Y];
		p->R[Y] = 5000L*VONE;
	} else if (p->R[Y] < -5000L*VONE) {
		p->V[Y] = -p->V[Y];
		p->R[Y] = -5000L*VONE;
	}
	p->speed = ihypot3d (p->V);

	p->a[Z] = TANG (p->a[Z] + TADJ(p->da[Z])*VONE);

	Mobj (p);

	object_update (p);
}

BODY FAR BoBox = {
	0,
	0,
	"BOX",
	&shape_box,
	gen_read,
	gen_term,
	create_box,
	gen_delete,
	dynamics_box,
	gen_hit
};
