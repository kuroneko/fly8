/* --------------------------------- vesa.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* VGA level video control functions.
*/

#include "fly.h"
#include "vesa.h"

#include <conio.h>
#include <dos.h>


Ulong	vesa_BankSwitches = 0;

static int	FAR vesa_page (Ulong base);
static int	FAR vesa_bank (int bank);

static Uint	Width = 640;

static void (FAR *VesaSwitch) (void) = NULL;
static int	use_vga = 1;

typedef struct {
	Uint	ax;
	Uint	bx;
	Uint	cx;
	Uint	dx;
	Uint	si;
	Uint	di;
	Uint	bp;
	Uint	f;
	Uint	ds;
	Uint	es;
} REGISTERS;

#define VGA_PAGE	((Uchar FAR *)0xa0000000L)
#define MY_OFF(p)	(((Ushort FAR *)&(p))[0])
#define MY_SEG(p)	(((Ushort FAR *)&(p))[1])

#define MAX_SYNC_WAIT	1000L	/* 1 second is long enough */

static int FAR
vesa_int10 (REGISTERS *regs)
{
	union REGS	inregs, outregs;
	struct SREGS    sregs;

	inregs.x.ax = regs->ax;
	inregs.x.bx = regs->bx;
	inregs.x.cx = regs->cx;
	inregs.x.dx = regs->dx;
	inregs.x.si = regs->si;
	inregs.x.di = regs->di;
	sregs.ds = regs->ds;
	sregs.es = regs->es;

	(void) int86x (0x10,&inregs,&outregs,&sregs);

	regs->ax = outregs.x.ax;
	regs->bx = outregs.x.bx;
	regs->cx = outregs.x.cx;
	regs->dx = outregs.x.dx;
	regs->si = outregs.x.si;
	regs->di = outregs.x.di;
	regs->ds = sregs.ds;
	regs->es = sregs.es;

	return (outregs.x.cflag);
}

extern int FAR
vesa_page (Ulong base)			/* Set visual address */
{
	REGISTERS	regs;
	Ulong		lasttime;
	int		port;

	if (2 == CD->colors)
		port = 0x3ba;		/* monochrome */
	else
		port = 0x3da;		/* colour */

	lasttime = st.lasttime;

	while (inp (port) & 0x01) {	/* wait for Display Enabled */
		sys_poll (23);
		if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
			LogPrintf ("%s: sync timed out\n", Gr->name);
			die ();
		}

	}

	if (use_vga) {
		base >>= 2;
		outp (0x3D4, 0x0d);
		iefbr14 ();
		outp (0x3D5, (Uchar)(base & 0x0ff));
		iefbr14 ();
		outp (0x3D4, 0x0c);
		iefbr14 ();
		outp (0x3D5, (Uchar)((base >> 8) & 0x0f));
		iefbr14 ();
	} else {
		regs.dx = (Uint)(base / Width);
		regs.cx = (Uint)(base % Width);
	        regs.ax = 0x4F07;
	        regs.bx = 0;
	        vesa_int10 (&regs);
	}


	while (inp (port) & 0x08) {	/* wait for Vert Sync*/
		sys_poll (24);
		if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
			LogPrintf ("%s: sync timed out\n", Gr->name);
			die ();
		}

	}
	while (!(inp (port) & 0x08)) {	/* wait for Vert Sync end */
		sys_poll (25);
		if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
			LogPrintf ("%s: sync timed out\n", Gr->name);
			die ();
		}

	}
	return (0);
}

extern int FAR
vesa_bank (int bank)			/* set active (64KB) page */
{
	Ushort	flags;
	static int	oldbank = 0;

	if (oldbank != bank && VesaSwitch) {
		oldbank = bank;
		++vesa_BankSwitches;

		bank *= 1;		/* granularity */

_asm {
		pushf
		pop flags
		cli

		mov dx,bank
		mov bx,0
		call DWORD PTR [VesaSwitch]

		mov dx,bank
		mov bx,1
		call DWORD PTR [VesaSwitch]

		push flags
		popf
}
	}
	return (0);
}

extern void FAR
vesa_palette (int color, int r, int g, int b)
{
	short	flags;

_asm {
	pushf
	pop flags
	cli
}

	outp (0x3c8, color);	(void)inp (0x080);
	iefbr14 ();
	outp (0x3c9, r >> 2);	(void)inp (0x080);
	iefbr14 ();
	outp (0x3c9, g >> 2);	(void)inp (0x080);
	iefbr14 ();
	outp (0x3c9, b >> 2);
	iefbr14 ();
_asm {
	push flags
	popf
}
}

static Uchar	FAR infomem[256] = {0};

extern void FAR
vesa_init (Uint mode, int width, int height, int xbytes)
{
	REGISTERS	regs;
	Uchar		FAR *info = infomem;

	VesaSwitch = NULL;

	regs.ax = 0x4F02;		/* set mode */
	regs.bx = mode;
	vesa_int10 (&regs);

	regs.ax = 0x4F01;		/* get info */
	regs.cx = mode;
	regs.es = MY_SEG (info);
	regs.di = MY_OFF (info);
	vesa_int10 (&regs);
	if (0x004F == regs.ax)
		memcpy ((char FAR*)&VesaSwitch, info+12, sizeof (VesaSwitch));

	Width = width;
	use_vga = 0;
	vesa_page (0x40000UL);		/* set visual address */

        regs.ax = 0x4F07;		/* Read back address */
	regs.bx = 1;
	vesa_int10 (&regs);

	if (0x004F != regs.ax ||
	    0 != (regs.bx & 0xff00) ||
	    0x40000UL != (regs.dx * (Ulong)Width + regs.cx))
		use_vga = 1;
	vesa_page (0UL);
	vInit (VGA_PAGE, width, height, xbytes, vesa_bank, vesa_page);
}

extern void FAR
vesa_term (void)
{
	REGISTERS	regs;

	regs.ax = 0x03;		/* text mode */
	vesa_int10 (&regs);
}

#if 0
Ulong	x = 0x76859403UL;

static void
pause (int i)
{
	int	j;

	for (; i > 0; --i)
		for (j = 1000; j > 0; --j)
			x = x * 0x12345678UL;
}

int
main (int argc, char *argv[])
{
	int	mode, p, x, y, rx, ry, c;

/* first set the vesa system.
*/
	if (argc > 1)
		c = argv[1][0];
	else
		c = '1';

	switch (c) {
	default:
	case '1':
		mode = 1;
		width = 640;
		height = 480;
		break;
	case '3':
		mode = 3;
		width = 800;
		height = 600;
		break;
	case '5':
		mode = 5;
		width = 1024;
		height = 768;
		break;
	case '7':
		mode = 7;
		width = 1280;
		height = 1024;
		break;
	}

	vesa_init (0x100+mode);

/* now initialize the drawing system.
*/
	vInit (VGA_PAGE, width, height, width, vesa_bank, vesa_page);
	pause (400);
	vSetWriteMode (T_MXOR);

/* squares in first page.
*/
	vSetActive (0);
	for (p = 0; p < 2*height; ++p) {
		x = rand () % (width-50);
		y = rand () % (height-50);
		c = rand () % 16;
		vMoveTo (   x,    y);
		vDrawTo (50+x,    y, c);
		vDrawTo (50+x, 50+y, c);
		vDrawTo (   x, 50+y, c);
		vDrawTo (   x,    y, c);
		pause (1);
	}
	pause (100);

/* scroll up.
*/
	for (p = 0; p <= height; p += 4)
		vesa_page (p*(Ulong)width);

/* circles in second page.
*/
	vSetActive (1);
	for (p = 0; p < 2*height; ++p) {
		x = rand () % (width-50);
		y = rand () % (height-50);
		rx = rand () % 21 + 5;
		ry = rand () % 21 + 5;
		c = rand () % 16;
		vEllipse (x+25, y+25, rx, ry, c);
		pause (1);
	}
	pause (100);

/* scroll back.
*/
	for (p = height; (p -= 4) >= 0;)
		vesa_page (p*(Ulong)width);
	pause (100);

	vesa_term ();

	printf ("use_vga      %u\n", use_vga);
	printf ("BankSwitches %lu\n", vesa_BankSwitches);

	exit (0);
	return (0);
}
#endif

#undef VGA_PAGE
#undef MY_OFF
#undef MY_SEG
#undef MAX_SYNC_WAIT
