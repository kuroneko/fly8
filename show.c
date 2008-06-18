/* -------------------------------- show.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* A 3D graphics engine.
*/

#include "fly.h"


#if 0
/*
 * This one tries a different projection. It produces a more uniform
 * distortion in wide angle. The method is to project the world onto a sphere
 * and then use the latitude/longitude as the x/y on the screen. Perspective
 * projection shows objects larger on the edges that in the center, this one
 * shows equal size objects (same angle of view) with equal size.
 * The problem is that straight lines show as arcs!
*/
#define PROJ(x,pr,z) \
	 (projtype ? fmul((pr)*2,ATAN((x),(z))) : muldiv((x),(pr),(z)))
#else
#define PROJ(x,pr,z)	muldiv((x),(pr),(z))
#endif

#define PROJPT(x,y,z,org,pr) \
	if (z) \
		gr_move (org[X]+PROJ(x,pr[X],z), org[Y]-PROJ(y,pr[Y],z)); \
	else \
		gr_move (org[X], org[Y])

#define PROJMV(x,y,z,org,pr) \
	if (z) \
		gr_draw (org[X]+PROJ(x,pr[X],z), org[Y]-PROJ(y,pr[Y],z)); \
	else \
		gr_draw (org[X], org[Y])

#define GETALPHA(a,b)	(tt2 = B[Z]/2 + b/2 + (tt1 =  a/2 - A[Z]/2))

#define CLIP(a,b)	(a + 2*muldiv(b/2-a/2,tt1,tt2))

#define XXXX_RIGHT	0x01
#define XXXX_LEFT	0x02
#define XXXX_TOP	0x04
#define XXXX_BOTTOM	0x08
#define XXXX_BACK	0x10

static VECT	NEAR V1 = {0}, NEAR A = {0}, NEAR B = {0};
static int	NEAR SH[2] = {0};	/* viewport shift from eye, x and y */
static int	NEAR PR[2] = {0};	/* perspective ratio */
static int	NEAR ORG[2] = {0};	/* pixel origin */
static VECT	NEAR R = {0};		/* translation vector */
static MAT	NEAR TT = {{0}};	/* rotation matrix */
static VERTEX	NEAR V[1] = {{{0}}};	/* vertex being shown */

LOCAL_FUNC int NEAR
adj (int vvvv)
{
	int	tt1, tt2, tt3;

/* A straight forward clipping routine. Clip againt the right, left
 * top and bottom in order.
*/
	if (vvvv & XXXX_TOP) {
		GETALPHA (A[Y], -B[Y]);
		if (tt2  && (tt3 = CLIP (A[Z], B[Z])) > 0) {
			V1[Y] = V1[Z] = tt3;
			V1[X] = CLIP (A[X], B[X]);
			if (V1[X] <= V1[Z] && V1[X] >= -V1[Z])
				return (0);
		}
	}
	if (vvvv & XXXX_BOTTOM) {
		GETALPHA (-A[Y], B[Y]);
		if (tt2  && (tt3 = CLIP (A[Z], B[Z])) > 0) {
			V1[Y] = -(V1[Z] = tt3);
			V1[X] = CLIP (A[X], B[X]);
			if (V1[X] <= V1[Z] && V1[X] >= -V1[Z])
				return (0);
		}
	}

	if (vvvv & XXXX_RIGHT) {
		GETALPHA (A[X], -B[X]);
		if (tt2  && (tt3 = CLIP (A[Z], B[Z])) > 0) {
			V1[X] = V1[Z] = tt3;
			V1[Y] = CLIP (A[Y], B[Y]);
			if (V1[Y] <= V1[Z] && V1[Y] >= -V1[Z])
				return (0);
		}
	}
	if (vvvv & XXXX_LEFT) {
		GETALPHA (-A[X], B[X]);
		if (tt2  && (tt3 = CLIP (A[Z], B[Z])) > 0) {
			V1[X] = -(V1[Z] = tt3);
			V1[Y] = CLIP (A[Y], B[Y]);
			if (V1[Y] <= V1[Z] && V1[Y] >= -V1[Z])
				return (0);
		}
	}
	return (1);
}

LOCAL_FUNC void NEAR FASTCALL
show_line (void)
{
	int		aaaa, vvvv;
	static int	bbbb, prev = 0;

/* V->V is in object coordinates
*/
	VMmul (A, V->V, TT);		/* rotate */

/* A is in viewer's coordinates, but at origin.
*/
	Vinc (A, R);			/* translate (R already rotated) */

/* A is now in viewer coordinates, in position.
 *
 * We now swap y and z in the viewer's coordinate system:
 * x was 'right' (still is)
 * y was 'forward' (now is 'up', so the screen is x-y)
 * z was 'up' (now is 'forward', perpendicular to the screen)
*/
	aaaa = A[Z];	/* aaaa used as temp */
	A[Z] = A[Y];
	A[Y] = aaaa;

/* shift the x and y to compensate for viewport offset.
 * SH is in viewers coordinates.
 * This is not properly done. SH[] is always zero for now.
 * Actualy, the "dynamic" scaling for truncation control makes this
 * shift WRONG!
*/
	A[X] -= SH[X];
	A[Y] -= SH[Y];

/* aaaa is clipping pattern:
 *	0x01: x >  z (too far right)
 *	0x02: x < -z (too far left)
 *	0x04: y >  z (too far up)
 *	0x08: y < -z (too far down)
 *	0x10: z <  0 (behind)
*/
 	aaaa = 0;
	if (A[X] > A[Z])
		aaaa |= XXXX_RIGHT;
	if (A[X] < -A[Z])
		aaaa |= XXXX_LEFT;
	if (A[Y] > A[Z])
		aaaa |= XXXX_TOP;
	if (A[Y] < -A[Z])
		aaaa |= XXXX_BOTTOM;
	if (A[Z] < 0)
		aaaa |= XXXX_BACK;

/* Use the bit pattern of the current point and the previous one to
 * attempt fast rejects/accepts and avoid un-necessary clipping.
 *
 * 'V_MOVE' means the segment into this point is not to be drawn.
 *
 * 'prev' is true if the last point was not drawn (which means the
 * graphics device is not at the current position).
 *
 * adj() clips one endpoint.
 *
 * PROJMV() projects a point and draw a line to it.
 *
 * PROJPT() projcts a point and issues command to move there without
 * drawing (just a re-positioning).
*/
	if (V_MOVE == V->flags || V_DUP == V->flags)
		goto noline;

	++STATS_CLIPCOUNT;

	if (aaaa == bbbb) {
		if (aaaa) {
			++STATS_CLIPOUT;
			goto reject;
		} else
			goto same;
	}
	if (aaaa & bbbb) {
		++STATS_CLIPOUT;
		goto reject;
	}
	if (0 == prev) {
		++STATS_CLIPINOUT;
	 	vvvv = aaaa;
		goto newout;
	}
	if (0 == bbbb)
		goto oldin;
	if (0 == aaaa)
		goto newin;

/* The case is undecided so one clipping is attempted. If it fails
 * then we reject this segment. This is the only time a failure is
 * expected. In the other cases, a failure of adj() means that an
 * unexpected problem prevented it from working - usualy a result
 * of lost resolution due to truncation.
*/
	if (adj (aaaa)) {		/* a trial clipping */
		++STATS_CLIPOUTHARD;
		goto noline;
	}
	PROJPT (V1[X], V1[Y], V1[Z], ORG, PR);
	vvvv = bbbb;
newout:
	if (adj (vvvv)) {		/* should not fail */
failed:
		++STATS_CLIPFAILED;
		goto noline;
	}
	++STATS_CLIPINHARD;
	PROJMV (V1[X], V1[Y], V1[Z], ORG, PR);
reject:
noline:
	prev = 1;
	goto ret;

oldin:
	PROJPT (B[X], B[Y], B[Z], ORG, PR);
	++STATS_CLIPINOUT;
	vvvv = aaaa;
	goto newout;
newin:
	++STATS_CLIPINOUT;

	if (adj (bbbb)) {		/* should not fail */
		vvvv = bbbb;
		goto failed;
	}
	PROJPT (V1[X], V1[Y], V1[Z], ORG, PR);
	goto newin1;
same:
	++STATS_CLIPIN;
	if (prev) {
		PROJPT (B[X], B[Y], B[Z], ORG, PR);
	}
newin1:
	PROJMV (A[X], A[Y], A[Z], ORG, PR);
	prev = 0;
ret:
	Vcopy (B, A);
	bbbb = aaaa;
}
#undef PROJ
#undef PROJPT
#undef PROJMV
#undef GETALPHA
#undef CLIP

static VERTEX FAR shape_dot[] = {
	{{0, 0, 0}, V_MOVE},
	{{0, 0, 0}, V_DRAW},
	{{0, 0, 0}, V_EOF}
};

LOCAL_FUNC void NEAR
show_object (int mode, OBJECT *obj , MAT VT, LVECT VR, long minextent,
	LVECT OR, VECT RR)
{
	VERTEX		*v;
	VECT		P;
	LVECT		LP;
	long		l, dist, extent;
	int		scale, too_far, too_near, obj_size, depth;
	int		t, b;
	int		color;

/* Get vector from viewr to object.
*/
	if (1 == mode)
		Vsub (LP, OR, VR);
	else
		Vsub (LP, obj->R, VR);

/* If the object is too far, show only a dot. This enables the
 * use of 16-bit arithmetic in a 32-bit world. If it is much further
 * then don't show it at all.
*/
	dist = labs (LP[X]);
	if ((l = labs (LP[Y])) > dist)
		dist = l;
	if ((l = labs (LP[Z])) > dist)
		dist = l;

	if (0 == mode) {
		if (dist/4/minextent > (long)SH(obj)->extent)
			return;
		obj_size = SH(obj)->extent;
		extent = dist + obj_size*(long)VONE;
	} else {
		obj_size = 1;
		extent = dist;
	}

	too_near = too_far = 0;
	if (extent == 0) {
		if (0 == mode)
			return;
		P[X] = (int)LP[X];
		P[Y] = (int)LP[Y];
		P[Z] = (int)LP[Z];
		scale = 1;
	} else if (extent > 0x7fffL*VONE) {

/* Bring far objects nearer.
*/
		too_far = 1;
		scale = (int)(extent / (0x7fff/2));
		scale = (scale/VONE)*VONE;
		P[X] = (int)(LP[X] / scale);
		P[Y] = (int)(LP[Y] / scale);
		P[Z] = (int)(LP[Z] / scale);
		scale /= VONE;
		obj_size /= scale;
	} else if (extent <= 0x7fff/4) {

/* Reduce truncation errors: push near points away.
*/
		too_near = 1;
		scale = (int)((0x7fff/2) / extent);
		P[X] = ((int)LP[X]) * scale;
		P[Y] = ((int)LP[Y]) * scale;
		P[Z] = ((int)LP[Z]) * scale;
		scale *= VONE;
		obj_size *= scale;
	} else if (extent <= 0x7fffL*VONE/4) {

/* Reduce truncation errors: push near points away.
*/
		too_near = 1;
		scale = (int)((0x7fffL*VONE/2) / extent);
		P[X] = (int)((LP[X] * scale) / VONE);
		P[Y] = (int)((LP[Y] * scale) / VONE);
		P[Z] = (int)((LP[Z] * scale) / VONE);
		obj_size *= scale;
	} else {

/* Shift out the fraction and move to 16-bits.
*/
		P[X] = (int)vuscale (LP[X]);
		P[Y] = (int)vuscale (LP[Y]);
		P[Z] = (int)vuscale (LP[Z]);
		scale = 1;
	}

/* Rotate to viewer's orientation.
*/
	if (1 == mode) {
		VMmul (RR, P, VT);
		return;
	}

	VMmul (R, P, VT);	/* object's origin in viewer coordinates */

/* Rough check for object out of view (trivial reject).
*/
	if (R[Y] <= -obj_size)		/* all behind */
		return;
	depth = R[Y]/2 + obj_size;	/* avoid truncation */
	if (iabs(R[X]/2) > (Uint)depth || iabs(R[Z]/2) > (Uint)depth)
		return;

/* Post conatenate Viewer's transposed matrix to the Object's.
*/
	VMmul (TT[0], obj->T[0], VT);	/* Mmul (TT, obj->T, VT);*/
	VMmul (TT[1], obj->T[1], VT);
	VMmul (TT[2], obj->T[2], VT);

/* Far objects shown as one dot.
*/
	if (dist/minextent > (long)SH(obj)->extent)
		v = shape_dot;
	else
		v = SH(obj)->v;

/* Set object color
*/
	if (dist*2/minextent > (long)SH(obj)->extent)
		color = ST_FAINT;	/* farther than dot/2 */
	else if (dist*4/minextent > (long)SH(obj)->extent)
		color = ST_DULL;	/* farther than dot/4 */
	else
		color = obj->color;
	gr_color (color);

/* Show each line segment in turn.
*/
	if (SH(obj)->flags & SH_FINE) {
		if (too_near) {
			t = scale;
			b = VONE;
		} else if (too_far) {
			t = 1;
			b = scale * VONE;
		} else {
			t = 1;
			b = VONE;
		}
	} else {
		if (too_near) {
			t = scale;
			b = 1;
		} else if (too_far) {
			t = 1;
			b = scale;
		} else {
			t = 1;
			b = 1;
		}
	}

	for (; v->flags != V_EOF; ++v) {
#if 0
		Vmuldiv (V->V, v->V, t, b);
#else
		if (1 == t) {
			if (1 == b)
				Vcopy (V->V, v->V);
			else {
				V->V[X] = v->V[X] / b;
				V->V[Y] = v->V[Y] / b;
				V->V[Z] = v->V[Z] / b;
			}
		} else if (1 == b) {
			V->V[X] = v->V[X] * t;
			V->V[Y] = v->V[Y] * t;
			V->V[Z] = v->V[Z] * t;
		} else {
			V->V[X] = muldiv (v->V[X], t, b);
			V->V[Y] = muldiv (v->V[Y], t, b);
			V->V[Z] = muldiv (v->V[Z], t, b);
		}
#endif
		V->flags = v->flags;
		show_line ();
	}
}

#define TSCALE	FCON(0.57735)	/* sqrt(1/3): cube diagonal to edge ratio */

/* mode:
 * 0 show world
 * 1 transform OR into RR
*/
extern void FAR
objects_show (int mode, VIEW *view, OBJECT *pov, LVECT OR, VECT RR)
{
	int		i;
	OBJECT		*p;
	MAT		VT;		/* rotates world to viewer */
	LVECT		R;		/* eye position */
	VECT		SC;		/* scaling */
	VECT		E, EYE;  	/* relative eye shift */
	int		j;
	int		l, t;
	long		minextent;	/* minimum extent ratio to show */

	if (1 != mode) {
		STATS_TCLIPOUT       += STATS_CLIPOUT;
		STATS_TCLIPIN        += STATS_CLIPIN;
		STATS_TCLIPINOUT     += STATS_CLIPINOUT;
		STATS_TCLIPOUTHARD   += STATS_CLIPOUTHARD;
		STATS_TCLIPFAILED    += STATS_CLIPFAILED;
		STATS_TCLIPINHARD    += STATS_CLIPINHARD;
		STATS_TCLIPCOUNT     += STATS_CLIPCOUNT;

		STATS_CLIPOUT         = 0;
		STATS_CLIPIN          = 0;
		STATS_CLIPINOUT       = 0;
		STATS_CLIPOUTHARD     = 0;
		STATS_CLIPFAILED      = 0;
		STATS_CLIPINHARD      = 0;
		STATS_CLIPCOUNT       = 0;

		Tm->Interval (TMR_START, 0L);
	}

/* This is the entry point of this module.
 *
 * Adjust for eye distance and viewport size so that after total
 * scaling there is no overflow. TSCALE is sqrt(1/3) and is the
 * minimal scaling needed due to the 3D rotation. The rest is a
 * distortion that accounts for the pictures aspect ratio; later,
 * the program works in a square (45 degrees) clipping area.
*/
	l = VP->z;				/* get minimum */
	if (l > VP->maxx)
		l = VP->maxx;
	if (l > VP->maxy)
		l = VP->maxy;
	SC[X] = muldiv (TSCALE, l, VP->maxx);	/* never divide by zero!!! */
	SC[Y] = muldiv (TSCALE, l, VP->maxy);
	SC[Z] = muldiv (TSCALE, l, VP->z);

/* Adjust for viewer's offset to the viewport.
 * (not quite right)
*/
	SH[X] = fmul (VP->x, SC[X]);
	SH[Y] = fmul (VP->y, SC[Y]);

/* Get 2D display area parameters.
*/
	get_area (view, ORG+X, ORG+Y, PR+X, PR+Y);

/* Swap y and z when changing to viewer coordinates.
*/
	t = SC[Y];
	SC[Y] = SC[Z];
	SC[Z] = t;

/* a mirror has left-right swapped.
*/
	if (VP->flags & VF_MIRROR)
		PR[X] = -PR[X];

/* Transpose viewer orientation matrix.
 * The object's T maps from the object to the world and for
 * the viewer we need the reverse mapping.
*/
	Mcopy (VT, pov->T);
	Mxpose (VT);

/* Rotate for viewer's gaze direction (and stereo cross-eye).
*/
	if (VP->rotz)
		Mrotz (VT, VP->rotz);
	if (VP->rotx)
		Mrotx (VT, VP->rotx);
	if (VP->roty)
		Mroty (VT, VP->roty);

/* Now do the scaling.
*/
	for (i = 0; i < 3; ++i)		/* scale */
		for (j = 0; j < 3; ++j)
			VT[j][i] = fmul (VT[j][i], SC[i]);

/* Find eye position.
*/
	EYE[X] = VP->eyex + VP->shift / VONE;
	EYE[Y] = VP->eyey;
	EYE[Z] = VP->eyez;
	VMmul (E, EYE, pov->T);
	Vadd (R, pov->R, E);

/* Calculate maximum distance for showing detail.
*/
#if 1
	minextent = VP->z * (long)VS->sizex / VP->maxx * VONE;
#else
	minextent = VP->z * (long)VS->sizex / VP->maxx * (VONE/2);
#endif
	if (minextent < 1)
		minextent = 1;

/* If only object position requested, get it and quit.
*/
	if (1 == mode) {
		show_object (1, 0, VT, R, minextent, OR, RR);
		return;
	}

/* Now show each object in turn.
*/
	for (i = 0, p = CL; p; p = p->next) {
		if (p->flags & F_VISIBLE) {
			show_object (0, p, VT, R, minextent, 0, 0);
			if (!i--) {
				i = 20;
				sys_poll (6);		/* poll frequently */
			}
		}
	}

	for (p = CO; p; p = p->next) {
		if (p == pov)			/* don't show viewer */
			continue;
		if (p->flags & F_VISIBLE) {
			show_object (0, p, VT, R, minextent, 0, 0);
			if (!i--) {
				i = 20;
				sys_poll (7);		/* poll frequently */
			}
		}
	}
	STATS_TIME3D += Tm->Interval (TMR_STOP, 10000L);
}

#undef TSCALE
