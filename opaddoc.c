/* --------------------------------- opaddoc.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: paddoc.
*/

#include "fly.h"


#define S	400

LOCAL_FUNC int FAR
paddoc_create (OBJECT *p)
{
	p->color = CC_GREEN;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);
	return (0);
}

extern int FAR
paddoc_init (BODY *b)
{
	VERTEX	*v;

	b->title = "PADDOC";
	b->flags |= BO_DYNSHAPE;
	if (F(NEW (b->shape)))
		return (1);
	if (F(v = (VERTEX *) memory_calloc (sizeof (*v), 6))) {
		DEL0(b->shape);
		return (1);
	}
	b->shape->v = v;

	v->V[X] = -S;	v->V[Y] = -S;	v->flags = V_MOVE;	++v;
	v->V[X] = -S;	v->V[Y] =  S;	v->flags = V_DRAW;	++v;
	v->V[X] =  S;	v->V[Y] =  S;	v->flags = V_DRAW;	++v;
	v->V[X] =  S;	v->V[Y] = -S;	v->flags = V_DRAW;	++v;
	v->V[X] = -S;	v->V[Y] = -S;	v->flags = V_DRAW;	++v;
					v->flags = 0;

	b->shape->extent = S;
	b->shape->weight = 1L;
	b->shape->drag = 0;
	b->shape->flags = SH_DYNVERTEX;

	b->term = gen_term;
	b->create = paddoc_create;
	b->delete = gen_delete;
	b->dynamics = gen_nodynamics;
	b->hit = gen_hit;

	return (0);
}

#undef S
