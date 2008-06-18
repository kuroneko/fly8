/* --------------------------------- grddraw.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for Microsoft Windows 95/NT4 DirectDraw
 * DirectDraw support by Christopher Collins ccollins@pcug.org.au
*/

#include <windows.h>

#include "fly.h"

#ifdef USE_DDRAW

/* CINTERFACE gives a slightly nicer iface to DDraw in C
*/
#define CINTERFACE
#include <ddraw.h>
#undef CINTERFACE

#include "common.h"
#include "bgr.h"

#define GRG_STATS	0x0001
#define INITED		0x8000

static HDC		memhdc = 0;

/* New Static Defs
*/
static LPDIRECTDRAW		myDD;
static LPDIRECTDRAWSURFACE	myDDPS;
static LPDIRECTDRAWSURFACE	myDDSS;
static LPDIRECTDRAWPALETTE	myDDP;
static DDSURFACEDESC		myDDSD;
static int			cur_page; /* current "edit" 1 = pri, 0 = sec */
static int			view_page; /* current "primary" 0 = orig, 1 = flipped */
static int			lastlock = 0;
static int			locked = 0;
static void			*vmemptr = NULL; /* needed to unlock memory again */
static RECT			rect = {0, 0, 0, 0};


LOCAL_FUNC void FAR
DDLock (void)
{
	HRESULT	ddret;

	if (locked)
		return;
	
	lastlock = (cur_page == view_page);
	ddret = IDirectDrawSurface_Lock(
			lastlock ? myDDPS : myDDSS,
			&rect, &myDDSD,
			DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);

/* this is a severe choke, but It should be fixed now.
*/
	if (ddret != DD_OK) {
		LogPrintf ("%s: DDLock failed\n", Gr->name);
		die ();
	}

	vmemptr = myDDSD.lpSurface;
	bSetActive ((char BPTR *)vmemptr);
	locked = 1;	
}

LOCAL_FUNC void FAR
DDUnlock (void)
{
	if (!locked)
		return;

	IDirectDrawSurface_Unlock (lastlock ? myDDPS : myDDSS, vmemptr);
	vmemptr = NULL;
	locked = 0;
}

LOCAL_FUNC void FAR
DDbMoveTo (Uint x, Uint y)
{
	DDLock();
	bMoveTo (x, y);
}

LOCAL_FUNC void FAR
DDbDrawTo (Uint x, Uint y, Uint color)
{
	DDLock();
	bDrawTo (x, y, color);
}

LOCAL_FUNC void FAR
DDbEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	DDLock();
	bDrawEllipse (x, y, rx, ry, c);
}

LOCAL_FUNC void FAR
DDbClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
	DDLock();
	bClear (x, y, sx, sy, c);
}

LOCAL_FUNC void FAR
DDbPolygon (int npoints, BUFLINE *points, Uint color)
{
	DDLock();
	bPolygon (npoints, points, color);
}

LOCAL_FUNC int FAR
DDSetActive (int page)
{
	DDUnlock();
	cur_page = !!page;

/* don't do anything, wait for the flush
*/
	return (0);
}

LOCAL_FUNC int FAR
DDSetVisual (int page)
{
	if (page == view_page)
		return (0);
	DDUnlock();
	view_page = !view_page;
	cur_page = !cur_page;
	IDirectDrawSurface_Flip (myDDPS, NULL, DDFLIP_WAIT);
	return (0);
}

LOCAL_FUNC int FAR
DDSetPalette (int n, long c)
{
	PALETTEENTRY	newcol;
	HRESULT		ddret;

	newcol.peRed = (Uchar)C_RGB_R (c);
	newcol.peGreen = (Uchar)C_RGB_G (c);
	newcol.peBlue = (Uchar)C_RGB_B (c);
	newcol.peFlags = 0;
	ddret = IDirectDrawPalette_SetEntries (myDDP, 0, n, 1, &newcol);
	if (ddret != DD_OK)
		return (-1);
	return (n);
}

LOCAL_FUNC void FAR
DDTerm (DEVICE *dev)
{
	DDUnlock ();
		
	IDirectDrawPalette_Release(myDDP);
	myDDP = NULL;
	IDirectDrawSurface_Release(myDDPS);
	myDDPS = NULL;
	myDDSS = NULL;
	IDirectDraw_Release(myDD);
	myDD = NULL;
	
	if (ghWndMain) {
		ReleaseDC (ghWndMain, hdc);
		ghWndMain = 0;
	}

	hdc = 0;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRG_STATS)
		LogStats ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC int FAR
DDInit (DEVICE *dev, char * options)
{
	HRESULT		ddres;	/* Result from DD Calls */
	DDSCAPS		ddscaps;
	int		i;
	PALETTEENTRY	newpal[256]; /* palette to init myDDP with */
	
	if (get_parg (options, "stats"))
		Gr->flags |= GRG_STATS;

	if (dev->sizex == 0 || dev->sizey == 0) {
		LogPrintf ("%s: Bad window size\n", Gr->name);
		goto badret;
	}

	ghWndMain = CreateWindow (
			(LPSTR)Fly8AppName,
			(LPSTR)Fly8Message,
			WS_OVERLAPPEDWINDOW,
			0,			/*  x  */
			0,			/*  y  */
			dev->sizex,		/* cx  */
			dev->sizey,		/* cy  */
			(HWND)0,		/* no parent */
			(HMENU)0,		/* use class menu */
			(HANDLE)Fly8Instance,	/* handle to window instance */
			(LPSTR)0		/* no params to pass on */
			);
	ShowWindow (ghWndMain, SW_SHOW );
	UpdateWindow (ghWndMain);

/* start of DD stuff -- still fairly experimental.
 * Create the pointer to the DDraw Driver
*/
	ddres = DirectDrawCreate(NULL, &myDD, NULL);
	if (ddres != DD_OK) {
		LogPrintf ("%s: DDCreate died\n", Gr->name);
		return (1);
	}
	ddres = IDirectDraw_SetCooperativeLevel(myDD, ghWndMain,
		DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
	if (ddres != DD_OK) {
		LogPrintf ("%s: DDSetCoopLevel died\n", Gr->name);
		return (2);
	}
/* Now set the screen mode, but force 8bpp
*/
	ddres = IDirectDraw_SetDisplayMode (myDD, dev->sizex, dev->sizey, 8);
	if (ddres != DD_OK) {
		if (DDERR_INVALIDMODE == ddres) {
			LogPrintf ("%s: Bad Mode Specified\n", Gr->name);
			return (3);
		} else {
			LogPrintf ("%s: DDSetMode died\n", Gr->name);
			return (4);
		}
	}

/* setup status flags
*/
	rect.top    = 0;
	rect.left   = 0;
	rect.right  = dev->sizex - 1;
	rect.bottom = dev->sizey - 1;
	dev->colors = 256;

/* now, create the surfaces
*/
	memset (&myDDSD, 0, sizeof(myDDSD));
	myDDSD.dwSize = sizeof(myDDSD);
	myDDSD.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	myDDSD.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                          DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	myDDSD.dwBackBufferCount = 1;
	ddres = IDirectDraw_CreateSurface(myDD, &myDDSD, &myDDPS, NULL);
	if (DD_OK != ddres) {
		LogPrintf ("%s: DDCreateSurface died\n", Gr->name);
		return (5);
	}
	cur_page = 0;

/* Create and bind the palette
 * to start with, generate a grayscale palette
*/
	for (i = 0; i < 256; ++i) {
		newpal[i].peRed = (Uchar)i;
		newpal[i].peBlue = (Uchar)i;
		newpal[i].peGreen = (Uchar)i;
		newpal[i].peFlags = 0;
	}
	ddres = IDirectDraw_CreatePalette (myDD,
			DDPCAPS_8BIT | DDPCAPS_ALLOW256,
			newpal, &myDDP, NULL);
	if (DD_OK != ddres) {
		LogPrintf ("%s: DDCreatePalette died\n", Gr->name);
		return (6);
	}
/* and now, bind it
*/
	ddres = IDirectDrawSurface_SetPalette (myDDPS, myDDP);
	if (DD_OK != ddres) {
		LogPrintf ("%s: DDSSetPalette died\n", Gr->name);
		return (7);
	}

/* get a pointer to the back buffer
*/
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddres = IDirectDrawSurface_GetAttachedSurface (myDDPS, &ddscaps,
			&myDDSS);
	if (DD_OK != ddres) {
		LogPrintf ("%s: DDSGetAttchdSurf died\n", Gr->name);
		return (8);
	}

	bSetSize (rect.right+1, rect.bottom+1, rect.right+1);
	view_page = 0;
	cur_page = 1;

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);

	return (0);
badret:
	DDTerm (dev);
	return (1);
}

/* Flush is no longer used
*/
#if 0
LOCAL_FUNC void FAR
DDFlush (void)
{
	DDUnlock();
	IDirectDrawSurface_Flip (myDDPS, NULL, DDFLIP_WAIT);
}
#endif 

struct GrDriver NEAR GrDDraw = {
	"GrDDraw",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	DDInit,
	DDTerm,
	DDbMoveTo,
	DDbDrawTo,
	DDSetVisual,
	DDSetActive,
	DDbClear,
	bSetWriteMode,
	DDSetPalette,
	DDbEllipse,
	DDbPolygon,
	NULL,		/* Flush */
	0		/* Shutters */
};

#undef GRG_STATS
#undef INITED
#endif /* #ifdef USE_DDRAW */
