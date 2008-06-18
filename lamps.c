/* --------------------------------- lamps.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Show indicators display (HDD_LAMPS).
*/

#include "plane.h"


/* Show one lamp, handling color and blinking.
*/

LOCAL_FUNC void NEAR
lamp_show (OBJECT *p, int x, int y, int ss, int lamp, char *name, int color)
{
	int	blink;
	int	state;

	blink = (Uint)st.present % 512 > (512/3);	/* 2Hz, 2/3 duty */
	state = EX->lamps[lamp];

	if ((state & LAMP_MRED) && (blink || !(state & LAMP_BRED)))
		color = ST_LAMPERR;
	else if ((state & LAMP_MGREEN) && (blink || !(state & LAMP_BGREEN)))
		color = ST_LAMPOK;

	if (color >= 0)
		stroke_str (x, y, name, ss, color);
}

/* Show the bottom-line message. If it is too long then scroll it.
*/

LOCAL_FUNC void NEAR
ad_show (int x, int y, int ss, int size, char *ad, int color)
{
	int	pix, off, t;

	pix = stroke_size (ad, ss);		/* total pixels */

	if (pix <= size) {
		size += x;			/* right margin */
		off = 0;
	} else {
		pix -= size + 1;		/* scrolled pixels */
		size += x;			/* right margin */
		off = (Uint)(st.present >> 4) % (pix+40);	/* 64pix/sec */
		if (off > pix) {
			if (off > pix+20)
				off = 0;	/* pause at start */
			else
				off = pix;	/* pause at end */
		}
		x -= off;
	}

	size -= num_size (9L, ss) - 1;		/* guard right margin */
	for (pix = 0; *ad; ++ad) {
		if (pix < off)
			t = char_size (*ad, ss);
		else {
			if (x > size)
				break;
			t = stroke_char (x, y, *ad, ss, color);
		}
		pix += t;
		x += t;
	}
}

/* Show the lamp display.
*/

extern void FAR
show_lamps (VIEW *view, OBJECT *p, int maxx, int maxy, int orgx, int orgy,
	int ss)
{
	int	x1, x2, x3, x4, xr, y0, y1, y2, y3, y4, y5;
	int	t, dy;
	char	*ad;
	HMSG	*q;

	y0 = orgy - maxy;		/* top margin */

	dy = (maxy * 2) / 5;		/* line spacing */
	if (ss < dy - 2)
		ss = dy - 2;
	t = (num_size (9L, ss) * 6 + 2) * 4 / 2;
	if (t > maxx)
		ss = muldiv (ss, maxx, t);

	x1 = orgx - maxx;		/* left   column */
	x2 = orgx - maxx/2 + 2;
	x3 = orgx + 2;
	x4 = orgx + maxx/2 + 2;		/* right  column */
	xr = orgx + maxx;		/* right margin */

	y1 = y0 + dy;			/* top line */
	y2 = y1 + dy;
	y3 = y2 + dy;
	y4 = y3 + dy;
	y5 = y4 + dy;			/* bottom line */

/* Show grid.
*/
	if (!(st.flags & SF_BLANKER)) {
		gr_color (ST_LAMPOFF);
		gr_move (x2-2, y0);
		gr_draw (x2-2, y4);
		gr_move (x3-2, y0);
		gr_draw (x3-2, y4);
		gr_move (x4-2, y0);
		gr_draw (x4-2, y4);

		gr_move (x1-1, y1+1);
		gr_draw (xr,   y1+1);
		gr_move (x1-1, y2+1);
		gr_draw (xr,   y2+1);
		gr_move (x1-1, y3+1);
		gr_draw (xr,   y3+1);
		gr_move (x1-1, y4+1);
		gr_draw (xr,   y4+1);
	}

/* Show lamps.
*/
	t = (st.flags & SF_BLANKER) ? -1 : ST_LAMPOFF;

	if (IS_PLANE(p)) {
		lamp_show (p, x1, y1, ss, LAMP_GLIMIT, "GLIMIT", t);
		lamp_show (p, x2, y1, ss, LAMP_STALL,  " STALL", t);
		lamp_show (p, x3, y1, ss, LAMP_FUEL,   " FUEL ", t);
		lamp_show (p, x4, y1, ss, LAMP_GEAR,   " GEAR ", t);


		lamp_show (p, x1, y2, ss, LAMP_ALT,    " ALT  ", t);
		lamp_show (p, x2, y2, ss, LAMP_PULLUP, "PULLUP", t);
		lamp_show (p, x4, y2, ss, LAMP_EJECT,  " EJECT", t);

		lamp_show (p, x1, y3, ss, LAMP_DAMAGE, "DAMAGE", t);
	}

/* Show last, most urgent message on screen.
*/
	t = -1;
	ad = welcome (-1);
	for (q = st.msg; q; q = q->next) {
		if (t < (int)q->flags) {
			t = (int)q->flags;
			ad = q->text;
		}
	}
	ad_show (x1, y5, ss, 2*maxx, ad, ST_HFG);
}
