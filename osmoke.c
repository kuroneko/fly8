/* --------------------------------- osmoke.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: smoke speck.
*/

#include "fly.h"


static SHAPE shape_smoke = {
	0,
	0,
	0,
	1L,		/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_smoke (OBJECT *p)
{
	if (!st.owner)
		return (1);

	p->color = CC_LGRAY;
	p->time = 5*TIMEPSEC;
	p->flags |= F_VISIBLE|F_DONE;

	LVcopy (p->R, st.owner->R);
	Vcopy (p->a, st.owner->a);
	Mcopy (p->T, st.owner->T);
        p->speed = p->V[Z] = 10*VONE;

        return (0);
}

#define DVSMOKE        1*VONE
#define DASMOKE        (D90/10)

LOCAL_FUNC void FAR
dynamics_smoke (OBJECT *p)
{
	p->V[X] += Frand()%(DVSMOKE+1) - DVSMOKE/2;
	p->V[Y] += Frand()%(DVSMOKE+1) - DVSMOKE/2;
	p->V[Z] += Frand()%(DVSMOKE+1) - DVSMOKE/2;
	p->speed = ihypot3d (p->V);
	p->a[X] = TANG(p->a[X] + Frand()%(DASMOKE+1) - DASMOKE/2);
	p->a[Y] = TANG(p->a[Y] + Frand()%(DASMOKE+1) - DASMOKE/2);
	p->a[Z] = TANG(p->a[Z] + Frand()%(DASMOKE+1) - DASMOKE/2);
	Mobj (p);

	object_update (p);
}

#undef DVSMOKE
#undef DASMOKE

BODY FAR BoSmoke = {
	0,
	0,
	"SMOKE",
	&shape_smoke,
	gen_read,
	gen_term,
	create_smoke,
	gen_delete,
	dynamics_smoke,
	gen_hit
};
