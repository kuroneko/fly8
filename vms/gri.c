/* --------------------------------- gri.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for X11, using PutImage (and Shm when possible).
*/

#include "fly.h"
#include "bgr.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef NO_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#include "grx.h"
#include "xkeys.h"


#define GRI_STATS	0x0001
#define GRI_SINGLE	0x0002
#define GRI_SHM		0x0004
#define INITED		0x1000

static DEVICE		*CurrentDevice = 0;

#define APPNAME		"fly8 GrI"
#define NOCOLOR		0xffff

static Display		*TheDisplay;
static GC		TheGC;
static Window		TheWindow, TheRoot;
static int		TheScreen;
static Colormap		TheColormap;
static Ulong		FgColor, BgColor;
static Uint		_width = 0, _height = 0, _xbytes = 0;

static XImage		*TheImages[2] = {NULL, NULL};
static XWindowAttributes	attr;
static int		using_shm = 0;

#ifndef NO_XSHM
static XShmSegmentInfo	xshminfo;
#endif

LOCAL_FUNC int
GriSetActive (int page)
{
	if (Gr->flags & GRI_SINGLE)
		page = 0;
	bSetActive (TheImages[page]->data);
	return (0);
}
  
LOCAL_FUNC int
GriSetVisual (int page)
{
	if (Gr->flags & GRI_SINGLE)
		page = 0;
#ifndef NO_XSHM
	if (using_shm)
		XShmPutImage (TheDisplay,
			TheWindow,
			TheGC,
			TheImages[page],
			0, 0, 0, 0, _width, _height, 0);
	else
#endif
		XPutImage (TheDisplay,
			TheWindow,
			TheGC,
			TheImages[page],
			0, 0, 0, 0, _width, _height);
	return (0);
}
  
LOCAL_FUNC int
GriSetPalette (int n, long c)
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

LOCAL_FUNC void
GriClearWin (void)
{
	XSetForeground(TheDisplay, TheGC, st.colors[CC_BLACK]);
	XFillRectangle(TheDisplay, TheWindow, TheGC, 0, 0,
		_width, _height);
	XSetForeground(TheDisplay, TheGC, st.colors[CC_WHITE]);
	st.flags |= SF_CLEARED;
}

LOCAL_FUNC void
GriDestroyImage (void)
{
	if (TheImages[0]) {
#ifndef NO_XSHM
		if (using_shm) {
			XShmDetach (TheDisplay, &xshminfo);
			shmdt (xshminfo.shmaddr);
			TheImages[0]->data = xshminfo.shmaddr = NULL;
			shmctl (xshminfo.shmid, IPC_RMID, 0);
		}
#endif
		XDestroyImage (TheImages[0]);
		TheImages[0] = NULL;
	}
	if (TheImages[1]) {
		XDestroyImage (TheImages[1]);
		TheImages[1] = NULL;
	}

}

static int	xerror = 0;

LOCAL_FUNC int
error_handler (Display *display, XErrorEvent *event)
{
     xerror = 1;

     return (0);
}

LOCAL_FUNC void
trap_errors (void)
{
     xerror = 0;
     XSetErrorHandler (error_handler);
}

LOCAL_FUNC int
untrap_errors (void)
{
     XSync (TheDisplay,0);
     XSetErrorHandler (NULL);
     return (xerror);
}


LOCAL_FUNC int
GriCreateImage (void)
{
	int	i;

#ifndef NO_XSHM
	using_shm = 0;
	if ((Gr->flags & GRI_SHM) && XShmQueryExtension(TheDisplay)) {
		trap_errors();
		TheImages[0] = XShmCreateImage (TheDisplay,
				DefaultVisual(TheDisplay, TheScreen), 
				attr.depth, ZPixmap, NULL, &xshminfo,
				_width, _height);
		if (untrap_errors())
			goto err0;
		if (!TheImages[0]) {
			LogPrintf ("%s: CreateImage failed\n", APPNAME);
			goto err0;
		}
		_xbytes = TheImages[0]->bytes_per_line;
		bSetSize (_width, _height, _xbytes);

		xshminfo.shmid = shmget (IPC_PRIVATE,
				_xbytes * TheImages[0]->height,
				IPC_CREAT | 0777);
		if (xshminfo.shmid < 0)
			goto err1;
		TheImages[0]->data = (char *) shmat(xshminfo.shmid, 0, 0);
		if (TheImages[0]->data == ((char *) -1))
			goto err2;

		xshminfo.shmaddr = TheImages[0]->data;
		xshminfo.readOnly = False;

		trap_errors();
		i = XShmAttach (TheDisplay, &xshminfo);
		if (untrap_errors() || !i) {
			shmdt (xshminfo.shmaddr);
err2:
			shmctl (xshminfo.shmid, IPC_RMID, 0);
err1:
			XDestroyImage (TheImages[0]);
err0:
			LogPrintf ("shm failed\n");
		} else {
			Gr->flags |= GRI_SINGLE;
			using_shm = 1;
			bSetWriteMode (T_MSET);
			GriSetActive (0);
			bScrClear (0, 0, _width, _height, st.colors[CC_BLACK]);
			return (0);
		}
	}
#endif
	if (F(TheImages[0] = XCreateImage (TheDisplay,
				DefaultVisual (TheDisplay, TheScreen),
				attr.depth, ZPixmap,
				0, NULL, _width, _height,
				XBitmapPad (TheDisplay), 0))) {
		GriDestroyImage ();
		return (1);
	}
	_xbytes = TheImages[0]->bytes_per_line;
	bSetSize (_width, _height, _xbytes);
	if (F(TheImages[0]->data = (char BPTR *)calloc (_xbytes, _height))) {
		GriDestroyImage ();
		return (1);
	}
	bSetWriteMode (T_MSET);
	GriSetActive (0);
	bScrClear (0, 0, _width, _height, st.colors[CC_BLACK]);

	if (!(Gr->flags & GRI_SINGLE)) {
		if (F(TheImages[1] = XCreateImage (TheDisplay,
				DefaultVisual (TheDisplay, TheScreen),
				attr.depth, ZPixmap,
				0, NULL, _width, _height,
				XBitmapPad (TheDisplay), 0))) {
			GriDestroyImage ();
			return (1);
		}
		if (F(TheImages[1]->data 
				= (char BPTR *)calloc (_xbytes, _height))) {
			GriDestroyImage ();
			return (1);
		}
	}
	bSetWriteMode (T_MSET);
	GriSetActive (1);
	bScrClear (0, 0, _width, _height, st.colors[CC_BLACK]);

	GriSetActive (0);

	return (0);
}

LOCAL_FUNC int
GriInit (DEVICE *dev, char *options)
{
	XEvent		event;
	int		i;

	Gr->flags = 0;
	if (get_parg (options, "single"))
		Gr->flags |= GRI_SINGLE;
	if (get_parg (options, "stats"))
		Gr->flags |= GRI_STATS;
	if (get_parg (options, "shm"))
		Gr->flags |= GRI_SHM;

	if (dev->sizex == 0 || dev->sizey == 0)
		return (1);

	TheDisplay = XOpenDisplay (""); 
	if (!TheDisplay) {
		LogPrintf ("%s: can't open display\n", APPNAME);
		die ();
	}
	CurrentDevice = dev;

	_width  = dev->sizex;
	_height = dev->sizey;

	TheScreen = DefaultScreen (TheDisplay);
	TheRoot = DefaultRootWindow (TheDisplay);
	FgColor = WhitePixel (TheDisplay, TheScreen);
	BgColor = BlackPixel (TheDisplay, TheScreen);
	TheWindow = XCreateSimpleWindow (TheDisplay, TheRoot,
		0, 0, _width, _height, 0, FgColor, BgColor);
	XSelectInput (TheDisplay, TheWindow, 
		ExposureMask|KeyPressMask|StructureNotifyMask);
	XMapRaised (TheDisplay, TheWindow);
	XSetStandardProperties (TheDisplay, TheWindow, APPNAME, APPNAME,
		0, NULL, 0, NULL);
	TheGC = DefaultGC (TheDisplay, TheScreen);
	XSetFunction (TheDisplay, TheGC, GXcopy);

	XGetWindowAttributes (TheDisplay, DefaultRootWindow (TheDisplay),
		&attr);

	if (attr.depth != 8) {
		LogPrintf ("%s: Image depth must be 8\n", APPNAME);
		die ();
	}

	do {
		XNextEvent (TheDisplay, &event);
	} while (event.type != Expose);

	TheColormap = DefaultColormap (TheDisplay, DefaultScreen (TheDisplay));

	dev->colors = 256;
	i = XDisplayCells (TheDisplay, TheScreen);
	if (i < 16) {
		GriSetPalette (CC_WHITE, C_WHITE);
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = st.colors[CC_WHITE];
		GriSetPalette (CC_BLACK, C_BLACK);
		i = 2;
	} else {
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = NOCOLOR;
		GriSetPalette (CC_WHITE, C_WHITE);
		GriSetPalette (CC_BLACK, C_BLACK);
	}
	dev->colors = i;

	if (Gr->flags & GRI_SINGLE)
		dev->npages = 1;
	else
		dev->npages = 2;

	if (GriCreateImage ()) {
		LogPrintf ("%s: CreateImage failed\n", APPNAME);
		die ();
	}

	if (using_shm)
		MsgWPrintf (-50, "%s: using shm.", APPNAME);

	if (Gr->flags & GRI_SINGLE)
		dev->npages = 1;

	GriSetVisual (0);
	GriSetActive (0);

        GriClearWin ();

	Gr->flags |= INITED;

	return (0);
}

LOCAL_FUNC void
GriTerm (DEVICE *dev)		/* done */
{
	int	i;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	GriDestroyImage ();

	for (i = 0; i < rangeof (st.colors); ++i)
		if (NOCOLOR != st.colors[i])
			{}	/* FreeThisColor */

	XDestroyWindow (TheDisplay, TheWindow);
	XCloseDisplay (TheDisplay);
	if (Gr->flags & GRI_STATS)
		LogStats ();
}

LOCAL_FUNC void

GriFlush (void)
{
	XFlush (TheDisplay);
	XSync  (TheDisplay, 0);
	(void) XEventsQueued (TheDisplay, QueuedAfterFlush);
}

LOCAL_FUNC int
GriGetMouse (int *win_x, int *win_y, char *btn, int *nbtn)
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
GriResize (XConfigureEvent *cf)
{
	sim_set ();
	screen_empty ();

	GriDestroyImage ();

	_width  = cf->width;
	_height = cf->height;
	if (GriCreateImage ()) {
		LogPrintf ("%s: Resize failed\n", APPNAME);
		die ();
	}

	set_screen (_width, _height);
	screen_start ();
	sim_reset ();
}

LOCAL_FUNC int
GriKread (void)
{
	XEvent	event;
	int	n;

	for (n = -1; n < 0;) {
		if (!XEventsQueued (TheDisplay, QueuedAlready)) {
			n = -1;
			break;
		}
		XNextEvent (TheDisplay, &event);
		switch (event.type) {
 		case  KeyPress:
			n = xGetKey (&event.xkey);
			break;
		case ConfigureRequest:
		case ConfigureNotify:
			GriResize (&event.xconfigure);
			GriClearWin ();
			break;
		default:
			GriClearWin ();
			break;
		}
	}

	return (n);
}

static struct GrxExtra GriExtra = {
	GriGetMouse,
	GriKread
};

struct GrDriver GrI = {
	"GrI",
	0,
	&GriExtra,
	0,
	GriInit,
	GriTerm,
	bMoveTo,
	bDrawTo,
	GriSetVisual,
	GriSetActive,
	0,	/* Clear */
	bSetWriteMode,
	GriSetPalette,
	bDrawEllipse,
	GriFlush,
	0	/* Shutters*/
};

#undef GRI_STATS
#undef GRI_SINGLE
#undef INITED
#undef APPNAME
#undef NOCOLOR
