/* --------------------------------- grsvga.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Linux SVGA graphics driver.
*/

#include "fly.h"

#if HAVE_SVGALIB

#include "grx.h"
#include "svga.h"


#define GRV_STATS	0x0001
#define INITED		0x8000


LOCAL_FUNC int FAR
GrvSetPalette (int index, long c)
{
	return (svga_palette (index, C_RGB_R (c), C_RGB_G (c), C_RGB_B (c)));
}

LOCAL_FUNC int FAR
GrvOptions (char *options)
{
	if (get_parg (options, "stats"))
		Gr->flags |= GRV_STATS;

	return (0);
}

LOCAL_FUNC int FAR
GrvInit (DEVICE *dev, char *options)
{
	int	i;
	int	row;

	if (GrvOptions (options))
		return (1);

	if (0 == dev->sizex || 0 == dev->sizey) {
		LogPrintf ("%s: Bad WxH in .vmd file\n", Gr->name);
		return (1);
	}

	if (0 == dev->mode) {
		LogPrintf ("%s: Must have video mode in .vmd file\n",
			Gr->name);
		return (1);
	}

	svga_init (dev->mode, dev->sizex, dev->sizey, dev->sizex);

	vSetWriteMode (T_MSET);
	GrvSetPalette (CC_BLACK, C_BLACK);

	for (i = 0; i < dev->npages; ++i) {
		vSetActive (i);
		for (row = 0; row < dev->sizey; ++row) {
			Gr->MoveTo (0, row);
			Gr->DrawTo (dev->sizex-1, row, st.colors[CC_BLACK]);
		}
	}

	vSetActive (0);

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
GrvTerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRV_STATS)
		LogStats ();
	svga_term ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

struct GrxExtra svgaExtra = {
	svga_GetMouse,
	svga_Kread
};

struct GrDriver NEAR GrSVGA = {
	"GrSVGA",
	0,
	&svgaExtra,
	0,		/* Devices */
	GrvInit,
	GrvTerm,
	vMoveTo,
	vDrawTo,
	vSetVisual,
	vSetActive,
	vClear,
	vSetWriteMode,
	GrvSetPalette,
	vEllipse,
	vPolygon,
	0,		/* Flush */
	0		/* Shutters */
};
#undef GRV_STATS
#undef INITED
#endif
