/* --------------------------------- Pcdos.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for PcDOS.
*/

#include "fly.h"

#include <signal.h>
#include <dos.h>
#include <conio.h>

/* Pcdos/console.c */
extern int	FAR PcPush (int c);

#if FLY8_BC2
#define _dos_getvect	getvect
#define _dos_setvect	setvect
#endif

#define INT_DIV0	0x0000

#ifdef FLY8_MSC
static void (CDECL INTERRUPT FAR *old_div0)() = 0;
#else
static void INTERRUPT (FAR * old_div0)() = 0;
#endif

#if FLY8_BC2
LOCAL_FUNC void CDECL LOADDS
#else
LOCAL_FUNC void FAR CDECL LOADDS
#endif
PcSigInt (int sig)
{
	signal (SIGINT, PcSigInt);

	PcPush (K_CTRL | 'c');
}

#if FLY8_BC2
LOCAL_FUNC void CDECL LOADDS
#else
LOCAL_FUNC void FAR CDECL LOADDS
#endif
PcSigShutdown (int sig)
{
#ifdef SIGBREAK
	signal (SIGBREAK, SIG_IGN);
#endif
#ifdef SIGTERM
	signal (SIGTERM,  SIG_IGN);
#endif

	MsgEPrintf (-10, "Shutdown requested");

	st.flags1 |= SF_TERM;
}

#ifdef SIGABRT
#if FLY8_BC2
LOCAL_FUNC void CDECL LOADDS
#else
LOCAL_FUNC void FAR CDECL LOADDS
#endif
PcSigAbort (int sig)
{
	MsgEPrintf (-10, "Abort requested");
	die ();
}
#endif

#ifdef FLY8_MSC
LOCAL_FUNC void FAR CDECL INTERRUPT
#else
LOCAL_FUNC void FAR INTERRUPT
#endif
PcDiv0 (int dummy)
{
	Uint		*p;
	int		i, j;
	Uchar		FAR * FAR *q, FAR *qq;
	extern		FAR C_MAIN ();
	Uchar		FAR *m = (Uchar *)C_MAIN;
#if defined(USE_ASM) && defined(SYS_MSDOS)
	Uchar		FAR *md = (Uchar *)muldiv;
	Uchar		FAR *fd = (Uchar *)fdiv;
	Uchar		FAR *ad = (Uchar *)dithadj;
#endif

	LogPrintf ("%s: divide overflow\n", Sys->name);

	p = (Uint *)&dummy;
	LogPrintf ("\n");
	LogPrintf ("stack at  %p:", p);
	for (j = 0, i = -16; i < 128; ++i, ++j) {
		if (!(j%8))
			LogPrintf ("\n%04x ", 2*i);
		else if (!(j%4))
			LogPrintf (" ");
		LogPrintf (" %04x", p[i]);
	}
	LogPrintf ("\n");

	q = (Uchar FAR * FAR *)(p + FAULT_OFFSET);
	qq = *q;

	LogPrintf ("\n");
	LogPrintf ("main   at %p\n", m);
#if defined(USE_ASM) && defined(SYS_MSDOS)
	if (qq > md && qq <= md+16) {
		LogPrintf ("fault in muldiv(), tracing back\n");
		q = (Uchar FAR * FAR *)(p + MULDIV_OFFSET);
		qq = *q;
	} else
	if (qq > fd && qq <= fd+16) {
		LogPrintf ("fault in fdiv(), tracing back\n");
		q = (Uchar FAR * FAR *)(p + MULDIV_OFFSET);
		qq = *q;
	} else
	if (qq > ad && qq <= ad+30) {
		LogPrintf ("fault in dithadj(), tracing back\n");
		q = (Uchar FAR * FAR *)(p + MULDIV_OFFSET);
		qq = *q;
	}
#endif
	LogPrintf ("fault at  %p", qq);
	for (j = 0, i = -64; i < 64; ++i, ++j) {
		if (!(j%16))
			LogPrintf ("\n%p ", qq+i);
		else if (!(j%4))
			LogPrintf (" ");
		LogPrintf ("%02x", qq[i]);
	}
	LogPrintf ("\n");
	LogPrintf ("+++1 %p %p\n", qq, m);
	LogPrintf ("Please run the 'find' batch file\n");

	die ();
}

LOCAL_FUNC int FAR
PcInit (char *options)
{
	old_div0 = _dos_getvect (INT_DIV0);
	_dos_setvect (INT_DIV0, PcDiv0);

	signal (SIGINT,   PcSigInt);

#ifdef SIGBREAK
	signal (SIGBREAK, PcSigShutdown);
#endif
#ifdef SIGTERM
	signal (SIGTERM,  PcSigShutdown);
#endif

#ifdef SIGABRT
	signal (SIGABRT,  PcSigAbort);
#endif

	return (0);
}

LOCAL_FUNC void FAR
PcTerm (void)
{
	signal (SIGINT,   SIG_DFL);

#ifdef SIGBREAK
	signal (SIGBREAK, SIG_DFL);
#endif
#ifdef SIGTERM
	signal (SIGTERM,  SIG_DFL);
#endif

#ifdef SIGABRT
	signal (SIGABRT,  SIG_DFL);
#endif
	_dos_setvect (INT_DIV0, old_div0);
}

LOCAL_FUNC Ulong FAR
PcDisable (void)
{
	short	flags;

	_asm {
		pushf
		pop	flags
		cli
	}
	return ((Ulong)flags);
}

LOCAL_FUNC void FAR
PcEnable (Ulong flags)
{
	short	t;

	t = (short)flags;
	_asm {
		push	t
		popf
	}
}

LOCAL_FUNC void FAR
PcShell (void)
{
	system ("command");
}

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
LOCAL_FUNC void FAR
PcBuildFileName (char *FullName, char *path, char *name, char *ext)
{
	FullName[0] = '\0';

	if (path) {
		strcat (FullName, path);
		strcat (FullName, "\\");
	}
	strcat (FullName, name);
	if (ext && ext[0]) {
		strcat (FullName, ".");
		strcat (FullName, ext);
	}
}

struct SysDriver NEAR SysDriver = {
	"PCDOS",
	0,
	NULL,	/* extra */
	PcInit,
	PcTerm,
	0,	/* poll */
	PcDisable,
	PcEnable,
	PcShell,
	PcBuildFileName
};
