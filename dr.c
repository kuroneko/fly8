/* ------------------------------------ dr.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dead reckoning matters. Will be used for the implementation of the
 * DIS protocol.
*/

#include "fly.h"


Given any Entity, we have:

	Entity linear velocity (v:V0) (World)
	Entity location (v:P0) (World)
	Entity orientation (m:R0) (World->Body)

	Dead Reckoning method
	Dead Reckoning Linear acceleration (v:A0) (World)
	Dead Reckoning Angular velocity (v:W) (Body)

	Delta time (dT)

The Dead Reckoning process then works as followes.

	w = sqrt (Wx**2+Wy**2+Wz**2)
	Phi = w * dT

	A = W/w

	I = Identity matrix

	AX =	  0	-Az	 Ay
		 Az	  0	-Ax
		-Ay	 Ax	  0

	A*At =	Ax*Ax	Ax*Ay	Ax*Az
		Ay*Ax	Ay*Ay	Ay*Az
		Az*Ax	Az*Ay	Az*Az

	DR = I*cos(Phi) - AX*sin(Phi) + A*At*(1-cos(Phi)

If the dead reckoning parameters are given in body axis then:

	Vb = Velocity in body axis (given)
	AB = Linear Acceleration in body axis (given)

Ab is the Effective Linear Acceleration in body axis:

	Ab = AB - AX*Vb

	dw = w*dT
	s = sin(dw)
	c = cos(dw)


	     dw-s        s     1-c
	R1 = ----*W*Wt + -*I + ----*OM
	     w**3        w     w**2


	     1/2*dw**2-c-dw*s+1        c+dw*s-1     s-dw*c
	R2 = ------------------*W*Wt + --------*I + ------*OM
	     w**4                      w**2         w**3


DRM_STATIC:
	All static, no change.

DRM_FPW:
	P = P0 + V0 * dT

DRM_RPW:
	P = P0 + V0 * dT
	R = DR * R0
	EA = EA0 + EAr*dT

DRM_RVW:
	P = P0 + V0 * dT + 1/2 * A0 * dT**2
	R = DR * R0
	EA = EA0 + EAr*dT

DRM_FVW:
	P = P0 + V0 * dT + 1/2 * A0 * dT**2

DRM_FPB:
	P = P0 +R**-1*(R1*Vb)

DRM_RPB:
	P = P0 +R**-1*(R1*Vb)
	R = DR * R0
	EA extracted from R

DRM_RVB:
	P = P0 +R**-1*(R1*Vb + R2*Ab)
	R = DR * R0
	EA extracted from R

DRM_FVB:
	P = P0 +R**-1*(R1*Vb + R2*Ab)



/*
 * The relationship between Fly8 and DIS is this:
 *
 *	Fly8 x		DIS y
 *	Fly8 a[x]	DIS Theta
 *	Fly8 y		DIS x
 *	Fly8 a[y]	DIS Phi
 *	Fly8 z		DIS -z
 *	Fly8 a[z]	DIS -Psi
 *
 * So, suppose we keep the names as:
 *
 *	x pitch
 *	y Roll
 *	z heading
 *
 * This is the Fly8 orientation matrix:
 *
 *	.[0]		.[1]		.[2]
 * [0].	cy*cz-sy*sz*sx	cy*sz+sy*cz*sx	-sy*cx	[0].
 * [1].	-cx*sz		cx*cz		sx	[1].
 * [2].	sy*cz+cy*sz*sx	sy*sz-cy*cz*sx	cy*cx	[2].
 *	.[0]		.[1]		.[2]
 *
 * First we apply
 *
 *	z -> -z (heading)
 *
 * so we apply: sz -> -sz)
 *
 *	.[0]		.[1]		.[2]
 * [0].	cy*cz+sy*sz*sx	-cy*sz+sy*cz*sx	-sy*cx	[0].
 * [1].	cx*sz		cx*cz		sx	[1].
 * [2].	sy*cz-cy*sz*sx	-sy*sz-cy*cz*sx	cy*cx	[2].
 *	.[0]		.[1]		.[2]
 *
 * Then we rename the axes:
 *
 *	x -> y
 *	y -> x
 *
 * so we swap rows 0 and 1, and columns 0 and 1:
 *
 *	.[0]		.[1]		.[2]
 * [0].	cx*cz		cx*sz		sx	[1].
 * [0].	-cy*sz+sy*cz*sx	cy*cz+sy*sz*sx	-sy*cx	[0].
 * [2].	-sy*sz-cy*cz*sx	sy*cz-cy*sz*sx	cy*cx	[2].
 *	.[0]		.[1]		.[2]
 *
 * Now we apply
 *
 *	z -> -z (the axis)
 *
 * so we negate row 2 and column 2:
 *
 *	.[0]		.[1]		.[2]
 * [0].	cx*cz		cx*sz		-sx	[1].
 * [0].	-cy*sz+sy*cz*sx	cy*cz+sy*sz*sx	sy*cx	[0].
 * [2].	+sy*sz+cy*cz*sx	-sy*cz+cy*sz*sx	cy*cx	[2].
 *	.[0]		.[1]		.[2]
 *
 * That's it. Some cosmetics:
 *
 *	.[0]		.[1]		.[2]
 * [0].	cx*cz		cx*sz		-sx	[1].
 * [0].	sy*cz*sx-cy*sz	sy*sz*sx+cy*cz	sy*cx	[0].
 * [2].	cy*cz*sx+sy*sz	cy*sz*sx-sy*cz	cy*cx	[2].
 *	.[0]		.[1]		.[2]
 *
 * However, this is a big lie. The two formulaes (the one above and
 * the one in appendix B) appear identical, but FLy8 keeps the b->w
 * matrix while DIS defined the w->b! Luckily, Fly8 also uses row vectors
 * when DIS uses column vectors so we are OK (the inverse is here the
 * transpose).
*/

