/* --------------------------------- vbe.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general VBE functions.
*/

#include "fly.h"
#include "vbe.h"

LOCAL_FUNC Ushort NEAR
V_Us (Uchar *p)
{
	return (p[0] + ((Ushort)p[1] << 8));
}

LOCAL_FUNC char * NEAR
VBE_get_string (Uint seg, Uint off)
{
	static char	room[512];
	Ulong		linear;

	linear = ((Ulong)seg << 4) + off;
	if (linear)
		dosmemget (linear, sizeof (room), room);
	else
		memset (room, 0, sizeof (room));
	return (room);
}

extern int FAR
VBE_get_info (vbe_info_t *t)
{
	__dpmi_regs	r;

	dosmemput ("VBE2", 4, tb_real);
	clear_dpmi_regs (&r);
	r.x.ax = 0x4F00;
	r.x.es = tb_seg;
	r.x.di = 0;
	__dpmi_int (0x10, &r);
	if (r.x.ax != 0x004f) {
		LogPrintf ("%s: could not get VBE info.\n",
			Gr->name);
		return (1);
	}
	if (t)
		dosmemget (tb_real, sizeof (*t), t);
	return (0);
}

extern int FAR
VBE_get_mode_info (int mode, vbe_mode_info_t *t)
{
	__dpmi_regs	r;

	clear_dpmi_regs (&r);
	r.x.ax = 0x4F01;
	r.x.cx = mode & 0x3fff;		/* remove clear+linear bits */
	r.x.es = tb_seg;
	r.x.di = 0;
	__dpmi_int (0x10, &r);
	if (r.x.ax != 0x004f) {
		LogPrintf ("%s: could not get VBE mode info.\n",
			Gr->name);
		return (1);
	}
	if (t)
		dosmemget (tb_real, sizeof (*t), t);
	return (0);
}

extern vbe_pmode_table_t * FAR
VBE_get_pmode_table (void)
{
	__dpmi_regs		r;
	int			i;
	vbe_pmode_table_t	*t;

	clear_dpmi_regs (&r);
	r.x.ax = 0x4F0A;
	r.x.bx = 0;
	__dpmi_int (0x10, &r);
	if (r.x.ax != 0x004f) {
		LogPrintf ("%s: could not get VBE prot-mode info.\n",
			Gr->name);
		return (NULL);
	}

	i = r.x.cx + offsetof (vbe_pmode_table_t, info);
	if (F(t = (vbe_pmode_table_t *)memory_alloc (i))) {
		LogPrintf ("%s: could not alloc vbe_pmode_table.\n",
			Gr->name);
		return (NULL);
	}
	t->pmode_size = r.x.cx;
	t->table_size = i;

	dosmemget (((Ulong)r.x.es << 4) + r.x.di, t->pmode_size, &t->info);

	t->SetWindowFunc =
		T(i = t->info.s.SetWindowFuncOff)
		? t->info.flat + i : NULL;
	t->SetDisplayStartFunc =
		T(i = t->info.s.SetDisplayStartFuncOff)
		? t->info.flat + i : NULL;
	t->SetPrimaryPaletteDataFunc =
		T(i = t->info.s.SetPrimaryPaletteDataFuncOff)
		? t->info.flat + i : NULL;
	t->PrivilegesTable =
		T(i = t->info.s.PrivilegesTableOff)
		? t->info.flat + i : NULL;

	return (t);
}

extern int FAR
VBE_get_sup_info (int sup, vbe_sup_info_t *t)
{
	__dpmi_regs	r;

	if (sup < 0x10 || sup > 0x0ff)
		return (1);

	clear_dpmi_regs (&r);
	r.x.ax = 0x4F00 + sup;
	r.x.bx = 0;
	r.x.es = tb_seg;
	r.x.di = 0;
	__dpmi_int (0x10, &r);
	if (r.x.ax != 0x004f)
		return (1);
	if (t)
		dosmemget (tb_real, sizeof (*t), t);
	return (0);
}

extern void FAR
VBE_show_info (vbe_info_t *t)
{
	Ushort	*p;
	int	i;

	LogPrintf ("\n%s: VBE info block:\n",
		Gr->name);
	LogPrintf ("%s: %-30s\"%4.4s\" version 0x%.4x\n",
		Gr->name, "VBE Signature", t->s.VbeSignature,
		(int)t->s.VbeVersion);
	LogPrintf ("%s: %-30s0x%.4x%.4x\n",
		Gr->name, "Capabilities", (int)t->s.CapabilitiesHigh,
		(int)t->s.CapabilitiesLow);

	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "OemString", (int)t->s.OemStringPtrSeg,
		(int)t->s.OemStringPtrOff,
		VBE_get_string ((int)t->s.OemStringPtrSeg,
					(int)t->s.OemStringPtrOff));

	LogPrintf ("%s: %-30s%.4x:%.4x\n",
		Gr->name, "VideoModes", (int)t->s.VideoModesPtrSeg,
		(int)t->s.VideoModesPtrOff);

	p = (Ushort *)VBE_get_string ((int)t->s.VideoModesPtrSeg,
						(int)t->s.VideoModesPtrOff);
	for (i = 0; (Ushort)0xffff != *p; ++p, ++i) {
		if (i >= 256) {
			LogPrintf ("%s: missing end of table!\n",
				Gr->name);
			break;
		}
		LogPrintf ("%s: %3u 0x%.4x\n",
			Gr->name, i, (Uint)*p);
	}

	LogPrintf ("%s: %-30s%ux64KB = %luKB\n",
		Gr->name, "TotalMemory", (int)t->s.TotalMemory,
		64L * t->s.TotalMemory);
	LogPrintf ("%s: %-30s0x%.4x\n",
		Gr->name, "OemSoftwareRevision",
		(int)t->s.OemSoftwareRevision);
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "VendorName", (int)t->s.VendorNamePtrSeg,
		(int)t->s.VendorNamePtrOff,
		VBE_get_string ((int)t->s.VendorNamePtrSeg,
					(int)t->s.VendorNamePtrOff));
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "ProductName", (int)t->s.ProductNamePtrSeg,
		(int)t->s.ProductNamePtrOff,
		VBE_get_string ((int)t->s.ProductNamePtrSeg,
					(int)t->s.ProductNamePtrOff));
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "ProductRevision",
		(int)t->s.ProductRevisionPtrSeg,
		(int)t->s.ProductRevisionPtrOff,
		VBE_get_string ((int)t->s.ProductRevisionPtrSeg,
					(int)t->s.ProductRevisionPtrOff));

	LogPrintf ("\n"); 
}

extern void FAR
VBE_show_pmode_table (vbe_pmode_table_t *t)
{
	int	i;
	Ushort	*sp, s;

	LogPrintf ("\n%s: VBE protected mode info block:\n",
		Gr->name);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "SetWindowFuncOff",
		(Uint)t->info.s.SetWindowFuncOff);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "SetDisplayStartFuncOff",
		(Uint)t->info.s.SetDisplayStartFuncOff);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "SetPrimaryPaletteDataFuncOff",
		(Uint)t->info.s.SetPrimaryPaletteDataFuncOff);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "PrivilegesTableOff",
		(Uint)t->info.s.PrivilegesTableOff);

	if (t->PrivilegesTable) {
		for (sp = (Ushort *)t->PrivilegesTable, i = 0;
				i < 100 && 0xffffU != (s = *sp); ++sp, ++i) {
			LogPrintf ("%s: %-30s%3u %.4x\n",
				Gr->name, "Port",
				i, (Ulong)s);
		}
	}
	LogPrintf ("\n"); 
}

extern void FAR
VBE_show_mode_info (int mode, vbe_mode_info_t *t)
{
	LogPrintf ("\n%s: VBE mode info block for mode 0x%x:\n",
		Gr->name, mode);
	LogPrintf ("%s: %-30s0x%.4x\n",
		Gr->name, "ModeAttributes",
		(int)t->s.ModeAttributes);
	LogPrintf ("%s: %-30s0x%.2x\n",
		Gr->name, "WinAAttributes",
		(int)t->s.WinAAttributes);
	LogPrintf ("%s: %-30s0x%.2x\n",
		Gr->name, "WinBAttributes",
		(int)t->s.WinBAttributes);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "WinGranularity",
		(int)t->s.WinGranularity);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "WindowSize",
		(int)t->s.WindowSize);
	LogPrintf ("%s: %-30s0x%.4x\n",
		Gr->name, "WinAStartSeg",
		(int)t->s.WinAStartSeg);
	LogPrintf ("%s: %-30s0x%.4x\n",
		Gr->name, "WinBStartSeg",
		(int)t->s.WinBStartSeg);
	LogPrintf ("%s: %-30s%.4x:%.4x\n",
		Gr->name, "RealModeFarFuncPtr",
		(int)t->s.RealModeFarFuncPtrSeg,
		(int)t->s.RealModeFarFuncPtrOff);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "BytesPerScanLine",
		(int)t->s.BytesPerScanLine);

	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "XResolution",
		(int)t->s.XResolution);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "YResolution",
		(int)t->s.YResolution);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "NumberOfPlanes",
		(int)t->s.NumberOfPlanes);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "BitsPerPixel",
		(int)t->s.BitsPerPixel);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "NumberOfBanks",
		(int)t->s.NumberOfBanks);
	LogPrintf ("%s: %-30s0x%x\n", 
		Gr->name, "MemoryModelType",
		(int)t->s.MemoryModelType);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "BankSizeKb",
		(int)t->s.BankSizeKb);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "NumberOfImages",
		(int)t->s.NumberOfImages);

	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "RedMaskSize",
		(int)t->s.RedMaskSize);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "RedFieldPosition",
		(int)t->s.RedFieldPosition);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "GreenMaskSize",
		(int)t->s.GreenMaskSize);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "GreenFieldPosition",
		(int)t->s.GreenFieldPosition);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "BlueMaskSize",
		(int)t->s.BlueMaskSize);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "BlueFieldPosition",
		(int)t->s.BlueFieldPosition);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "RsvdMaskSize",
		(int)t->s.RsvdMaskSize);
	LogPrintf ("%s: %-30s%u\n",
		Gr->name, "RsvdFieldPosition",
		(int)t->s.RsvdFieldPosition);
	LogPrintf ("%s: %-30s0x%x\n",
		Gr->name, "DirectColorModeInfo",
		(int)t->s.DirectColorModeInfo);

	LogPrintf ("%s: %-30s0x%.4x%.4x\n", 
		Gr->name, "PhysBasePtr",
		(int)t->s.PhysBasePtrHigh,
		(int)t->s.PhysBasePtrLow);
	LogPrintf ("%s: %-30s0x%.4x%.4x\n", 
		Gr->name, "OffScreenMemoryPtr",
		(int)t->s.OffScreenMemoryPtrHigh,
		(int)t->s.OffScreenMemoryPtrLow);
	LogPrintf ("%s: %-30s%u\n", 
		Gr->name, "OffScreenMemoryKb",
		(int)t->s.OffScreenMemoryKb);

	LogPrintf ("\n"); 
}

extern void FAR
VBE_show_modes_info (vbe_info_t *t)
{
	vbe_mode_info_t	vbe_mode_info;
	Ushort		modes[256];
	Ushort		*p;
	int		i;

	LogPrintf ("\n%s: video modes info:\n",
		Gr->name);

	memcpy (modes, VBE_get_string ((int)t->s.VideoModesPtrSeg,
				(int)t->s.VideoModesPtrOff), sizeof (modes));
	for (i = 0, p = modes; (Ushort)0xffff != *p; ++p, ++i) {
		if (i >= 256) {
			LogPrintf ("%s: more than 256 modes!\n",
				Gr->name);
			break;
		}
		if (VBE_get_mode_info (*p, &vbe_mode_info)) {
			LogPrintf ("%s: no mode info for 0x%x.\n",
				Gr->name, (Uint)*p);
			continue;
		}
		VBE_show_mode_info (*p, &vbe_mode_info);
	}
}

extern void FAR
VBE_show_sup_info (int sup, vbe_sup_info_t *t)
{
	int	i, j, c;
	char	temp[10];

	LogPrintf ("\n%s: Supplemental VBE info block for function 0x%x:\n",
		Gr->name, sup);

	if (memcmp ("VBE/", t->s.SupVbeSignature, 4)) {
		LogPrintf ("%s: Invalid info block\n",
			Gr->name);
		return;
	}

	for (i = j = 0; i < rangeof (t->s.SupVbeSignature); ++i) {
		c = t->s.SupVbeSignature[i];
		temp[i] = isprint (c) ? c : ' ';
	}
	temp[i] = '\0';
	LogPrintf ("%s: %-30s\"%s\" version 0x%.4x\n",
		Gr->name, "Signature", temp,
		(int)V_Us(t->s.SupVbeVersion));

	LogPrintf ("%s: %-30s",
		Gr->name, "supported sub-functions (hex)");
	for (i = 0; i < 8; ++i) {
		c = (int)t->s.SupVbeSubFunc[i];
		for (j = 0; j < 8; ++j, c >>= 1) {
			if (c & 1)
				LogPrintf ("%02x ", i*8+j);
		}
	}
	LogPrintf ("\n");

	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "OemVendorName",
		(int)V_Us(t->s.OemVendorNamePtrSeg),
		(int)V_Us(t->s.OemVendorNamePtrOff),
		VBE_get_string ((int)V_Us(t->s.OemVendorNamePtrSeg),
					(int)V_Us(t->s.OemVendorNamePtrOff)));
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "OemProductName",
		(int)V_Us(t->s.OemProductNamePtrSeg),
		(int)V_Us(t->s.OemProductNamePtrOff),
		VBE_get_string ((int)V_Us(t->s.OemProductNamePtrSeg),
					(int)V_Us(t->s.OemProductNamePtrOff)));
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "OemProductRev",
		(int)V_Us(t->s.OemProductRevPtrSeg),
		(int)V_Us(t->s.OemProductRevPtrOff),
		VBE_get_string ((int)V_Us(t->s.OemProductRevPtrSeg),
					(int)V_Us(t->s.OemProductRevPtrOff)));
	LogPrintf ("%s: %-30s%.4x:%.4x \"%s\"\n",
		Gr->name, "OemString",
		(int)V_Us(t->s.OemStringPtrSeg),
		(int)V_Us(t->s.OemStringPtrOff),
		VBE_get_string ((int)V_Us(t->s.OemStringPtrSeg),
					(int)V_Us(t->s.OemStringPtrOff)));
}

