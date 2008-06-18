/* --------------------------------- ocrater.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object: crater left by bomb.
*/

#include "fly.h"


static SHAPE shape_crater = {
	0,
	0,
	0,
	1L, /* Now they know how many holes it takes to fill the Albert Hall */
	0		/* drag */
};

#define CLIFE	(5*60*TIMEPSEC)
#define SLIFE	(1*60*TIMEPSEC)
#define SRATE	4

LOCAL_FUNC int FAR
create_crater (OBJECT *p)
{
	p->color = CC_RED;
	p->time = CLIFE;
	p->flags |= F_VISIBLE;
	LVcopy (p->R, st.owner->R);
	Mident (p->T);
	return (0);
}

LOCAL_FUNC void FAR
dynamics_crater (OBJECT *p)
{
	int	n;

	if (st.flags1 & SF_SMOKE) {
		n = CLIFE - p->time;
		n = SLIFE - n;
		if (n > 0) {
			n = muldiv (n, SRATE, SLIFE);
			n = TADJ (n);
			while (n-- > 0)
				create_object (O_SMOKE, 1);
		}
	}
	object_update (p);
}

#undef CLIFE
#undef SLIFE
#undef SRATE

BODY FAR BoCrater = {
	0,
	0,
	"CRATER",
	&shape_crater,
	gen_read,
	gen_term,
	create_crater,
	gen_delete,
	dynamics_crater,
	gen_hit
};
