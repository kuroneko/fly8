/* --------------------------------- stores.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Show stores (HDD_STORES).
*/

#include "plane.h"


/* Status is:
 *
 * blank	retracted.
 * L		lowered
 * G		has ground contact
 * green	fully extended, safe
 * red		in transit, unsafe
*/
LOCAL_FUNC int FAR
show_gear (int x, int y, xshort pos, Ushort ground, int s)
{
	int	c, color;

	c = (0 == pos) ? ' ' : (ground ? 'G' : 'L');
	color = (100 == pos) ? ST_LAMPOK : ST_LAMPERR;
	return (stroke_char (x, y, c, s, color));
}

extern void FAR
show_stores (VIEW *view, OBJECT *p, int maxx, int maxy, int orgx, int orgy,
	int ss)
{
	int	hud1, knots, blink;
	Ushort	equip;
	int	xl, xr, x, y, x0, y0, tx, ty, dd;
	int	i, t, mm, hh;
	long	range = 0;

	if (!IS_PLANE (p))
		return;

	hud1 = EX->hud1;
	knots = hud1 & HUD_KNOTS;
	equip = EX->equip;
	blink = ((int)st.present)&0x0080;

	dd = num_size (9L, ss);
	xl = orgx - maxx + 2;
	xr = orgx;
	y = orgy - maxy + ss;
	y0 = (ss*3) >> 1;

/* Show fuel status.
*/
	y += ss;
	x0 = xl;
	t = (int) (EX->fuel / EP->fuel_capacity);
	x0 += stroke_str (x0, y, "FUEL ", ss, (t < 10)
						? ST_LAMPERR : ST_LAMPOK);
	stroke_num (x0, y, EX->fuel/100, ss, ST_HFG);

	y += y0;
	x0 = xl;
	x0 += stroke_str  (x0, y, "TIME ", ss, ST_HFG);
	t = EX->fuelRate;
	if (t) {
		mm = (int)(EX->fuel / (60L*t));
		range = p->speed/VONE * 60L * mm;
		hh = mm / 60;
		mm -= 60*hh;
		if (hh > 99)
			hh = mm = 99;
		x0 += stroke_num  (x0, y, hh, ss, ST_HFG);
		x0 += stroke_char (x0, y, ':', ss, ST_HFG);
		stroke_frac (x0, y, mm, 2, 0, ss, ST_HFG);
	}

	y += y0;
	x0 = xl;
	x0 += stroke_str (x0, y, "RNGE ", ss, ST_HFG);
	if (t) {
		range /= knots ? 1852L : 1000L;
		stroke_num (x0, y, range, ss, ST_HFG);
	}

/* Show gear status.
*/
	y += y0;
	x0 = xl;
	x0 += stroke_str (x0, y, "GEAR ", ss, ST_HFG);
	x0 += show_gear  (x0, y, EX->gear[0], equip & EQ_GEAR1, ss);
	x0 += show_gear  (x0, y, EX->gear[1], equip & EQ_GEAR2, ss);
	x0 += show_gear  (x0, y, EX->gear[2], equip & EQ_GEAR3, ss);
	x0 += show_gear  (x0, y, EX->gear[3], equip & EQ_GEAR4, ss);
	x0 += show_gear  (x0, y, EX->gear[4], equip & EQ_GEAR5, ss);

/* Show other digital stuff.
*/
	y = orgy + maxy - ss;

	x0 = xl;
	x0 += stroke_str (x0, y, "SPL ", ss, ST_HFG);
	if (EX->spoilers)
		stroke_num (x0, y, EX->spoilers, ss, ST_HFG);
	y -= y0;

	x0 = xl;
	x0 += stroke_str (x0, y, "FLP ", ss, ST_HFG);
	if (EX->flaps)
		stroke_num (x0, y, EX->flaps, ss, ST_HFG);
	y -= y0;

	x0 = xl;
	x0 += stroke_str (x0, y, "RDR ", ss, ST_HFG);
	if (EX->rudder) {
		x0 += stroke_char (x0, y, (EX->rudder > 0) ? 'L' : 'R', ss,
			ST_HFG);
		t = abs(EX->rudder);
		 stroke_num (x0, y, t , ss, ST_HFG);
	}
	y -= y0;

	if (EX->brake) {
		x0 = xl;
		x0 += stroke_str (x0, y, "BRK ", ss, ST_HFG);
		stroke_num (x0, y, EX->brake, ss, ST_HFG);
	}
	y -= y0;

	x0 = xl;
	if (EX->flags & PF_CHASE) {
		x0 += stroke_str (x0, y, EX->target ? "CHASE" :"AUTO ",
			ss, ST_HFG);
		x0 += dd;
	} else
		x0 += 6*dd;
	if (EX->flags & PF_KILL)
		stroke_str (x0, y, "KILL", ss, ST_HFG);
	y -= y0;

	x0 = xl;
	x0 += stroke_str (x0, y, "HLT ", ss, ST_HFG);
	stroke_num (x0, y, p->damage, ss,
				(p->damage <= 3) ? ST_LAMPERR : ST_HFG);
	y -= y0;

/* Show weapons.
*/
	y = orgy - maxy + ss;
	for (i = 1; i <= WE_LAST; ++i) {
		y += y0;
		stroke_str (xr, y, get_wname (i), ss, ST_LAMPOK);
		x0 = xr + 6*dd;
		x0 += stroke_num (x0, y, EX->stores[i-1], ss, ST_HFG);
		if (EX->weapon == i)
			show_rect ((xr+x0)>>1, y-(ss>>1),
				((x0-xr)>>1)+2, (ss>>1)+2, ST_LAMPOK, 0);
	}

/* Show throttle and power
*/
	tx = maxx >> 5;
	ty = maxy >> 5;
	x = orgx + maxx - tx - 4;		/* position */
	y = orgy + maxy;			/* bottom */
	y0 = maxy + maxy - ss - ty;		/* height */

	gr_color (ST_HFG);
	gr_move (x, y);				/* reference bar */
	gr_draw (x, y-y0);

	if (100 == EX->throttle)
		t = 75 + muldiv (EX->afterburner, 100-75, 100);
	else {
		t = muldiv (EX->throttle, 75, 100);
		if (t < 0) {
			t = -t;
		}
	}
	t = muldiv (t, y0, 100);
	if (!EX->airbrake || blink)
		show_trig (x-tx, y-t, tx, ty, ST_HFG);	/* throttle */

	t = muldiv (EX->power, y0, 10000);
	gr_move (x,    y-t);			/* power */
	gr_draw (x+tx, y-t);
}
