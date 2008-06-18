/* --------------------------------- altitude.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the Head Up Display: altitude
*/

#include "plane.h"


extern void FAR
show_altitude (HUD *h, OBJECT *p, int sx, int sy, int maxx, int maxy, int orgx,
	int orgy, int ttx, int tty, int tx, int ty, int ss, int shifty,
	int VVD[2])
{
	int	hud, hud1, big, fine, xfine, fa18, f16, f15, fcl, knots;
	int	ether;
	int	x, y, base, ybase, s, x0, y0, dm, dd, i, ex;
	int	res, unit, vtick;
	int	scale_ref, scale_len_l, scale_len_h, savedec;
	long	t, alt;

	hud = EX->hud;
	hud1 = EX->hud1;
	fine = hud & HUD_FINE;
	xfine = hud & HUD_XFINE;
	big = hud & HUD_BIG;
	i = hud1 & HUD_TYPES;
	fa18 = i == HUD_FA18;
	f16  = i == HUD_F16;
	f15  = i == HUD_F15;
	fcl  = i == HUD_CLASSIC;
	ether = i == HUD_ETHER;
	knots = hud1 & HUD_KNOTS;

	if (sx < 100) {
		if (xfine)
			fine = 1;
		xfine = 0;
	}

	dd = num_size (9L, ss);

	alt = p->R[Z];				/* altitude */
	if (knots)
		alt = alt*328/100/VONE;		/* feet */
	else
		alt /= VONE;			/* meters */

	gr_color (ST_HFG);

	if (fa18 || ether) {
		if (!(EX->hud2 & HUD_ALTITUDE))
			return;

		s = (ss*7+3)/6;			/* the large font */
		x0 = num_size (9L, s);
		y0 = 3*dd + 2*x0 + 1;
		if (h->flags & HF_ETHERFRAME) {
			i = maxx;
			ex = ETHERALT;
		} else {
			i = sx;
			ex = F18ALT;
		}
		x = i-tx;
		y = y0 + fmul (i, ex);
		if (x > y)
			x = y;
		x += orgx;
		gr_move (x,     orgy);		/* outline */
		gr_draw (x,     orgy+s+1);
		gr_draw (x-y0,  orgy+s+1);
		gr_draw (x-y0,  orgy);
		gr_draw (x,     orgy);

		y = orgy + s;
		i = (int)(labs (alt) % 1000);
		t = alt / 1000;
		if (t == 0) {
			dm = num_size ((long)i, s);
			stroke_num (x-dm, y, (long)i, s, ST_HFG);
		} else {
			x0 = x - 3*dd;
			dm = num_size (t, s);
			stroke_num (x0-dm, y, t, s, ST_HFG);
			y -= (s-ss)/2;		/* center the digits */
			stroke_frac (x0, y, (long)i, 3, 0, ss, ST_HFG);
		}

		y = orgy - ss/2;
		t = p->V[Z]*60L;			/* vertical speed */
		if (knots)
			t = t*328/100/VONE;		/* feet */
		else
			t /= VONE;
		t = t/10*10;				/* round to 10s */
		dm = num_size (t, ss);
		stroke_num (x-dm, y, t, ss, ST_HFG);
	} else {
	    int		frac;

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

	    if (f15) {
		if (EX->equip & EQ_GEAR) {
			res   = 20;		/* res*100*VONE is wraparound */
			unit  = 20;		/* tick number multiplier */
			vtick = 5*VONE;		/* v. velocity per tick */
		} else {
			res   = 100;
			unit  = 100;
			vtick = 100*VONE;
		}
		frac = 0;
	    } else if (EX->equip & EQ_GEAR) {
		vtick = 10*VONE;
	    	if (f16) {
			res   = 20;
			unit  = -5;
			frac  = 1;
	    	} else {
			res   = 10;
			unit  = 100;
			frac  = 0;
	    	}
	    } else {
		res   = 100;
		unit  = 1;
		vtick = 100*VONE;
		frac  = 1;
	    }

	    if (alt >= 0) {
		x0 = (int)(alt % (100L*res));
		ex = (int)(alt/(100L*res));
	    } else {
		x0 = 100*res-(int)((-alt) % (100L*res));
		ex = -(int)(-alt/(100L*res) + 1);
	    }
	    s = x0/res;
	    y0 = x0 - s*res;
	    ybase = get_center (p, orgy, sy, VVD);
	    y0 = ybase + muldiv (y0, sy, scale_ref*res);

	    base = orgx;
	    if (f15)
	    	base += fmul (sx, F15ALT);
	    else if (f16)
	    	base += fmul (sx, F16ALT);
	    else
		base += sx;

	    dm = (f15||f16) ? 2+2*tx : 2+tx;

/* aoa at top of scale
*/
	    if (f16||f15) {
		t = ANG2DEG00(EX->aoa)/10;
		x = base + (f15 ? -dd : 4*dd);		/* right margin */
		x -= frac_size (t, 0, 1, ss);
		y = muldiv (sy, scale_len_h, scale_ref);
		y += f15 ? -ss/2 : ss;
		stroke_frac (x, ybase-y, t, 0, 1, ss, ST_HFG);
	    }

	    if (!(EX->hud2 & HUD_ALTITUDE))
		return;

/* draw scale line.
*/
	    if (f15) {
		y = muldiv (sy, scale_len_h, scale_ref);
		gr_move (base, ybase - y);		/* top */
		y = muldiv (sy, scale_len_l, scale_ref);
		gr_draw (base, ybase + y);		/* bottom */
	    }

/* The scale is drawn bottom to top.
*/
	    savedec = f16 ? stroke_decimal (',') : -1;
	    for (i = 1-scale_len_l, s += i; i <= scale_len_h; ++i, ++s) {
		y = y0 - muldiv (sy, i, scale_ref);
		if (f15||f16) {
			if (0 == s%5) {
				if (unit >= 0)
					t = (ex*100L + s)*unit;
				else
					t = (ex*100L + s)/(-unit);
				if (big)
					dm = -num_size (t, ss)-2*tx;
				stroke_frac (base+dm, y+ss/2, t,
					(f16 && t >= 0 && t < 1000) ? 3 : 0,
					frac, ss, ST_HFG);
				gr_move (base, y);
				gr_draw (base+2*ttx, y);
			} else {
				gr_move (base, y);
				gr_draw (base+1*ttx, y);
			}
		} else if (0 == s%10) {
			gr_move (base, y);
			gr_draw (base+3*ttx, y);
			if (xfine || (i >= -11 && i <= 11)) {
				t = (ex*10 + s/10)*unit;
				if (big)
					dm = -num_size (t, ss)-tx;
				stroke_num (base+dm, y-2, t, ss, ST_HFG);
			}
		} else if (fine) {
			if (0 == s%2) {
				gr_move (base, y);
				gr_draw (base+1*ttx, y);
			}
		} else if (0 == s%5) {
			gr_move (base, y);
			gr_draw (base+2*ttx, y);
		} else if (xfine) {
			gr_move (base, y);
			gr_draw (base+1*ttx, y);
		}
	    }
	    if (savedec >= 0)
		    stroke_decimal (savedec);

	    if (big)
		base -= 3*tx;
	    if (f15) {
		    gr_move (base-2*tx, ybase-ty); /* reading mark */
		    gr_draw (base,      ybase);
		    gr_draw (base-2*tx, ybase+ty);
	    } else {
		    gr_move (base,      ybase); /* reading mark */
		    gr_draw (base-(f16?4:3)*tx, ybase);
	    }
	    if (fcl) {

/* show vertical velocity beside the scale
*/
		unit = vtick*5;			/* full scale range */
		s = p->V[Z];			/* vz */
		if (knots)
			s = 3*s + fmul (s, 4601);	/* feet */
		if (s > unit)
			s = unit;
		if (s < -unit)
			s = -unit;
		s = muldiv (sy, s, unit+vtick);
		gr_move (base-tx, ybase);
		gr_draw (base-tx, ybase-s);

		y0 = muldiv (sy, vtick, unit+vtick);
		if (s > 0) {
			for (y = 0; y <= s; y += y0) {
				gr_move (base-tx, ybase-y);
				gr_draw (base,    ybase-y);
			}
		} else if (s < 0) {
			for (y = 0; y >= s; y -= y0) {
				gr_move (base-tx, ybase-y);
				gr_draw (base,    ybase-y);
			}
		}
	    }

	    while (f16) {
		OBJECT	*target;
		LVECT	LL;
		long	range;
		int	closure, i, xx;

		y = ybase + muldiv (sy, scale_len_l, scale_ref) + ss/2;
		y0 = orgy+shifty+sy;		/* HUD bottom */
		x = base - 4*dd;

		y += ss + ss/2;		/* R data not implemented yet */
		if (y > y0)
			break;
		stroke_str (x, y, "R ", ss, ST_HFG);

		y += ss + ss/2;
		if (y > y0)
			break;
		xx = x;
		xx += stroke_str (xx, y, "AL ", ss, ST_HFG);
		savedec = stroke_decimal (',');
		stroke_frac (xx, y, alt/10, 4, 2, ss, ST_HFG);
		stroke_decimal (savedec);

		if (F(target = EX->target)) {
			y += 2*ss;
			goto no_target;
		}
		Vsub (LL, p->R, target->R);
		range = lhypot3d (LL);
		if (range)
			closure = (int)(-(LL[X]*(p->V[X] - EX->tspeed[X]) +
					  LL[Y]*(p->V[Y] - EX->tspeed[Y]) +
					  LL[Z]*(p->V[Z] - EX->tspeed[Z])
					 ) / (range*VONE));
		else
			closure = 0;

		y += ss;
		if (y > y0)
			break;
		if (knots) {
			if (range < 1853L*VONE) {
				i = (int)(range/VONE);
				t = fmul (i, FCON(.0328));	/* ft*100 */
				stroke_frac (x+2*dd, y, t, 3, 0, ss, ST_HFG);
			} else {
				t = range*10L/(VONE*1853L);	/* mi/10 */
				stroke_frac (x, y, t, 4, 1, ss, ST_HFG);
			}
		} else {
			if (range < 1000L*VONE) {
				t = range/10/VONE;
				stroke_frac (x+2*dd, y, t, 3, 0, ss, ST_HFG);
			} else {
				t = range/100/VONE;
				stroke_frac (x, y, t, 4, 1, ss, ST_HFG);
			}
		}

		y += ss;
		if (y > y0)
			break;
		if (knots)
			t = fmul (closure, FCON(3.6/1.853));	/* knots */
		else
			t = fmul (closure, FCON(3.6/2))*2;	/* Km/h */
		stroke_frac (x + (t<0 ? 0 : dd), y, t, 4, 0, ss, ST_HFG);
no_target:
		y += ss;
		if (y > y0)
			break;
		xx = x;

		i = EX->ils - 1;
		if (i < 0)
			i = p->home;
		range = ldist3d (p->R, ils[i].R);
		if (knots)
			t = range/VONE/1853;		/* kts */
		else
			t = range/VONE/1000;		/* km */

		xx += stroke_frac (xx, y, t, 3, 0, ss, ST_HFG);
		xx += stroke_char (xx, y, '>', ss, ST_HFG);
		stroke_frac (xx, y, (long)i, 2, 0, ss, ST_HFG);

		break;
	    }
	}
}
