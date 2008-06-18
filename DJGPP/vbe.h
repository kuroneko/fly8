/* --------------------------------- vbe.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* VBE 2.0 data types and support functions.
*/

#ifndef FLY8_DJGPP_VBE_H
#define FLY8_DJGPP_VBE_H

#include "djgpp.h"

typedef union vbe_info {
	struct {
		char	VbeSignature[4];
		Ushort	VbeVersion;
		Ushort	OemStringPtrOff;
		Ushort	OemStringPtrSeg;
		Ushort	CapabilitiesLow;
		Ushort	CapabilitiesHigh;
		Ushort	VideoModesPtrOff;
		Ushort	VideoModesPtrSeg;
		Ushort	TotalMemory;
		Ushort	OemSoftwareRevision;
		Ushort	VendorNamePtrOff;
		Ushort	VendorNamePtrSeg;
		Ushort	ProductNamePtrOff;
		Ushort	ProductNamePtrSeg;
		Ushort	ProductRevisionPtrOff;
		Ushort	ProductRevisionPtrSeg;
	} s;
	Uchar		flat[512];
} vbe_info_t;

typedef union vbe_pmode_info {
	struct {
		Ushort	SetWindowFuncOff;
		Ushort	SetDisplayStartFuncOff;
		Ushort	SetPrimaryPaletteDataFuncOff;
		Ushort	PrivilegesTableOff;
	} s;
	Uchar		flat[1];
} vbe_pmode_info_t;

/* This is a copy of the above with the addresses resolved
*/
typedef struct vbe_pmode_table {
	Uchar			*SetWindowFunc;
	Uchar			*SetDisplayStartFunc;
	Uchar			*SetPrimaryPaletteDataFunc;
	Uchar			*PrivilegesTable;
	Ushort			pmode_size;
	Ushort			table_size;
	vbe_pmode_info_t	info;
} vbe_pmode_table_t;

typedef union vbe_mode_info {
	struct {
		Ushort	ModeAttributes;
		Uchar	WinAAttributes;
		Uchar	WinBAttributes;
		Ushort	WinGranularity;
		Ushort	WindowSize;
		Ushort	WinAStartSeg;
		Ushort	WinBStartSeg;
		Ushort	RealModeFarFuncPtrOff;
		Ushort	RealModeFarFuncPtrSeg;
		Ushort	BytesPerScanLine;
		Ushort	XResolution;
		Ushort	YResolution;
		Uchar	CharWidthPixels;
		Uchar	CharHeightPixels;
		Uchar	NumberOfPlanes;
		Uchar	BitsPerPixel;
		Uchar	NumberOfBanks;
		Uchar	MemoryModelType;
		Uchar	BankSizeKb;
		Uchar	NumberOfImages;
		Uchar	Reserved1[1];

		Uchar	RedMaskSize;
		Uchar	RedFieldPosition;
		Uchar	GreenMaskSize;
		Uchar	GreenFieldPosition;
		Uchar	BlueMaskSize;
		Uchar	BlueFieldPosition;
		Uchar	RsvdMaskSize;
		Uchar	RsvdFieldPosition;
		Uchar	DirectColorModeInfo;

		Ushort	PhysBasePtrLow;
		Ushort	PhysBasePtrHigh;
		Ushort	OffScreenMemoryPtrLow;
		Ushort	OffScreenMemoryPtrHigh;
		Ushort	OffScreenMemoryKb;
	} s;
	Uchar		flat[256];
} vbe_mode_info_t;

typedef union vbe_sup_info {
	struct {
		Uchar	SupVbeSignature[7];
		Uchar	SupVbeVersion[2];
		Uchar	SupVbeSubFunc[8];
		Uchar	OemVendorNamePtrOff[2];
		Uchar	OemVendorNamePtrSeg[2];
		Uchar	OemProductNamePtrOff[2];
		Uchar	OemProductNamePtrSeg[2];
		Uchar	OemProductRevPtrOff[2];
		Uchar	OemProductRevPtrSeg[2];
		Uchar	OemStringPtrOff[2];
		Uchar	OemStringPtrSeg[2];
	} s;
	Uchar		flat[256];
} vbe_sup_info_t;


/* vbe.c
*/
extern int FAR	VBE_get_info (vbe_info_t *t);
extern int FAR	VBE_get_mode_info (int mode, vbe_mode_info_t *t);
extern vbe_pmode_table_t * FAR	VBE_get_pmode_table (void);
extern int FAR	VBE_get_sup_info (int sup, vbe_sup_info_t *t);

extern void FAR	VBE_show_info (vbe_info_t *t);
extern void FAR	VBE_show_pmode_table (vbe_pmode_table_t *t);
extern void FAR	VBE_show_mode_info (int mode, vbe_mode_info_t *t);
extern void FAR	VBE_show_modes_info (vbe_info_t *t);
extern void FAR	VBE_show_sup_info (int sup, vbe_sup_info_t *t);

/* vbeasm.x
*/
extern long	vbeCallFunc5 (long bank);
extern long	vbeCallFunc7 (int pixel, int line);
extern long	vbeCallFunc9 (int color, Uchar *rgb);

#endif
