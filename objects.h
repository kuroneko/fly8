/* --------------------------------- objects.h ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Object types.
*/

#ifndef FLY8_OBJECTS_H
#define FLY8_OBJECTS_H

/* NOTE: must keep (by hand) parms.h in sync with this enum!!!
*/

enum oname {
	O_EOF = -1,
	O_GROUND = 0, 	/*  0 */	/* REMEMBER to update parms.h! */
	O_PLANE,  	/*  1 */
	O_BOX,  	/*  2 */
	O_RUNWAY,  	/*  3 */
	O_M61,  	/*  4 */
	O_TARGET,  	/*  5 */
	O_BROKEN, 	/*  6 */
	O_VIEWER,  	/*  7 */
	O_CHUTE,  	/*  8 */
	O_HOUSE,  	/*  9 */
	O_TOWER,  	/* 10 */
	O_LOW,  	/* 11 */
	O_GTARGET,  	/* 12 */
	O_MK82, 	/* 13 */
	O_CRATER,  	/* 14 */
	O_SMOKE, 	/* 15 */
	O_CAR, 		/* 16 */

	O_INT, 		/* 17 */	/* number of   internal objects */
	O_LOCAL = O_INT+5,		/* room for  5 local    objects */
	O_ALL = O_LOCAL+10,		/* room for 10 external objects */
	O_DEFINE = 0x1000
};

typedef enum oname ONAME;

#endif
