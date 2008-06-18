/* --------------------------------- grAmiga.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* graphics driver for the Amiga: By Michael Taylor
*/

#include "fly.h"

#include "amigainc.h"


extern struct IntuitionBase *IntuitionBase;
#ifdef _DCC
extern struct GfxBase *GfxBase;
#else
struct GfxBase *GfxBase = NULL;
#endif

#define INTUITION_REV	33L
#define DEPTH		3  
#define WIDTH		(dev->sizex)
#define HEIGHT		(dev->sizey)

#define YOFFSET		0
#define XOFFSET		0

static int height, width;

static struct Screen *screen1 = NULL;
static struct NewScreen Fly8Screen = {
    0,                /* the LeftEdge must be zero */
    0,                /* TopEdge */
    640,              /* Width (high-resolution) */
    400,  	      /* Height (NTSC interlace)  */
    DEPTH,            /* Depth 2^DEPTH = number of colours */
    1, 0,             /* DetailPen and BlockPen specifications */
    HIRES,            /* the high-resolution display mode */
    CUSTOMSCREEN,     /* the screen type */
    NULL,             /* no special font */
    "Fly8  by  Eyal Lebedinsky", /* the screen title */
    NULL,             /* no special screen gadgets */
    NULL              /* no CustomBitMap */
};

struct Window *window1 = NULL;
static struct NewWindow Fly8Window = {
	0,
	YOFFSET,
	320,
	200,
	0,1,         /* Plain vanilla DetailPen and BlockPen.       */
	CLOSEWINDOW, /* Tell program when close gadget has been hit */
	WINDOWCLOSE | SMART_REFRESH | ACTIVATE | WINDOWDRAG | BORDERLESS |
	WINDOWDEPTH | WINDOWSIZING | NOCAREREFRESH | RAWKEY,
	NULL,             /* Pointer to the first gadget -- */
	                  /*   may be initialized later.    */
	NULL,             /* No checkmark.   */
	NULL,		  /* Window title.   */
	NULL,             /* Attach a screen later.  */
	NULL,             /* No bitmap.          */
	100,		  /* Minimum width.      */
	25,		  /* Minimum height.     */
	0xFFFF,           /* Maximum width.      */
	0xFFFF,           /* Maximum height.     */
	CUSTOMSCREEN      /* A screen of our own. */
};

#define WHITE  		0xfff
#define BRICKRED 	0xd00
#define RED    		0xf00
#define REDORANGE 	0xf80
#define ORANGE 		0xf90
#define GOLDENORANGE	0xfb0
#define YELLOW 		0xfd0
#define LEMONYELLOW	0xff0
#define LIMEGREEN	0xbf0
#define LIGHTGREEN	0x8e0
#define GREEN  		0x0f0
#define DARKGREEN	0x2c0
#define FORESTGREEN	0x0b1
#define BLUEGREEN	0x0bb
#define AQUA   		0x0db
#define LIGHTAQUA	0x1fb
#define SKYBLUE		0x6fe
#define LIGHTBLUE	0x6ce
#define BLUE   		0x00f
#define BRIGHTBLUE	0x51f
#define DARKBLUE	0x06d
#define PURPLE 		0x91f
#define VIOLET 		0xc1f
#define MAGENTA		0xf1f
#define PINK   		0xfac
#define TAN    		0xdb9
#define BROWN  		0xc80
#define DARKBROWN 	0xa87
#define LIGHTGREY   	0xccc
#define GRAY   		0x999
#define BLACK  		0x000

#define COLOR0 0
#define COLOR1 1
#define COLOR2 2
#define COLOR3 3

static int cleanExit(int exitStatus);

static void 
GrMoveTo (int x, int y)
{
	Move (window1->RPort, x, y+YOFFSET);
}

static void 
GrDrawTo (int x, int y, Uint color)
{
	SetAPen(window1->RPort, (UBYTE)color);
	Draw (window1->RPort, x, y+YOFFSET);
}

static int FAR
GrWriteMode (int mode)
{
#if 0
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
#endif
	return (0);
}

static int
SetPalette (int n, long c)
{
	int	r, g, b;

	r = 0x0ff & ((int)(c      )); 
	g = 0x0ff & ((int)(c >>  8)); 
	b = 0x0ff & ((int)(c >> 16));
	SetRGB4(&screen1->ViewPort, (SHORT)n,
		(UBYTE)(r >> 4), (UBYTE)(g >> 4), (UBYTE)(b >> 4));
	return (0);
}

static int 
GrInit (DEVICE *dev)
{
	static UWORD colortable[] =
	{
		BLACK, WHITE, RED, GREEN, BLUE, BROWN, MAGENTA, GRAY, YELLOW,
		LEMONYELLOW, LIGHTBLUE, REDORANGE, BRICKRED, ORANGE, 
		GOLDENORANGE, LIMEGREEN, LIGHTGREEN, DARKGREEN, FORESTGREEN, 
		BLUEGREEN, AQUA, LIGHTAQUA, SKYBLUE, BRIGHTBLUE, DARKBLUE,
		PURPLE, VIOLET, PINK, TAN, DARKBROWN, LIGHTGREY, WHITE
	};


#ifndef _DCC
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 33L);
	if (GfxBase == NULL) {
	    printf("Error: Cannot open Graphics library\n");
	    return cleanExit(ERROR_INVALID_RESIDENT_LIBRARY);
	}
#endif
	height = HEIGHT;
    	Fly8Screen.Width = WIDTH;
    	Fly8Screen.Height = HEIGHT;
    	if (dev->colors == 32)
		Fly8Screen.Depth = 5;
    	else if (dev->colors == 16)
		Fly8Screen.Depth = 4;
    	else if (dev->colors == 8)
		Fly8Screen.Depth = 3;
    	else if (dev->colors == 4)
		Fly8Screen.Depth = 2;
    	else if (dev->colors == 2)
		Fly8Screen.Depth = 1;
	if (WIDTH == 320)
	    	Fly8Screen.ViewModes = 0;
    	else if (HEIGHT + YOFFSET >= 400)
	    	Fly8Screen.ViewModes = HIRES | LACE;
    	else
	    	Fly8Screen.ViewModes = HIRES;

    	/* Open the screen */
    	screen1 = OpenScreen(&Fly8Screen);
    	if (screen1 == NULL) {
    		printf ("Error: could not create screen\n");
		return cleanExit(RETURN_WARN);
	}
		
	/* Initialize the screen ViewPort's ColorMap. */
	screen1->ViewPort.ColorMap = GetColorMap(32L);
	if (screen1->ViewPort.ColorMap == NULL) {
    		printf ("Error: could not create screen\n");
	    	return cleanExit(ERROR_NO_FREE_STORE);
	}
	LoadRGB4(&screen1->ViewPort, (UWORD *)colortable, (WORD)32);

	/* initialize the new windows attributes */
	Fly8Window.LeftEdge  = 0;
	Fly8Window.TopEdge   = 0;
	Fly8Window.Width     = dev->sizex; /*WIDTH*/
	Fly8Window.Height    = HEIGHT + YOFFSET;
	Fly8Window.MinWidth  = Fly8Window.Width;
	Fly8Window.MinHeight = Fly8Window.Height;
	Fly8Window.Title     = (unsigned char *)"Fly8";
    	Fly8Window.Screen    = screen1;
	Fly8Window.DetailPen = 0;
	Fly8Window.BlockPen  = 1;
	Fly8Window.Title = NULL;
	Fly8Window.Flags = BORDERLESS | SMART_REFRESH | NOCAREREFRESH | RMBTRAP;
	Fly8Window.IDCMPFlags = MOUSEMOVE | MOUSEBUTTONS | RAWKEY;
    	/* Open the window */

    	window1 = OpenWindow(&Fly8Window);
    	if (window1 == NULL) {
    		printf ("Error: could not create graphics window 1\n");
		return cleanExit(RETURN_WARN);
	}

	black   = 0;	/* cannot change */
	white   = 1;	/* cannot change */
	if (dev->colors < 16) {
		red = blue = magenta = green = white;
		brown = gray = hudlow = hudhigh = white;
		lblue = lred = white;
		SetPalette (black,   C_BLACK);
		SetPalette (white,   C_WHITE);
	} else {	
		red     = 2;	/* do not change! */
		blue    = 4;	/* do not change! */
		magenta = 6;	/* do not change! */
		green   = 3;
		brown   = 5;
		gray    = 7;
		hudlow  = 8;
		hudhigh = 9;
		lblue   = 10;
		lred    = 11;
/*		white   = 15;	keep 15 for OR'ed white */
		SetPalette (black,   C_BLACK);
		SetPalette (white,   C_WHITE);
		SetPalette (red,     C_RED);
		SetPalette (blue,    C_BLUE);
		SetPalette (magenta, C_MAGENTA);
		SetPalette (green,   C_GREEN);
		SetPalette (brown,   C_BROWN);
		SetPalette (gray,    C_GRAY);
		SetPalette (hudlow,  C_LYELLOW);
		SetPalette (hudhigh, C_YELLOW);
		SetPalette (lred,    C_LIGHTRED);
		SetPalette (lblue,   C_SKYBLUE);
	}
	
	/* Simple form of setting drawing area to BLACK */
	SetRast(window1->RPort, black);

	ActivateWindow (window1);
	RefreshWindowFrame (window1);

	dev->npages = 1;
	return (0);
}

static void
GrTerm (DEVICE *dev)
{
	cleanExit (0);
}

static int 
cleanExit(int exitStatus)
{
	short i;

    	if (window1)
		CloseWindow( window1 );

/* Free the color map created by GetColorMap(). */
	if (screen1->ViewPort.ColorMap)
	    FreeColorMap(screen1->ViewPort.ColorMap);

	if (screen1)
		CloseScreen( screen1 );
		
#ifndef _DCC
	if (GfxBase)
	    CloseLibrary((struct Library *)GfxBase);
#endif
	if (exitStatus)
		return (exitStatus);
}

static int
GrSetActive (int page)
{return (0);}

static int
GrSetVisual (int page)
{return (0);}

static void FAR
Ellipse (register int x1, register int y1, int rx, int ry, register Uint color)
{
	int	ax, bx, cx, dx, ay, by, cy, dy;

	ax = fmul ( 3196, rx);		/* sin (pi/16) */
	ay = fmul ( 3196, ry);
	bx = fmul ( 9102, rx);		/* sin (3*pi/16) */
	by = fmul ( 9102, ry);
	cx = fmul (13623, rx);		/* sin (5*pi/16) */
	cy = fmul (13623, ry);
	dx = fmul (16069, rx);		/* sin (7*pi/16) */
	dy = fmul (16069, ry);

	GrMoveTo (x1+dx, y1-ay);
	GrDrawTo (x1+cx, y1-by, color);
	GrDrawTo (x1+bx, y1-cy, color);
	GrDrawTo (x1+ax, y1-dy, color);
	GrDrawTo (x1-ax, y1-dy, color);
	GrDrawTo (x1-bx, y1-cy, color);
	GrDrawTo (x1-cx, y1-by, color);
	GrDrawTo (x1-dx, y1-ay, color);
	GrDrawTo (x1-dx, y1+ay, color);
	GrDrawTo (x1-cx, y1+by, color);
	GrDrawTo (x1-bx, y1+cy, color);
	GrDrawTo (x1-ax, y1+dy, color);
	GrDrawTo (x1+ax, y1+dy, color);
	GrDrawTo (x1+bx, y1+cy, color);
	GrDrawTo (x1+cx, y1+by, color);
	GrDrawTo (x1+dx, y1+ay, color);
	GrDrawTo (x1+dx, y1-ay, color);
}

static void
Sync (void)
{}

static void
Flush (void)
{}

static void
GrShutters (int eye)
{}

struct GrDriver GrAmiga = {
	"GrAmiga",
	0,
	0,
	GrInit,
	GrTerm,
	GrMoveTo,
	GrDrawTo,
	GrSetVisual,
	GrSetActive,
	0,
	GrWriteMode,
	SetPalette,
	Ellipse,
	Flush,
	GrShutters
};
#undef INTUITION_REV
#undef DEPTH
#undef WIDTH
#undef HEIGHT
#undef YOFFSET
#undef XOFFSET
#undef WHITE
#undef BRICKRED
#undef RED
#undef REDORANGE
#undef ORANGE
#undef GOLDENORANGE
#undef YELLOW
#undef LEMONYELLOW
#undef LIMEGREEN
#undef LIGHTGREEN
#undef GREEN
#undef DARKGREEN
#undef FORESTGREEN
#undef BLUEGREEN
#undef AQUA
#undef LIGHTAQUA
#undef SKYBLUE
#undef LIGHTBLUE
#undef BLUE
#undef BRIGHTBLUE
#undef DARKBLUE
#undef PURPLE
#undef VIOLET
#undef MAGENTA
#undef PINK
#undef TAN
#undef BROWN
#undef DARKBROWN
#undef LIGHTGREY
#undef GRAY
#undef BLACK
#undef COLOR0
#undef COLOR1
#undef COLOR2
#undef COLOR3
