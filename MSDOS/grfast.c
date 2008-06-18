/* --------------------------------- grfast.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics driver, uses low level graphics primitives, 256 colors only.
*/

#include "fly.h"
#include "gr.h"

#include <conio.h>


#ifdef DEBUG_S3
extern void	FAR GrfMoveS3 (int x1, int y1);
extern void	FAR GrfDrawS3 (int x2, int y2, Uint c);
#endif

extern void	FAR LogStats (void);
extern Ulong	FAR _BankSwitches;

#define DOSYNC		0x0001
#define BIGPAGE		0x0006
#define POSTSYNC	0x0008
#define INITED		0x8000

#define GRF_STATS	(GRT_MASK+1)

#define MAX_SYNC_WAIT	1000L	/* 1 second is long enough */

static int	FAR width = 0, FAR height = 0;
static long	FAR pagesize = 0L;
static long	FAR psize[] = {0L, 512L*1024L, 1024L*1024L, 2048L*1024L};


LOCAL_FUNC int FAR
GrfSetActive (int page)
{
	GrSetActive (page * pagesize);
	return (0);
}

LOCAL_FUNC int FAR
GrfSetVisual (int page)		/* done */
{
	Ulong	lasttime;
	int	ret;
	int	port;

	if (2 == CD->colors)
		port = 0x3ba;		/* monochrome */
	else
		port = 0x3da;		/* colour */
	lasttime = st.lasttime;

	if (CD->flags & DOSYNC) {
		while (inp (port) & 0x01) {	/* wait for Display Enabled */
			sys_poll (31);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
	}

	ret = GrSetVisual (page * pagesize);

	if (CD->flags & (DOSYNC|POSTSYNC)) {
		while (inp (port) & 0x08) {	/* wait for Vert Sync*/
			sys_poll (32);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
		while (!(inp (port) & 0x08)) {	/* wait for Vert Sync end */
			sys_poll (33);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
	}
	return (ret);
}

#undef MAX_SYNC_WAIT

LOCAL_FUNC int FAR
GrfSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		mode = 0;
		break;
	case T_MOR:
		mode = GrOR;
		break;
	case T_MXOR:
		mode = GrXOR;
		break;
	}
	GrSetWriteMode (mode);
	return (0);
}

LOCAL_FUNC int FAR
GrfSetPalette (int index, long c)
{
	_GrSetColor (index, C_RGB_R (c), C_RGB_G (c), C_RGB_B (c));
	return (index);
}

LOCAL_FUNC int FAR
GrfOptions (char *options)
{
	char		*p;
	struct chip	*c;
	long		temp;
	int		ret = 0;

	if (F(p = get_siarg (options, 1))) {
		LogPrintf ("%s: Missing video type\n", Gr->name);
		ret = 1;
	} else {
		for (c = chips;; ++c) {
			if (!c->name) {
				LogPrintf ("%s: Bad video type %s\n",
					Gr->name, p);
				STRfree (p);
				ret = 1;
				break;
			}
			if (!stricmp (p, c->name)) {
				Gr->flags |= c->type;
				break;
			}
		}
		STRfree (p);
	}

	if (get_parg (options, "stats"))
		Gr->flags |= GRF_STATS;

#if defined(DEBUG_GR)
#ifdef DEBUG_S3
	if (get_parg (options, "accel")) {
		LogPrintf ("%s: Using 'accel' drawing\n", Gr->name);
		Gr->DrawTo  = GrfDrawS3;
		Gr->MoveTo  = GrfMoveS3;
		Gr->Clear   = NULL;
		Gr->Polygon = NULL;
	} else
#endif
	if (get_parg (options, "usec")) {
		LogPrintf ("%s: Using 'c' drawing\n", Gr->name);
		Gr->DrawTo  = GrDrawToC;
		Gr->MoveTo  = GrMoveToC;
		Gr->Clear   = GrClearC;
		Gr->Polygon = GrPolygonC;
	} else if (get_parg (options, "usebasic")) {
		LogPrintf ("%s: Using 'basic' drawing\n", Gr->name);
		Gr->DrawTo  = GrDrawTo;
		Gr->MoveTo  = GrMoveTo;
		Gr->Clear   = GrClear;
		Gr->Polygon = GrPolygon;
	} else {
		LogPrintf ("%s: Using 'asm' drawing\n", Gr->name);
		Gr->DrawTo  = GrDrawToA;
		Gr->MoveTo  = GrMoveToA;
		Gr->Clear   = GrClearA;
		Gr->Polygon = GrPolygonA;
	}
#endif

	if (!get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	return (ret);
}

LOCAL_FUNC int FAR
GrfInit (DEVICE *dev, char *options)
{
	int	i;
	int	row;

	if (GrfOptions (options))
		return (1);

	if (dev->sizex == 0 || dev->sizey == 0) {
		LogPrintf ("%s: Bad WxH in .vmd file\n", Gr->name);
		return (1);
	}
	width = dev->sizex;
	height = dev->sizey;
	pagesize = T(i = dev->flags & BIGPAGE)
			? psize[i/2]
			: (long)width * (long)height;
	if (!dev->mode) {
		LogPrintf ("%s: Must have video mode in .vmd file\n",
			Gr->name);
		return (1);
	}
	GrSetXY (dev->sizex, dev->sizey);
	GrSetBiosMode (dev->mode);
	GrSetType (Gr->flags & GRT_MASK, (int)(dev->npages*pagesize/1024L),
		dev->sizex);

#ifdef DEBUG_GR
	while (GrAllocCell () >= 0)		/* get 2-255 */
		;
#endif

	if (GrfSetVisual (0))
		dev->npages = 1;

	GrfSetWriteMode (T_MSET);
	st.colors[CC_BLACK] = GrfSetPalette (CC_BLACK, C_BLACK);

	for (i = dev->npages; --i >= 0;) {
		GrfSetActive (i);
		for (row = 0; row < dev->sizey; ++row) {
			Gr->MoveTo (0, row);
			Gr->DrawTo (dev->sizex-1, row, st.colors[CC_BLACK]);
		}
	}

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
GrfTerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	LogPrintf ("%s: BankSwitches %s\n", Gr->name, show_ul (_BankSwitches));
	if (Gr->flags & GRF_STATS)
		LogStats ();

	GrSetType (GRT_NONE, 256, 200);
	GrSetBiosMode (0x03);		/* text 80x25 */

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC int FAR
GrfShutters (int eye)
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


struct GrDriver NEAR GrFast = {
	"GrFast",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	GrfInit,
	GrfTerm,
	GrMoveToA,
	GrDrawToA,
	GrfSetVisual,
	GrfSetActive,
	GrClearA,
	GrfSetWriteMode,
	GrfSetPalette,
	GrEllipse,
	GrPolygonA,
	0,		/* Flush */
	GrfShutters
};
#undef DOSYNC
#undef BIGPAGE
#undef POSTSYNC
#undef GRF_STATS
#undef INITED
#undef MAX_SYNC_WAIT
