/* --------------------------------- kbdmgr.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Keyboard drivers manager.
*/

#include "fly.h"


extern struct KbdDriver NEAR* FAR
kbrd_init (char *name)
{
	struct KbdDriver	NEAR* FAR* p;
	char			*options;
	int			n;

	p = KbdDrivers;
	if (name) {
		if (T(options = strchr (name, ':')))
			n = options - name;
		else
			n = strlen (name);

		for (; *p; ++p)
			if (!strnicmp ((*p)->name, name, n) && !(*p)->name[n])
				break;
	}
	return (*p);
}

extern void FAR
kbrd_term (void)
{}
