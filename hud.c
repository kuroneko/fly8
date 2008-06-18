/* --------------------------------- hud.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the Head Up Display.
*/

#include "plane.h"


#define DOGRAPH		(st.debug & DF_GPX)

extern void FAR
show_hud (VIEW *view, OBJECT *pov, OBJECT *p, int orgx, int orgy,
	int maxx, int maxy, int mode)
{
	int	hud, hud1, big, htype, limit, front, hon;
	int	sx, sy, tx, ty, ttx, tty;
	int	x, y, ss, clipx, clipy, shifty;
	HUD	h[1];

/* hud only for planes, in front view
*/
	if (!IS_PLANE(p) || (HDT_HUD != mode && !scenery (mode))) {
		alarm_set (0);
		return;
	}

	hud = EX->hud;
	hud1 = EX->hud1;
	big = hud & HUD_BIG;
	htype = hud1 & HUD_TYPES;
	front = !VP->rotz && !VP->rotx;
	x = HDT_FRONT == mode && (hud & HUD_ON) && front;
	hon = x || HDT_HUD == mode;
	limit = (x && (hud1 & HUD_LIMIT)) || HDT_HUD == mode;

	get_square (view, maxx, maxy, &sx, &sy);

	if (0 == (tx = (sx+32)/64))
		tx = 1;
	if (0 == (ty = (sy+32)/64))
		ty = 1;

	shifty = EX->hudshift;
	if (HDT_HUD == mode) {
		orgy -= fmul (sy, shifty);
	} else {
		ANGLE	a;
		int	hudarea;

		y = sy + fmul (sy, shifty);
		if ((maxy>>1) >= y)
			hudarea = FCON(1.99);
		else
			hudarea = fdiv (maxy, y);	/* largest allowed */
		a = DEG2ANG (EX->hudarea);
		y = muldiv (VP->z, SIN (a), COS (a));
		if (y < VP->maxy) {			/* fits in window */
			y = muldiv (y, maxy, VP->maxy);	/* y pixels */
			y = fdiv (y, sy);		/* ratio */
			if (y < hudarea) {		/* fits in square */
				hudarea = y;
			}
		}
		sx = fmul (sx, hudarea);
		sy = fmul (sy, hudarea);
		if (sx > maxx) {
			sy = muldiv (sy, maxx, sx);
			sx = maxx;
		}
		if (sy > maxy) {
			sx = muldiv (sx, maxy, sy);
			sy = maxy;
		}
	}

	ss = fmul (sy, EX->hudFontSize);
	if (ss < 8)
		ss = 8;

	if (!big && HUD_CLASSIC == htype) {
		x = 3*tx;
		y = 2+tx + num_size (99L, ss);
		if (y > x)
			x = y;
		if (maxx-x < sx) {
			x = maxx-x;
			sy = muldiv (sy, x, sx);
			sx = x;
		}

		y = 2+3*ty+ss;
		if (maxy-y < sy) {
			y = maxy-y;
			sx = muldiv (sx, y, sy);
			sy = y;
		}
	}
	if (0 == (tx = (sx+32)/64))
		tx = 1;
	if (0 == (ty = (sy+32)/64))
		ty = 1;

	shifty = fmul (sy, shifty);

	if (limit) {
		clipx = sx;
		clipy = sy;
	} else {
		clipx = maxx;
		clipy = maxy;
	}

	h->flags = 0;

	h->orgx = orgx;		/* window sizes */
	h->orgy = orgy;
	h->maxx = maxx;
	h->maxy = maxy;

	h->shifty = shifty;		/* hud sizes */
	h->cx = orgx;
	h->cy = orgy+shifty;
	h->sx = sx;
	h->sy = sy;

	h->clipx =  clipx;		/* clip sizes */
	h->clipy =  clipy;
	h->clipr =  clipx;		/* clip rectangle, relative */
	h->clipl = -clipx;
	h->clipt =  clipy - shifty;
	h->clipb = -clipy - shifty;

	h->right  = orgx + h->clipr;	/* hud border, absolute */
	h->left   = orgx + h->clipl;
	h->top    = orgy - h->clipt;
	h->bottom = orgy - h->clipb;

	h->tx = h->ttx = tx;
	h->ty = h->tty = ty;
	h->ss = ss;
	h->dd = num_size (9L, ss);
	h->width = 0;
	h->height = 0;
	h->fg = ST_HFG;
	h->fgi = ST_HFGI;
	h->VV[X] = h->VV[Y] = 0;

	if (WIN_ETHER == st.windows && HUD_ETHER == (hud1 & HUD_TYPES)) {
	    	h->flags |= HF_ETHER;
		if (st.flags & SF_MAIN)
		    	h->flags |= HF_ETHERFRAME;
	}

	if (h->flags & HF_ETHERFRAME) {
		get_area (&st.hdd[5].view, 0, 0, &x, &y);
		h->etherx  = (h->maxx + x) / 2 + 1;
		h->ethery  = (h->maxy + y) / 2 + 1;
		x = h->etherx - h->maxx - 2;
		y = h->ethery - h->maxy - 2;
		get_square (&st.hdd[5].view, x*8, y*8, &x, &y);
		h->ethertx = x/8;
		h->etherty = y/8;
	}

	hud_alarm (h, p, ST_HFGI, mode, hon);

	if (hon) {
		if (big) {
			h->ttx = -h->ttx;
			h->tty = -h->tty;
		}
		ttx = h->ttx;
		tty = h->tty;
/* hud border
*/
		if ((hud1 & HUD_BORDER) && HDT_FRONT == mode)
			show_rect (h->cx, h->cy, h->sx, h->sy, ST_HBO, 0);

/* velocity vector
*/
		show_vv (h, view, p, ST_HFG);

/* waterline mark.
*/
		show_wl (h, p, ST_HFG);

/* pitch ladder is centered on the vv or the waterline mark
*/
		show_pitch (h, view, p, sx, sy, maxx, maxy, orgx, orgy,
			ttx, tty, tx, ty, ss, shifty, mode, h->VV);

/* heading on upper/lower edge
*/
		show_heading (h, view, p, sx, sy, maxx, maxy, orgx, orgy,
			ttx, tty, tx, ty, ss, shifty, h->VV);

/* waypoint (experimental).
*/
		show_waypoint (h, view, p);

/* altitude on right edge
*/
		show_altitude (h, p, sx, sy, maxx, maxy, orgx, orgy, ttx, tty,
			tx, ty, ss, shifty, h->VV);
/* speed on left edge
*/
		show_speed (h, p, sx, sy, maxx, maxy, orgx, orgy,
			ttx, tty, tx, ty, ss, shifty, h->VV);
/* ILS
*/
		show_ils (h, p, sx, sy, orgx, orgy, ss, shifty);

/* Bullets trail history.
*/
		show_trail (h, view, p);
	}

/* Show radar stuff
*/
	if (HDT_HUD == mode || scenery (mode))
		show_radar (h, view, p, pov, orgx, orgy, maxx, maxy, tx, ty,
			ss, clipx, clipy, sx, sy, limit ? shifty : 0, h->VV,
			mode, hon);

/* ailerons/elevators cursor (helps keypad/mouse mode)
*/
	if ((hud & HUD_CURSOR) && p->pointer) {
		x = orgx + muldiv (-p->pointer->a[0], sx-tx, 100);
		y = orgy + muldiv (-p->pointer->a[1], sy-ty, 100) + shifty;

		show_rect (x, y, tx, ty, ST_CFG, 0);
	}

/* cross hair
*/
	if ((hud & HUD_PLUS) || (HDT_MAP == mode || HDT_RADAR == mode))
		show_bplus (h->orgx, h->orgy, h->tx*3, h->ty*3, FCON(0.125),
			ST_HFG);

/* debug: show Cm/alpha graph.
*/
	if (DOGRAPH) {
		show_plus (h->cx, h->cy, h->sx, h->sy, ST_CFG);
		x = fmul (EX->aoa, sx*3);
		if (x > sx)
			x = sx;
		else if (x < -sx)
			x = -sx;
		y = EX->misc[5];
		if (y > 10000)
			y = 10000;
		else if (y < -10000)
			y = -10000;
		y = muldiv (y , sy-ty, 10000);
		x = h->cx + x;
		y = h->cy - y;
		show_rect (x, y, tx, ty, ST_CFG, 0);	/* show point */
	}
}

extern void FAR
show_num (int x, int y, long t, int s, int c, int orgx, int orgy, int maxx,
	int maxy, int shifty)
{
	int	dxs, dxc, dys, dyc, l, h;

	num_extent (t, s, &dxs, &dxc, &dys, &dyc);

	--maxx;			/* fight truncation errors */
	--maxy;

	l = orgx-maxx-x;
	h = orgx+maxx-x;
	if (0 > h || 0 < l)
		return;
	if (dxc > h || dxc < l)
		return;
	if (-dys > h || -dys < l)
		return;
	if (dxc-dys > h || dxc-dys < l)
		return;

	l = orgy+shifty-maxy-y;
	h = orgy+shifty+maxy-y;
	if (0 > h || 0 < l)
		return;
	if (-dxs > h || -dxs < l)
		return;
	if (-dyc > h || -dyc < l)
		return;
	if (-dxs-dyc > h || -dxs-dyc < l)
		return;

	stroke_num (x, y, t, s, c);
}

extern void FAR
add_segment (int x1, int y1, int x2, int y2, int orgx, int orgy,
	int sx, int sy, int shifty)
{
	int	i, z1, z2, xl, xh, yl, yh;

/* Not quite midpoint clipping, if both ends are out then we reject the
 * segment which is mostly ok.
*/
	xh = orgx+sx;
	xl = orgx-sx;
	yh = orgy+sy+shifty;
	yl = orgy-sy+shifty;

	z1 = x1>xh || x1<xl || y1>yh || y1<yl;
	z2 = x2>xh || x2<xl || y2>yh || y2<yl;

	if (z1) {
		if (z2)
			return;
		i = x1; x1 = x2; x2 = i;
		i = y1; y1 = y2; y2 = i;
	} else if (!z2) {
		gr_line (x1, y1, x2, y2);
		return;
	}

	gr_move (x1, y1);

	i = iabs(x2-x1);
	z1 = iabs(y2-y1);
	if (i < z1)
		i = z1;
	for (; i > 1; i >>= 1) {
		z1 = (x1 + x2)/2;
		z2 = (y1 + y2)/2;
		if (z1>xh || z1<xl || z2>yh || z2<yl) {
			x2 = z1;
			y2 = z2;
		} else {
			x1 = z1;
			y1 = z2;
		}
	}

	gr_draw (x1, y1);
}

extern void FAR
add_dash (int x1, int y1, int x2, int y2, int ndash, int ratio,
	int orgx, int orgy, int sx, int sy)
{
	register int	dx, dy;
	int		i, rx, ry, xl, xh, yl, yh;

	if (!ndash)
		return;

	xl = orgx - sx;
	xh = orgx + sx;
	yl = orgy - sy;
	yh = orgy + sy;

	ratio /= ndash;
	x2 -= x1;
	y2 -= y1;

/* Do symmetric truncation.
*/
	if (x2 < 0)
		rx = -fmul (-x2, ratio);
	else
		rx = fmul (x2, ratio);
	if (y2 < 0)
		ry = -fmul (-y2, ratio);
	else
		ry = fmul (y2, ratio);

	for (i = 0; i < ndash; ++i) {
		dx = x1 + muldiv (x2, i, ndash);
		dy = y1 + muldiv (y2, i, ndash);
		if (dx < xl || dx > xh || dy < yl || dy > yh)
			continue;
		gr_move (dx, dy);
		dx += rx;
		dy += ry;
		if (dx < xl || dx > xh || dy < yl || dy > yh)
			continue;
		gr_draw (dx, dy);
	}
}

extern void FAR
screen_coords (VIEW *view, VECT RR)
{
	int	s;

	s = VP->z;				/* get minimum */
	if (s > VP->maxx)
		s = VP->maxx;
	if (s > VP->maxy)
		s = VP->maxy;
	RR[X] = muldiv (RR[X], s, VP->maxx);
	RR[Z] = muldiv (RR[Z], s, VP->maxy);
	RR[Y] = muldiv (RR[Y], s, VP->z);
}

/* clip the point in R into the screen point D. Note that is the point is
 * inside the screen then a simple projection is done. Otherwise, a point
 * on the edge is returned. If the depth is negative then still clip to
 * the edge.
*/
extern int FAR
clip_to_screen (int D[2], VECT R, int maxx, int maxy, int clipx, int clipy,
	int shifty)
{
	int	off_screen, clip, x, y, ry, t;

	off_screen = 0;			/* some classic clipping */

	if (R[Y] <= 0) {
		ry = -R[Y];
		clip = 0;
	} else
		clip = ry = R[Y];

/* Establish position relative to the clipping pyramid for the screen.
*/
	if (R[X] >= clip)
		off_screen |= 1;	/* right */
	else if (-R[X] >= clip)
		off_screen |= 2;	/* left */

	if (R[Z] >= clip)
		off_screen |= 4;	/* top */
	else if (-R[Z] >= clip)
		off_screen |= 8;	/* bottom */

/* Resolve the corner areas into the correct clipping edge.
*/
	if (off_screen == 5)		/* top right */
		if (R[X] > R[Z])
			off_screen = 1;
		else
			off_screen = 4;
	else if (off_screen == 9)	/* bottom right */
		if (R[X] > -R[Z])
			off_screen = 1;
		else
			off_screen = 8;
	else if (off_screen == 6)	/* top left */
		if (-R[X] > R[Z])
			off_screen = 2;
		else
			off_screen = 4;
	else if (off_screen == 10)	/* bottom left */
		if (-R[X] > -R[Z])
			off_screen = 2;
		else
			off_screen = 8;
	else
		{}

/* Now do the projection and clipping together.
*/
	switch (off_screen) {
	default:
	case 0:
		if (ry == 0)
			x = y = 0;
		else {
			x = muldiv (maxx, R[X], ry);
			y = muldiv (maxy, R[Z], ry);
		}
		break;
	case 1:						/* right */
		x = maxx;
		if (R[X] == 0)
			y = 0;
		else
			y = muldiv (maxy, R[Z], R[X]);
		break;
	case 2:						/* left */
		x = -maxx;
		if (R[X] == 0)
			y = 0;
		else
			y = -muldiv (maxy, R[Z], R[X]);
		break;
	case 4:						/* top */
		if (R[Z] == 0)
			x = 0;
		else
			x = muldiv (maxx, R[X], R[Z]);
		y = maxy;
		break;
	case 8:						/* bottom */
		if (R[Z] == 0)
			x = 0;
		else
			x = -muldiv (maxx, R[X], R[Z]);
		y = -maxy;
		break;
	}
	if (off_screen)
		off_screen = clip ? 1 : 2;

/* Finally check for 2D clipping (for the window) and do it.
*/
	if (x >= clipx) {
		y = muldiv (clipx, y, x);
		x = clipx;
		if (!off_screen)
			off_screen = 1;
	} else if (x <= -clipx) {
		y = -muldiv (clipx, y, x);
		x = -clipx;
		if (!off_screen)
			off_screen = 1;
	}
	if (y >= (t = clipy-shifty)) {
		x = muldiv (t, x, y);
		y = t;
		if (!off_screen)
			off_screen = 1;
	} else if (y <= (t = -clipy-shifty)) {
		x = muldiv (t, x, y);
		y = t;
		if (!off_screen)
			off_screen = 1;
	}

	D[X] = x;
	D[Y] = y;
	return (off_screen);
}

extern void FAR
clip_to_ether (HUD *h, int D[2], int x, int y)
{
	if (D[X] >= x)
		D[X] = h->etherx;
	else if (D[X] <= -x)
		D[X] = -h->etherx;
	if (D[Y] >= y)
		D[Y] = h->ethery;
	else if (D[Y] <= -y)
		D[Y] = -h->ethery;
}

extern int FAR
keep_inside (int *x, int *y, int xl, int xh, int yl, int yh, int orgx,
	int orgy, int clipx, int clipy, int shifty)
{
	int	t;
	int	ret = 0;

	if (xl > xh)
		(t = xl, xl = xh, xh = t);
	if (*x > (t = orgx+clipx-xh))
		*x = t, ret = 1;
	else if (*x < (t = orgx-clipx-xl))
		*x = t, ret = 1;

	if (yl > yh)
		(t = yl, yl = yh, yh = t);
	orgy += shifty;
	if (*y > (t = orgy+clipy-yh))
		*y = t, ret = 1;
	else if (*y < (t = orgy-clipy-yl))
		*y = t, ret = 1;

	return (ret);
}

extern int FAR
is_in (HUD *h, int x, int y, int dx, int dy)
{
	return (x < h->right-dx && x > h->left+dx &&
		y > h->top+dy   && y < h->bottom-dy);
}

#undef DOGRAPH
