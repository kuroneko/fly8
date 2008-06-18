/* --------------------------------- compass.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* display a compass on the edge of this window.
*/

#include "plane.h"


extern void FAR
show_compass (HUD *h, OBJECT *p, int mode)
{
	int	i, j, tx, ty, x0, y0, x1, y1, x2, y2, dd, dd2, mx, my;
	int	sqr, ortho;
	int	D[2];
	VECT	RR;
	ANGLE	a;

	if (!(HDD_COMPASS & EX->hdd))
		return;

	if (HDT_MAP == mode)
		a = 0;
	else if (HDT_RADAR == mode)
		a = -p->a[Z];
	else
		return;

	RR[Y] = -FONE;
	tx = num_size (9L, h->ss)+1;
	ty = h->ss/2+1;

	my = h->ss;
	mx = muldiv (my, h->sx, h->sy);

	sqr = HDD_SQRCOMPASS & EX->hdd;
	ortho = sqr && (HDD_ORTCOMPASS & EX->hdd);
	a = -a;
	for (i = 0; i < 36; i += 1, a += DEG(10)) {
		RR[X] = SIN(a);
		RR[Z] = COS(a);
		if (sqr)
			clip_to_screen (D, RR, h->maxx, h->maxy, h->maxx,
				h->maxy, 0);
		else {
			D[X] = fmul (RR[X], h->maxx);
			D[Y] = fmul (RR[Z], h->maxy);
		}
		x0 = h->orgx+D[X];
		y0 = h->orgy-D[Y];
		j = i%3;
		if (i >= 10 ) {
			dd2 = tx;
			dd  = tx*2;
		} else {
			dd2 = tx/2;
			dd  = tx;
		}
		if (ortho) {
			if (D[X] == h->maxx) {		/* right */
				x1 = mx;
				x2 = dd;
				y1 = 0;
				y2 = ty;
			} else if (D[X] == -h->maxx) {	/* left */
				x1 = -mx;
				x2 = -2;
				y1 = 0;
				y2 = ty;
			} else if (D[Y] == h->maxy) {	/* top */
				x1 = 0;
				x2 = dd2;
				y1 = my;
				y2 = h->ss+2;
			} else if (D[Y] == -h->maxy) {	/* bottom */
				x1 = 0;
				x2 = dd2;
				y1 = -my;
				y2 = -2;
			} else			/* should never reach here */
				continue;
		} else {
			x1 = muldiv (D[X], mx, h->maxx);
			y1 = muldiv (D[Y], my, h->maxy);
			x2 = fmul (RR[X], dd2) + dd2;
			y2 = fmul (RR[Z],  ty) +  ty;
		}
		if (j) {
			x1 /= 2;
			y1 /= 2;
		}
		gr_color (ST_COMPASS);
		gr_move (x0,    y0);
		gr_draw (x0-x1, y0+y1);
		if (!j) {
			x0 -= x1+x2;
			y0 += y1+y2;
			if (ortho) {
				x1 = h->maxx-mx;
				y1 = h->maxy-my;
				if (y0 >= h->orgy+y1)
					y0 = h->orgy+y1;
				else if (y0-h->ss < h->orgy-y1)
					y0 = h->orgy-y1+h->ss;
				else if (x0+dd > h->orgx+x1)
					x0 = h->orgx+x1-dd;
				else if (x0 <= h->orgx-x1)
					x0 = h->orgx-x1+1;
			}
			stroke_num (x0, y0, i, h->ss, ST_COMPASS);
		}
	}
}
