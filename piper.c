/* --------------------------------- piper.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Show the piper (aiming reticule) on the Head Up Display.
*/

#include "plane.h"


static int	NEAR Csin[] = {8192, 14189}, NEAR Ccos[] = {14189, 8192};

extern void FAR
show_piper (HUD *h, OBJECT *obj, OBJECT *target, int x, int y, int dx, int dy,
	int ds, int mode, int off_screen, int dist, int tti, int closure,
	int orgx, int orgy, int clipx, int clipy, int hbottom, int hleft,
	int ss, int shifty)
{
	int	hud1, f15, f16, fa18, ether, knots;
	int	datax, datay, nlines, detail;
	int	t, i, d, dd, ax, ay, bx, by, rx, ry, ratio;
	ANGLE	a;
	long	tc, td;
	char	*name;
	VECT	R;

	hud1 = EE(obj)->hud1;
	i = hud1 & HUD_TYPES;
	f16   = i == HUD_F16;
	f15   = i == HUD_F15;
	fa18  = i == HUD_FA18;
	ether = i == HUD_ETHER;
	knots = hud1 & HUD_KNOTS;

	if (knots) {
		tc = closure + fmul (closure, 15465);	/* knots */
		td = 3L*dist + fmul (dist, 4601); /* feet */
	} else {
		tc = closure;
		td = dist;
	}

	dd = num_size (9L, ss);

	if (NEWTGT(obj))
		t = NEWTGT(obj)--&1;
	else
		t = 1;

	if (t) {
		ratio = f16 ? F16RAIMC : (f15 ? F15RAIMC : F18RAIMC);
		rx = fmul (dx, ratio);
		ry = fmul (dy, ratio);
		gr_color (ST_HFG);
		gr_ellipse (x, y, rx, ry);
		if (ds > 1) {
			if (hud1 & HUD_THICK)
				gr_ellipse (x, y, rx+1, ry+1);
/* Target distance
*/
			gr_move (x, y-ry);	/* 12 */
			gr_draw (x, y-dy);
			gr_move (x, y+ry);	/* 6 */
			gr_draw (x, y+dy);
			gr_move (x-rx, y);	/* 9 */
			gr_draw (x-dx, y);
			gr_move (x+rx, y);	/* 3 */
			gr_draw (x+dx, y);
			for (i = 0; i < 2; ++i) {	/* ticks */
				ax = fmul(Csin[i], dx);
				ay = fmul(Ccos[i], dy);
				bx = fmul (ax, ratio);
				by = fmul (ay, ratio);
				gr_move (x+bx, y+by);
				gr_draw (x+ax, y+ay);
				gr_move (x-bx, y+by);
				gr_draw (x-ax, y+ay);
				gr_move (x-bx, y-by);
				gr_draw (x-ax, y-ay);
				gr_move (x+bx, y-by);
				gr_draw (x+ax, y-ay);
			}

			gr_color (ST_HFGI);

			if (td > 11500L)		/* marker */
				i = 11500;
			else
				i = (int)td;
			a = muldiv (i, D90, 12000)*4;	/* 360Deg = 12k */
			ax = fmul(SIN (a), rx);
			ay = fmul(COS (a), ry);
			bx = fmul (ax, ratio);		/* use same ratio */
			by = fmul (ay, ratio);
			gr_move (x+ax, y-ay);
			gr_draw (x+bx, y-by);

/* Aspect angle(m61) or tti (mk82)
*/
			if (WE_MK82 == EE(obj)->weapon) {
				if (tti > 120)
					a = 0;
				else
					a = muldiv (D90/6, tti, 10)*2;
			} else {
				VxMmul (R, target->V, obj->T);
				t = ihypot2d (R[X], R[Z]);
				a = ATAN (t, -R[Y]);
			}
			ax = fmul (dx, SIN (a));
			ay = fmul (dy, COS (a));
			t = f16 ? F16RASPECT : (FONE-ratio)/2;
			bx = fmul (dx, t);
			by = fmul (dy, t);
			keep_inside (&x, &y, ax-bx, ax+bx, -ay-by, -ay+by,
				orgx, orgy, clipx, clipy, shifty);
			gr_ellipse (x+ax, y-ay, bx, by);
			if (WE_M61 == EE(obj)->weapon) {
/* Target closure rate
*/
				if (fa18||ether) {
					if (tc >= 0)	/* show 10s */
						tc = ((tc+5)/10)*10;
					else
						tc = ((tc-5)/10)*10;
					d = num_size (tc, ss);
					ax = x+dx;
					ay = y+dy+ss*5/2;
					keep_inside (&ax, &ay, -d, 2*dd,
						-ss-ss/2, 0, orgx, orgy,
						clipx, clipy, shifty);
					stroke_num (ax-d, ay-ss/2, tc, ss,
						ST_HFG);
					stroke_str (ax,   ay, "Vc", ss, ST_HFG);
				}
/* Target acceleration vector
*/
				if (hud1 & HUD_ACCVECT) {
					VxMmul (R, EE(obj)->taccel, obj->T);
					t = ihypot2d (R[X], R[Z]);
					if (t < 10*VONE)
						t = 10*VONE;
					ax = muldiv (R[X], dx, t);
					ay = muldiv (R[Z], dy, t);
					gr_color (ST_HFG);
					gr_move (x,    y);
					gr_draw (x+ax, y-ay);
				}
			}
		}
		gr_color (ST_HFG);
		if (off_screen == 1) {			/* ahead */
			gr_move (x,    y+ry);
			gr_draw (x,    y-ry);
			gr_move (x+rx, y);
			gr_draw (x-rx, y);
		} else if (off_screen == 2){		/* behind */
			d = SIN (D90/2);
			ax = fmul(d, rx);
			ay = fmul(d, ry);
			gr_move (x-ax, y+ay);
			gr_draw (x+ax, y-ay);
			gr_move (x-ax, y-ay);
			gr_draw (x+ax, y+ay);
		} else {
			if (f16) {			/* center circle */
				rx = fmul (dx, F16RPIP);
				ry = fmul (dy, F16RPIP);
				gr_ellipse (x, y, rx, ry);
			}
			gr_color (ST_HFGI);
			gr_move (x, y);			/* center dot */
			gr_draw (x, y);
		}
	}

	if (EE(obj)->hud & HUD_DATA) {
		name = get_name (obj, target, mode);
		detail = (1 == ds) || (2&mode);
		nlines = (detail ? 3 : 0) + !!name;

		if (2&mode) {
			datax = hleft;
			datay = hbottom+shifty-(nlines-1)*ss;
		} else {
			if (y+dy+nlines*ss >= orgy+clipy+shifty)
				datay = y-dy-2-(nlines-1)*ss;
			else
				datay = y+dy+ss;
			datax = 4*dd;
			if (name) {
				t = stroke_size (name, ss);
				if (t < datax)
					t = datax;
			} else
				t = datax;

			t = orgx+clipx-t;
			datax = x-dx;
			if (datax > t)
				datax = t;
		}

		show_data (obj, datax, datay, detail, knots, dist, closure,
			name, tti, mode, ss, ST_HFG);
	}
}
