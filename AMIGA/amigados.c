/* --------------------------------- amigados.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for AMIGADOS.
*/

#include "fly.h"

#include "amigainc.h"


#ifndef _DCC
struct IntuitionBase *IntuitionBase=NULL;
#endif

static int FAR
init (void)
{
#ifndef _DCC
 	/* open the intuition library */
	IntuitionBase = (struct IntuitionBase *)
		OpenLibrary("intuition.library", 33L);
 	if (IntuitionBase == NULL) {
		printf("Error: Cannot open Intuition library\n");
	    	exit(ERROR_INVALID_RESIDENT_LIBRARY);
    	}
#endif
	return (0);
}

static void FAR
term (void)
{
#ifndef _DCC
	if (IntuitionBase)
	    CloseLibrary((struct Library *)IntuitionBase);
#endif
	return;
}

static void FAR
poll (void)
{}

static short FAR
disable (void)
{
	return (0);
}

static void FAR
enable (short flags)
{
}

static void FAR
shell (void)
{
	MsgWPrintf (20, "shell not implemented");
}

static void FAR
BuildFileName (char *FullName, char *path, char *name, char *ext)

/* Build file name from parts.
   path is NULL for "current directory".
   path is ""   for "root directory".
*/

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

struct SysDriver SysDriver = {
	"AMIGADOS",
	0,
	init,
	term,
	poll,
	disable,
	enable,
	shell,
	BuildFileName
};
