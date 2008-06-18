/* -------------------------------- land.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Look after the landscape. These objects do not move or get simulated!
*/

#include "fly.h"


#define RANGE	50

extern int FAR
land_init (void)
{
	BODY	*b;

	CL = CLT = 0;

	if (!create_land (O_GROUND)		/* must be first! */
	    || !create_land (O_LOW))		/* must be second! */
		return (1);

	if (lnd_read () < 0)
		return (1);

	if (!(st.flags&SF_LANDSCAPE))
		return (0);

	b = bodies_new (-1);
	if (!b)
		return (1);
	b->init = paddoc_init;
	if ((*b->init) (b)) {
		bodies_del (b->name);
		return (1);
	}

	st.landx = 30000;		/* force landscaping */
	st.landy = 30000;

	return (0);
}

extern void FAR
land_term (void)
{
	list_clear (&CL);
}

LOCAL_FUNC int NEAR
land_add (ONAME lname, int nx, int ny)
{
	OBJECT	*p;
	int	control;

	Fsrand (nx);
	Fsrand (ny ^ Frand ());
	control = Frand () % 512;

	if (control > 3)
		return (0);

	if (T(p = create_land (lname))) {
		p->flags |= F_LAND;
		p->R[X] = nx * 1000L * VONE;
		p->R[Y] = ny * 1000L * VONE;
		if (control)		/* paddoc */
			p->R[Z] = 0;
		else {			/* cloud */
			p->R[Z] = (Frand() % 2000 + 2000)*(long)VONE;
			p->color = CC_WHITE;
		}
	}

	return (0);
}

extern int FAR
land_update (OBJECT *pov)
{
	long	minx, miny, maxx, maxy;
	int	x, y, xl, xh, yl, yh, xxl, xxh, xx, yy;
	OBJECT	*p;
	ONAME	lname;

	if (F(p = CL) || O_GROUND != p->name) {
		MsgPrintf (-100, "Missing GROUND object!");
		return (1);
	}
	st.bodies[p->name]->dynamics (p);

	if (F(p = p->next) || O_LOW != p->name) {
		MsgPrintf (-100, "Missing LOW object!");
		return (1);
	}
	st.bodies[p->name]->dynamics (p);

	if (!(st.flags&SF_LANDSCAPE))
		return (0);

	x = (int)(pov->R[X] / VONE / 1000);		/* get square */
	y = (int)(pov->R[Y] / VONE / 1000);

	if (x == st.landx && y == st.landy)
		return (0);

	minx = (x-RANGE)*1000L*VONE;
	maxx = (x+RANGE)*1000L*VONE;
	miny = (y-RANGE)*1000L*VONE;
	maxy = (y+RANGE)*1000L*VONE;
	for (p = CL; p;) {				/* delete old */
		if ((p->flags & F_LAND) &&
		    (p->R[X] < minx || p->R[X] > maxx || 
		     p->R[Y] < miny || p->R[Y] > maxy))
		    	p = delete_land (p);
		else
			p = p->next;
	}

	if (st.landx < x) {
		xxl = x-RANGE;
		xh  = x+RANGE;
		xxh = st.landx+RANGE;
		xl  = xxh+1;
		if (xl < xxl)
			xl = xxl;
	} else {
		xl  = x-RANGE;
		xxh = x+RANGE;
		xxl = st.landx-RANGE;
		xh  = xxl-1;
		if (xh > xxh)
			xh = xxh;
	}

	if (st.landy < y) {
		yh = y+RANGE;
		yl = st.landy+RANGE+1;
		if (yl < y-RANGE)
			yl = y-RANGE;
	} else {
		yl = y-RANGE;
		yh = st.landy-RANGE-1;
		if (yh > y+RANGE)
			yh = y+RANGE;
	}

	st.landx = x;
	st.landy = y;

	if (-1 == (lname = body_name ("PADDOC")))
		return (1);

	for (xx = xl; xx <= xh; ++xx) {
		for (yy = y-RANGE; yy <= y+RANGE; ++yy) {
			if (land_add (lname, xx, yy))
				return (0);
		}
	}

	for (xx = xxl; xx <= xxh; ++xx) {
		for (yy = yl; yy <= yh; ++yy) {
			if (land_add (lname, xx, yy))
				return (0);
		}
	}

	return (0);
}

#undef RANGE
