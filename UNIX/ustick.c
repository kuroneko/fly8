/* --------------------------------- ustick.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the joystick as a pointing device for Linux/X11.
 *
 *  This one sits above joyport.c/gameport.c and under stick.c.
*/

#include "fly.h"

#if HAVE_JOYSTICK

#include "ustick.h"

#include <stdlib.h>


extern USTICK	gameport;
extern USTICK	gpport;
extern USTICK	joyport;

static USTICK	*stick = NULL;


extern int
initstick (int which, char *options, int opts)
{
	if (get_parg (options, "game"))
		stick = &gameport;
	else if (get_parg (options, "gp"))
		stick = &gpport;
	else
		stick = &joyport;

	MsgPrintf (-50, "joystick device: %s", stick->name);

	return (stick->init (which, options, opts));
}

extern int
termstick (int which, int opts)
{
	int	ret;

	if (stick) {
		ret = stick->term (which, opts);
		stick = NULL;
	} else
		ret = 0;

	return (ret);
}

extern Uint
readstick (int which, STICK *j, int mask, int opts)
{
	return (stick ? stick->read (which, j, mask, opts) : 0x0ff);
}

#endif /* if HAVE_JOYSTICK */
