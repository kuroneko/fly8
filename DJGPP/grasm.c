/* --------------------------------- grasm.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics driver (direct video access, 256 colors only).
 *
 * Uses the bgr.c functions, which then use low.c/bgrasm.x at the lower
 * levels.
*/

#include "fly.h"
#include "grasm.h"

#include <pc.h>

static int	verbose = 0;

LOCAL_FUNC int FAR
GraSetPalette (int index, long color)
{
	long	flags;

	flags = Sys->Disable ();
	outportb (0x3c8, index);
	outportb (0x3c9, C_RGB_R (color) >> 2);
	outportb (0x3c9, C_RGB_G (color) >> 2);
	outportb (0x3c9, C_RGB_B (color) >> 2);
	Sys->Enable (flags);

	return (index);
}

LOCAL_FUNC int FAR
GraGetOptions (char *options)
{
	char	*p;
	int	ret = 0;
	long	temp;

	if (F(p = get_siarg (options, 1))) {
		LogPrintf ("%s: Missing video type\n", Gr->name);
		ret = 1;
	} else {
		if (!stricmp (p, "et4k"))
			Gr->flags |= GRL_TYPE_T4K;
		else if (!stricmp (p, "vesa"))
			Gr->flags |= GRL_TYPE_VESA;
		else {
			LogPrintf ("%s: Bad video type %s\n", Gr->name, p);
			ret = 1;
		}
		STRfree (p);
	}

	if (get_parg (options, "stats"))
		Gr->flags |= GRL_STATS;

	if (get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	if (T(p = get_parg (options, "verbose"))) {
		verbose = 1;
		while ('+' == *p++)
			++verbose;
	} else
		verbose = 0;

	return (ret);
}

LOCAL_FUNC int FAR
GraInit (DEVICE *dev, char *options)
{
	int	i;

	if (GraGetOptions (options))
		return (1);

	if (dev->sizex == 0 || dev->sizey == 0) {
		LogPrintf ("%s: bad screen size for mode\n",
			Gr->name);
		return (1);
	}

	if (GrlInit (dev->mode, verbose, &dev->sizex, &dev->sizey,
								&dev->npages))
		return (1);

	bSetWriteMode (T_MSET);
	st.colors[CC_BLACK] = GraSetPalette (CC_BLACK, C_BLACK);

	for (i = 0; i < dev->npages; ++i) {
		GrlSetActiveBase (i);
		bClear (0, 0, dev->sizex, dev->sizey, st.colors[CC_BLACK]);
	}

	GrlSetVisualBase (0);
	GrlSetActiveBase (0);

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
GraTerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRL_STATS)
		LogStats ();

	GrlTerm ();

	LogPrintf ("%s: term ok\n",
		Gr->name);
}

LOCAL_FUNC int FAR
GraShutters (int eye)
{
	if (st.misc[7]) {
		if (eye >= 0)
			outportb (st.misc[7]+4, 1+2*eye);
		else if (-1 == eye)
			outportb (st.misc[7]+4, 1);		/* on */
		else if (-2 == eye)
			outportb (st.misc[7]+4, 0);		/* off */
		return (0);				/* have shutters */
	} else
		return (1);				/* no shutters */
}

struct GrDriver NEAR GrASM = {
	"GrASM",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	GraInit,
	GraTerm,
	bMoveTo,
	bDrawTo,
	GrlSetVisualBase,
	GrlSetActiveBase,
	bClear,
	bSetWriteMode,
	GraSetPalette,
	bDrawEllipse,
	bPolygon,	/* Polygon */
	0,		/* Flush */
	GraShutters
};
