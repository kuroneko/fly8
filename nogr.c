/* --------------------------------- nogr.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Internal Gr functions when no driver support is available.
*/

#include "fly.h"


extern void FAR
NoEllipse (Uint x1, Uint y1, Uint rx, Uint ry, Uint color)
{
	int	ax, bx, cx, dx, ay, by, cy, dy;

	ax = fmul ( 3196, rx);		/* sin (pi/16) */
	ay = fmul ( 3196, ry);
	bx = fmul ( 9102, rx);		/* sin (3*pi/16) */
	by = fmul ( 9102, ry);
	cx = fmul (13623, rx);		/* sin (5*pi/16) */
	cy = fmul (13623, ry);
	dx = fmul (16069, rx);		/* sin (7*pi/16) */
	dy = fmul (16069, ry);

	Gr->MoveTo (x1+dx, y1-ay);
	Gr->DrawTo (x1+cx, y1-by, color);
	Gr->DrawTo (x1+bx, y1-cy, color);
	Gr->DrawTo (x1+ax, y1-dy, color);
	Gr->DrawTo (x1-ax, y1-dy, color);
	Gr->DrawTo (x1-bx, y1-cy, color);
	Gr->DrawTo (x1-cx, y1-by, color);
	Gr->DrawTo (x1-dx, y1-ay, color);
	Gr->DrawTo (x1-dx, y1+ay, color);
	Gr->DrawTo (x1-cx, y1+by, color);
	Gr->DrawTo (x1-bx, y1+cy, color);
	Gr->DrawTo (x1-ax, y1+dy, color);
	Gr->DrawTo (x1+ax, y1+dy, color);
	Gr->DrawTo (x1+bx, y1+cy, color);
	Gr->DrawTo (x1+cx, y1+by, color);
	Gr->DrawTo (x1+dx, y1+ay, color);
	Gr->DrawTo (x1+dx, y1-ay, color);
}

extern void FAR
NoClear (Uint x, Uint y, Uint sx, Uint sy, Uint color)
{
#if POLL_RATE
	int	poll = 0;
#endif

	if (!sx || !sy)
		return;

	for (sy += y, sx += x-1; y < sy; ++y) {
		Gr->MoveTo (x,  y);
		Gr->DrawTo (sx, y, color);
#if POLL_RATE
		if (!(++poll % POLL_RATE))
			sys_poll (2);
#endif
	}
}

extern void FAR
NoPolygon (int npoints, BUFLINE *points, Uint color)
{
	Uint	y;			/* current y */
	int	fwdx, fwdy, bwdx, bwdy;	/* current edge tops */
	int	fdx, fdy, bdx, bdy;	/* interpolation */
	BUFLINE	*lastpoint;		/* end of 'points' */
	BUFLINE	*fwd, *bwd;		/* current point */
	BUFLINE	*nfwd, *nbwd;		/* next point */
#if POLL_RATE
	int	poll = 0;
#endif

	if (npoints < 2)
		return;

	if (2 == npoints) {
		Gr->MoveTo (points[0], points[1]);
		Gr->DrawTo (points[2], points[3], color);
		return;
	}

/* locate the top vertex (use fwd as temp).
*/
	lastpoint = points + 2*(npoints - 1);
	y = (fwd = points)[Y];
	for (bwd = points+2; bwd <= lastpoint; bwd += 2) {
		if (y > bwd[Y])
			y = (fwd = bwd)[Y];
	}

/* run through the edges, top to bottom.
*/
	nfwd = nbwd = fwd;
	for (;;) {
		if (y == nfwd[Y]) {
			do {
				fwd = nfwd;
				if (fwd == lastpoint)
					nfwd = points;
				else
					nfwd = fwd + 2;
			} while (y == nfwd[Y]);
			if ((fdy = nfwd[Y] - fwd[Y]) < 0)
				break;
			fdx = nfwd[X] - (fwdx = fwd[X]);
			fwdy = 0;
		}

		if (y == nbwd[Y]) {
			do {
				bwd = nbwd;
				if (bwd == points)
					nbwd = lastpoint;
				else
					nbwd = bwd - 2;
			} while (y == nbwd[Y]);
			if ((bdy = nbwd[Y] - bwd[Y]) < 0)
				break;
			bdx = nbwd[X] - (bwdx = bwd[X]);
			bwdy = 0;
		}

		Gr->MoveTo (fwdx + muldiv (fwdy, fdx, fdy), y);
		Gr->DrawTo (bwdx + muldiv (bwdy, bdx, bdy), y, color);

		++y;
		++fwdy;
		++bwdy;

#if POLL_RATE
		if (!(++poll % POLL_RATE))
			sys_poll (2);
#endif
	}

/* do bottom tip/edge.
*/
	Gr->MoveTo (fwdx + fdx, y);
	Gr->DrawTo (bwdx + bdx, y, color);
}
