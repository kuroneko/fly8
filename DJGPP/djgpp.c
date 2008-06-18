/* --------------------------------- djgpp.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for UNIX.
*/

#include "fly.h"
#include "djgpp.h"


/* ints.s */
extern void IntOn (Ulong flags);
extern Ulong IntOff (void);


Ulong			tb_size = 4096;
Ushort			tb_seg = 0;
Ulong			tb_real = 0;

static _go32_dpmi_seginfo	xfer_info;


static int FAR
dj_init (char * options)
{

/* get us a private transfer buffer
*/
	xfer_info.size = (tb_size + 15) >> 4;
	if (_go32_dpmi_allocate_dos_memory (&xfer_info)) {
		LogPrintf ("%s: could not alloc xfer buffer\n",
			Sys->name);
		return (1);
	}
	tb_seg = xfer_info.rm_segment;
	tb_real = tb_seg << 4;

	return (0);
}

static void FAR
dj_term (void)
{
	if (tb_seg) {
		_go32_dpmi_free_dos_memory (&xfer_info);
		tb_seg = 0;
	}
}

static Ulong FAR
dj_disable (void)
{
	return (IntOff ());
}

static void FAR
dj_enable (Ulong flags)
{
	IntOn (flags);
}

static void FAR
DjBuildFileName (char *FullName, char *path, char *name, char *ext)

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/

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

struct SysDriver SysDriver = {
	"DJGPP",
	0,
	NULL,	/* extra */
	dj_init,
	dj_term,
	0,	/* Poll */
	dj_disable,
	dj_enable,
	0, 	/* Shell */
	DjBuildFileName
};
