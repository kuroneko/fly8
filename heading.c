/* --------------------------------- heading.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the Head Up Display: heading.
*/

#include "plane.h"


extern void FAR
show_heading (HUD *h, VIEW *view, OBJECT *p, int sx, int sy, int maxx, int maxy,
	int orgx, int orgy, int ttx, int tty, int tx, int ty, int ss,
	int shifty, int VVD[2])
{
	int	hud, hud1, top, big, fine, xfine, fa18, f16, f15, fc, knots;
	int	ether, gear, scale_ref, scale_len, x, y, base, dbase;
	int	s, sl, x0, y0, dm, i, ex, ty1, ty2;
	long	t;
	ANGLE	a;
	LVECT	*R;
	OBJECT	*target;

	hud = EX->hud;
	hud1 = EX->hud1;
	fine = hud & HUD_FINE;
	xfine = hud & HUD_XFINE;
	big = hud & HUD_BIG;
	top = hud1 & HUD_TOP;
	knots = hud1 & HUD_KNOTS;
	i = hud1 & HUD_TYPES;
	fa18  = i == HUD_FA18;
	f16   = i == HUD_F16;
	f15   = i == HUD_F15;
	fc    = i == HUD_CLASSIC;
	ether = i == HUD_ETHER;
	gear  = EX->equip & EQ_GEAR;

	if (sx < 100) {
		if (xfine)
			fine = 1;
		xfine = 0;
	}

	if (f15) {
		top = !gear;
		scale_len = top ? 20 : 16;
		scale_ref = 20;
		sl = fmul (sx, F15HEADS);
	} else if (f16) {
		scale_ref = scale_len = 12;
		sl = fmul (sx, F16HEADS);
		big = 1;
	} else if (fa18) {
		scale_ref = scale_len = 16;
		sl = fmul (sx, F18HEADS);
	} else if (ether) {
		if (h->flags & HF_ETHERFRAME)
			sl = maxx - ss;		/* use full window */
		else
			sl = sx - ss;		/* use full HUD */
		scale_ref = scale_len = 25;
	} else /* classic */ {
		scale_ref = 25;
		scale_len = 16;
		sl = sx;
	}

/* This logic will show true heading (the hud will match the scenery). The
 * problem is that the hud is too narrow for comfortable reading.
*/
	if (EX->hud3 & HUD_TRUEHEADING) {
		x = muldiv (VP->maxx, sl, maxx);
		a = ATAN (x, VP->z);
		scale_ref = ANG2DEG (a);	/* HUD width in degrees */
		if (scale_ref < 1)
			scale_ref = 1;
		scale_len = scale_ref;
	}

	ty1 = -tty;
	ty2 = 2*ty1;
	if (f15) {
		base = orgy - fmul (sy, top ? F15HEADH : F15HEADL);
		dbase = base+ty2;
	} else if (f16) {
		ty1 = -ty1;
		ty2 = -ty2;

/* The heading scale follows the vv. In 'gear' mode it is at the top, but not
 * more than F16HEADTOP above the vv. In 'normal' mode it is low at
 * F16CNTR+F16HEAD but sinks away from the vv.
*/
		if (gear) {
			base = ty*(3+4);
			y = -VVD[Y] - fmul (sy, F16HEADTOP);	/* vv center */
			if (base < y)
				base = y;
			base += orgy;
			dbase = base + ty2 + ss;
		} else {
			base = fmul (sy, F16CNTR+F16HEAD);
			y = -VVD[Y] + fmul (sy, RVV) + ty2*2;	/* vv limit */
			if (base < y)
				base = y;
			dbase = base+ty2+ss;
			y = sy + shifty;		/* bottom line */
			if (dbase > y) {
				dbase = y;
				base = dbase - (ty2 + ss);
			}
			base += orgy;
			dbase += orgy;
		}
	} else if (fa18) {
		base = orgy - fmul (sy, gear ? F18HEADG : F18HEAD);
		dbase = base+ty2;
	} else if (top) {
		base = orgy - sy + shifty;
		dbase = base+ty2;
		if (big)
			dbase += ss;
	} else {
		base = orgy + sy + shifty;
		ty1 = -ty1;
		ty2 = -ty2;
		dbase = base+ty2;
		if (!big)
			dbase += ss;
	}

	gr_color (ST_HFG);

/* show aoa on left.
*/
	if (fc) {
		if (top)
			y = base + ss;
		else
			y = base;

		x = orgx - sx;
		stroke_char (x, y, 'A', ss, ST_HFG);
		stroke_frac (x+ss, y, ANG2DEG00(EX->aoa)/10,
			0, 1, ss, ST_HFG);
	}

	if (!(EX->hud2 & HUD_HEADING) || (EX->hudmode & HM_DECLUTTER))
		return;

/* Show heading scale.
*/
	a = -p->a[Z];
	if (a >= 0) {
		y0 = (int)(a % (D90/9));
		ex = 10*(int)(a / (D90/9));
	} else {
		a = -a;
		y0 = D90/9 - (int)(a % (D90/9));
		ex = -10*(int)(a / (D90/9)) - 10;
	}

	dm = num_size (9L, ss);
	if (hud&HUD_FULLHEADING)
		dm = dm * 3 / 2;

	s = muldiv (y0, 90, D90);		/* degrees */
	x0 = -y0 + muldiv (s, D90, 90);		/* fraction */
	x0 = orgx + muldiv (x0, sl, D90/90*scale_ref);
	if (f15 || (fa18 && top)) {
		x = muldiv (scale_len, sl, scale_ref);
		gr_move (orgx - x, base);
		gr_draw (orgx + x, base);
	}
	for (i = 1-scale_len, s += i; i <= scale_len; ++i, ++s) {
		x = x0 + muldiv (i, sl, scale_ref);
		if (0 == s%10) {
			gr_move (x, base);
			gr_draw (x, base+ty2);
			t = (ex + s + 360) % 360;
			if (hud&HUD_FULLHEADING)
				stroke_frac (x-dm, dbase, t, 3, 0, ss, ST_HFG);
			else if (!fc)
				stroke_frac (x-dm, dbase, t/10, 2, 0, ss,
					ST_HFG);
			else if (xfine || (i >= -11 && i <= 11)) {
				dm = num_size (t, ss);
				stroke_num (x-dm/2, dbase, t, ss, ST_HFG);
			}
		} else if (fine) {
			if (0 == s%2) {
				gr_move (x, base);
				gr_draw (x, base+ty1);
			}
		} else if (0 == s%5) {
			gr_move (x, base);
			gr_draw (x, base+(xfine?ty2:ty1));
		} else if (xfine) {
			gr_move (x, base);
			gr_draw (x, base+ty1);
		}
	}

	if (fa18||f15) {
		x = tx + tx/2;
		gr_move (orgx-x, base+2*ty);	/* inverted V */
		gr_draw (orgx,   base);
		gr_draw (orgx+x, base+2*ty);
	} else {
		y = base;
		if (f16) {
			y0 = (ty1 = -ty1);
		} else if (big) {
			y += ty2;
			if (top)
				y += ss;
			else
				y -= ss;
			y0 = ty1;
		} else
			y0 = -ty1;

		gr_move (orgx, y); /* reading mark */
		gr_draw (orgx, y+(f16?4:3)*y0);
	}

/* show ils/target heading.
*/
	if (EX->hud2 & HUD_ILS)
		R = &ils[EX->ils-1].R;
	else if (T(target = EX->target) && target->id == EX->tid)
		R = &target->R;
	else
		R = 0;

	if (R) {
		long		hx, hy;
		ANGLE		ref;

		hx = ((*R)[X] - p->R[X])/VONE;
		hy = ((*R)[Y] - p->R[Y])/VONE;
		i = ihypot2d ((int)(hx/100), (int)(hy/100));
		EX->ilsRange = knots ? fmul (i, FONE/18*10) : i;

		t = labs(hx) + labs(hy);
		while (t > 0x7fffL) {
			hx >>= 1;
			hy >>= 1;
			t  >>= 1;
		}
		a = ATAN ((int)hx, (int)hy);
		EX->ilsHeading = a;
		a = TANG(a + p->a[Z]);
		ref = DEG2ANG (scale_len);
		if (a > ref)
			a = ref;
		else if (a < -ref)
			a = -ref;

		ref = DEG2ANG (scale_ref);
		x = orgx + muldiv ((int)a, sl, (int)ref);
		y = fa18 ? -ty2 : ty1;
		gr_color (ST_HFGI);
		gr_move (x-tx, base+y);
		gr_draw (x,    base);
		gr_draw (x+tx, base+y);
	}
}
