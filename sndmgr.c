/* --------------------------------- sndmgr.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound drivers manager.
*/

#include "fly.h"


extern struct SndDriver NEAR* FAR
sound_init (char *name)
{
	struct SndDriver	NEAR* FAR* p;
	char			*options;
	int			n;

	p = SndDrivers;
	if (name) {
		if (T(options = strchr (name, ':')))
			n = options - name;
		else
			n = strlen (name);

		if (!n)
			return (&SndNone);

		for (; *p; ++p)
			if (!strnicmp ((*p)->name, name, n) && !(*p)->name[n])
				return (*p);
		return (0);
	}
	if (!*p)
		return (&SndNone);
	return (*p);
}

extern void FAR
sound_term (void)
{}
