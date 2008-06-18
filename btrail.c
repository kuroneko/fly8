/* --------------------------------- btrail.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the bullet trail history on the Head Up Display.
*/

#include "plane.h"


/* Show bullets trail history.
 *
 * -d0 and P[] is the previous point distance and position.
 * -d1 and Q[] is the currect  point distance and position.
 * -d  and R[] is the needed   point distance and position.
 * -d takes the values of the displayed points, which start at the closest
 * (BULMIN) and end at the farthest (BULMAX) in equal steps (BULSEG). The
 * points some intervals (BULSTEP) are marked with a small bar while the
 * target distance (ddd) is marked with a tiny bead.
 * -R[] is interpolated from P[] and Q[].
*/

#define BULSEG	(int)(750/3.28)		/* segment size (meters) */
#define BULSTEP	(2*BULSEG)		/* marked step size */
#define BULMIN	(BULSEG/2)		/* nearest point shown */
#define BULMAX	(3*BULSTEP)		/* farthest point shown */

extern void FAR
show_trail (HUD *h, VIEW *view, OBJECT *p)
{
	OBJECT		*target;
	BTRAIL		*h1, *h2;
	VECT		P, Q, R, V;
	int		D1[2], D2[2];
	int		d, n, d0, d1, dd, dd0, dd1, ddd;
	int		t, off, tx, ty, first;

	if (!(EX->hud2 & HUD_BTRAIL))
		return;

	if (T(target = EX->target)) {
		ddd = (int)(ldist3d (p->R, target->R)/VONE);
		if (ddd > BULMAX)
			ddd = BULMAX;
	} else
		ddd = BULMAX;
	tx = fmul (h->sx, F16RBUL);
	ty = fmul (h->sy, F16RBUL);
	D1[X] = h->orgx;
	D1[Y] = h->orgy;
	off = 0;
	if (ddd < BULSEG) {
		n = BULSEG;
		d = ddd;
	} else {
		d = BULSEG;
		n = d + BULSEG;
	}
	gr_color (ST_HFG);
	first = 1;
	for (h2 = 0, h1 = EX->btrail; h1; h2 = h1, h1 = h1->next) {

/* find first point with a distance (d1) >= the desired segment (d).
*/
		if ((d1 = (int)(ldist3d (p->R, h1->R)/VONE)) < d)
			continue;
		t = 1 + d1/1000;
		if (h2) {
			d0 = (int)(ldist3d (p->R, h2->R)/VONE);
			P[X] = (int)((h2->R[X] - p->R[X])/t);
			P[Y] = (int)((h2->R[Y] - p->R[Y])/t);
			P[Z] = (int)((h2->R[Z] - p->R[Z])/t);
		} else {
			d0 = 0;
			P[X] = P[Y] = P[Z] = 0;
		}
		Q[X] = (int)((h1->R[X] - p->R[X])/t);
		Q[Y] = (int)((h1->R[Y] - p->R[Y])/t);
		Q[Z] = (int)((h1->R[Z] - p->R[Z])/t);
l1:
		dd  = d1 - d0;			/* segment size */
		dd0 = d  - d0;			/* leading part */
		dd1 = d1 - d;			/* trailing part */

		if (d1 <= d0 || d < d0 || d1 < d)
			break;
		R[X] = muldiv (Q[X], dd0, dd) + muldiv (P[X], dd1, dd);
		R[Y] = muldiv (Q[Y], dd0, dd) + muldiv (P[Y], dd1, dd);
		R[Z] = muldiv (Q[Z], dd0, dd) + muldiv (P[Z], dd1, dd);

		VxMmul (V, R, p->T);
		screen_coords (view, V);
		off = clip_to_screen (D2, V, h->maxx, h->maxy, h->clipx-tx,
			h->clipy-ty, h->shifty);
		if (first && d > BULMIN) {
			D1[X] = muldiv (D2[X], BULMIN, d);
			D1[Y] = muldiv (D2[Y], BULMIN, d);
			D1[X] = h->orgx + D1[X];
			D1[Y] = h->orgy - D1[Y];
		}
		D2[X] = h->orgx + D2[X];
		D2[Y] = h->orgy - D2[Y];
		add_segment (D1[X], D1[Y], D2[X], D2[Y],
			h->orgx, h->orgy, h->clipx, h->clipy, h->shifty);
		if (off)
			break;
		if (ddd == d) {
			gr_ellipse (D2[X], D2[Y], tx, ty);
		} else if (!(d%BULSTEP)) {
			gr_move (D2[X]-tx, D2[Y]);
			gr_draw (D2[X]+tx, D2[Y]);
		}
		D1[X] = D2[X];
		D1[Y] = D2[Y];

/* advance 'd' to the next point.
*/
		if (d >= BULMAX)
			break;

		if (d < ddd && ddd < n)
			d = ddd;
		else {
			d = n;
			n += BULSEG;
		}
		first = 0;
		if (d1 >= d)
			goto l1;	/* point inside the same segment */
	}
}
#undef BULSEG
#undef BULSTEP
#undef BULMIN
#undef BULMAX
