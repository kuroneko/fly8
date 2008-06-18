/* --------------------------------- ogen.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Some generic object routines.
*/

#include "fly.h"


/* Some generic routines for bodies.
*/

extern int FAR
gen_init (BODY *b)
{return (0);}

extern int FAR
gen_read (BODY *b)
{
	return (shape_read (b->shape, b->title));
}

extern void FAR
gen_term (BODY *b)
{
	if (b->shape) {
		if (b->shape->flags & SH_DYNVERTEX)
			b->shape->v = shape_free (b->shape->v);
		if (b->flags & BO_DYNSHAPE)
			DEL0 (b->shape);
	}
	b->init = gen_init;
	b->term = gen_term;
	b->create = gen_nocreate;
	b->delete = gen_delete;
	b->dynamics = gen_nodynamics;
	b->hit = gen_nohit;
}

extern int FAR
gen_create (OBJECT *p)
{
	p->color = CC_WHITE;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);
	return (0);
}

extern int FAR
gen_nocreate (OBJECT *p)
{return (1);}

extern void FAR
gen_delete (OBJECT *p)
{}

extern void FAR
gen_dynamics (OBJECT *p)
{
	if (((st.flags1 & SF_USEG) && (p->shflags & SH_G)) ||
	    (p->flags & F_HIT)) {
		if (iabs (p->speed) < (Uint)(VMAX-10*GACC)) {
			p->V[Z] -= TADJ (GACC);
			p->speed = ihypot3d (p->V);
		}
	}

	if (p->flags & F_HIT) {
		if (FOREVER == p->time)
			p->color = rand()%2 ? ST_FIRE2 : ST_FIRE1;
		else
			p->color = (p->time/(TIMEPSEC/10))&1
						? ST_FIRE2 : ST_FIRE1;
		if (O_PLANE == p->name) {
			st.owner = p;
			object_break (rand()%2, p->speed, 1, 0);
		}
		if (!(p->flags & F_IMPORTED) &&
		    p->time == FOREVER && p->R[Z] <= 0) {
			p->flags |= F_DEL|F_MOD;
			if (O_PLANE == p->name) {
				st.owner = p;
				object_break (rand()%16+5, p->speed,
					SH(p)->extent, 1);
			}
		}
	} else if (O_PLANE == p->name)
		plane_smoke (p);

	object_update (p);

	if (p->da[X] || p->da[Y] || p->da[Z]) {
		if (p->da[X]) {
			p->a[X] = TANG(p->a[X] + TADJ(p->da[X])*VONE);
			if (iabs(p->a[X]) > D90) {
				p->a[X] = D180 - p->a[X];
				p->a[Y] = D180 + p->a[Y];
				p->da[X] = -p->da[X];
			}
		}
		if (p->da[Y])
			p->a[Y] = TANG(p->a[Y] + TADJ(p->da[Y])*VONE);
		if (p->da[Z])
			p->a[Z] = TANG(p->a[Z] + TADJ(p->da[Z])*VONE);
		Mobj (p);
	}
}

extern void FAR
gen_nodynamics (OBJECT *p)
{}

extern void FAR
gen_hit (OBJECT *obj, int speed, int extent, int damaging)
{
	if (obj->damage <= 0 && !(obj->flags & F_HIT)) {
		obj->color = ST_FIRE1;
		obj->flags |= F_HIT|F_MOD;
		obj->time = 5*TIMEPSEC;
	}

	if (obj->name == O_TARGET /*|| obj->name == O_GTARGET*/)
		obj->flags |= F_DEL;	/* no notification */
/* explode.
*/
	st.owner = obj;
	if (obj->damage >= -20)		/* limit vicious loops */
		object_break (rand()%5 + 2, speed, extent, 0);
	object_rand (obj, speed, extent, 1);
}

extern void FAR
gen_nohit (OBJECT *obj, int speed, int extent, int damaging)
{}
