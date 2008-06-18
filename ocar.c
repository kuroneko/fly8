/* --------------------------------- ocar.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object O_CAR: a basic ground roving vehicle.
*/

#include "fly.h"


static SHAPE shape_car = {
	0,
	0,
	SH_BEHIT,
	50*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_car (OBJECT *p)
{
	Mident (p->T);
	p->R[Z]  = 10*VONE;

	p->color = ST_FRIEND;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	p->damage = 5;
	p->damaging = 1;

	return (0);
}

/* pointer usage:
 *
 * a[0]	right/left turn.
 * a[1]	ac/de-celerate.
*/

LOCAL_FUNC void FAR
dynamics_car (OBJECT *p)
{
	POINTER	*ptr;

	if (p->flags & F_HIT) {
		gen_dynamics (p);
		return;
	}

	if (F(ptr = p->pointer))
		return;
	if (pointer_read (ptr, 1)) {
		MsgEPrintf (10, "pointer lost!");
		Snd->Effect (EFF_NO_POINTER, SND_ON);
		return;
	}

/* angular components
*/
	if (ptr->b[3])
		(*ptr->control->Center)(ptr);

	p->da[Z] = muldiv (DEG(30), ptr->l[0], 100);
	p->a[Z] = TANG(p->a[Z] + TADJ(p->da[Z]));
	Mobj (p);

/* linear components
*/
	p->ab[Y] = muldiv (100*VONE, -ptr->l[1], 100);
	p->vb[Y] += TADJ(p->ab[Y]);
	VMmul (p->V, p->vb, p->T);
	p->speed = p->vb[Y];	/* by definition */

	memset (ptr->b, 0, sizeof (ptr->b));	/* clear all buttons */

	object_update (p);
}

BODY FAR BoCar = {
	0,
	0,
	"CAR",
	&shape_car,
	gen_read,
	gen_term,
	create_car,
	gen_delete,
	dynamics_car,
	gen_hit
};
