/* --------------------------------- waypoint.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint waypoint stuff on the Head Up Display.
*/

#include "plane.h"


extern void FAR
show_waypoint (HUD *h, VIEW *view, OBJECT *p)
{
	LVECT	*R;
	OBJECT	*target;
	VECT	RR;
	int	D[2];
	int	ind, out, x, y, tx, ty, rx, ry;
	long	hx, hy, t;
	ANGLE	a;

	if (!(EX->hud2 & HUD_WAYPOINT))
		return;

	if (T(target = EX->target) && target->id == EX->tid) {
		R = &target->R;
		ind = 0;
	} else {
		ind = EX->ils;
		if (ind < 1)
			ind = 1;
		R = &ils[ind-1].R;
		ind = 1;
	}

/* Object highlighting diamond.
*/
	if (ind) {
		tx = (h->dd+1)/2;
		ty = (muldiv (h->dd, h->sy, h->sx)+1)/2;

		objects_show (1, view, p, *R, RR);
		if (h->flags & HF_ETHER) {
			x = h->maxx;
			y = h->maxy;
		} else {
			x = h->sx;
			y = h->sy;
		}
		out = clip_to_screen (D, RR, h->maxx, h->maxy, x-tx,
				y-ty, h->shifty);
		if (out && h->flags & HF_ETHER) {
			clip_to_ether (h, D, x-tx, y-ty);
			tx = h->ethertx;
			ty = h->etherty;
			out = 0;
		}
		D[X] = h->orgx + D[X];
		D[Y] = h->orgy - D[Y];
		show_diamond (D[X], D[Y], tx, ty, ST_HFG, out);
	}

/* Object position pointer.
*/
	hx = ((*R)[X] - p->R[X])/VONE;
	hy = ((*R)[Y] - p->R[Y])/VONE;
	t = labs(hx) + labs(hy);
	while (t > 0x7fffL) {
		hx >>= 1;
		hy >>= 1;
		t  >>= 1;
	}
	a = TANG(ATAN ((int)hx, (int)hy) + p->a[Z]);

	tx = EX->ldstep + 2*EX->hudFontSize;
	x = fmul (h->sx, tx);			/* pointer position */
	y = fmul (h->sy, tx);
	if (a < -DEG(5)) {
		x = -x;
		y = -y;
	} else if (a < DEG(5)) {
		x = muldiv (x, a, DEG(5));
		y = muldiv (y, a, DEG(5));
	}
	x = fmul (x, p->cosy);
	y = fmul (y, p->siny);
	x = h->orgx + h->VV[X] + x;
	y = h->orgy - h->VV[Y] - y;

	tx = 4*h->tx;				/* pointer size */
	ty = 4*h->ty;
	rx = tx/4;				/* circle radius */
	ry = ty/4;
	tx = fmul (tx, SIN(a));
	ty = fmul (ty, COS(a));

#if 0
	if (is_in (h, x, y, rx, ry) && is_in (h, x+tx, y-ty, 0, 0))
#else
	if (x > h->right-rx)
		x = h->right-rx;
	else if (x < h->left+rx)
		x = h->left+rx;
	if (y < h->top+ry)
		y = h->top+ry;
	else if (y > h->bottom-ry)
		y = h->bottom-ry;

	if (x > h->right-tx)
		x = h->right-tx;
	else if (x < h->left-tx)
		x = h->left-tx;
	if (y < h->top+ty)
		y = h->top+ty;
	else if (y > h->bottom+ty)
		y = h->bottom+ty;
#endif
		show_ptr (x, y, rx, ry, tx, ty, ST_HFG,
			(EX->hud1 & HUD_TYPES) != HUD_CLASSIC);
}
