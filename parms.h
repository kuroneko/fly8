/* --------------------------------- parms.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common header for plane parm modules.
*/

#ifndef FLY8_PARMS_H
#define FLY8_PARMS_H

#include "colors.h"
#include "shape.h"

#define	VONE		16
#define	D180		32768
#define	D90		16384
#define	VD90		(D90/VONE)
#define	VD180		(VD90*2)
#define	VD360		(VD90*4)
#define	DEG2ANG(x)	((x)*D90/90)

#define	FONE		16384
#define	FCON(c)		(FONE*(c))

#define	HUD_TYPE	256
#define	HUD_CLASSIC	(0*HUD_TYPE)
#define	HUD_FA18	(1*HUD_TYPE)
#define	HUD_F16		(2*HUD_TYPE)
#define	HUD_F15		(3*HUD_TYPE)
#define	HUD_FLIR	(4*HUD_TYPE)

#define MODEL_BASIC	0
#define MODEL_CLASSIC	1
#define MODEL_XPLANE	2
#define MODEL_YPLANE	3
#define MODEL_FPLANE	4

#define F(x)		FCON(x)
#define F10(x)		FCON(x/10)
#define VV(x)		(VONE*VONE*(x))
#define V(x)		(VONE*(x))
#define G		9.810
#define D(x)		DEG2ANG(x)
#define DV(x)		((x)*VD90/90)
#define I10(x)		((x)/10)

#define O_EOF		-1
#define O_GROUND	 0
#define O_PLANE		 1
#define O_BOX		 2
#define O_RUNWAY	 3
#define O_M61		 4
#define O_TARGET	 5
#define O_BROKEN	 6
#define O_VIEWER	 7
#define O_CHUTE		 8
#define O_HOUSE		 9
#define O_TOWER		10
#define O_LOW		11
#define O_GTARGET	12
#define O_MK82		13
#define O_CRATER	14
#define O_SMOKE		15
#define O_CAR		16

#define O_INT		17
#define O_LOCAL		(O_INT+5)
#define O_DEFINE	0x1000

#endif
