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
 * bScrClear (x, y, sizex, sizey, color)
 * bDrawEllipse (cx, cy, rx, ry, color)
*/

#include "fly.h"
#include "bgr.h"


static int	bGrWriteMode = T_MSET;
static char	BPTR *bActiveBase = 0;
static Uint	bWidth = 0, bHeight = 0, bXbytes = 0;
static Uint	x1 = 0, y1 = 0, xydone = 0;
static char	BPTR *pva = 0;

extern int FAR
bSetActive (char BPTR *base)
{
	bActiveBase = base;
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

#ifndef USE_ASMLINE

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
	if (T_MSET == bGrWriteMode) { \
		DO_LINE (=) \
	} else if (T_MOR == bGrWriteMode) { \
		DO_LINE (|=) \
	} else /* if (T_MXOR == bGrWriteMode) */ { \
		DO_LINE (^=) \
	}

INLINED static char BPTR * NEAR
SLoop (iva, count, sv, c)
char	BPTR *iva;
int	count;
int	sv;
int	c;
{
#define DO_PIXEL(op) \
	*(iva += sv) op c;

	DO_MODE_LINE

#undef DO_PIXEL

	return (iva);
}

INLINED static char BPTR * NEAR
ILoop (iva, dy, dx, dvx, dvy, c)
register char	BPTR *iva;
register Uint	dy;
Uint		dx;
register int	dvx;
int		dvy;
int		c;
{
	register short	err;
	short		count;

/* dx > dy  never == !!!
 * dx > 0
 * dy >= 0
*/

	count = dx;
	err = dx >> 1;

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
	if (x >= bWidth)
		x = bWidth-1;
	if (y >= bHeight)
		y = bHeight-1;
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
	if (x2 >= bWidth)
		x2 = bWidth-1;
	if (y2 >= bHeight)
		y2 = bHeight-1;
#endif
	t = 0;
	if (!xydone) {
		t = 1;
		xydone = 1;
		pva = bActiveBase + x1 + y1*(long)bXbytes;
		if (T_MSET == bGrWriteMode)
			*pva = c;
		else if (T_MOR == bGrWriteMode)
			*pva |= c;
		else if (T_MXOR == bGrWriteMode)
			*pva ^= c;
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
			if (T_MSET != bGrWriteMode)
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
	bGrWriteMode = mode;
	return (0);
}

extern void FAR
bScrClear (Uint x, Uint y, Uint sizex, Uint sizey, Uint color)
{
	for (sizey += y, sizex += x-1; y < sizey; ++y)
		bDrawLine (x, y, sizex, y, color);
}

/* Based on the version from ACM TOG V11N3 (July 1992), this one is
 * modified to do 2 quads at a time to reduce VGA page flips.
*/

#define POINT(x,y,c) \
	do {						\
		if (T_MSET == bGrWriteMode)		\
			*(xxyy+(x)+(y))  = (Uchar)(c);	\
		else if (T_MOR == bGrWriteMode)		\
			*(xxyy+(x)+(y)) |= (c);		\
		else if (T_MXOR == bGrWriteMode)	\
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
	crit1 = -(a2/4 + a%2 + b2);
	crit2 = -(b2/4 + b%2 + a2);
	crit3 = -(b2/4 + b%2);
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
