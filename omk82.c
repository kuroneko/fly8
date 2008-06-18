/* --------------------------------- omk82.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: MK82 bomb.
*/

#include "plane.h"


extern int FAR
BombSpeed (OBJECT *p, VECT V)
{
	Vcopy (V, p->V);
	return (iabs(p->speed));
}

extern int FAR
BombIP (LVECT R, VECT V, long tz, LVECT IP)
{
	int	v, dt;
	long	z;

	z = R[Z]-tz;
	v = V[Z];
	dt = v + SQRT (v*(long)v + 2L*GACC*z);

	IP[X] = R[X] + dt*(long)V[X]/GACC;
	IP[Y] = R[Y] + dt*(long)V[Y]/GACC;
	IP[Z] = tz;
	return (muldiv (dt, 100, GACC));
}

extern void FAR
BombToGnd (OBJECT *p)
{
	int	v, a;

	v = TADJ (p->V[Z]);
	if (v) {
		a = fdiv ((int)p->R[Z], v);
		p->R[X] -= fmul (a, TADJ (p->V[X]));
		p->R[Y] -= fmul (a, TADJ (p->V[Y]));
	}
	p->R[Z] = 0;
}

#define EX_V	(300*VONE)
#define EX_A	(VD360*4)

LOCAL_FUNC int NEAR
BombExplode (OBJECT *p)
{
	int	i;
	OBJECT	*b;

	st.owner = p;
	for (i = 0; i < 16; ++i) {
		b = create_object (O_BROKEN, 1);
		if (!b)
			break;
		b->flags |= F_HIT;
		b->V[X] = Frand()%(EX_V+1) - EX_V/2;
		b->V[Y] = Frand()%(EX_V+1) - EX_V/2;
		b->V[Z] = Frand()%(EX_V+1);
		b->speed = ihypot3d (b->V);
		b->da[X] = Frand()%(EX_A+1) - EX_A/2;
		b->da[Y] = Frand()%(EX_A+1) - EX_A/2;
		b->da[Z] = Frand()%(EX_A+1) - EX_A/2;
	}
	create_object (O_CRATER, 1);
	if (st.quiet)
		Snd->Effect (EFF_MK82_EXPLODE, SND_ON);
	return (i);
}

static SHAPE shape_mk82 = {
	0,
	0,
	SH_G|SH_HIT|SH_LOCALSIM,
	227*1000L,	/* weight */
	FONE		/* drag */
};

LOCAL_FUNC int FAR
create_mk82 (OBJECT *p)
{
	OBJECT	*owner;
	E_BOMB	*b;
	long	tz;

	if (F(owner = st.owner))
		return (1);

	p->color = CC_WHITE;
	p->time = FOREVER;
	p->damage = 1;
	p->damaging = 10;
	p->owner = owner;
	p->ownerid = owner->id;
	if (IS_PLANE(owner))
		++EE(owner)->misc[11];			/* count bomb */

	LVcopy (p->R, owner->R);
	Vcopy (p->V, owner->V);
	Vcopy (p->a, owner->a);
	Mcopy (p->T, owner->T);
	p->speed = owner->speed;

	if (owner->flags & F_IMPORTED) {
		p->flags |= F_IMPORTED;
		p->rplayer = owner->rplayer;
	} else if (T(NEW (b))) {
		p->e_type = ET_BOMB;
		p->extra = b;

		if (IS_PLANE(owner) &&
		    EE(owner)->target &&
		    EE(owner)->target->id == EE(owner)->tid)
			tz = EE(owner)->target->R[Z];
		else
			tz = 0L;
		b->timpact = st.present + 10L*BombIP (p->R, p->V, tz, b->IP);
	}
	p->flags |= F_VISIBLE|F_DONE;

	if (st.quiet)
		Snd->Effect (EFF_MK82_SHOOT, SND_ON);

	if (owner == CC)
		++st.nbullets;
	return (0);
}

LOCAL_FUNC void FAR
delete_mk82 (OBJECT *p)
{
	OBJECT	*owner;

	if (T(owner = get_owner (p)) && IS_PLANE(owner))
		--EE(owner)->misc[11];	/* count bomb */
}

LOCAL_FUNC void FAR
dynamics_mk82 (OBJECT *p)
{
	VECT	AA;

	if (p->flags & F_HIT) {
		gen_dynamics (p);
		return;
	}
	if (p->R[Z] <= 0) {
		BombToGnd (p);
		BombExplode (p);
		p->flags |= F_DEL|F_MOD;
		return;
	}
	AA[X] = AA[Y] = 0;
	AA[Z] = -GACC;
#if 0						/* testing */
{
	int	vmag, q, drag;

	vmag = iabs(p->speed/VONE);
	q = muldiv (vmag, vmag, 128);
	drag = fmul (q, FCON(0.01));
	acc = muldiv (drag, 128, 250);		/* weight = 250 Kg */

	AA[X] -= muldiv (p->V[X], p->speed, acc);
	AA[Y] -= muldiv (p->V[Y], p->speed, acc);
	AA[Z] -= muldiv (p->V[Z], p->speed, acc);
}
#else
#endif

	p->V[X] += TADJ(AA[X]);
	p->V[Y] += TADJ(AA[Y]);
	p->V[Z] += TADJ(AA[Z]);
#if 0
{
	ANGLE	daX;

	p->da[X] = v[Z]/64;

	daX = TADJ(p->da[X])*VONE;

	Mident (p->T);
	Mrotx (p->T, daX);
	fMroty (p->T, p->siny, p->cosy);
	fMrotx (p->T, p->sinx, p->cosx);
	fMrotz (p->T, p->sinz, p->cosz);
	Mangles (p, p->T, p->a, 0);
}
#endif
	p->speed = ihypot3d (p->V);
	object_update (p);
}

BODY FAR BoMK82 = {
	0,
	0,
	"MK82",
	&shape_mk82,
	gen_read,
	gen_term,
	create_mk82,
	delete_mk82,
	dynamics_mk82,
	gen_hit
};

#undef EX_V
#undef EX_A

