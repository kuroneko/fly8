/* --------------------------------- nosystem.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dummy system driver.
*/

#include "fly.h"


LOCAL_FUNC Ulong FAR
nsys_disable (void)
{return (0);}

LOCAL_FUNC void FAR
nsys_enable (Ulong flags)
{}

LOCAL_FUNC void FAR
nsys_build (char *FullName, char *path, char *name, char *ext)
{
	FullName[0] = '\0';

	if (path) {
		strcat (FullName, path);
		strcat (FullName, "/");
	}
	strcat (FullName, name);
	if (ext && ext[0]) {
		strcat (FullName, ".");
		strcat (FullName, ext);
	}
}

struct SysDriver NEAR SysNone = {
	"NoSystem",
	0,
	NULL,
	0,
	0,
	0,	/* poll */
	nsys_disable,
	nsys_enable,
	0,
	nsys_build
};
