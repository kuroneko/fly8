/* --------------------------------- ohouse.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object 'house'.
*/

#include "fly.h"


static SHAPE shape_house = {
	0,
	0,
	0,
	2000*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_house (OBJECT *p)
{
	p->color = CC_RED;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);
	return (0);
}

BODY FAR BoHouse = {
	0,
	0,
	"HOUSE",
	&shape_house,
	gen_read,
	gen_term,
	create_house,
	gen_delete,
	gen_dynamics,
	gen_hit
};
