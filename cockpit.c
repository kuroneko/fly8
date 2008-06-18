/* --------------------------------- cockpit.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Render instruments etc.
*/

#include "plane.h"


#define	IFG	ST_HFG		/* instruments color */
#define	MFG	ST_HFGI		/* instruments marker color */

extern void FAR
show_inst (VIEW *view, OBJECT *obj)
{
	int	orgx, orgy, maxx, maxy, sizex, sizey, sizexx, tx, ty, v, x, y;
	int	spitch, croll, sroll, sdir, cdir;

	if (!obj || !IS_PLANE(obj))
		return;

/* Set screen area and adjust aspect ratio.
*/
	get_area (view, &orgx, &orgy, &maxx, &maxy);
	get_square (view, maxx, maxy, &sizex, &sizey);

	orgx += maxx;				/* lower right corner */
	orgy += maxy;
	sizex /= 8;
	sizey /= 4;
	orgx -= sizex;
	orgy -= sizey;
	tx = sizex/8;
	ty = sizey/8;

	gr_color (IFG);

	gr_move (orgx + sizex, orgy + sizey);	/* outline */
	gr_draw (orgx + sizex, orgy - sizey);
	gr_draw (orgx - sizex, orgy - sizey);
	gr_draw (orgx - sizex, orgy + sizey);
	gr_draw (orgx + sizex, orgy + sizey);

	sizexx = sizex - tx;
	gr_move (orgx - sizex,  orgy);		/* ticks */
	gr_draw (orgx - sizexx, orgy);
	gr_move (orgx + sizex,  orgy);
	gr_draw (orgx + sizexx, orgy);

	gr_color (MFG);

	v = muldiv (EE(obj)->rudder, sizex, 90);	/* rudder */
	gr_move (orgx + v, orgy + sizey);
	gr_draw (orgx + v, orgy + sizey - ty);

	v = muldiv (EE(obj)->throttle, sizey, 100);	/* throttle */
	gr_move (orgx + sizex,  orgy - v);
	gr_draw (orgx + sizexx, orgy - v);

#define	VLIMIT	VONE*300
	v = obj->V[Z];					/* vz */
	if (v > VLIMIT)
		v = VLIMIT;
	if (v < -VLIMIT)
		v = -VLIMIT;
	v = muldiv (v, sizey, VLIMIT);
#undef	VLIMIT

	x = orgx - sizex;
	y = orgy - v;
	gr_move (x - tx, y - ty);
	gr_draw (x + tx, y + ty);
	gr_move (x - tx, y + ty);
	gr_draw (x + tx, y - ty);

	sroll  = obj->siny;			/* plane */
	croll  = obj->cosy;
	spitch = obj->a[X];
	spitch = fmul (spitch, sizey)>>1;

	x = fmul (croll, sizexx);
	y = fmul (sroll, sizey)>>1;

	orgy  -= spitch;

	gr_color (IFG);

	gr_move (orgx - x, orgy - y);
	gr_draw (orgx + x, orgy + y);

	x = fmul (sroll, sizexx)/4;
	y = fmul (croll, sizey)/8;

	gr_move (orgx,     orgy);
	gr_draw (orgx + x, orgy - y);
	orgy += spitch;
	orgx -= 2*sizex;

/* direction
*/
	x = sizex;
	y = sizey>>1;

	orgy += y;

	sdir  = obj->sinz;
	sdir  = fmul (sdir,  sizexx);
	cdir  = obj->cosz;
	cdir  = fmul (cdir,  y);

	gr_move (orgx + x, orgy + y);		/* outline */
	gr_draw (orgx - x, orgy + y);
	gr_draw (orgx - x, orgy - y);
	gr_draw (orgx + x, orgy - y);

	gr_move (orgx, orgy);
	gr_draw (orgx - sdir, orgy - cdir);	/* hand */
	orgx -= 2*sizex;

/* speed
*/

#define	MAXSPEED	(VONE*600)
	x = obj->speed;
	if (x > MAXSPEED)
		x = MAXSPEED;
	else if (x < 0)
		x = 0;
	x = muldiv (x, 2*sizex, MAXSPEED);
	y = sizey>>1;
#undef MAXSPEED
	gr_move (orgx + sizex, orgy - y);	/* scale */
	gr_draw (orgx - sizex, orgy - y);
	gr_move (orgx,             orgy + y);	/* hand */
	gr_draw (orgx - sizex + x, orgy - y);
}

#undef IFG
#undef MFG
