/* ---------------------------------- os2.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for OS/2
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#undef DEBUG_OS2

#include <os2.h>

#include "fly.h"

#include "common.h"

char	Fly8AppName[10]  = "Fly8";
char	Fly8Message[15]  = "Fly8";


LOCAL_FUNC int
OS2Init (char * options)
{
/* Initialize the presentation manager, and create a message queue.
*/
	hab = WinInitialize (0);
	hmq = WinCreateMsgQueue (hab, 0);

	LogPrintf ("OS/2: init ok\n");

	return (0);
}

LOCAL_FUNC void
OS2Term (void)
{
/* destroy the window here if not already done so! 
 * this is because the Dive driver core dumps if it done
 * in the graphics term subroutine
*/
	if (ghWndMain != NULLHANDLE)
		WinDestroyWindow (ghWndMain);

/* Destroy the message queue and uninitialize the presentation manager.
*/
   	WinDestroyMsgQueue (hmq);
   	WinTerminate (hab);

	LogPrintf ("OS/2: term ok\n");
}

LOCAL_FUNC Ulong
OS2Disable (void)
{return (0);}

LOCAL_FUNC void
OS2Enable (Ulong flags)
{flags=flags;}

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
LOCAL_FUNC void
OS2BuildFileName (char *FullName, char *path, char *name, char *ext)
{
	FullName[0] = '\0';

	if (path) {
		strcat (FullName, path);
		strcat (FullName, "\\");
	}
	strcat (FullName, name);
	if (ext && ext[0]) {
		strcat (FullName, ".");
		strcat (FullName, ext);
	}
}

struct SysDriver  SysDriver = {
	"OS/2",
	0,
	NULL,	/* extra */
	OS2Init,
	OS2Term,
	0,	/* Poll */
	OS2Disable,
	OS2Enable,
	0, 	/* Shell */
	OS2BuildFileName
};

static char	badmsg[1024];

LOCAL_FUNC void
OS2Baderr (char *msg)
{
	FILE	*errs;

	errs = fopen ("c:\\temp\\fly8w.err", ATMODE);
	if (errs) {
		fprintf (errs, msg);
		fclose (errs);
	}
}
