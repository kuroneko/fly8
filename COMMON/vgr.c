/* --------------------------------- vgr.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* drawing primitives for a banked video system (like VGA).
 *
 * About the 'poll' business:
 * On some slow platforms (msdos) we must poll during long operations.
 * Yes, this is a very bad hack...
*/

#include "fly.h"
#include "vgr.h"

#ifndef VGASET
#define VGASET(b, o, c, n)	memset ((b)+(o), (c), (n))
#endif

#define BANK_FIX(va,vb) \
	if ((Uint)((va)^(vb)) >= 0xc000U) {	\
		va &= 0x0ffffU;			\
		if ((va) >= 0xc000U)		\
			vSetBank (--vBank);	\
		else if ((va) < 0x4000U)	\
			vSetBank (++vBank);	\
	}

#define SET_PIXEL(va,c) \
	if (T_MSET == vWriteMode)	\
		vMem[va] = (Uchar)(c);	\
	else if (T_MOR == vWriteMode)	\
		vMem[va] |= (c);	\
	else				\
		vMem[va] ^= (c)

static Uchar	FAR *vMem = NULL;
static Uint	vWidth = 0, vHeight = 0, vXbytes = 0;
static int	(FAR *vSetBank) (int bank) = NULL;
static int	(FAR *vSetBase) (Ulong base) = NULL;
static Ulong	vActiveBase = 0;
static int	vBank = 0;
static int	vWriteMode = T_MSET;
static Uint	x1 = 0, y1 = 0, xydone = 0;
static Ushort	pva = 0;
#ifdef USE_DIVTAB
static Uint	FAR vDivTab[2048] = {0};
#endif

extern int FAR
vSetWriteMode (int mode)
{
	vWriteMode = mode;
	return (0);
}
	
extern int FAR
vSetActive (int page)
{
	vActiveBase = page * (Ulong)vXbytes * (Ulong)vHeight;
	xydone = 0;
	return (0);
}
	
extern int FAR
vSetVisual (int page)			/* Set visual address */
{
	if (vSetBase)
		return (vSetBase (page * (Ulong)vXbytes * (Ulong)vHeight));
	else
		return (1);
}

extern int FAR
vSetMem (Uchar *vmem)
{
	vMem = vmem;
	vActiveBase = 0;
	return (0);
}

extern int FAR
vSetSize (Uint width, Uint height, Uint xbytes)
{
	vWidth  = width;
	vHeight = height;
	vXbytes = xbytes;
	return (0);
}

extern void FAR
vInit (Uchar *vmem, Uint width, Uint height, Uint xbytes,
	int (FAR *setbank) (int page),
	int (FAR *setbase) (Ulong base))
{
	vMem = vmem;
	vWidth = width;
	vHeight = height;
	vXbytes = xbytes;
	vSetBank = setbank;
	vSetBase = setbase;

	vSetWriteMode (T_MSET);
	vSetActive (0);
	vSetVisual (0);

#ifdef USE_DIVTAB
	{
		int	i;

		vDivTab[0] = 0L;
		for (i = 1; i < rangeof (vDivTab); ++i)
			vDivTab[i] = (Uint)(0x00010000UL / i);
	}
#endif
}
	
/* This does pure C drawing.
*/

INLINED static Ushort NEAR
vILoop (Uint va, Uint dy, Uint dx, int dvx, int dvy, int c)
{
	int	err;
	int	count;
	Uint	vb;

/* dx > dy  never =!!!
 * dx > 0
 * dy >= 0
*/
	count = dx;
	err = dx >> 1;

	if (T_MSET == vWriteMode) {
		while (--count >= 0) {
			vb = va;
			va += dvx;
			if ((err -= dy) < 0) {
				err += dx;
				va += dvy;
			}
			BANK_FIX (va, vb);
			vMem[va] = (Uchar)c;
		}
	} else if (T_MOR == vWriteMode) {
		while (--count >= 0) {
			vb = va;
			va += dvx;
			if ((err -= dy) < 0) {
				err += dx;
				va += dvy;
			}
			BANK_FIX (va, vb);
			vMem[va] |= c;
		}
	} else {
		while (--count >= 0) {
			vb = va;
			va += dvx;
			if ((err -= dy) < 0) {
				err += dx;
				va += dvy;
			}
			BANK_FIX (va, vb);
			vMem[va] ^= c;
		}
	}

	return ((Ushort)va);
}

INLINED static Ushort NEAR
vSLoop (Uint va, int count, int sv, int c)
{
	Uint	vb;

	if (T_MSET == vWriteMode) {
		while (--count >= 0) {
			vb = va;
			va += sv;
			BANK_FIX (va, vb);
			vMem[va] = (Uchar)c;
		}
	} else if (T_MOR == vWriteMode) {
		while (--count >= 0) {
			vb = va;
			va += sv;
			BANK_FIX (va, vb);
			vMem[va] |= c;
		}
	} else {
		while (--count >= 0) {
			vb = va;
			va += sv;
			BANK_FIX (va, vb);
			vMem[va] ^= c;
		}
	}

	return ((Ushort)va);
}

INLINED static Ushort NEAR
vGrAddr (Uint x, Uint y)
{
	Ulong	xy;
	int	bank;

	bank = (int)((xy = x + vActiveBase + y*(Ulong)vXbytes) >> 16);
	if (bank != vBank)
		vSetBank (vBank = bank);

	return ((Ushort)(0x0ffffU & (Ushort)xy));
}

extern void FAR
vMoveTo (Uint x, Uint y)
{
#if DEBUG_VGR
	if (x >= vWidth) {
		++STATS_GRERRMOVEXHIGH;
		x = vWidth-1;
	}
	if (y >= vHeight) {
		++STATS_GRERRMOVEYHIGH;
		y = vHeight-1;
	}
#endif
	x1 = x;
	y1 = y;
	xydone = 0;
}

extern void FAR
vDrawTo (Uint x2, Uint y2, Uint c)
{
	int	dx, dy, svx, svy;
	int	t;

#if DEBUG_VGR
	if (x2 >= vWidth) {
		++STATS_GRERRDRAWXHIGH;
		x2 = vWidth-1;
	}
	if (y2 >= vHeight) {
		++STATS_GRERRDRAWYHIGH;
		y2 = vHeight-1;
	}
#endif
	t = 0;
	if (!xydone) {
		t = 1;
		xydone = 1;
		pva = vGrAddr (x1, y1);
		SET_PIXEL (pva, c);
	}

	svx = 1;
	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		svx = -1;
	}

	svy = vXbytes;
	if ((dy = y2 - y1) < 0) {
		dy = -dy;
		svy = -svy;
	}

	if (dx > dy) {
		if (0 == dy) {
			if (T_MSET != vWriteMode)
				pva = vSLoop (pva, dx, svx, c);
			else if (svx >= 0) {
				Ushort	room;

				room = (Ushort)(0xffffU - pva);
				if ((Ushort)dx <= room) {
					VGASET (vMem, pva+1, c, dx);
					pva += dx;
				} else {
					if (room > 0)
						VGASET (vMem, pva+1, c, room);
					vSetBank (++vBank);
					pva = dx - room - 1;
					VGASET (vMem, 0, c, pva+1);
				}
			} else {
				if (pva >= (Ushort)dx) {
					pva -= dx;
					VGASET (vMem, pva, c, dx);
				} else {
					VGASET (vMem, 0, c, pva);
					vSetBank (--vBank);
					dx -= pva;
					VGASET (vMem,
						pva = (Ushort)(0x10000UL - dx),
						c, dx);
				}
			}
		} else
			pva = vILoop (pva, dy, dx, svx, svy, c);
		++GrStats[dx+t];
	} else if (dx < dy) {
		if (0 == dx)
			pva = vSLoop (pva, dy, svy, c);
		else
			pva = vILoop (pva, dx, dy, svy, svx, c);
		++GrStats[dy+t];
	} else /* if (dx == dy) */ {
		if (dx)
			pva = vSLoop (pva, dx, svx+svy, c);
		++GrStats[dx+t];
	}

	x1 = x2;
	y1 = y2;
}

extern void FAR
vClear (Uint x, Uint y, Uint sx, Uint sy, Uint color)
{
	long	room;
#if POLL_RATE
	int	poll = 0;
#endif

	if (!sx || !sy)
		return;

	if (T_MSET == vWriteMode) {
		pva = vGrAddr (x, y);
		room = 0x10000UL - pva;		/* room to end of bank */
		for (; sy--; ++y) {
			if (room <= 0) {
				room += 0x10000UL;
				vSetBank (++vBank);
			}
			if ((long)sx > room) {
				VGASET (vMem, pva, color, (Uint)room);
				vSetBank (++vBank);
				VGASET (vMem, 0, color, sx-(Uint)room);
				pva  = vXbytes - (Uint)room;
				room = 0x10000UL - pva;
			} else {
				VGASET (vMem, pva, color, sx);
				pva  += vXbytes;
				room -= vXbytes;
			}
#if POLL_RATE
			if (!(++poll % POLL_RATE))
				sys_poll (2);
#endif
		}
		vMoveTo (x+sx-1, y-1);
	} else {
		for (sx += x-1; sy--; ++y) {
			vMoveTo (x,  y);
			vDrawTo (sx, y, color);
#if POLL_RATE
			if (!(++poll % POLL_RATE))
				sys_poll (2);
#endif
		}
	}
}


/* Based on the acm version from TOG V11N3 (July 1992), this one is
 * modified to do 2 quads at a time to limit VGA bank flips.
*/

#define POINT(x,y,c) \
	do { 							\
		if (xy = xxyy+(x)+(y), (int)(xy>>16) != vBank)	\
			vSetBank (vBank = (int)(xy>>16));	\
		va = 0x0ffffU & (Ushort)xy;			\
		SET_PIXEL (va, c); 				\
	} while (0)

#define INCX() \
	(x++, b2x += b2, dxt += d2xt, t += dxt)

#define INCY() \
	(y--, a2y -= a2, dyt += d2yt, t += dyt, yy -= vXbytes)

extern void FAR
vEllipse (Uint xc, Uint yc, Uint a, Uint b, Uint c)
{
	long	xxyy;
	Ulong	xy;
	Ushort	va = 0;
	int	x = 0, y = b;
	long	yy = y*(long)vXbytes;
	long	a2 = (long)a*a, b2 = (long)b*b;
	long	crit1 = -(long)(a2/4 + a%2 + b2);
	long	crit2 = -(long)(b2/4 + b%2 + a2);
	long	crit3 = -(long)(b2/4 + b%2);
	long	t = -a2*y;
	long	dxt = 2*b2*x, dyt = -2*a2*y;
	long	d2xt = 2*b2, d2yt = 2*a2;
	long	b2x = b2*x, a2y = a2*y;

	if (xc < a || xc+a >= vWidth ||
	    yc < b || yc+b >= vHeight)
		return;

	xxyy = xc + vActiveBase + yc * (long)vXbytes;

	while (y >= 0 && x <= (int)a) {
		POINT (x, yy, c);
		if (x && y)
			POINT (-x,  yy, c);
		if (t + b2x <= crit1 || t + a2y <= crit3)
			INCX ();
		else if (t - a2y > crit2)
			INCY ();
		else {
			INCX ();
			INCY ();
		}
	}

	x = 0, y = b;
	yy = y*(long)vXbytes;
	t = -a2*y;
	dxt = 2*b2*x, dyt = -2*a2*y;
	d2xt = 2*b2, d2yt = 2*a2;
	b2x = b2*x, a2y = a2*y;

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
#undef BANK_FIX
#undef SET_PIXEL
#undef POINT
#undef INCX
#undef INCY

extern void FAR
vPolygon (int npoints, BUFLINE *points, Uint color)
{
	Uint	y;			/* current y */
	int	fwdx, fwdy, bwdx, bwdy;	/* current edge tops */
	int	fdx, fdy, bdx, bdy;	/* interpolation */
	int	fx, bx;			/* segment endpoints */
	BUFLINE	*lastpoint;		/* end of 'points' */
	BUFLINE	*fwd, *bwd;		/* current point */
	BUFLINE	*nfwd, *nbwd;		/* next point */
	long	room;
	Ushort	va, n;
#if POLL_RATE
	int	poll = 0;
#endif

	if (npoints < 2)
		return;

	if (2 == npoints) {
		vMoveTo (points[1], points[2]);
		vDrawTo (points[3], points[4], color);
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

	if (T_MSET == vWriteMode) {
		xydone = 0;		/* don't bother keeping currency */
		va = vGrAddr (0, y);
		room = 0x10000UL - va;		/* room to end of bank */
	}

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
		if (T_MSET == vWriteMode) {
			if (fx > bx) {
				n  = fx;
				fx = bx;
				bx = n;
			}
			n = bx - fx + 1;
			if ((long)fx >= room)
				vSetBank (++vBank);
			va = (Ushort)(0x0ffffU & (va + fx));
			room = 0x10000UL - va;
			if ((long)n > room) {
				VGASET (vMem, va, color, (Uint)room);
				n -= (Uint)room;
				va = 0;
				room = 0x10000UL;
				vSetBank (++vBank);
			}
			VGASET (vMem, va, color, n);
			n += vXbytes - bx - 1;
			va   += n;
			room -= n;
		} else {
			vMoveTo (fx, y);
			vDrawTo (bx, y, color);
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
	if (T_MSET == vWriteMode) {
		if (fx > bx) {
			n  = fx;
			fx = bx;
			bx = n;
		}
		n = bx - fx + 1;
		if ((long)fx >= room)
			vSetBank (++vBank);
		va = (Ushort)(0x0ffffU & (va + fx));
		room = 0x10000UL - va;
		if ((long)n > room) {
			VGASET (vMem, va, color, (Uint)room);
			n -= (Uint)room;
			va = 0;
			vSetBank (++vBank);
		}
		VGASET (vMem, va, color, n);
	} else {
		vMoveTo (fx, y);
		vDrawTo (bx, y, color);
	}
}
