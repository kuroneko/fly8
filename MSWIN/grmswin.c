/* --------------------------------- grmswin.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for Microsoft Windows.
 * Windows support by Michael Taylor miket@pcug.org.au
*/

#include <windows.h>

#include "fly.h"
#include "common.h"


#define INITED		0x8000

static HDC      	memhdc;
static HBITMAP		mybm1, oldbm=0;
static RECT		rect;
static HBRUSH		brushes[256];
static HPEN 		pens[256];
static COLORREF		colors[256];
static HPALETTE 	hpal;
static HPALETTE 	oldhpal;
static PALETTEENTRY	ape[256];
static int 		currx = 0, curry = 0, plotpt = 0;
static int 		ncolors = 0;

static void FAR
GrmChangeSize ()
{
        RECT rect;

	GetClientRect (ghWndMain, &rect);
        
  	CS->sizex = (xshort)rect.right;
	CS->sizey = (xshort)rect.bottom;
  	set_screen (rect.right, rect.bottom);
	if (oldbm) {		/* resize the bitmap as well */
		SelectObject (memhdc, oldbm);
		DeleteObject (mybm1);
		mybm1 = CreateCompatibleBitmap (hdc, rect.right, rect.bottom);
		oldbm = SelectObject (memhdc, mybm1);
	}
	FillRect (memhdc, &rect, GetStockObject (BLACK_BRUSH));
	repaint ();
}

extern void FAR
MSResetPalette (void)
{
        if (hpal) {
		SelectPalette (memhdc, hpal, FALSE);
		RealizePalette (memhdc);  /* remap colours as we lost the focus */
		if (memhdc != hdc) {
			SelectPalette (hdc, hpal, FALSE);
			RealizePalette (hdc);  /* remap colours as we lost the focus */
		}
	}
}

static int FAR
GrmSetActive (int page)
{
	page = page;
	return (0);
}

static int FAR
GrmSetVisual (int page)
{
	page = page;
	return (0);
}

static int FAR
GrmSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		mode = R2_COPYPEN;
		break;
	case T_MOR:
		mode = R2_MERGEPEN;
		break;
	case T_MXOR:
		mode = R2_XORPEN;
		break;
	}
	SetROP2 (memhdc, mode);

	return (0);
}

static int FAR
GrmSetPalette (int n, long c)
{
	Uint	r, g, b;

	if (ncolors < 16 && (CC_BLACK != n && CC_WHITE != n))
		return (st.colors[CC_WHITE]);

	r = C_RGB_R (c);
	g = C_RGB_G (c);
	b = C_RGB_B (c);

	c = RGB (r, g, b);
	
     	ape[n].peRed = 	 LOBYTE(r);
     	ape[n].peGreen = LOBYTE(g);
    	ape[n].peBlue =  LOBYTE(b);
    	ape[n].peFlags = 0;

	SetPaletteEntries (hpal, 0, 256, ape);
	RealizePalette (memhdc);
	if (memhdc != hdc) {
		SelectPalette (hdc, hpal, FALSE);
		RealizePalette (hdc);
	}

	if (pens[n])
		DeleteObject (pens[n]);
	pens[n] = CreatePen (PS_SOLID, 0, 
			PALETTEINDEX(GetNearestPaletteIndex (hpal, c)));

	if (brushes[n])
		DeleteObject (brushes[n]);
	brushes[n] = CreateSolidBrush (
			PALETTEINDEX(GetNearestPaletteIndex (hpal, c)));

	colors[n] = c;
	return (n);
}

static Uint    last_color = 0xffff;

static void
GrmMoveTo (Uint x, Uint y)
{
	if (plotpt) {
		SetPixel (memhdc, currx, curry, colors[last_color]);
		plotpt = 0;
	}
#if SYS_MSWIN_32
	{
		POINT	dummy;

		MoveToEx (memhdc, x, y, &dummy);
	}
#else
	MoveTo (memhdc, x, y);
#endif
	currx = x;
	curry = y;
}

static void
GrmDrawTo (Uint x, Uint y, Uint c)
{
  	if (last_color != c) {
    		if (!pens[c])
			MessageBox((HWND)0, 
				   (LPCSTR)"No pen created for that colour",
				   (LPCSTR)"Fly8 Error", MB_OK);
		else
			SelectObject (memhdc, pens[c]);
		last_color = c;
  	}

  	LineTo (memhdc, x, y);
  	currx = x;
	curry = y;
	plotpt = 1;
}

static void FAR
GrmClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
	rect.left   = x;
	rect.top    = y;
	rect.right  = x + sx;
	rect.bottom = y + sy;

	FillRect (memhdc, &rect, brushes[c]);

	currx = rect.right;
	curry = rect.bottom;
	last_color = c;
	plotpt = 1;
}

static void FAR
GrmPolygon (int npoints, BUFLINE *points, Uint c)
{
	POINT	wpoints[50];
	int	i;

	if (npoints > rangeof (wpoints))
		return;

	for (i = 0; i < npoints; ++i) {
		wpoints[i].x =  *points++;
		wpoints[i].y =  *points++;
	}
	SelectObject (memhdc, pens[c]);		/* border */
	SelectObject (memhdc, brushes[c]);		/* fill */
	Polygon (memhdc, (POINT FAR *)wpoints, npoints);
}

static void FAR
GrmTerm (DEVICE *dev)
{
	int i;
	
	dev = dev;

	for (i = 0; i < 256; ++i) {
		if (pens[i]) {
			DeleteObject (pens[i]);
			pens[i] = 0;
		}
		if (brushes[i]) {
			DeleteObject (brushes[i]);
			brushes[i] = 0;
		}
	}
	
	SelectPalette (hdc, oldhpal, FALSE);
	DeleteObject (hpal);
	hpal = 0;
	
	if (oldbm) {
		SelectObject (memhdc, oldbm);
		DeleteObject (mybm1);
		mybm1 = 0;
		DeleteDC (memhdc);
		memhdc = 0;
		oldbm = 0;
	}
	
	if (ghWndMain) {
		ReleaseDC (ghWndMain, hdc);
		ghWndMain = 0;
	}

	hdc = 0;

	if (Gr->flags & INITED) {
		Gr->flags &= ~INITED;
		LogPrintf ("%s: term ok\n", Gr->name);
	}
}

static int FAR
GrmInit (DEVICE *dev, char * options, int blt)
{
	int		i, r, g, b;
	LOGPALETTE	*plgpl;
	
	if (Gr->flags & INITED) {
		LogPrintf ("%s: init - already inited\n", Gr->name);
		return (1);
	}

	if (dev->sizex == 0 || dev->sizey == 0) {
		LogPrintf ("%s: init - bas sizes\n", Gr->name);
		return (1);
	}

	for (i = 0; i < 256; ++i) {
		colors[i] = 0;
		pens[i] = 0;
		brushes[i] = 0;
	}
	hpal = 0;
	mybm1 = 0;
	memhdc = 0;
	oldbm = 0;
	hdc = 0;
	ghWndMain = 0;
	
	ghWndMain = CreateWindow (
			(LPSTR)Fly8AppName,
			(LPSTR)Fly8Message,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			0,			/*  x  */
			0,			/*  y  */
			dev->sizex,		/* cx  */
			dev->sizey,		/* cy  */
			(HWND)0,		/* no parent */
			(HMENU)0,		/* use class menu */
			(HANDLE)Fly8Instance,	/* handle to window instance */
			(LPSTR)0		/* no params to pass on */
			);

/* Make window visible
*/
	ShowWindow (ghWndMain, SW_SHOW );
	UpdateWindow (ghWndMain);

	memhdc = hdc = GetDC (ghWndMain);
	GetClientRect (ghWndMain, &rect);
  	CS->sizex = (xshort)rect.right;
  	CS->sizey = (xshort)rect.bottom;
  	set_screen (rect.right, rect.bottom);
	
/* decide if double buffering using bit-blitting is possible 
*/
	if (blt && (GetDeviceCaps (hdc, RASTERCAPS) & RC_BITBLT) && 
			T(memhdc = CreateCompatibleDC (hdc)) && 
			T(mybm1 = CreateCompatibleBitmap (hdc,
						CS->sizex, CS->sizey))) {
		oldbm = SelectObject (memhdc, mybm1);
		FillRect (memhdc, &rect, GetStockObject (BLACK_BRUSH));
		SelectObject (memhdc, mybm1);
		SetPaletteEntries (hpal, 0, 256, ape);
		RealizePalette (memhdc);
		dev->npages = 1;	/* double-bufferred */
	} else {
		if (mybm1)
			DeleteObject (mybm1);
		if (blt)
			MessageBox ((HWND)0, (LPCSTR)"Cannot use BitBlt",
						(LPCSTR)"Fly8 Message", MB_OK);
		memhdc = hdc;
		oldbm = 0L;
		dev->npages = 1;
	}

	SetROP2 (memhdc, R2_COPYPEN);

	plgpl = (LOGPALETTE *) malloc (
			sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));

	plgpl->palNumEntries = 256;
	plgpl->palVersion = 0x300;

	for (i = 0, r = 0, g = 127, b = 127; i < 256;
					i++, r += 1, g += 1, b += 1) {
    		ape[i].peRed   = plgpl->palPalEntry[i].peRed   = LOBYTE (r);
    		ape[i].peGreen = plgpl->palPalEntry[i].peGreen = LOBYTE (g);
    		ape[i].peBlue  = plgpl->palPalEntry[i].peBlue  = LOBYTE (b);
    		ape[i].peFlags = plgpl->palPalEntry[i].peFlags = 0;
	}
	hpal = CreatePalette (plgpl);
	free (plgpl);
	oldhpal = SelectPalette (memhdc, hpal, FALSE);
	SetPaletteEntries (hpal, 0, 256, ape);
	RealizePalette (memhdc);

	SetPolyFillMode(memhdc, ALTERNATE);

	dev->colors = ncolors = GetDeviceCaps (memhdc, NUMCOLORS);

#ifdef USE_WING	
	usingWinG = 0;
#endif
	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);

badret:
	GrmTerm (dev);
	return (1);
}

static int FAR
GrmInitS (DEVICE *dev, char * options)
{
        return (GrmInit (dev, options, 0));
}
        
static int FAR
GrmInitB (DEVICE *dev, char * options)
{
        return (GrmInit (dev, options, 1));
}
        
static void
GrmFlush (void)
{
	if (resetSSize) {
	        GrmChangeSize ();
	        resetSSize = 0;
	} else
		BitBlt (hdc, 0, 0, CS->sizex, CS->sizey, memhdc, 0, 0, 
			SRCCOPY); 
}

static void
GrmFlushS (void)
{
	if (resetSSize) {
	        GrmChangeSize ();
	        resetSSize = 0;
	}
}

struct GrDriver NEAR GrMSWinB = {
	"GrBitBlt",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	GrmInitB,
	GrmTerm,
	GrmMoveTo,
	GrmDrawTo,
	GrmSetVisual,
	GrmSetActive,
	GrmClear,
	GrmSetWriteMode,
	GrmSetPalette,
	0,		/* Ellipse */
	GrmPolygon,
	GrmFlush,
	0		/* Shutters */
};

struct GrDriver NEAR GrMSWinS = {
	"GrGDI",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	GrmInitS,
	GrmTerm,
	GrmMoveTo,
	GrmDrawTo,
	GrmSetVisual,
	GrmSetActive,
	GrmClear,
	GrmSetWriteMode,
	GrmSetPalette,
	0,		/* Ellipse */
	GrmPolygon,
	GrmFlushS,
	0		/* Shutters */
};
#undef INITED
