/* --------------------------------- pitch.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the Head Up Display: pitch ladder
*/

#include "plane.h"


/* Pendulum ticks:	     5      15     30     45 */
static int	F18Psin[] = {1428,  4240,  8192,  11585};
static int	F18Pcos[] = {16322, 15827, 14189, 11585};

/* Pendulum ticks:	     10     20     30     45 */
static int	F16Psin[] = {2845,  5604,  8192,  11585};
static int	F16Pcos[] = {16135, 15396, 14189, 11585};


/* Get pull-up cue data.
*/

#define SAFETY	200

extern void FAR
get_cue (OBJECT *p)
{
	OBJECT	*target;
	ANGLE	a;
	int	tt;
	long	hh;
	Ulong	dt;
	LVECT	LL;

	tt = p->speed / VONE * 5;
	hh = p->R[Z]/VONE;
	if (tt <= 0 || hh < 0) {
		a = DEG (90);
		tt = FONE;
	} else if (WE_MK82 == EX->weapon &&
	   T(target = EX->target) && target->id == EX->tid) {
		Vsub (LL, target->R, p->R);
		dt = lhypot3d (LL) / VONE;
		if (dt <= SAFETY) {
			a = DEG (90);
			tt = 0;
		} else if (tt < (int)(dt-SAFETY))
			goto std_cue;
		else {
			a  = ACOS (fdiv (iabs ((int)(LL[Z]/VONE)), (int)dt));
			a += ASIN (fdiv (SAFETY, (int)dt));
			a -= DEG (90);
			if (a > p->a[X])
				tt = fdiv ((int)(dt-SAFETY), tt);
			else
				tt = FONE;
		}
	} else {
std_cue: ;
		if (hh >= tt) {
			a = DEG (-90);
			tt = FONE;
		} else {
			a = -ASIN (fdiv ((int)hh, tt));
			tt = -p->V[Z] / VONE * 5;
			if (tt > (int)hh)
				tt = fdiv ((int)hh, tt);
			else
				tt = FONE;
		}
	}
	a = TANG(a - p->a[X]);
	EX->misc[16] = a;
	EX->misc[17] = tt;
}
#undef SAFETY

extern void FAR
show_pitch (HUD *h, VIEW *view, OBJECT *p, int sx, int sy, int maxx, int maxy,
	int orgx, int orgy, int ttx, int tty, int tx, int ty, int ss,
	int shifty, int type, int VVD[2])
{
	int	hud, hud1, hudtype, f16, color, ladder, funnel;
	int	x, y, x0, y0, ex, tt;
	int	i, j, xt, yt, dx, dy, dd, rev;
	int	sroll, croll, pitch, xroll, yroll, xnum, ynum, xslant, yslant;
	int	xgap, ygap, xtip, ytip, l, px, py, n, nn, ldegrees;
	int	dx0, dy0, dx1, dy1;
	ANGLE	langles, hudarea, a;
	VECT	RR;

	hud = EX->hud;
	hud1 = EX->hud1;
	hudtype = hud1 & HUD_TYPES;
	f16  = hudtype == HUD_F16;
	ladder = EX->ladder;
	funnel = T(ladder & LD_FUNNEL);

	sroll = p->siny;
	croll = p->cosy;

/* Select ladder center (vv or waterline).
*/
	if (!(EX->hud & HUD_VV) || (ladder & LD_FIXED) || p->vb[Y] < 2*VONE)
		px = py = 0;
	else {
		get_vv (p, RR);
		tt = fmul (RR[X], croll) + fmul (RR[Z], sroll);
		tt = muldiv (tt, VVPERIOD-VVDELAY, VVPERIOD);
		RR[X] = fmul (tt, croll);
		RR[Z] = -fmul (tt, sroll);
		screen_coords (view, RR);

		if ((Uint)RR[Y] <= iabs (RR[X])/2 ||
		    (Uint)RR[Y] <= iabs (RR[Z])/2)
			goto noladder;
		px = muldiv(RR[X], maxx, RR[Y]);
		py = muldiv(RR[Z], maxy, RR[Y]);
	}

/* Choose ladder pitch.
*/
	if (HDT_HUD == type) {
		hudarea = DEG2ANG (EX->hudarea);
		y = muldiv (view->viewport->z, SIN (hudarea), COS (hudarea));
		y += fmul (y, EX->hudshift);
		if (y > view->viewport->maxy) {
			y = fmul (view->viewport->z, FONE + EX->hudshift);
			hudarea = ATAN (view->viewport->maxy, y);
		}
		langles = hudarea;
	} else {
		y = muldiv (view->viewport->maxy, sy, maxy);
		langles = ATAN (y, view->viewport->z);
		hudarea = 0;	/* avoid compiler warning */
	}
	ldegrees = ANG2DEG00 (langles);

#if 0
	if (EX->hudmode & HM_DECLUTTER)
		ldegrees *= 2;		/* sparse ladder only */
	if (ldegrees < 300)
		ldegrees = 1;		/* degrees per step */
	else if (ldegrees < 500) 	/* must be a factor of 90 */
		ldegrees = 2;
	else if (ldegrees < 1500)
		ldegrees = 5;
	else if (ldegrees < 2300)
		ldegrees = 10;
	else if (ldegrees < 4500)
		ldegrees = 15;
	else
		ldegrees = 30;
#else
	if (EX->hudmode & HM_DECLUTTER)
		ldegrees = fmul (ldegrees, FCON(1/1.25)); /* sparse ladder */
	else
		ldegrees = fmul (ldegrees, FCON(1/2.50)); /* full ladder */
	if (ldegrees < 100)
		ldegrees = 1;		/* degrees per step */
	else if (ldegrees < 200) 	/* must be a factor of 90 */
		ldegrees = 2;
	else if (ldegrees < 500)
		ldegrees = 5;
	else if (ldegrees < 1000)
		ldegrees = 10;
	else if (ldegrees < 1500)
		ldegrees = 15;
	else
		ldegrees = 30;
#endif
	langles = DEG2ANG (ldegrees);	/* angle of step */

	if (HDT_HUD == type) {
		x0 = muldiv (sx, langles, hudarea);
		y0 = muldiv (sy, langles, hudarea);
	} else {
		x0 = muldiv (maxx, langles, ATAN (view->viewport->maxx,
				view->viewport->z));
		y0 = muldiv (maxy, langles, ATAN (view->viewport->maxy,
				view->viewport->z));
	}

	x0 = fmul (sroll, x0);
	y0 = fmul (croll, y0);

	dd = num_size (9L, ss);

	if (ladder & LD_ERECT) {
		xnum = fmul (croll, ss);
		ynum = fmul (sroll, ss);
	} else {
		xnum = fmul (croll, 2*dd+2);
		ynum = fmul (sroll, 2*dd+2);
	}

	xt = fmul (sroll, 16*ss);
	yt = fmul (croll, 16*ss);

	sroll = fmul (sroll, sy);	/* convert to hud pixels */
	croll = fmul (croll, sx);

/* pull-up cue
*/
	if (!(EX->hud3 & HUD_CUE))
		goto no_cue;

	a = TANG(EX->misc[16]);
	if (DEG (-90) == a)
		goto no_cue;

	xgap  = fmul (croll, PULLUPCUEGAP);
	ygap  = fmul (sroll, PULLUPCUEGAP);
	xroll = fmul (croll, PULLUPCUE);
	yroll = fmul (sroll, PULLUPCUE);
	xslant = fmul (croll, PULLUPCUETIP);
	yslant = fmul (sroll, PULLUPCUETIP);
	xtip = xt/(16*2);
	ytip = yt/(16*2);

	x = orgx + px - muldiv (x0, a, langles);
	y = orgy + py - muldiv (y0, a, langles);

	if (a > 0) {
		if (keep_inside (&x, &y, xroll, -xroll, yroll, -yroll,
						orgx, orgy, sx, sy, shifty))
			if (((Uint)st.present) % 250 < 125)
				goto no_cue;
	}

	gr_color (ST_HFG);
	add_segment (x-xgap, y+ygap, x-xroll, y+yroll, orgx,
		orgy, sx, sy, shifty);
	add_segment (x+xgap, y-ygap, x+xroll, y-yroll, orgx,
		orgy, sx, sy, shifty);

	add_segment (x-xroll, y+yroll, x-xslant-xtip, y+yslant-ytip,
		orgx,orgy, sx, sy, shifty);
	add_segment (x+xroll, y-yroll, x+xslant-xtip, y-yslant-ytip,
		orgx, orgy, sx, sy, shifty);
no_cue:	;

	if (!(hud & HUD_LADDER))
		goto noladder;

	xgap  = fmul (croll, EX->ldgap);	/* gap pixels */
	ygap  = fmul (sroll, EX->ldgap);

	xtip = fmul (sroll, EX->ldtip);		/* tip size */
	ytip = fmul (croll, EX->ldtip);

	xslant = 0;
	yslant = 0;

	a = p->a[X];
	l = ANG2DEG (a);			/* degrees */
	if (a >= 0) {
		pitch = (int)(a % langles);
		ex = (int)(a / langles);
	} else {
		a = -a;
		pitch = langles - (int)(a % langles);
		ex = -(int)(a / langles + 1);
	}
	pitch *= (90/ldegrees);
	px += fmul (pitch, x0);
	py += fmul (pitch, y0);

	color = ST_HFG;

	for (i = -3; i <= 3; ++i) {
		x = px - i*x0;
		y = py - i*y0;

		n = (ex + i) * ldegrees;
		nn = n-l;

		if (n > 90) {
			n = 180 - n;
			rev = 1;
		} else if (n < -90) {
			n = -180 - n;
			rev = 1;
		} else
			rev = 0;

		if (ladder & LD_COLOR)
			color = n < 0 ? CC_LRED :
				(n > 0 ? CC_LBLUE : CC_LGRAY);
		gr_color (color);

		tt = n ? EX->ldstep :
			((EX->equip & EQ_GEAR) ? EX->ldstepg : EX->ldstep0);
		xroll = fmul (croll, tt);
		yroll = fmul (sroll, tt);

		nn = abs(n);
		if (90 == nn && (ladder & LD_ZENITH)) {
			dx = 2*tx;
			dy = 2*ty;
			if (iabs(x) < (Uint)(sx-dx) &&
			    y < sy-dy+shifty && y > -sy+dy+shifty) {
				x += orgx;
				y += orgy;
				gr_ellipse (x, y, dx, dy);
				if (n < 0) {
					tt = SIN (D90/2);
					dx = fmul (tt, dx);
					dy = fmul (tt, dy);
					gr_move (x-dx, y+dy);
					gr_draw (x+dx, y-dy);
					gr_move (x-dx, y-dy);
					gr_draw (x+dx, y+dy);
				}
			}
			continue;
		}

		x += orgx;
		y += orgy;

		if (rev) {
			xt = -xt;
			yt = -yt;
			xtip = -xtip;
			ytip = -ytip;
			xnum = -xnum;
			ynum = -ynum;
			xroll = -xroll;
			yroll = -yroll;
			xgap = -xgap;
			ygap = -ygap;
		}

		if (ladder & LD_SLANT) {
			if (90 == nn)
				xslant = yslant = 0;
			else {
				xslant = muldiv (xt, nn, 16*90/2);
				yslant = muldiv (yt, nn, 16*90/2);
				if (funnel != (n < 0)) {
					xslant = -xslant;
					yslant = -yslant;
				}
			}
		}

/* Show the steps with tips.
*/
		if (funnel) {
			dx0 = xgap;		/* right tip root */
			dy0 = ygap;
			dx1 = xroll;		/* right step */
			dy1 = yroll;
		} else {
			dx0 = xroll;		/* right step */
			dy0 = yroll;
			dx1 = xgap;		/* right tip root */
			dy1 = ygap;
		}
		if ((nn || (ladder & LD_TIP0)) && nn != 90) {
			if (n < 0 && (ladder & LD_NEGTIP)) {
				dx = -xtip;
				dy = -ytip;
			} else {
				dx = xtip;
				dy = ytip;
			}
			add_segment (x+dx0, y-dy0, x+dx0+dx, y-dy0+dy,
				orgx, orgy, sx, sy, shifty);
			add_segment (x-dx0, y+dy0, x-dx0+dx, y+dy0+dy,
				orgx, orgy, sx, sy, shifty);
		}
		if (n < 0 && EX->ldndash) {
			add_dash (x+dx0, y-dy0, x+dx1+xslant, y-dy1+yslant,
				EX->ldndash, FCON(0.5), orgx, orgy+shifty,
				sx, sy);
			add_dash (x-dx0, y+dy0, x-dx1+xslant, y+dy1+yslant,
				EX->ldndash, FCON(0.5), orgx, orgy+shifty,
				sx, sy);
		} else {
			add_segment (x+dx0, y-dy0, x+dx1+xslant, y-dy1+yslant,
				orgx, orgy, sx, sy, shifty);
			add_segment (x-dx0, y+dy0, x-dx1+xslant, y+dy1+yslant,
				orgx, orgy, sx, sy, shifty);
		}

/* Show the pitch numerals.
*/
		if (n) {
			dx0 = x-xroll;
			dy0 = y+yroll;
			dx1 = x+xroll;
			dy1 = y-yroll;

			if (ladder & LD_ERECT) {
				dx = -((nn >= 10) ? dd : dd/2);
				dy = (ladder & LD_UNDER) ? ss : ss/2;
				dx0 += -xnum;
				dy0 +=  ynum;
				dx1 +=  xnum;
				dy1 += -ynum;
			} else {
				stroke_angle (rev
					? TANG(D180+p->a[Y]) : p->a[Y]);
				if (ladder & LD_UNDER) {
					dx = xt/16;
					dy = yt/16;
				} else {
					dx = xt/(16*2);
					dy = yt/(16*2);
				}
				if (nn < 10) {
					dx0 += -xnum/2;
					dy0 +=  ynum/2;
				} else {
					dx0 += -xnum;
					dy0 +=  ynum;
				}
				dx1 +=  xnum/4;
				dy1 += -ynum/4;
			}
			if (!(ladder & LD_RIGHT))
				show_num (dx0+dx, dy0+dy, (long)nn, ss, color,
					orgx, orgy, sx, sy, shifty);
			show_num (dx1+dx, dy1+dy, (long)nn, ss, color,
				orgx, orgy, sx, sy, shifty);
		}

		if (rev) {
			xt = -xt;
			yt = -yt;
			xtip = -xtip;
			ytip = -ytip;
			xnum = -xnum;
			ynum = -ynum;
			xgap = -xgap;
			ygap = -ygap;
		}
	}
	if (!(ladder & LD_ERECT))
		stroke_angle (0);
noladder:

/* Show ground pointer marks.
*/
	if (EX->hud1 & HUD_PENDULUM && !(EX->hudmode & HM_DECLUTTER)) {
		if (f16) {
			dx0 = F16GPTICK;
			dy0 = F16GPPOS;
			x   = F16GPSIZE;
		} else {
			dx0 = F18GPTICK;
			dy0 = F18GPPOS;
			if (hudtype == HUD_ETHER) {
				x = FONE + dx0;
			} else
				x = F18GPSIZE;
		}
		px = fmul (sx, x);
		py = fmul (sy, x);
		y = fmul (py, dx0);
		dy0 = orgy + fmul (sy, dy0);
		gr_color (ST_HFG);
		gr_move (orgx, dy0+py);
		gr_draw (orgx, dy0+py+y);
		for (i = 0; i < 4; ++i) {
			if (f16) {
				dx = F16Psin[i];
				dy = F16Pcos[i];
			} else {
				dx = F18Psin[i];
				dy = F18Pcos[i];
			}
			dx = fmul (px, dx);		/* tick base */
			dy = fmul (py, dy);
			x = fmul (dx, dx0);		/* tick size */
			y = fmul (dy, dx0);
			if (0 == i || (f16 && i < 2)) {	/* small tick */
				x = (x+1)/2;
				y = (y+1)/2;
			}
			gr_move (orgx+dx,   dy0+dy); /* +ve */
			gr_draw (orgx+dx+x, dy0+dy+y);
			gr_move (orgx-dx,   dy0+dy); /* -ve */
			gr_draw (orgx-dx-x, dy0+dy+y);
		}

/* The ground pointer itself.
*/
		a = p->a[Y];
		if (a > DEG(50)) {
			a = DEG(50);
			j = 1;
		} else if (a < -DEG(50)) {
			a = -DEG(50);
			j = 1;
		} else
			j = 0;
		if (!j || (((int)st.present)&0x0080)) {
			x  = SIN (a);
			y  = COS (a);
			dx = fmul (px, x);		/* arrow point */
			dy = fmul (py, y);
			xt = fmul (fmul (px, y), dx0/2); /* arrow base tip */
			yt = fmul (fmul (py, x), dx0/2);
			px = fmul (dx, dx0);		/* arrow mid base */
			py = fmul (dy, dx0);
			dx += orgx;
			dy += dy0;

			gr_move (dx,       dy);
			gr_draw (dx-px+xt, dy-py-yt);
			gr_draw (dx-px-xt, dy-py+yt);
			gr_draw (dx,       dy);
		}
	}

/* Zenith marker. Experimental.
*/
	if (ladder & LD_SUN) {
		int	D[2];

		RR[X] = p->T[X][Z];
		RR[Y] = p->T[Y][Z];
		RR[Z] = p->T[Z][Z];
		screen_coords (view, RR);
		dx = tx+(tx>>1);
		dy = ty+(ty>>1);
		i = h->flags & HF_ETHERFRAME;
		if (i) {
			px = h->maxx;
			py = h->maxy;
		} else {
			px = h->sx;
			py = h->sy;
		}
		px -= dx;
		py -= dy;
		j = clip_to_screen (D, RR, maxx, maxy, px, py, shifty);
		if (j && i) {
			clip_to_ether (h, D, px, py);
			dx = h->ethertx;
			dy = h->etherty;
		}
		gr_color (ST_SUN);
		gr_ellipse (orgx+D[X], orgy-D[Y], dx, dy);
	}
}
