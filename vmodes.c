/* --------------------------------- vmodes.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read in the video modes table.
*/

#include "fly.h"


LOCAL_FUNC int NEAR
get_mode (char *line, DEVICE *dev)
{
	char	*p;
	long	l;
	int	errs;

	errs = 0;

	if (T(p = get_siarg (line, 0)))
		dev->name = p;
	else
		++errs;
	if (get_niarg (line, 1, &l))
		++errs;
	else
		dev->mode = (int)l;
	if (get_niarg (line, 2, &l))
		++errs;
	else
		dev->colors = (int)l;
	if (get_niarg (line, 3, &l))
		++errs;
	else
		dev->left = (int)l;
	if (get_niarg (line, 4, &l))
		++errs;
	else
		dev->top = (int)l;
	if (get_niarg (line, 5, &l))
		++errs;
	else
		dev->sizex = (int)l;
	if (get_niarg (line, 6, &l))
		++errs;
	else
		dev->sizey = (int)l;
	if (get_niarg (line, 7, &l))
		++errs;
	else
		dev->npages = (int)l;
	if (get_niarg (line, 8, &l))
		++errs;
	else
		dev->lengx = (int)l;
	if (get_niarg (line, 9, &l))
		++errs;
	else
		dev->lengy = (int)l;
	if (get_niarg (line, 10, &l))
		++errs;
	else
		{}	/* obsolete field */
	if (get_niarg (line, 11, &l))
		++errs;
	else
		{}	/* obsolete field */
	if (get_niarg (line, 12, &l))
		dev->flags = 0;
	else
		dev->flags = (int)l;

	dev->right  = dev->left + dev->sizex - 1;
	dev->bottom = dev->top  + dev->sizey - 1;

	return (errs);
}

extern int FAR
vm_read (void)
{
	FILE	*vm;
	DEVICE	*dev, *tail;
	int	errs, i, l;

	if (!st.vmdname)
		st.vmdname = STRdup ("fly");

	Sys->BuildFileName (st.filename, st.fdir, st.vmdname, VMD_EXT);
	vm = fopen (st.filename, RTMODE);
	if (!vm) {
		LogPrintf ("vmd: could not open file %s\n", st.filename);
		return (1);
	}

	tail = 0;
	dev = 0;
	Gr->devices = 0;
	errs = 0;

	for (i = 1; fgets (st.filename, sizeof (st.filename), vm); ++i) {
		if (st.filename[0] == '\n' || st.filename[0] == '#')
			continue;
		if (F(NEW (dev))) {
			LogPrintf ("vmd: out of memory\n");
			++errs;
			continue;
		}
		l = strlen(st.filename);
		st.filename[l-1] = '\0';
		if (get_mode (st.filename, dev)) {
			LogPrintf ("vmd: bad line %d\n", i);
			++errs;
			DEL0 (dev);
			continue;
		}
		dev->next = 0;
		if (tail)
			tail->next = dev;
		else
			Gr->devices = dev;
		tail = dev;
		dev->pdevice = 0;
	}
	fclose (vm);

	return (errs);
}

extern void FAR
vm_free (void)
{
	DEVICE	*dev, *next;

	if (!Gr)
		return;

	for (dev = Gr->devices; dev; dev = next) {
		next = dev->next;
		dev->name = STRfree (dev->name);
		DEL (dev);
	}
	Gr->devices = 0;
}
