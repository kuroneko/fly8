/* --------------------------------- grvesa.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* VESA/VBE graphics driver, uses low level graphics primitives,
 * 256 colors only. This one is self contained (with vesa.c and vgr.c).
*/

#include "fly.h"
#include "vesa.h"

#include <conio.h>


#define GRV_STATS	0x0001
#define INITED		0x8000


LOCAL_FUNC int FAR
GrvSetPalette (int n, long c)
{
	vesa_palette (n, C_RGB_R (c), C_RGB_G (c), C_RGB_B (c));
	return (n);
}

LOCAL_FUNC int FAR
GrvOptions (char *options)
{
	long		temp;

	if (get_parg (options, "stats"))
		Gr->flags |= GRV_STATS;

	if (!get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

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

	vesa_init (dev->mode, dev->sizex, dev->sizey, dev->sizex);

	if (vSetVisual (0))
		dev->npages = 1;

	vSetWriteMode (T_MSET);
	st.colors[CC_BLACK] = GrvSetPalette (CC_BLACK, C_BLACK);

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

	LogPrintf ("%s: BankSwitches %s\n", Gr->name,
		show_ul (vesa_BankSwitches));
	if (Gr->flags & GRV_STATS)
		LogStats ();
	vesa_term ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC int FAR
GrvShutters (int eye)
{
	if (st.misc[7]) {
		if (eye >= 0)
			outp (st.misc[7]+4, 1+2*eye);
		else if (-1 == eye)
			outp (st.misc[7]+4, 1);		/* on */
		else if (-2 == eye)
			outp (st.misc[7]+4, 0);		/* off */
		return (0);				/* have shutters */
	} else
		return (1);				/* no shutters */
}


struct GrDriver NEAR GrVesa = {
	"GrVesa",
	0,
	NULL,		/* Extra */
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
	0, 		/* Polygon */
	0,		/* Flush */
	GrvShutters
};
#undef GRV_STATS
#undef INITED
