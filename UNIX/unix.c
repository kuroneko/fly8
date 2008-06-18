/* --------------------------------- unix.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for UNIX.
*/

#include "fly.h"
#include "grx.h"

static Ulong FAR
UnixDisable (void)
{return (0);}

static void FAR
UnixEnable (Ulong flags)
{}

static void FAR
UnixBuildFileName (char *FullName, char *path, char *name, char *ext)

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
{
	FullName[0] = '\0';

	if (path) {
		strcat (FullName, path);
#if !SYS_VMS
		strcat (FullName, "/");
#endif
	}
	strcat (FullName, name);
	if (ext && ext[0]) {
		strcat (FullName, ".");
		strcat (FullName, ext);
	}
}

struct SysDriver SysDriver = {
	"UNIX",
	0,
	NULL,	/* extra */
	0,	/* Init */
	0,	/* Term */
	0,	/* Poll */
	UnixDisable,
	UnixEnable,
	0,	/* Shell */
	UnixBuildFileName
};
