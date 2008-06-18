/* --------------------------------- speed.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the Head Up Display: speed
*/

#include "plane.h"


extern void FAR
show_speed (HUD *h, OBJECT *p, int sx, int sy, int maxx, int maxy,
	int orgx, int orgy, int ttx, int tty, int tx, int ty, int ss,
	int shifty, int VVD[2])
{
	int	hud, hud1, big, fine, xfine, fa18, f16, f15, fc, ether, knots;
	int	cas, x, y, base, ybase, s, x0, y0, dd, dm, i, ex, v, speed;
	int	scale_ref, scale_len_l, scale_len_h, g, dx, dy = 0;
	long	t;
	char	*w;

	hud = EX->hud;
	hud1 = EX->hud1;
	fine = hud & HUD_FINE;
	xfine = hud & HUD_XFINE;
	big = hud & HUD_BIG;
	i = hud1 & HUD_TYPES;
	fa18 = i == HUD_FA18;
	f16  = i == HUD_F16;
	f15  = i == HUD_F15;
	fc  = i == HUD_CLASSIC;
	ether = i == HUD_ETHER;
	knots = hud1 & HUD_KNOTS;

	if (sx < 100) {
		if (xfine)
			fine = 1;
		xfine = 0;
	}

	dd = num_size (9L, ss);

	speed = p->speed;
	if (EX->hud2 & HUD_CALIBRATED) {
		airdata (p->R[Z], &i, 0, 0, 0);
		speed = fmul (speed, i);
		cas = 'C';
	} else
		cas = 'T';

	if (knots) {
		v = speed/2;
		v = (v + fmul (v, 15465))/(VONE/2);	/* knots */
	} else
		v = speed/VONE;				/* meter/sec */

	g = (muldiv (EX->Gforce, 100, GACC) + (EX->Gforce>0 ? 5 : -5)) / 10;
    	if (EX->maxG < g)
	    	EX->maxG = g;

	gr_color (ST_HFG);

	if (fa18 || ether) {
		s = (ss*7+3)/6;				/* the large font */
		dm = num_size (9L, s);
		dm = 4*dm + 1;
		if (h->flags & HF_ETHERFRAME) {
			i = maxx;
			ex = ETHERSPD;
		} else {
			i = sx;
			ex = F18SPD;
		}
		x = i;
		y = dm + fmul (i, ex) + dd;
		if (x > y)
			x = y;
		x = orgx - x;
		x0 = x+dm;				/* right side */

		if (!(EX->hud2 & HUD_SPEED))
			goto no_speed18;

		gr_move (x,   orgy);	/* outline */
		gr_draw (x,   orgy+s+1);
		gr_draw (x0,  orgy+s+1);
		gr_draw (x0,  orgy);
		gr_draw (x,   orgy);

		stroke_char (x-dd, orgy+s, cas, ss, ST_HFG);

		t = v;
		dm -= num_size (t, s);
		stroke_num (x+dm, orgy+s, t, s, ST_HFG);
no_speed18:
		if (EX->hud2 & HUD_DIRECTOR) {
			y = orgy + s/2;
			dx = tx * 2;
			if (EX->misc[10] > 10*VONE) {
				dy = ty * 2;
				if (EX->misc[10] > 50*VONE)
					i = ST_HFGI;
				else
					i = ST_HFG;
			} else if (EX->misc[10] < -10*VONE) {
				dy = -ty * 2;
				if (EX->misc[10] < -50*VONE)
					i = ST_HFGI;
				else
					i = ST_HFG;
			} else
				i = -1;

			if (i >= 0)
				show_trig (x0+dx+2, y, dx, dy, i);
		}

		dm = dd*3/2;
		y0 = fmul(sy, F18LINE);
		if (y0 < s)
			y0 = s;
		y = orgy+fmul(sy, F18LINE0);
		if (EX->hud2 & HUD_BETA) {
			ANGLE	beta;

			beta = p->speed ? ASIN (fdiv (p->vb[X], p->speed))
					: 0;
			stroke_char (x,    y-y0, 'B', ss, ST_HFG);
			stroke_frac (x+dm, y-y0, ANG2DEG00(beta)/10, 0, 1,
				ss, ST_HFG);
		}
		stroke_char (x, y, 'A', ss, ST_HFG);
		stroke_frac (x+dm, y, ANG2DEG00(EX->aoa)/10, 0, 1, ss,
				ST_HFG);

		y += y0;
		if (!(EX->equip & EQ_GEAR)) {
			stroke_char (x, y, 'M', ss, ST_HFG);
			t = EX->mach/10;
			stroke_frac (x+dm, y, t, 0, 2, ss, ST_HFG);
		}

		y += y0;
		stroke_char (x, y, 'G', ss, ST_HFG);
		stroke_frac (x+dm, y, (long)g, 0, 1, ss, ST_HFG);

		y += y0;
		stroke_frac (x+dm, y, EX->maxG, 0, 1, ss, ST_HFG);

		if (!ether && T(i = EX->weapon)) {
		    	y = orgy + fmul (sy, F18WEAPON);
		    	w = get_wname (EX->weapon);
		    	dm = stroke_size (w, s);

		    	y += y0;		/* weapon */
		    	stroke_str (orgx-dm/2, y, w, s, ST_HFG);

		    	y += y0;		/* rounds left */
		    	stroke_num (orgx-dm/2, y,
				(long)EX->stores[i-1], s, ST_HFG);
		}
	} else {
	    if (f15) {
		    scale_ref = 15;
		    scale_len_l = 8;
		    scale_len_h = scale_len_l;
	    } else if (f16) {
		    scale_ref = 40;
		    scale_len_l = 8;
		    scale_len_h = scale_len_l+1;
	    } else {
		    scale_ref = 30;
		    scale_len_l = EX->tapelen;
		    scale_len_h = scale_len_l;
	    }

	    if (v >= 0) {
		x0 = (int)(v % 100L);
		ex = (int)(v/100);
	    } else {
		x0 = 100-(int)((-v) % 100L);
		ex = -(int)(-v/100 + 1);
	    }
	    s = x0/10;
	    y0 = x0 - s*10;
	    ybase = get_center (p, orgy, sy, VVD);
	    y0 = ybase + muldiv (y0, sy, scale_ref*10);

	    base = orgx;
	    if (f15)
	    	base -= fmul (sx, F15SPD);
	    else if (f16)
	    	base -= fmul (sx, F16SPD);
	    else
	    	base -= sx;

	    dm = f16 ? 2*tx : 2+tx;

	    if (!(EX->hud2 & HUD_SPEED))
		goto no_speed16;

/* draw scale line.
*/
	    if (f15) {
		y = muldiv (sy, scale_len_h, scale_ref);
		gr_move (base, ybase - y);		/* top */
		y = muldiv (sy, scale_len_l, scale_ref);
		gr_draw (base, ybase + y);		/* bottom */
	    }

	    for (i = 1-scale_len_l, s += i; i <= scale_len_h; ++i, ++s) {
		y = y0 - muldiv (i, sy, scale_ref);
		if (f16||f15) {
			if (f15)
				y = 2*ybase - y;
			if (0 == s%5) {
				t = ex*10 + s;
				if (f15)
					t *= 10;
				if (!big)
					dm = -num_size (t, ss)-2*tx;
				stroke_num (base+dm, y+ss/2, t, ss, ST_HFG);
				gr_move (base,       y);
				gr_draw (base-2*ttx, y);
			} else {
				gr_move (base,       y);
				gr_draw (base-1*ttx, y);
			}
		} else if (0 == s%10) {
			gr_move (base, y);
			gr_draw (base -3*ttx, y);
			if (xfine || (i >= -11 && i <= 11)) {
				t = ex + s/10;
				if (!big)
					dm = -num_size (t, ss)-tx;
				stroke_num (base+dm, y-2, t,
					ss, ST_HFG);
			}
		} else if (fine) {
			if (0 == s%2) {
				gr_move (base, y);
				gr_draw (base-1*ttx, y);
			}
		} else if (0 == s%5) {
			gr_move (base, y);
			gr_draw (base-2*ttx, y);
		} else if (xfine) {
			gr_move (base, y);
			gr_draw (base-1*ttx, y);
		}
	    }

/* reading mark
*/
	    if (big)
		base += 3*tx;
	    if (f15) {
		    gr_move (base+2*tx, ybase-ty);
		    gr_draw (base,      ybase);
		    gr_draw (base+2*tx, ybase+ty);
		    stroke_char (base+2*tx, ybase+ss/2, cas, ss, ST_HFG);
	    } else {
		    gr_move (base,      ybase);
		    gr_draw (base+(f16?4:3)*tx, ybase);
		    stroke_char (base+tx, ybase-1, cas, ss, ST_HFG);
	    }
no_speed16:
	    while (f16) {
		y = ybase - muldiv (sy, scale_len_h, scale_ref) - (ss>>1);
		x = base;
		if (g >= 0 && g < 100)
			x += dd;
		stroke_frac (x, y, (long)g, 0, 1, ss, ST_HFG);

		y = ybase + muldiv (sy, scale_len_l, scale_ref) - (ss>>1);
		y0 = orgy+shifty+sy;		/* HUD bottom */

		y += ss;
		if (y > y0)
			break;
		stroke_str (base+dd, y, get_wname (EX->weapon), ss, ST_HFG);

		y += ss;
		if (y > y0)
			break;
		t = EX->mach/10;
		stroke_frac (base+dd, y, t, 0, 2, ss, ST_HFG);

		y += ss;
		if (y > y0)
			break;
		t = EX->maxG;
		x = base-2*dd;
		if (EX->maxG >= 100)
			x -= dd;
		stroke_frac (x, y, t, 0, 1, ss, ST_HFG);
		if (EX->weapon) {
			t = (long)EX->stores[EX->weapon-1];
			x = base + 4*dd - num_size (t, ss);
			if (x < base)
				x = base;
			stroke_num (x, y, t, ss, ST_HFG);
		}

		y += ss;
		if (y > y0)
			break;
		if (WE_M61 == EX->weapon)
			w = EX->target ? "LCOS" : "DGFT";
		else if (WE_MK82 == EX->weapon)
			w = "CCIP";
		else if (EX->weapon)
			w = "XXXX";
		else if (EX->hud2 & HUD_ILS)
			w = "ILS";
		else
			w = "NAV";
		stroke_str (base-2*dd, y, w, ss, ST_HFG);

		break;
	    }
	    while (f15||fc) {
		if (fc)
			base += 3*dd;
		y = ybase + muldiv (sy, scale_len_l, scale_ref) + ss;
		y0 = orgy+shifty+sy;		/* HUD bottom */

		y += ss;
		if (y > y0)
			break;
		stroke_str (base-3*dd, y, get_wname (EX->weapon), ss, ST_HFG);

		y += ss;
		if (y > y0)
			break;
		t = EX->mach;
		x = base + 2*dd - frac_size (t, -1, 3, ss);
		stroke_frac (x, y, t, -1, 3, ss, ST_HFG);

		y += ss;
		if (y > y0)
			break;
		t = g;
		x = base - frac_size (t, 0, 1, ss);
		stroke_frac (x, y, t, 0, 1, ss, ST_HFG);
		stroke_char (base+dd, y, 'G', ss, ST_HFG);

		y += ss;
		if (y > y0)
			break;
		if (EX->weapon) {
			t = (long)EX->stores[EX->weapon-1];
			x = base - num_size (t, ss);
			stroke_num (x, y, t, ss, ST_HFG);
		}

		break;
	    }
	}
}
