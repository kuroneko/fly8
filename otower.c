/* --------------------------------- otower.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* object control tower.
*/

#include "fly.h"


static SHAPE shape_tower = {
	0,
	0,
	0,
	100000*1000L,	/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_tower (OBJECT *p)
{
	p->color = CC_BROWN;
	p->time = FOREVER;
	p->flags |= F_VISIBLE;
	Mident (p->T);
	return (0);
}

BODY FAR BoTower = {
	0,
	0,
	"TOWER",
	&shape_tower,
	gen_read,
	gen_term,
	create_tower,
	gen_delete,
	gen_dynamics,
	gen_hit
};
