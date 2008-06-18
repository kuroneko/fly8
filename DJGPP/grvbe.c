/* --------------------------------- grvbe.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* djgpp VBE 2.0 graphics driver. It uses the VBE functions in vbe.c.
*/

#include "fly.h"
#include "bgr.h"
#include "vgr.h"
#include "vbe.h"

#include <pc.h>

#if 2 == SYS_DJGPP
#include <sys/nearptr.h>
#include <crt0.h>
int _crt0_startup_flags = _CRT0_FLAG_NEARPTR;
#endif

#define DOSYNC		0x0001

/* These are exported to vbeasm.x
*/
char		*vbeFunc5 = NULL;
char		*vbeFunc7 = NULL;
char		*vbeFunc9 = NULL;

static char	*vbePrivi = NULL;

#define GRV_STATS	0x0001
#define GRV_PROTF5	0x0002
#define GRV_REALF5	0x0004
#define GRV_ALLF5	(GRV_PROTF5|GRV_REALF5)
#define GRV_PROTF7	0x0008
#define GRV_PROTF7XY	0x0010
#define GRV_ALLF7	(GRV_PROTF7|GRV_PROTF7XY)
#define GRV_PROTF9	0x0020
#define GRV_HWF9	0x0040
#define GRV_VGAF9	0x0080
#define GRV_ALLF9	(GRV_PROTF9|GRV_HWF9|GRV_VGAF9)
#define GRV_OKF5	0x0100
#define GRV_OKF7	0x0200
#define GRV_OKF9	0x0400
#define GRV_VBEAF	0x0800
#define GRV_BANKED	0x2000
#define GRV_LINEAR	0x4000
#define INITED		0x8000

static vbe_info_t	vbe_info;
#define VbeInfo		vbe_info.s

static vbe_pmode_table_t	*vbe_pmode_table = NULL;

static vbe_mode_info_t	vbe_mode_info;
#define VbeModeInfo	vbe_mode_info.s

static vbe_sup_info_t	vbe_sup_info;
#define VbeSupInfo	vbe_sup_info.s

static Ulong		vga_real = 0;
static Ulong		vga_real_size = 0;
static char		*vga_near = NULL;	/* near address of VGA frame */
#if 2 == SYS_DJGPP
static __dpmi_meminfo	vga_info;
#endif

static Ulong		BankSwitches = 0L;
static int		isVBE = 0;
static int		Xbytes = 0;
static Ulong		pageSize = 0;
static int		verbose = 0;

static struct GrDriver NEAR GrVBEv;
static struct GrDriver NEAR GrVBEb;


LOCAL_FUNC int FAR
Grvbe_setbank (int bank)
{
	__dpmi_regs	r;
	int		ret;

	if (Gr->flags & GRV_LINEAR)
		ret = 0;		/* should not be called */
	else if (Gr->flags & GRV_PROTF5) {
		vbeCallFunc5 (bank);
		ret = 0;
	} else if (Gr->flags & GRV_REALF5) {
		clear_dpmi_regs (&r);
		r.x.dx = bank;
		r.x.cs = VbeModeInfo.RealModeFarFuncPtrSeg;
		r.x.ip = VbeModeInfo.RealModeFarFuncPtrOff;
		__dpmi_simulate_real_mode_procedure_retf (&r);
		ret = r.x.ax != 0x004f;
	} else if (isVBE) {
		clear_dpmi_regs (&r);
		r.x.ax = 0x4f05;
		r.h.bh = 0;
		r.h.bl = 0;		/* window */
		r.x.dx = bank;
		__dpmi_int (0x10, &r);
		ret = r.x.ax != 0x004f;
	} else
		ret = 1;
	++BankSwitches;

	if (ret && !(Gr->flags & GRV_OKF5))
		return (ret);
	return (0);
}

LOCAL_FUNC int FAR
Grvbe_setvisual (Ulong base)
{
	__dpmi_regs	r;
	int		ret;

/* wait for Display Enabled
*/
	if (CD->flags & DOSYNC) {
		while (inportb (0x3da) & 0x01)
			sys_poll (20);
	}

/* set display start address
*/
	if (Gr->flags & GRV_PROTF7) {
		if (Gr->flags & GRV_PROTF7XY)
			vbeCallFunc7 (base % Xbytes, base / Xbytes);
		else {
			base >>= 2;
			vbeCallFunc7 (base&0x0ffffU, (base>>16)&0x0ffffU);
		}
		ret = 0;
	} else if (isVBE) {
		clear_dpmi_regs (&r);
		r.x.ax = 0x4f07;
		r.x.bx = 0;
		r.x.cx = base % Xbytes;		/* start pixel */
		r.x.dx = base / Xbytes;		/* start line */
		__dpmi_int (0x10, &r);
		ret = r.x.ax != 0x004f;
	} else
		ret = 1;

	if (ret && !(Gr->flags & GRV_OKF7))
		return (ret);

	if (CD->flags & DOSYNC) {

/* wait for start of Vert Sync
*/
		while (inportb (0x3da) & 0x08)
			sys_poll (21);

/* wait for end of Vert Sync
*/
		while (!(inportb (0x3da) & 0x08))
			sys_poll (22);
	}

	return (0);
}

LOCAL_FUNC int FAR
GrvbebSetActiveBase (int page)
{
	bSetActive (vga_near + page * pageSize);
	return (0);
}

LOCAL_FUNC int FAR
GrvbebSetVisualBase (int page)
{
	return (Grvbe_setvisual (page * pageSize));
}

LOCAL_FUNC int FAR
GrvbeSetPalette (int index, long color)
{
	__dpmi_regs	r;
	long		flags;
	char		rgb[4];
	int		ret;

	if (Gr->flags & GRV_PROTF9) {
		rgb[0] = C_RGB_B (color) >> 2;
		rgb[1] = C_RGB_G (color) >> 2;
		rgb[2] = C_RGB_R (color) >> 2;
		rgb[3] = 0;
		vbeCallFunc9 (index, rgb);
		ret = 0;
	} else if (Gr->flags & GRV_HWF9) {
		flags = Sys->Disable ();
		outportb (0x3c8, index);
		outportb (0x3c9, C_RGB_R (color) >> 2);
		outportb (0x3c9, C_RGB_G (color) >> 2);
		outportb (0x3c9, C_RGB_B (color) >> 2);
		Sys->Enable (flags);
		ret = 0;
	} else if (Gr->flags & GRV_VGAF9) {
		clear_dpmi_regs (&r);
		r.x.ax = 0x1010;	/* set individual color reg VGA */
		r.x.bx = index;		/* first register */
		r.h.dh = C_RGB_R (color) >> 2;
		r.h.ch = C_RGB_G (color) >> 2;
		r.h.cl = C_RGB_B (color) >> 2;
		__dpmi_int (0x10, &r);
		ret = r.x.flags & 0x1;
	} else {
		rgb[0] = C_RGB_B (color) >> 2;
		rgb[1] = C_RGB_G (color) >> 2;
		rgb[2] = C_RGB_R (color) >> 2;
		rgb[3] = 0;
		dosmemput (rgb, sizeof (rgb), tb_real);

		clear_dpmi_regs (&r);
		r.x.ax = 0x4f09;
		r.h.bl = 0;		/* set palette data */
		r.x.cx = 1;		/* one register */
		r.x.dx = index;		/* first register */
		r.x.es = tb_seg;
		r.x.di = 0;
		__dpmi_int (0x10, &r);
		ret = r.x.ax != 0x004f;
	}

	if (ret && !(Gr->flags & GRV_OKF9))
		return (-1);
	return (index);
}

LOCAL_FUNC int FAR
GrvbeOptions (char *options)
{
	char	*p;
	long	temp;

	if (get_parg (options, "stats"))
		Gr->flags |= GRV_STATS;

	if (get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	if (T(p = get_parg (options, "verbose"))) {
		verbose = 1;
		while ('+' == *p++)
			++verbose;
	} else
		verbose = 0;

	Gr->flags |= GRV_PROTF5|GRV_PROTF7|GRV_PROTF9;

	if (get_parg (options, "intf5"))
		Gr->flags &= ~GRV_ALLF5;
	if (get_parg (options, "realf5")) {
		Gr->flags &= ~GRV_ALLF5;
		Gr->flags |= GRV_REALF5;
	}
	if (get_parg (options, "okf5"))
		Gr->flags |= GRV_OKF5;

	if (get_parg (options, "xyf7")) {
		Gr->flags &= ~GRV_ALLF7;
		Gr->flags |= GRV_PROTF7|GRV_PROTF7XY;
	}
	if (get_parg (options, "intf7"))
		Gr->flags &= ~GRV_ALLF7;
	if (get_parg (options, "okf7"))
		Gr->flags |= GRV_OKF7;

	if (get_parg (options, "intf9"))
		Gr->flags &= ~GRV_ALLF9;
	if (get_parg (options, "hwf9")) {
		Gr->flags &= ~GRV_ALLF9;
		Gr->flags |= GRV_HWF9;
	}
	if (get_parg (options, "vgaf9")) {
		Gr->flags &= ~GRV_ALLF9;
		Gr->flags |= GRV_VGAF9;
	}
	if (get_parg (options, "okf9"))
		Gr->flags |= GRV_OKF9;

	if (get_parg (options, "vbeaf"))
		Gr->flags |= GRV_VBEAF;

	if (get_parg (options, "banked"))
		Gr->flags |= GRV_BANKED;

	return (0);
}

LOCAL_FUNC void NEAR
Grvbe_term (void)
{
	__dpmi_regs	r;

	if (vbe_pmode_table) {
		memory_free (vbe_pmode_table, vbe_pmode_table->table_size);
		vbe_pmode_table = NULL;
	}

#if 2 == SYS_DJGPP
	if (vga_near) {
		__dpmi_free_physical_address_mapping (&vga_info);
		vga_near = NULL;
	}
#endif

/* set text mode
*/
	clear_dpmi_regs (&r);
	r.x.ax = 0x03;
	__dpmi_int (0x10, &r);
	if (r.h.ah)
		LogPrintf ("%s: set text mode 0x%x failed.\n",
			Gr->name, 0x03);
}

LOCAL_FUNC int NEAR
Grvbe_init (DEVICE *dev, Uint mode, int width, int height, int xbytes)
{
	__dpmi_regs	r;
	int		i;

	vbe_pmode_table = NULL;
	vga_near = NULL;

	vbeFunc5 = vbeFunc7 = vbeFunc9 = vbePrivi = NULL;
	vga_real = 0;
	BankSwitches = 0L;
	isVBE = 0;
	Xbytes = 320;	/* whatever */

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

/* set requested mode.
 *
 * Try linear frame buffer (VBE).
*/
	if (!(Gr->flags & GRV_BANKED) && (0x100 & mode)) {
		mode |= 0x4000;
		clear_dpmi_regs (&r);
		r.x.ax = 0x4f02;
		r.x.bx = mode;
		__dpmi_int (0x10, &r);
		if (0x004f == r.x.ax) {
			isVBE = 1;
			goto have_mode;
		}
		LogPrintf ("%s: video mode 0x%x VBE set failed (linear).\n",
			Gr->name, mode);
	}

/* Try banked frame buffer (VBE).
*/
	mode &= ~0x4000;
	if (0x100 & mode) {
		clear_dpmi_regs (&r);
		r.x.ax = 0x4f02;
		r.x.bx = mode;
		__dpmi_int (0x10, &r);
		if (0x004f == r.x.ax) {
			isVBE = 1;
			goto have_mode;
		}
		LogPrintf ("%s: video mode 0x%x VBE set failed (banked).\n",
			Gr->name, mode);
	}

/* Try VGA mode.
*/
	clear_dpmi_regs (&r);
	r.x.ax = mode;
	__dpmi_int (0x10, &r);
	if (r.h.ah) {
		LogPrintf ("%s: video mode 0x%x VGA set failed.\n",
			Gr->name, mode);
		goto badret;
	}

have_mode:
	if (verbose)
		LogPrintf ("%s: video mode 0x%x set ok.\n",
			Gr->name, mode);

/* get VBE mode info for requested mode
*/
	if (isVBE && !VBE_get_mode_info (mode, &vbe_mode_info)) {
		if (verbose)
			VBE_show_mode_info (mode, &vbe_mode_info);
		if ((mode & 0x4000) && (VbeModeInfo.ModeAttributes & 0x080)) {
			if (!VbeModeInfo.PhysBasePtrHigh) {
				LogPrintf ("%s: %s %04x%04x\n",
					Gr->name,
					"bad linear frame buffer address",
					(Uint)VbeModeInfo.PhysBasePtrHigh,
					(Uint)VbeModeInfo.PhysBasePtrLow);
			} else
				Gr->flags |= GRV_LINEAR;
		}
		if (dev->sizex > VbeModeInfo.XResolution ||
		    dev->sizey > VbeModeInfo.YResolution) {
			LogPrintf ("%s: mode (%ux%u) too small for %ux%u\n",
				Gr->name,
				(Uint)VbeModeInfo.XResolution,
				(Uint)VbeModeInfo.YResolution,
				dev->sizex, dev->sizey);
			goto badret;
		}
		if (VbeModeInfo.BitsPerPixel != 8) {
			LogPrintf ("%s: mode (%ubpp) not 256 colors!\n",
				Gr->name,
				(Uint)VbeModeInfo.BitsPerPixel);
			goto badret;
		}

		if (0 == VbeModeInfo.RealModeFarFuncPtrSeg &&
		    0 == VbeModeInfo.RealModeFarFuncPtrOff) {
			if (Gr->flags & GRV_REALF5)
				LogPrintf ("%s: no real mode func5\n",
					Gr->name);
			Gr->flags &= ~GRV_REALF5;
		} else
			Gr->flags |= GRV_REALF5;

		Xbytes = VbeModeInfo.BytesPerScanLine;
		if (Gr->flags & GRV_LINEAR) {
			vga_real = ((Ulong)VbeModeInfo.PhysBasePtrHigh << 16)
						+ VbeModeInfo.PhysBasePtrLow;
			vga_real_size = (Ulong)VbeInfo.TotalMemory * 64*1024L;
		} else {
			vga_real = (Ulong)VbeModeInfo.WinAStartSeg << 4;
			vga_real_size = 1024L * VbeModeInfo.BankSizeKb;
		}

if (Gr->flags & GRV_VBEAF) {
	for (i = 0x10; i <= 0x0ff; ++i) {
		if (!VBE_get_sup_info (i, &vbe_sup_info)) {
			if (verbose)
				VBE_show_sup_info (i, &vbe_sup_info);
			else
				LogPrintf ("%s: %s 0x%02x supported\n",
					Gr->name,
					"VBE supplemental function", i);
			if (verbose) {
				int	i, j;

				for (i = 0; i < 16*16; i += 16) {
					LogPrintf ("%s: %04x", Gr->name, i);
					for (j = 0; j < 16; ++j) {
						if (!(j%4))
							LogPrintf (" ");
						LogPrintf ("%02x",
							vbe_sup_info.flat[i+j]);
					}
					LogPrintf ("\n");
				}
			}
		}
	}
}
	} else {
		isVBE = 0;
		Gr->flags &= ~GRV_REALF5;
		Xbytes = xbytes;
		vga_real = 0x0a0000L;
		vga_real_size = 0x010000L;
	}
	dev->npages = VbeInfo.TotalMemory * 64*1024L / height / Xbytes;
	pageSize = height * (Ulong)Xbytes;

/* get VBE protected-mode info (optional).
*/
	if (isVBE && T(vbe_pmode_table = VBE_get_pmode_table ())) {
		if (verbose)
			VBE_show_pmode_table (vbe_pmode_table);

		if (F(vbeFunc5 = vbe_pmode_table->SetWindowFunc))
			Gr->flags &= ~GRV_PROTF5;
		if (F(vbeFunc7 = vbe_pmode_table->SetDisplayStartFunc))
			Gr->flags &= ~GRV_PROTF7;
		if (F(vbeFunc9 = vbe_pmode_table->
					SetPrimaryPaletteDataFunc))
			Gr->flags &= ~GRV_PROTF9;
		vbePrivi = vbe_pmode_table->PrivilegesTable;
	} else
		Gr->flags &= ~(GRV_PROTF5|GRV_PROTF7|GRV_PROTF9);

/* establish addressability to VGA frame
*/
#if 1 == SYS_DJGPP
	vga_near = (char *)0xd0000000;
#else
	vga_info.size = vga_real_size;
	vga_info.address = vga_real;
	if(__dpmi_physical_address_mapping (&vga_info) == -1) {
		LogPrintf ("%s: %s (at 0x%.8lx size 0x%.8lx) failed!\n",
			Gr->name, "physical mapping of VGA frame",
			vga_real, vga_real_size);
		goto badret;
	}
	vga_near = (char *)(vga_info.address + __djgpp_conventional_base);
#endif

/* test the setup
*/
	if (!(Gr->flags & GRV_LINEAR))
	while (Grvbe_setbank (0)) {
		if (Gr->flags & GRV_PROTF5) {
			LogPrintf ("%s: prot-mode func5 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_PROTF5;
		} else if (Gr->flags & GRV_REALF5) {
			LogPrintf ("%s: real-mode func5 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_REALF5;
		} else /* int10 VBE */ {
			LogPrintf ("%s: int10 func5 failed\n",
				Gr->name);
			LogPrintf ("%s: total func5 failure, aborting\n",
				Gr->name);
			goto badret;
		}
	}
	if (Gr->flags & GRV_PROTF5)
		Gr->flags &= ~GRV_REALF5;

	while (Grvbe_setvisual (0)) {
		if (Gr->flags & GRV_PROTF7) {
			LogPrintf ("%s: prot-mode func7 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_ALLF7;
		} else /* int10 VBE */ {
			LogPrintf ("%s: int10 func7 failed\n",
				Gr->name);
			LogPrintf ("%s: %s %s\n",
				Gr->name,
				"total failure for func7,",
				"disabling double buffering");
			dev->npages = 1;
			break;
		}
	}

	while (GrvbeSetPalette (CC_BLACK, C_BLACK)) {
		if (Gr->flags & GRV_PROTF9) {
			LogPrintf ("%s: prot-mode func9 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_ALLF9;
		} else if (Gr->flags & GRV_VGAF9) {
			LogPrintf ("%s: int10 (VGA) func9 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_ALLF9;
			Gr->flags |= GRV_HWF9;
		} else if (Gr->flags & GRV_HWF9) {
			LogPrintf ("%s: hardware func9 failed\n",
				Gr->name);
			LogPrintf ("%s: total func9 failure, aborting\n",
				Gr->name);
			goto badret;
		} else /* int10 VBE */ {
			LogPrintf ("%s: int10 (VBE) func9 failed\n",
				Gr->name);
			Gr->flags &= ~GRV_ALLF9;
			Gr->flags |= GRV_VGAF9;
		}
	}

/* report status
*/
	if (Gr->flags & GRV_LINEAR)
		LogPrintf ("%s: using linear frame buffer.\n",
			Gr->name);
	else if (Gr->flags & GRV_PROTF5)
		LogPrintf ("%s: using prot mode func5.\n",
			Gr->name);
	else if (Gr->flags & GRV_REALF5)
		LogPrintf ("%s: using real mode func5.\n",
			Gr->name);
	else
		LogPrintf ("%s: using int10 (VBE) for func5.\n",
			Gr->name);

	if (Gr->flags & GRV_PROTF7)
		LogPrintf ("%s: using prot mode func7%s.\n",
			Gr->name,
			Gr->flags & GRV_PROTF7XY ? "(x,y)" : "");
	else
		LogPrintf ("%s: using int10 (VBE) for func7.\n",
			Gr->name);

	if (Gr->flags & GRV_PROTF9)
		LogPrintf ("%s: using prot mode func9.\n",
			Gr->name);
	else if (Gr->flags & GRV_HWF9)
		LogPrintf ("%s: using h'ware for func9.\n",
			Gr->name);
	else if (Gr->flags & GRV_VGAF9)
		LogPrintf ("%s: using int10 (VGA) for func9.\n",
			Gr->name);
	else
		LogPrintf ("%s: using int10 (VBE) for func9.\n",
			Gr->name);

	i = Gr->flags;
	if (Gr->flags & GRV_LINEAR) {
		memcpy (Gr, &GrVBEb, sizeof (*Gr));
		Gr->flags = i;
		bSetSize (width, height, Xbytes);
		bSetActive (vga_near);
	} else {
		memcpy (Gr, &GrVBEv, sizeof (*Gr));
		Gr->flags = i;
		BankSwitches = 0L;
		Grvbe_setbank (0);
		vInit (vga_near, width, height, Xbytes, Grvbe_setbank,
			Grvbe_setvisual);
	}

	return (0);

badret:
	Grvbe_term ();
	return (1);
}

LOCAL_FUNC void FAR
GrvbeTerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED)) {
		Grvbe_term ();		/* just in case */
		return;
	}
	Gr->flags &= ~INITED;

	if (BankSwitches)
		LogPrintf ("%s: BankSwitches %s\n",
			Gr->name, show_ul (BankSwitches));
	if (Gr->flags & GRV_STATS)
		LogStats ();
	Grvbe_term ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC int FAR
GrvbeInit (DEVICE *dev, char *options)
{
	int	i;

	if (Gr->flags & INITED)
		GrvbeTerm (dev);	/* just in case */
	Gr->flags = 0;

	if (GrvbeOptions (options))
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

	if (Grvbe_init (dev, dev->mode, dev->sizex, dev->sizey, dev->sizex))
		return (1);

	Gr->SetWriteMode (T_MSET);
	st.colors[CC_BLACK] = Gr->SetPalette (CC_BLACK, C_BLACK);

	for (i = 0; i < dev->npages; ++i) {
		Gr->SetActive (i);
		Gr->Clear (0, 0, dev->sizex, dev->sizey, st.colors[CC_BLACK]);
	}

	Gr->SetVisual (0);
	Gr->SetActive (0);

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC int FAR
GrvShutters (int eye)
{
	if (st.misc[7]) {
		if (eye >= 0)
			outportb (st.misc[7]+4, 1+2*eye);
		else if (-1 == eye)
			outportb (st.misc[7]+4, 1);	/* on */
		else if (-2 == eye)
			outportb (st.misc[7]+4, 0);	/* off */
		return (0);				/* have shutters */
	} else
		return (1);				/* no shutters */
}

struct GrDriver NEAR GrVBE = {
	"GrVBE",
	0,
	NULL,
	NULL,
	GrvbeInit,
	GrvbeTerm,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static struct GrDriver NEAR GrVBEv = {
	"GrVBEv",
	0,
	NULL,		/* Extra */
	NULL,		/* Devices */
	GrvbeInit,
	GrvbeTerm,
	vMoveTo,
	vDrawTo,
	vSetVisual,
	vSetActive,
	vClear,
	vSetWriteMode,
	GrvbeSetPalette,
	vEllipse,
	vPolygon,
	NULL,		/* Flush */
	GrvShutters
};

static struct GrDriver NEAR GrVBEb = {
	"GrVBEb",
	0,
	NULL,		/* Extra */
	NULL,		/* Devices */
	GrvbeInit,
	GrvbeTerm,
	bMoveTo,
	bDrawTo,
	GrvbebSetVisualBase,
	GrvbebSetActiveBase,
	bClear,
	bSetWriteMode,
	GrvbeSetPalette,
	bDrawEllipse,
	bPolygon,	/* Polygon */
	NULL,		/* Flush */
	GrvShutters
};

#undef DOSYNC
#undef GRV_STATS
#undef GRV_PROTF5
#undef GRV_REALF5
#undef GRV_ALLF5
#undef GRV_PROTF7
#undef GRV_PROTF7XY
#undef GRV_ALLF7
#undef GRV_PROTF9
#undef GRV_HWF9
#undef GRV_VGAF9
#undef GRV_ALLF9
#undef GRV_OKF5
#undef GRV_OKF7
#undef GRV_OKF9
#undef GRV_BANKED
#undef GRV_LINEAR
#undef INITED
#undef VbeInfo
#undef VbeModeInfo
#undef VbeSupInfo
