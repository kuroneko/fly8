/* --------------------------------- om61.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: M61 bullet. Also bullet related routines.
*/

#include "plane.h"


extern int FAR
BulletSpeed (OBJECT *p, VECT V)
{
	int	bspeed;

	bspeed = p->speed;			/* rel bullet speed */
	if (bspeed > VMAX-BULLETV)
		bspeed = VMAX - bspeed;
	else
		bspeed = BULLETV;
	Vscale (V, p->T[Y], bspeed);		/* rel bullet vector */
	Vinc (V, p->V);				/* abs bullet vector */
	bspeed = ihypot3d (V);			/* abs bullet speed */
	return (bspeed);
}

static SHAPE shape_m61 = {
	0,
	0,
	SH_G|SH_HIT,
	100L,		/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_m61 (OBJECT *p)
{
	ANGLE	errmag, errdir;
	int	v;

	if (!st.owner)
		return (1);

	p->color = CC_WHITE;
	p->time = 3*TIMEPSEC;
	p->owner = st.owner;
	p->ownerid = st.owner->id;
	p->damage = 1;
	p->damaging = 1;
	p->flags |= F_DONE;
	if (st.owner->flags & F_IMPORTED) {
		p->flags |= F_IMPORTED;
		p->rplayer = st.owner->rplayer;
	}

	LVcopy (p->R, st.owner->R);
	Vcopy (p->a, st.owner->a);

	errmag = Frand() % (BULLETSCATTER+1);
	errmag = muldiv (errmag, errmag, BULLETSCATTER);
	errdir = (Frand()%D90)*4;
	p->a[X] = TANG(p->a[X] + fmul (errmag, COS(errdir)));
	p->a[Z] = TANG(p->a[Z] + fmul (errmag, SIN(errdir)));
	Mobj (p);

	if (st.owner->speed > VMAX - BULLETV)
		v = VMAX - st.owner->speed;
	else
		v = BULLETV;
	Vscale (p->V, p->T[Y], v);
	Vinc (p->V, st.owner->V);
	v = Frand () % st.interval;
	p->R[X] += muldiv (p->V[X], v, 1000);
	p->R[Y] += muldiv (p->V[Y], v, 1000);
	p->R[Z] += muldiv (p->V[Z], v, 1000);
	p->speed = ihypot3d (p->V);

	if (st.quiet)
		Snd->Effect (EFF_M61_SHOOT, SND_ON);

	if (st.owner == CC)
		++st.nbullets;
	return (0);
}

BODY FAR BoM61 = {
	0,
	0,
	"M61",
	&shape_m61,
	gen_read,
	gen_term,
	create_m61,
	gen_delete,
	gen_dynamics,
	gen_hit
};

