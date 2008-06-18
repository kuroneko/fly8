/* --------------------------------- ochute.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: parachute.
*/

#include "fly.h"


static SHAPE shape_chute = {
	0,
	0,
	SH_G|SH_LOCALSIM,
	200*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_chute (OBJECT *p)
{
	if (!st.owner)
		return (1);

	p->color = CC_GREEN;
	p->time = FOREVER;		/* let them land */
	p->flags |= F_VISIBLE;
	p->speed = st.owner->speed;

	p->owner   = st.owner;		/* the chute owns the plane */
	p->ownerid = st.owner->id;

	st.owner->owner = p;		/* the plane owns the chute */
	st.owner->ownerid = p->id;
	if (st.owner->flags & F_EXPORTED)
		p->flags |= F_EXPORTED;

	Vcopy (p->V, st.owner->V);
	LVcopy (p->R, st.owner->R);
	p->da[Z] = VD90/18;
	Mident (p->T);

	return (0);
}

extern void FAR
dynamics_chute (OBJECT *p)
{
	int	force;

	if (p->R[Z] <= 0) {				/* we landed */
		p->R[Z] = 0;
		p->V[Z] = 0;
		if (!(p->flags & F_IMPORTED) &&		/* native */
		    F(get_owner (p))) {			/* plane gone */
			if (p->gpflags & GPF_PILOT) {	/* not a drone */
				CC->gpflags |= GPF_PILOT; /* activate plane */
				CC->flags &= ~F_STEALTH;
				CC->flags |= F_MOD;
			}
			p->flags |= F_DEL|F_MOD;	/* delete chute */
		}
		return;
	}
	p->V[X] -= TADJ(p->V[X]/2);
	p->V[Y] -= TADJ(p->V[Y]/2);

	p->da[Z] -= TADJ(p->da[Z]/2);

	force = -p->V[Z]*2 - GACC;	/* stabilize at 5 m/sec */
					/* too fast but looks OK */
	p->V[Z] += TADJ (force);
	p->speed = ihypot3d (p->V);

	if (p->da[X] || p->da[Y] || p->da[Z]) {
		p->a[X] = TANG(p->a[X] + TADJ(p->da[X])*VONE);
		p->a[Y] = TANG(p->a[Y] + TADJ(p->da[Y])*VONE);
		p->a[Z] = TANG(p->a[Z] + TADJ(p->da[Z])*VONE);
		Mobj (p);
	}

	object_update (p);
}

BODY FAR BoChute = {
	0,
	0,
	"CHUTE",
	&shape_chute,
	gen_read,
	gen_term,
	create_chute,
	gen_delete,
	dynamics_chute,
	gen_hit
};
