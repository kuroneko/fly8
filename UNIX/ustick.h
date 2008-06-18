/* --------------------------------- ustick.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

#ifndef FLY8_UNIX_USTICK_H
#define FLY8_UNIX_USTICK_H

#include "stick.h"

/* Common joystick definitions for unix joystick drivers.
*/

typedef struct ustick	USTICK;
struct ustick {
	char	*name;
	int	(FAR *init) (int which, char *options, int opts);
	int	(FAR *term) (int which, int opts);
	Uint	(FAR *read) (int which, STICK *s, int mask, int opts);
};

#endif
