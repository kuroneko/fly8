/* --------------------------------- low.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Low level componnents for grasm/line.
 *
 * GrlSetActiveBase (Ulong b)
 * GrlSetVisualBase (Ulong b)
 * GrlScrClear (int x, int y, int sizex, int sizey, Uint color)
*/

#include "fly.h"
#include "grasm.h"
#include "vbe.h"

#include <pc.h>

#if 2 == SYS_DJGPP
#include <sys/nearptr.h>
#endif


#if 2 == SYS_DJGPP
static vbe_info_t	vbe_info;
#define VbeInfo		vbe_info.s

static vbe_mode_info_t	vbe_mode_info;
#define VbeModeInfo	vbe_mode_info.s

static __dpmi_meminfo	vga_info;
#endif

static char	BPTR *vga_near = 0;
static Ulong	VisualBase = 0;
static Uint	xBytes = 0;
static Uint	xSize = 0;
static Uint	ySize = 0;
static Ulong	pageSize = 0;
static int	verbose = 0;


LOCAL_FUNC int FAR
GrlSetBiosMode (int n)
{
	__dpmi_regs	r;

	clear_dpmi_regs (&r);
	if (n >= 0x100) {
		r.x.ax = 0x4f02;
		r.x.bx = n;
	} else
		r.x.ax = n;
	__dpmi_int (0x10, &r);
	return (r.h.ah || ((n >= 0x100) && (r.h.al != 0x4f)));
}

extern int FAR
GrlSetActiveBase (int page)
{
	bSetActive (vga_near + page * pageSize);
	return (0);
}

LOCAL_FUNC void FAR
set_visualbase (Ulong base)	      /* t4k/VBE specific!!!!!!!!!!!!!!! */
{
	int		i;
	Ulong		flags;
	__dpmi_regs	r;

	if (base == VisualBase)
		return;

	switch (Gr->flags & GRL_TYPES) {
	case GRL_TYPE_T4K:
		flags = Sys->Disable ();
		outportb (0x3d4, 0x0d); /* start base low */
		outportb (0x3d5, (int)((base>>2)&0x00ff));

		outportb (0x3d4, 0x0c); /* start base high */
		outportb (0x3d5, (int)((base>>10)&0x00ff));

		outportb (0x3d4, 0x33); /* start base very high */
		i = inportb (0x3d5);
		outportb (0x3d5, (i&~3)|(int)((base>>18)&3));
		Sys->Enable (flags);
		break;
	case GRL_TYPE_VESA:
		clear_dpmi_regs (&r);
		r.x.ax = 0x4F07;
		r.x.bx = 0;
		r.x.dx = base / xBytes;
		r.x.cx = base % xBytes;
		__dpmi_int (0x10, &r);
		break;
	default:
		/* oops! */
		break;
	}
	VisualBase = base;
}

extern int FAR
GrlSetVisualBase (int page)
{
	if (CD->flags & DOSYNC) {
		while (inportb (0x3da) & 0x01)	/* wait for Display Enabled */
			sys_poll (20);
	}
	set_visualbase ((long)page * pageSize);
	if (CD->flags & DOSYNC) {
		while (inportb (0x3da) & 0x08)	/* wait for Vert Sync*/
			sys_poll (21);
		while (!(inportb (0x3da) & 0x08)) /* wait for Vert Sync end */
			sys_poll (22);
	}
	return (0);
}

extern int FAR
GrlTerm (void)
{
#if 1 == SYS_DJGPP
{
	union REGS	r;

	r.x.ax = 0x0ff01;	/* default text */
	r.x.cx = 0;
	r.x.dx = 0;
	int86 (0x10, &r, &r);	/* must use int86()!!! */
}
#endif

#if 2 == SYS_DJGPP
	if (vga_near) {
		__dpmi_free_physical_address_mapping (&vga_info);
		vga_near = NULL;
	}
	GrlSetBiosMode (0x03);
#endif

	return (0);
}

extern int FAR
GrlInit (int mode, int Verbose, int *sizex, int *sizey, int *npages)
{
	verbose = Verbose;
	xSize = *sizex;
	ySize = *sizey;
	xBytes = xSize;
	pageSize = (long)xBytes * ySize;

#if 1 == SYS_DJGPP
{
	union REGS	r;

	r.x.ax = 0x0ff06;	/* WxH graphics */
	r.x.cx = xSize;
	r.x.dx = ySize;
	int86 (0x10, &r, &r);	/* must use int86()!!! */
	vga_near = VGA_PAGE;

	if (mode &&  GrlSetBiosMode (mode)) {
		LogPrintf ("%s: SetBiosMode failed\n",
			Gr->name);
		return (1);
	}
}
#else
{
	__dpmi_regs	r;

/* get VBE info
*/
	if (VBE_get_info (&vbe_info))
		goto badret;
	if (verbose)
		VBE_show_info (&vbe_info);

/* show all modes
*/
	if (verbose >= 2)
		VBE_show_modes_info (&vbe_info);

/* set requested mode
*/
	clear_dpmi_regs (&r);
	r.x.ax = 0x4f02;
	r.x.bx = mode;
	__dpmi_int (0x10, &r);
	if (r.x.ax != 0x004f) {
		LogPrintf ("%s: video mode 0x%x VBE set failed.\n",
			Gr->name, mode);
		clear_dpmi_regs (&r);
		r.x.ax = mode;
		__dpmi_int (0x10, &r);
		if (r.h.ah) {
			LogPrintf ("%s: video mode 0x%x VGA set failed.\n",
				Gr->name, mode);
			goto badret;
		}
	}
	if (verbose)
		LogPrintf ("%s: video mode 0x%x set ok.\n",
			Gr->name, mode);

/* get VBE mode info for requested mode
*/
	if (VBE_get_mode_info (mode, &vbe_mode_info))
		goto badret;
	if (verbose)
		VBE_show_mode_info (mode, &vbe_mode_info);

	if (!(VbeModeInfo.ModeAttributes & 0x0080)) {
		LogPrintf ("%s: Linear frame buffer not supported\n",
		Gr->name);
		goto badret;
	}

	if (xSize > VbeModeInfo.XResolution ||
	    ySize > VbeModeInfo.YResolution) {
		LogPrintf ("%s: mode small for requested WxH\n",
			Gr->name);
	    	goto badret;
	}

	if (VbeModeInfo.BitsPerPixel != 8) {
		LogPrintf ("%s: mode not 256 colore!\n",
			Gr->name);
	    	goto badret;
	}

	if (!VbeModeInfo.PhysBasePtrHigh) {
		LogPrintf ("%s: bad linear buffer address!\n",
			Gr->name);
	    	goto badret;
	}

	xBytes = VbeModeInfo.BytesPerScanLine;
	pageSize = (long)xBytes * ySize;
	*npages = VbeInfo.TotalMemory * 64*1024L / pageSize;

	vga_info.size    = (Ulong)VbeInfo.TotalMemory * 64*1024L;
	vga_info.address = ((Ulong)VbeModeInfo.PhysBasePtrHigh << 16)
						+ VbeModeInfo.PhysBasePtrLow;
	if (__dpmi_physical_address_mapping (&vga_info) == -1) {
		LogPrintf ("%s: physical mapping of linear buffer failed!\n",
			Gr->name);
		return (1);
	}
	vga_near = (void *)(vga_info.address + __djgpp_conventional_base);
}
#endif

	bSetSize (xBytes, ySize, xBytes);
	GrlSetActiveBase (0);
	VisualBase = 0;

	return (0);

badret:
	GrlTerm ();
	return (1);
}
