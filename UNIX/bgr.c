/* --------------------------------- bgr.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Drawing primitives for a dumb buffer with 1 byte per pixel.
 * Can be very fast when usage of low level assembly support is available.
 *
 * bSetActive (base)
 * bSetSize (width, height, xbytes)
 * bMoveTo (x, y)
 * bDrawTo (x, y, color)
 * bDrawLine (x0, y0, x1, y1, color)
 * bSetWriteMode (mode)
 * bClear (x, y, sizex, sizey, color)
 * bDrawEllipse (cx, cy, rx, ry, color)
 *
 * About the 'poll' business:
 * On some slow platforms (msdos) we must poll during long operations.
 * Yes, this is a very bad hack...
*/

#include "fly.h"
#include "bgr.h"


#define GrSET	0x00
#define GrXOR	0x01
#define GrOR	0x02
#define GrXXX	0x03

char		_bWriteMode = GrSET;
static char	BPTR *bActiveBase = 0;
static Uint	bWidth = 0, bHeight = 0, bXbytes = 0;
static Uint	x1 = 0, y1 = 0, xydone = 0;
static char	BPTR *pva = 0;

extern int FAR
bSetActive (char BPTR *base)
{
	bActiveBase = base;
	x1 = y1 = 0;
	xydone = 0;

	return (0);
}

extern int FAR
bSetSize (Uint width, Uint height, Uint xbytes)
{
	bWidth  = width;
	bHeight = height;
	bXbytes = xbytes;

	return (0);
}

#ifndef USE_BGRASM

#define SLoop	bSimpleLoop
#define ILoop	bInnerLoop
#define vgaset	memset

#if UNROLL_DO_LINE
#define DO_LINE(op) \
	while ((count -= 4) >= 0) { \
		DO_PIXEL (op) \
		DO_PIXEL (op) \
		DO_PIXEL (op) \
		DO_PIXEL (op) \
	} \
	switch (count) { \
	case -1:	DO_PIXEL (op) \
	case -2:	DO_PIXEL (op) \
	case -3:	DO_PIXEL (op) \
	}
#else
#define DO_LINE(op) \
	while (--count >= 0) { \
		DO_PIXEL (op) \
	}
#endif

#define DO_MODE_LINE \
	if (GrSET == _bWriteMode) { \
		DO_LINE (=) \
	} else if (GrOR == _bWriteMode) { \
		DO_LINE (|=) \
	} else /* if (GrXOR == _bWriteMode) */ { \
		DO_LINE (^=) \
	}

INLINED static char BPTR * NEAR
SLoop (char BPTR *iva, int count, int sv, int c)
{
#define DO_PIXEL(op) \
	*(iva += sv) op c;

	DO_MODE_LINE

#undef DO_PIXEL

	return (iva);
}

INLINED static char BPTR * NEAR
ILoop (register char BPTR *iva, register Uint dy, Uint dx, register int dvx,
	int dvy, int c)
{
	register short	err;
	short		count;

/* dx > dy  never == !!!
 * dx > 0
 * dy >= 0
*/

	count = (short)dx;
	err = (short)(dx >> 1);

#define DO_PIXEL(op) \
	iva += dvx; \
	if ((err -= dy) < 0) { \
		err += dx; \
		iva += dvy; \
	} \
	*iva op c;

	DO_MODE_LINE

#undef DO_PIXEL

	return (iva);
}
#undef DO_LINE
#undef DO_MODE_LINE
#endif

extern void FAR
bMoveTo (Uint x, Uint y)
{
#if DEBUG_BGR
	if (x >= bWidth) {
		++STATS_GRERRMOVEXHIGH;
		x = bWidth-1;
	}
	if (y >= bHeight) {
		++STATS_GRERRMOVEYHIGH;
		y = bHeight-1;
	}
#endif
	x1 = x;
	y1 = y;
	xydone = 0;
}

extern void FAR
bDrawTo (Uint x2, Uint y2, Uint c)
{
	int	dx, dy, svx, svy;
	int	t;

#if DEBUG_BGR
	if (x2 >= bWidth) {
		++STATS_GRERRDRAWXHIGH;
		x2 = bWidth-1;
	}
	if (y2 >= bHeight) {
		++STATS_GRERRDRAWYHIGH;
		y2 = bHeight-1;
	}
#endif
	t = 0;
	if (!xydone) {
		t = 1;
		xydone = 1;
		pva = bActiveBase + x1 + y1*(long)bXbytes;
		if (GrSET == _bWriteMode)
			*pva = (char)c;
		else if (GrOR == _bWriteMode)
			*pva |= (char)c;
		else if (GrXOR == _bWriteMode)
			*pva ^= (char)c;
	}

	svx = 1;
	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		svx = -1;
	}

	svy = bXbytes;
	if ((dy = y2 - y1) < 0) {
		dy = -dy;
		svy = -svy;
	}

	if (dx > dy) {
		if (dy)
			pva = ILoop (pva, dy, dx, svx, svy, c);
		else {
			if (GrSET != _bWriteMode)
				pva = SLoop (pva, dx, svx, c);
			else {
				char	BPTR *pvb;

				pvb = pva + x2 - x1;		/* end point */
				vgaset ((svx>0 ? (pva+1) : pvb), c, dx);
				pva = pvb;
			}
		}
		++GrStats[dx+t];
	} else if (dx < dy) {
		if (dx)
			pva = ILoop (pva, dx, dy, svy, svx,  c);
		else
			pva = SLoop (pva, dy, svy, c);
		++GrStats[dy+t];
	} else /* if (dx == dy) */ {
		if (dx)
			pva = SLoop (pva, dy, svx+svy, c);
		++GrStats[dx+t];
	}

	x1 = x2;
	y1 = y2;
}

extern void FAR
bDrawLine (Uint xa, Uint ya, Uint xb, Uint yb, Uint c)
{
	if (xa != x1 || ya != y1 || !xydone)
		bMoveTo (xa, ya);

	bDrawTo (xb, yb, c);
}

extern int FAR
bSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		mode = GrSET;
		break;
	case T_MOR:
		mode = GrOR;
		break;
	case T_MXOR:
		mode = GrXOR;
		break;
	}
	_bWriteMode = (char)mode;
	return (0);
}

extern void FAR
bClear (Uint x, Uint y, Uint sizex, Uint sizey, Uint color)
{
#if POLL_RATE
	int	poll = 0;
#endif

#if DEBUG_BGR
	if (x+sizex > bWidth) {
		++STATS_GRERRDRAWXHIGH;
		if (x >= bWidth)
			x = bWidth-1;
		sizex = bWidth-x;
	}
	if (y+sizey > bHeight) {
		++STATS_GRERRDRAWYHIGH;
		if (y >= bHeight)
			y = bHeight-1;
		sizey = bHeight-y;
	}
#endif
	if (!sizex || !sizey)
		{}
	else if (GrSET == _bWriteMode) {
		xydone = 1;
		x1 = x + sizex - 1;
		y1 = y + sizey - 1;
		pva = bActiveBase + x + y*(long)bXbytes;
		for (sizey += y; y < sizey; ++y) {
			vgaset (pva, color, sizex);
			pva += bXbytes;		/* advance one row */
#if POLL_RATE
			if (!(++poll % POLL_RATE))
				sys_poll (2);
#endif
		}
		pva += sizex - 1 - bXbytes;
	} else {
		for (sizey += y, sizex += x-1; y < sizey; ++y) {
			bMoveTo (x, y);
			bDrawTo (sizex, y, color);
#if POLL_RATE
			if (!(++poll % POLL_RATE))
				sys_poll (2);
#endif
		}
	}
}

/* Based on the version from ACM TOG V11N3 (July 1992), this one is
 * modified to do 2 quads at a time to reduce VGA page flips.
*/

#define POINT(x,y,c) \
	do {						\
		if (GrSET == _bWriteMode)		\
			*(xxyy+(x)+(y))  = (Uchar)(c);	\
		else if (GrOR == _bWriteMode)		\
			*(xxyy+(x)+(y)) |= (c);		\
		else if (GrXOR == _bWriteMode)	\
			*(xxyy+(x)+(y)) ^= (c);		\
	} while (0)

#define INCX() \
	(x++, b2x += b2, dxt += d2xt, t += dxt)

#define INCY() \
	(y--, a2y -= a2, dyt += d2yt, t += dyt, yy -= bXbytes)

extern void FAR
bDrawEllipse (Uint xc, Uint yc, Uint a, Uint b, Uint c)
{
	char	BPTR *xxyy;
	int	x, y;
	long	yy, a2, b2, crit1, crit2, crit3, t, dxt, dyt, d2xt, d2yt;
	long	b2x, a2y;

	if (xc < a || xc+a >= bWidth ||
	    yc < b || yc+b >= bHeight)
		return;
	xxyy = bActiveBase + xc + yc * (long)bXbytes;

	x = 0;
	y = b;
	yy = y * (long)bXbytes;
	a2 = (long)a * a;
	b2 = (long)b * b;
	crit1 = -(a2/4 + (int)(a%2) + b2);
	crit2 = -(b2/4 + (int)(b%2) + a2);
	crit3 = -(b2/4 + (int)(b%2));
	t = -a2 * y;
	dxt =  2 * b2 * x;
	dyt = -2 * a2 * y;
	d2xt = 2 * b2;
	d2yt = 2 * a2;
	b2x = b2 * x;
	a2y = a2 * y;

	while (y >= 0 && x <= (int)a) {
		POINT (x, yy, c);
		if (x && y)
			POINT (-x, yy, c);
		if (t + b2x <= crit1 || t + a2y <= crit3)
			INCX ();
		else if (t - a2y > crit2)
			INCY ();
		else {
			INCX ();
			INCY ();
		}
	}

	x = 0;
	y = b;
	yy = y * (long)bXbytes;
	t = -a2 * y;
	dxt =  2 * b2 * x;
	dyt = -2 * a2 * y;
	d2xt = 2 * b2;
	d2yt = 2 * a2;
	b2x = b2 * x;
	a2y = a2 * y;

	while (y >= 0 && x <= (int)a) {
		if (x || y)
			POINT (-x, -yy, c);
		if (x && y)
			POINT ( x, -yy, c);
		if (t + b2x <= crit1 || t + a2y <= crit3)
			INCX ();
		else if (t - a2y > crit2)
			INCY ();
		else {
			INCX ();
			INCY ();
		}
	}
}
#undef POINT
#undef INCX
#undef INCY

extern void FAR
bPolygon (int npoints, BUFLINE *points, Uint color)
{
	Uint	y;			/* current y */
	int	fwdx, fwdy, bwdx, bwdy;	/* current edge tops */
	int	fdx, fdy, bdx, bdy;	/* interpolation */
	int	fx, bx;			/* segment endpoints */
	BUFLINE	*lastpoint;		/* end of 'points' */
	BUFLINE	*fwd, *bwd;		/* current point */
	BUFLINE	*nfwd, *nbwd;		/* next point */
#if POLL_RATE
	int	poll = 0;
#endif

	if (npoints < 2)
		return;

	if (2 == npoints) {
		bMoveTo (points[1], points[2]);
		bDrawTo (points[3], points[4], color);
		return;
	}

/* locate the top vertex (use fwd as temp).
*/
	lastpoint = points + 2*(npoints - 1);
	y = (fwd = points)[1];
	for (bwd = points+2; bwd <= lastpoint; bwd += 2) {
		if (y > bwd[1])
			y = (fwd = bwd)[1];
	}

/* run through the edges, top to bottom.
*/
	nfwd = nbwd = fwd;
	pva = bActiveBase + y*(long)bXbytes;
	xydone = 0;		/* don't bother keeping currency */
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

		fx = fwdx + muldiv (fwdy, fdx, fdy);
		bx = bwdx + muldiv (bwdy, bdx, bdy);
		if (GrSET == _bWriteMode) {
			if (fx <= bx)
				vgaset (pva+fx, color, bx-fx+1);
			else
				vgaset (pva+bx, color, fx-bx+1);
			pva += bXbytes;
		} else {
			bMoveTo (fx, y);
			bDrawTo (bx, y, color);
		}
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
	fx = fwdx + fdx;
	bx = bwdx + bdx;
	if (GrSET == _bWriteMode) {
		if (fx <= bx)
			vgaset (pva+fx, color, bx-fx+1);
		else
			vgaset (pva+bx, color, fx-bx+1);
		pva += bXbytes;
	} else {
		bMoveTo (fx, y);
		bDrawTo (bx, y, color);
	}
}

#undef GrSET
#undef GrXOR
#undef GrOR
#undef GrXXX
