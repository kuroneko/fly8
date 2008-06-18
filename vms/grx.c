/* --------------------------------- grx.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for X11.
 *
 * Support for double buffering added August 19, 1993
 * by Niclas Wiberg (nicwi@isy.liu.se).
*/

#include "fly.h"

#ifdef __DECC
#if __DECC_VER < 50200000
typedef char *caddr_t;
#endif
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "grx.h"
#include "xkeys.h"


#define GRX_SINGLE	0x0001
#define GRX_DIRECT	0x0002
#define INITED		0x1000

static DEVICE		*CurrentDevice = 0;

#define APPNAME		"fly8 GrX"
#define NOCOLOR		0xffff

static Display		*TheDisplay;
static GC		TheGC;
static Window		TheWindow, TheRoot;
static int		TheScreen;
static Colormap		TheColormap;
static unsigned long	FgColor, BgColor;
static int		xold = 0, yold = 0;
static int		last_color = -1;

static Pixmap		ThePixmaps[2];
static Drawable		TheDrawable;

LOCAL_FUNC void
GrxMoveTo (Uint x, Uint y)
{
	xold = x;
	yold = y;
}

LOCAL_FUNC void
GrxDrawTo (Uint x, Uint y, Uint c)
{
	if (last_color != c) {
		last_color = c;
		XSetForeground(TheDisplay, TheGC, c);
	}
	XDrawLine(TheDisplay, TheDrawable, TheGC, xold, yold, x, y);
	xold = x;
	yold = y;
}

LOCAL_FUNC int FAR
GrxSetActive (int page)
{
	if (Gr->flags & (GRX_DIRECT|GRX_SINGLE))
		return (0);
	TheDrawable = ThePixmaps[page];
	return (0);
}
  
LOCAL_FUNC int FAR
GrxSetVisual (int page)
{
	if (Gr->flags & GRX_DIRECT)
		return (0);
	XCopyArea (TheDisplay,
		ThePixmaps[page],
		TheWindow,
		TheGC,
		0, 0, CS->sizex, CS->sizey, 0, 0);
	return (0);
}
  
LOCAL_FUNC int FAR
GrxSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		mode = GXcopy;
		break;
	case T_MOR:
		mode = GXor;
		break;
	case T_MXOR:
		mode = GXxor;
		break;
	}
	XSetFunction (TheDisplay, TheGC, mode);
	return (0);
}

LOCAL_FUNC int FAR
GrxSetPalette (int n, long c)
{
	XColor	color;

	if (CurrentDevice->colors < 16)
		return (0);
	if (NOCOLOR != st.colors[n])
		{}	/* FreeThisColor */
	color.red   = C_RGB_R (c) << 8;
	color.green = C_RGB_G (c) << 8;
	color.blue  = C_RGB_B (c) << 8;
	XAllocColor (TheDisplay, TheColormap, &color);
	st.colors[n] = color.pixel;
	return (0);
}

LOCAL_FUNC void FAR
GrxEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	if (last_color != c) {
		last_color = c;
		XSetForeground (TheDisplay, TheGC, c);
	}
	XDrawArc (TheDisplay, TheDrawable, TheGC, x-rx, y-ry, 2*rx, 2*ry,
			0, 360*64);
}

LOCAL_FUNC void
GrxClearWin (void)
{
	XSetForeground(TheDisplay, TheGC, st.colors[CC_BLACK]);
	XFillRectangle(TheDisplay, TheDrawable, TheGC, 0, 0,
		CS->sizex, CS->sizey);
	XSetForeground(TheDisplay, TheGC, st.colors[CC_WHITE]);
	st.flags |= SF_CLEARED;
}

LOCAL_FUNC int FAR
GrxInit (DEVICE *dev, char *options)
{
	XEvent	event;
	int		i;

	Gr->flags = 0;
	if (get_parg (options, "direct"))
		Gr->flags |= GRX_DIRECT;
	else if (get_parg (options, "single"))
		Gr->flags |= GRX_SINGLE;

	if (dev->sizex == 0 || dev->sizey == 0)
		return (1);

	TheDisplay = XOpenDisplay (""); 
	if (!TheDisplay) {
		LogPrintf ("%s: can't open display\n", APPNAME);
		die ();
	}
	CurrentDevice = dev;

	TheScreen = DefaultScreen (TheDisplay);
	TheRoot = DefaultRootWindow (TheDisplay);
	FgColor = WhitePixel (TheDisplay, TheScreen);
	BgColor = BlackPixel (TheDisplay, TheScreen);
	TheWindow = XCreateSimpleWindow (TheDisplay, TheRoot,
		0, 0, dev->sizex, dev->sizey, 0, FgColor, BgColor);
	XSelectInput (TheDisplay, TheWindow, 
		ExposureMask|KeyPressMask|StructureNotifyMask);
	XMapRaised (TheDisplay, TheWindow);
	XSetStandardProperties (TheDisplay, TheWindow, APPNAME, APPNAME,
		0, NULL, 0, NULL);
	TheGC = DefaultGC (TheDisplay, TheScreen);

	if (Gr->flags & GRX_DIRECT)
		TheDrawable = TheWindow;
	else {
		ThePixmaps[0] = XCreatePixmap(TheDisplay,
			    RootWindow(TheDisplay, TheScreen),
			    dev->sizex, dev->sizey,
			    DisplayPlanes(TheDisplay, TheScreen));
		if (!(Gr->flags & GRX_SINGLE))
			ThePixmaps[1] = XCreatePixmap(TheDisplay,
				    RootWindow(TheDisplay, TheScreen),
				    dev->sizex, dev->sizey,
				    DisplayPlanes(TheDisplay, TheScreen));
		TheDrawable = ThePixmaps[0];
	}

	do {
		XNextEvent (TheDisplay, &event);
	} while (event.type != Expose);


	if (Gr->flags & (GRX_DIRECT|GRX_SINGLE))
		dev->npages = 1;
	else
		dev->npages = 2;

	TheColormap = DefaultColormap (TheDisplay, DefaultScreen (TheDisplay));

	dev->colors = 256;
	i = XDisplayCells (TheDisplay, TheScreen);
	if (i < 16) {
		GrxSetPalette (CC_WHITE, C_WHITE);
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = st.colors[CC_WHITE];
		GrxSetPalette (CC_BLACK, C_BLACK);
		i = 2;
	} else {
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = NOCOLOR;
		GrxSetPalette (CC_WHITE, C_WHITE);
		GrxSetPalette (CC_BLACK, C_BLACK);
	}
	dev->colors = i;

	GrxSetVisual (0);
	GrxSetActive (0);

        GrxClearWin ();

	Gr->flags |= INITED;

	return (0);
}

LOCAL_FUNC void FAR
GrxTerm (DEVICE *dev)		/* done */
{
	int	i;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (!(Gr->flags & GRX_DIRECT)) {
		XFreePixmap(TheDisplay, ThePixmaps[0]);
		if (!(Gr->flags & GRX_SINGLE))
			XFreePixmap(TheDisplay, ThePixmaps[1]);
	}

	for (i = 0; i < rangeof (st.colors); ++i)
		if (NOCOLOR == st.colors[i])
			{}	/* FreeThisColor */

	XDestroyWindow (TheDisplay, TheWindow);
	XCloseDisplay (TheDisplay);
}

LOCAL_FUNC void FAR
GrxFlush (void)
{
	XFlush (TheDisplay);
	XSync  (TheDisplay, 0);
	(void) XEventsQueued (TheDisplay, QueuedAfterFlush);
}

LOCAL_FUNC int FAR
GrxGetMouse (int *win_x, int *win_y, char *btn, int *nbtn)
{
	Window		root, child;
	int		ret, root_x, root_y;
	unsigned int	keys_buttons;

	ret = XQueryPointer (TheDisplay, TheWindow, &root, &child, &root_x,
		&root_y, win_x, win_y, &keys_buttons);

	btn[0] = T(keys_buttons & Button3Mask);
	btn[1] = T(keys_buttons & Button1Mask);
	btn[2] = T(keys_buttons & Button2Mask);
	*nbtn = 3;
	return (0);
}

LOCAL_FUNC void
GrxResize (XConfigureEvent *cf)
{
	sim_set ();
	screen_empty ();

	if (!(Gr->flags & GRX_DIRECT)) {
		XFreePixmap(TheDisplay, ThePixmaps[0]);
		if (!(Gr->flags & GRX_SINGLE))
			XFreePixmap(TheDisplay, ThePixmaps[1]);
	}

	if (!(Gr->flags & GRX_DIRECT)) {
		ThePixmaps[0] = XCreatePixmap(TheDisplay,
			RootWindow (TheDisplay, TheScreen),
			cf->width, cf->height,
			DisplayPlanes (TheDisplay, TheScreen));
		if (!(Gr->flags & GRX_SINGLE))
			ThePixmaps[1] = XCreatePixmap(TheDisplay,
				RootWindow (TheDisplay, TheScreen),
				cf->width, cf->height,
				DisplayPlanes (TheDisplay, TheScreen));
		TheDrawable = ThePixmaps[0];
	}

	set_screen (cf->width, cf->height);
	screen_start ();
	sim_reset ();
}

LOCAL_FUNC int FAR
GrxKread (void)
{
	XEvent	event;
	int	n;

	for (n = -1; n < 0;) {
		if (!XEventsQueued (TheDisplay, QueuedAlready))
			return (-1);
		XNextEvent (TheDisplay, &event);
		switch (event.type) {
 		case  KeyPress:
			n = xGetKey (&event.xkey);
			break;
		case ConfigureRequest:
		case ConfigureNotify:
			GrxResize (&event.xconfigure);
			GrxClearWin ();
			break;
		default:
			GrxClearWin ();
			break;
		}
	}

	return (n);
}

struct GrxExtra GrxExtra = {
	GrxGetMouse,
	GrxKread
};

struct GrDriver GrX = {
	"GrX",
	0,
	&GrxExtra,
	0,
	GrxInit,
	GrxTerm,
	GrxMoveTo,
	GrxDrawTo,
	GrxSetVisual,
	GrxSetActive,
	0,	/* Clear */
	GrxSetWriteMode,
	GrxSetPalette,
	GrxEllipse,
	GrxFlush,
	0	/* Shutters*/
};

#undef GRX_SINGLE
#undef GRX_DIRECT
#undef INITED
#undef APPNAME
#undef NOCOLOR
