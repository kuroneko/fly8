/* --------------------------------- shape.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Object shape header.
*/

#ifndef FLY8_SHAPE_H
#define FLY8_SHAPE_H

#define V_METERS	2
#define V_FINE		1

#define V_EOF		0
#define V_MOVE		1
#define V_DRAW		2
#define V_DUP		3

#define SH_G		0x0001
#define SH_HIT		0x0002
#define SH_BEHIT	0x0004
#define SH_LOCALSIM	0x0008
#define SH_FINE		0x0010
#define SH_DYNVERTEX	0x0020

#endif
