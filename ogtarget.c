/* --------------------------------- ogtarget.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: ground target.
*/

#include "fly.h"


#define SRATE	10

static SHAPE shape_gtarget = {
	0,
	0,
	SH_BEHIT,
	10000*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_gtarget (OBJECT *p)
{
	p->color = CC_WHITE;
	p->time = FOREVER;
	p->damage = 10;
	p->damaging = 1;
	p->flags |= F_VISIBLE|F_EXPORTED;
	p->R[X] = (Frand () % 10000 - 5000)*(long)VONE;
	p->R[Y] = (Frand () % 10000 - 5000)*(long)VONE;
	Mident (p->T);
	return (0);
}

LOCAL_FUNC void FAR
dynamics_gtarget (OBJECT *p)
{
	int	n;

	if (p->damage < 10 && (st.flags1 & SF_SMOKE)) {
		n = 10 - p->damage;
		for (n = TADJ (n); n-- > 0;)
			create_object (O_SMOKE, 1);
		if (p->flags & F_HIT)
			p->color = (p->time/(TIMEPSEC/10))&1
						? ST_FIRE2 : ST_FIRE1;
	}
	object_update (p);
}

#undef CLIFE
#undef SLIFE
#undef SRATE

BODY FAR BoGtarget = {
	0,
	0,
	"GTARGET",
	&shape_gtarget,
	gen_read,
	gen_term,
	create_gtarget,
	gen_delete,
	dynamics_gtarget,
	gen_hit
};
