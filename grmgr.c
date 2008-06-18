/* --------------------------------- grmgr.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Top level for display-device drivers
*/

#include "fly.h"


extern DEVICE * FAR
devices_select (char *dname)
{
	long	xmode;
	char	*p;
	DEVICE	*dev;

	if (vm_read ())
		return (0);

	if (!dname)			/* return default */
		return (Gr->devices);

	if (!get_niarg (dname, 0, &xmode))
		xmode = 0;

	p = get_siarg (dname, 0);
	for (dev = Gr->devices; dev; dev = dev->next) {
		if (!stricmp (dev->name, p)) {
			if (xmode)
				dev->mode = (int)xmode;
			break;
		}
	}
	if (!dev) {
		LogPrintf ("grmgr: bad mode name \"%s\"\n", dname);
		vm_free ();
	}
	STRfree (p);
	return (dev);
}

extern void FAR
devices_release (void)
{
	vm_free ();
}

extern struct GrDriver NEAR* FAR
devices_init (char *name)
{
	struct GrDriver	NEAR* FAR* p;
	char		*options;

	p = GrDrivers;
	if (T(options = get_siarg (name, 0))) {
		for (; *p; ++p)
			if (!stricmp ((*p)->name, options))
				break;
		if (!*p)
			LogPrintf ("grmgr: bad device name \"%s\"\n", name);
		STRfree (options);
	}
	return (*p);
}

extern void FAR
devices_term (void)
{
	if (!Gr)
		return;

	devices_release ();
	Gr = 0;
}
