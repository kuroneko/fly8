/* --------------------------------- panel.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* show the control panel data.
*/

#include "plane.h"


#define GEARVLIMIT	(150*VONE)

extern void FAR
show_panel (VIEW *view, OBJECT *p, int maxx, int maxy, int orgx, int orgy,
	int ss)
{
	int	hud1, knots, blink;
	int	xl, xr, y, x0, y0, dm, dd, ch;
	long	t;

	hud1 = EX->hud1;
	knots = hud1 & HUD_KNOTS;
	blink = ((int)st.present)&0x0080;

	dd = num_size (9L, ss);

	dm = dd*3/2;
	xr = orgx + maxx - 7*dd;
	xl = xr - 7*dd;
	y = orgy + maxy - 2;
	y0 = ss*3/2;

	if (EX->flags & PF_CHASE) {
		if (EX->target)
			stroke_str (xl, y, "CHASE", ss, ST_HFG);
		else
			stroke_str (xl, y, "AUTO", ss, ST_HFG);
	}
	if (EX->flags & PF_KILL)
		stroke_str (xr, y, "KILL", ss, ST_HFG);
	y -= ss;

	if (EX->brake) {
		stroke_str (xl, y, "BRK", ss, ST_HFG);
		stroke_num (xl+dm+2*dd, y, EX->brake, ss, ST_HFG);
	}
	if (EX->flags & PF_ONGROUND)
		stroke_str (xr, y, "TAXI", ss, ST_HFG);
	else if (EX->equip & EQ_GEAR) {
		if (p->speed < GEARVLIMIT || blink)
			stroke_str (xr, y, "GEAR", ss, ST_HFG);
	} else if (EX->fuelRate) {
		int	mm, hh;

		mm = (int)(EX->fuel / (60L*EX->fuelRate));
		hh = mm / 60;
		mm -= 60*hh;
		if (hh > 99)
			hh = mm = 99;
		stroke_str (xr, y, "FT", ss, ST_HFG);
		x0 = xr+dm+dd;
		x0 += stroke_num (x0, y, hh, ss, ST_HFG);
		x0 += stroke_char (x0, y, ':', ss, ST_HFG);
		stroke_frac (x0, y, mm, 2, 0, ss, ST_HFG);
	}
	y -= y0;

	stroke_char (xl, y, 'D', ss, ST_HFG);
	stroke_num (xl+dm, y, (long)p->damage, ss, ST_HFG);
	stroke_char (xr, y, 'F', ss, ST_HFG);
	stroke_num (xr+dm, y, EX->fuel/100, ss, ST_HFG);
	y -= ss;

	if (EX->spoilers) {
		stroke_str (xl, y, "SPL", ss, ST_HFG);
		stroke_num (xl+dm+2*dd, y, EX->spoilers, ss,
				ST_HFG);
	}
	if (EX->flaps) {
		stroke_str (xr, y, "FLP", ss, ST_HFG);
		stroke_num (xr+dm+2*dd, y, EX->flaps, ss,
				ST_HFG);
	}
	y -= ss;

	if (EX->rudder) {
		stroke_str (xl, y, (EX->rudder > 0) ? "RDL" : "RDR",
			ss, ST_HFG);
		t = abs(EX->rudder);
		x0 = xl+dm+2*dd;
		x0 += stroke_num (x0, y, t , ss, ST_HFG);
	}
	if (EX->airbrake) {
		if (blink) {
			stroke_str (xr, y, "AIR", ss, ST_HFG);
			stroke_num (xr+dm+2*dd, y, EX->airbrake, ss, ST_HFG);
		}
	} else {
		stroke_str (xr, y, "TST", ss, ST_HFG);
		t = (int)(EX->thrust * 100L / EP->ab_thrust);
		stroke_num (xr+dm+2*dd, y, t, ss, ST_HFG);
	}
	y -= y0;

	stroke_char (xl, y, 'T', ss, ST_HFG);
	t = EX->throttle + EX->afterburner/20;
	stroke_num (xl+dm, y, t, ss, ST_HFG);
#if 0
	stroke_char (xr, y, 'E', ss, ST_HFG);
	t = (int)(EX->thrust * 100L / EP->mil_thrust);
#else
	stroke_char (xr, y, 'P', ss, ST_HFG);
	t = EX->power / 100;
#endif
	stroke_num (xr+dm, y, t, ss, ST_HFG);
	y -= y0;
	if (EX->hdd & HDD_NAV) {

/* Show simple x/y from home.
*/
		t = p->R[Y]/(VONE*10);
		if (knots)
			t = t * 54 / 100;
		if (t < 0) {
			ch = 'S';
			t = -t;
		} else
			ch = 'N';
		stroke_char (xl, y, ch, ss, ST_HFG);
		stroke_frac (xl+dm, y, t, 0, 2, ss, ST_HFG);

		t = p->R[X]/(VONE*10);
		if (knots)
			t = t * 54 / 100;
		if (t < 0) {
			ch = 'W';
			t = -t;
		} else
			ch = 'E';
		stroke_char (xr, y, ch, ss, ST_HFG);
		stroke_frac (xr+dm, y, t, 0, 2, ss, ST_HFG);
		y -= ss;
/*
 * Show correct latitude/longitude.
*/
		if (p->flags & F_KEEPNAV) {
			t = p->latitude;
			if (t < 0) {
				ch = 'S';
				t = -t;
			} else
				ch = 'N';
			x0 = xl;
			x0 += stroke_char (x0, y, ch, ss, ST_HFG);
			x0 += dd/2;
			x0 += stroke_frac (x0, y, t/60, 2, 0, ss, ST_HFG);
			x0 += stroke_char (x0, y, ':',        ss, ST_HFG);
			      stroke_frac (x0, y, t%60, 2, 0, ss, ST_HFG);

			t = p->longitude;
			if (t < 0) {
				ch = 'W';
				t = -t;
			} else
				ch = 'E';
			x0 = xr;
			x0 += stroke_char (x0, y, ch, ss, ST_HFG);
			x0 += dd/2;
			x0 += stroke_frac (x0, y, t/60, 3, 0, ss, ST_HFG);
			x0 += stroke_char (x0, y, ':',        ss, ST_HFG);
			      stroke_frac (x0, y, t%60, 2, 0, ss, ST_HFG);
			y -= ss;
		}
		stroke_str (xl, y, (char *)show_time ("TIME ", st.present), ss,
			ST_HFG);
	}
}

#undef GEARVLIMIT
