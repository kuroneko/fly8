/* --------------------------------- unix.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for UNIX.
*/

#include "fly.h"
#include "grx.h"


extern void	FAR com_poll (void);

static int FAR
init (void)
{return (0);}

static void FAR
term (void)
{}

static void FAR
poll (void)
{
#ifndef VMS
	com_poll ();
#endif
}

static Ulong FAR
disable (void)
{return (0);}

static void FAR
enable (Ulong flags)
{}

static void FAR
shell (void)
{
	MsgWPrintf (20, "shell not implemented");
}

static void FAR
BuildFileName (char *FullName, char *path, char *name, char *ext)

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
{
	FullName[0] = '\0';

	if (path) {
		strcat (FullName, path);
#ifndef VMS
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
	"VMS",
	0,
	NULL,	/* extra */
	init,
	term,
	poll,
	disable,
	enable,
	shell,
	BuildFileName
};
