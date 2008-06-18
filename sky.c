/* --------------------------------- sky.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Render the sky algorithmically.
*/

#include "fly.h"


#define NSIZE	(st.SkyLines*2+1)

static ANGLE	*skylines = 0;

extern int FAR
sky_init (void)
{
	int	i;
	ANGLE	a, v;

	if (skylines)
		sky_term ();

	skylines = (ANGLE *) memory_alloc (NSIZE * sizeof (*skylines));
	if (!skylines) {
		MsgEPrintf (-50, "No Sky");
		st.flags &= ~SF_SKY;
		return (1);
	}
	skylines[0] = 0;
	skylines[st.SkyLines] = D90;
	skylines[2*st.SkyLines] = D180;

	for (i = 1; i < st.SkyLines; ++i) {
		a = muldiv (i, D90, st.SkyLines);
		v = fmul (D90, SIN (a));
		skylines[st.SkyLines-i] = D90 - v;
		skylines[st.SkyLines+i] = D90 + v;
	}
	return (0);
}

extern void FAR
sky_term (void)
{
	if (!skylines)
		return;

	memory_free (skylines, NSIZE * sizeof (*skylines));
	skylines = 0;
}

LOCAL_FUNC void NEAR
show_solid_sky (int x[2], int y[2], int orgx, int orgy, int sizex, int sizey,
	int croll, int sroll)
{
	int	yorder, xl, xr, yy, ymax;
	int	cs, cg, cl, cr;

	cr = cs = ST_SKY;
	cl = cg = ST_EARTH;
	if (croll < 0) {
		yy = cs;
		cs = cg;
		cg = yy;
	}
	if (sroll < 0) {
		yy = cl;
		cl = cr;
		cr = yy;
	}

	xl = orgx - sizex;
	xr = orgx + sizex;

	if (NULL == x || NULL == y) {
		gr_color (cs);
		gr_clear (xl, orgy-sizey, 2*sizex+1, 2*sizey+1);
		return;
	}

	y[0] = orgy - y[0];
	y[1] = orgy - y[1];
	x[0] = orgx + x[0];
	x[1] = orgx + x[1];

	yorder = y[0] > y[1];

/* start at the top
*/
	yy = orgy - sizey;

/* do top rectangle (top to y[yorder]-1)
*/
	ymax = y[yorder];
	if (yy < ymax) {
		gr_color (cs);
		gr_clear (xl, yy, 2*sizex+1, ymax-yy);
		yy = ymax;
	}

/* do middle rectangle (y[yorder] to y[1-yorder]).
*/

/* If we are perfectly horizontal, draw horizon in SKY color.
*/
	if (y[0] == y[1]) {
		gr_color (ST_SKY);
		gr_move  (xl, yy);
		gr_draw  (xr, yy);
	} else {
		BUFLINE	*p;

/* here we draw two regions, one sky-coloured, one ground. The regions are
 * a trapezoid with one vertical side and they often degenerate into a
 * triangle.
*/

		gr_color (cl);
		yy = (xl != x[yorder]);
		p = gr_nop (1+2*(3+yy));
		*p++ = T_POLYGON+3+yy;

		if (yy) {
			*p++ = xl;
			*p++ = y[yorder];
		}

		*p++ = x[yorder];
		*p++ = y[yorder];

		*p++ = x[1-yorder];
		*p++ = y[1-yorder];

		*p++ = xl;
		*p++ = y[1-yorder];


		gr_color (cr);
		yy = (xr != x[1-yorder]);
		p = gr_nop (1+2*(3+yy));
		*p++ = T_POLYGON+3+yy;

		*p++ = x[yorder];
		*p++ = y[yorder];

		*p++ = xr;
		*p++ = y[yorder];

		*p++ = xr;
		*p++ = y[1-yorder];

		if (yy) {
			*p++ = x[1-yorder];
			*p++ = y[1-yorder];
		}

		yy = y[1-yorder] + 1;
	}

/* do bottom rectangle (y[1-yorder]+1 to bottom)
*/
	ymax = orgy + sizey;
	if (yy <= ymax) {
		gr_color (cg);
		gr_clear (xl, yy, 2*sizex+1, ymax-yy+1);
	}
}

extern void FAR
show_sky (VIEW *view, OBJECT *p)
{
	int	orgx, orgy, maxx, maxy, sizex, sizey;
	int	sroll, croll, srollh, crollh, spitch, cpitch;
	int	h, xh, yh;
	int	i, k, n, x[2], y[2], slope, ma, mb, solid;
	ANGLE	a, z;
	AVECT	aa;
	MAT	T;

	if (!skylines || !(st.flags & SF_SKY))
		return;

	solid = st.flags1 & SF_SOLIDSKY;

	get_area (view, &orgx, &orgy, &sizex, &sizey);

	maxx = VP->maxx;
	maxy = VP->maxy;

	if (maxx < (i = (sizex+1) << 1))
		maxx = i;
	if (maxy < (i = (sizey+1) << 1))
		maxy = i;

	Mident (T);
	if (VP->roty)
		Mroty (T, -VP->roty);
	if (VP->rotx)
		Mrotx (T, -VP->rotx);
	if (VP->rotz)
		Mrotz (T, -VP->rotz);
	fMroty (T, p->siny, p->cosy);
	fMrotx (T, p->sinx, p->cosx);
	fMrotz (T, p->sinz, p->cosz);
	Mangles (0, T, aa, 0);

	i = ihypot2d (maxx, maxy);
	a = ATAN (i, VP->z);

	i = aa[X] - a;
	for (n = 0; n < NSIZE; ++n)
		if (skylines[n] >= i)
			break;

	sroll  = SIN (-aa[Y]);
	croll  = COS (-aa[Y]);

	srollh = muldiv (sroll, sizex, maxx);
	crollh = muldiv (croll, sizey, maxy);

	slope = iabs (fmul (croll, sizey)) >= iabs (fmul (sroll, sizex));

	sroll  = muldiv (sroll, sizey, maxy);
	croll  = muldiv (croll, sizex, maxx);

	if (!solid)
		gr_color (ST_SKY);

    for (; n < NSIZE; ++n) {
	if (solid && n)
		break;
	z = skylines[n]			/* angle being shown */
	    -aa[X];			/* relative pitch of line */
	if (iabs (z) > (Uint)a)		/* trivial reject */
		break;

	spitch = SIN (z);
	cpitch = COS (z);

	h = muldiv (VP->z, spitch, cpitch);

	xh = fmul (h, srollh);
	yh = fmul (h, crollh);

	if (!croll) {
		if (xh > sizex || xh < -sizex)
			continue;
		x[1] =   x[0] = xh;
		y[1] = -(y[0] = sizey);
	} else if (!sroll) {
		if (yh > sizey || yh < -sizey)
			continue;
		x[1] = -(x[0] = sizex);
		y[1] =   y[0] = yh;
	} else {

/* This is a classic 2D clipping. I use the line slope as the starting point
 * I then get in ma the result code which selects the next test. The loop is
 * just the means for doing the two tests in an arbitrary order.
*/
		ma = slope;
		mb = 1|2;
		for (i = 0, k = 0; k < 2; ++k) {
			if (ma) {
				if (mb & 1) {	/* right */
					x[i] = sizex;
					y[i] = yh - muldiv (x[i]-xh, sroll,
									croll);
					ma = (y[i] > sizey)
						| ((y[i] < -sizey) << 1);
					if (!ma && ++i >= 2)
						break;
				} else
					ma = 0;

				if (mb & 2) {	/* left */
					x[i] = -sizex;
					y[i] = yh + muldiv (xh-x[i], sroll,
									croll);
					mb = (y[i] > sizey)
						| ((y[i] < -sizey) << 1);
					if (!mb && ++i >= 2)
						break;
				} else
					mb = 0;

				if (ma & mb)
					break;
				mb |= ma;
				ma = 0;
			} else {
				if (mb & 1) {	/* bottom */
					y[i] = sizey;
					x[i] = xh - muldiv (y[i]-yh, croll,
									sroll);
					ma = (x[i] > sizex)
						| ((x[i] < -sizex) << 1);
					if (!ma && ++i >= 2)
						break;
				} else
					ma = 0;

				if (mb & 2) {	/* top */
					y[i] = -sizey;
					x[i] = xh + muldiv (yh-y[i], croll,
									sroll);
					mb = (x[i] > sizex)
						| ((x[i] < -sizex) << 1);
					if (!mb && ++i >= 2)
						break;
				} else
					mb = 0;

				if (ma & mb)
					break;
				mb |= ma;
				ma = 1;
			}
		}
		if (i < 2)
			continue;
	}

/* a mirror has left-right swapped.
*/
	if (VP->flags & VF_MIRROR) {
		x[0] = -x[0];
		x[1] = -x[1];
		if (solid)
			sroll = -sroll;
	}

	if (solid) {
		show_solid_sky (x, y, orgx, orgy, sizex, sizey, croll, sroll);
		return;
	}

	gr_move (orgx+x[0], orgy-y[0]);
	gr_draw (orgx+x[1], orgy-y[1]);
    }

    if (solid)
	show_solid_sky (NULL, NULL, orgx, orgy, sizex, sizey, aa[X], 0);
}
#undef NSIZE
