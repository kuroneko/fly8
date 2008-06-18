/* --------------------------------- obroken.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: broken.
*/

#include "fly.h"


static SHAPE shape_broken = {
	0,
	0,
	SH_G,
	10*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_broken (OBJECT *p)
{
	if (!st.owner)
		return (1);

	p->color = CC_GREEN;
	p->time = (Frand()%TIMEPSEC)*2 + TIMEPSEC;
	p->flags |= F_VISIBLE;
	p->owner   = st.owner;
	p->ownerid = st.owner->id;
	p->damage = 1;
	p->damaging = 1;
	if (st.owner->flags & F_IMPORTED) {
		p->flags |= F_IMPORTED;
		p->rplayer = st.owner->rplayer;
	}
	LVcopy (p->R, st.owner->R);
	Mident (p->T);
	return (0);
}

BODY FAR BoBroken = {
	0,
	0,
	"BROKEN",
	&shape_broken,
	gen_read,
	gen_term,
	create_broken,
	gen_delete,
	gen_dynamics,
	gen_hit
};
