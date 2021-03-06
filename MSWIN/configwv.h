/* --------------------------------- config.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: Microsoft WIN 3.1, vc
 * Microsoft Windows  by Michael Taylor (miket@pcug.org.au)
*/

#ifndef FLY8_CONFIG_H
#define FLY8_CONFIG_H

#define	INIFILE	"fly.ini"	/* name of ini file */
#define	LOGFILE	"fly.log"	/* name of log file */
#define MAC_EXT "mac"		/* binary macros file extension */
#define MAX_EXT "max"		/* text macros file extension */
#define VMD_EXT "vmd"		/* video modes file extension */
#define PRM_EXT "prm"		/* plane parameters file extension */
#define SHP_EXT "vxx"		/* object shape file extension */
#define NAV_EXT "nav"		/* nav data file extension */
#define LND_EXT "lnd"		/* landscape file extension */

#define	RTMODE	"rt"		/* fopen file mode: read  text */
#define	RBMODE	"rb"		/* fopen file mode: read  binary */
#define	WTMODE	"w"		/* fopen file mode: write text  */
#define	WBMODE	"wb"		/* fopen file mode: write binary */
#define	ATMODE	"at"		/* fopen file mode: read/write text */
#define	ABMODE	"ab"		/* fopen file mode: read/write binary */

#define PATHSEP	';'		/* path separator */

#undef FAR
#define FAR		__far
#undef NEAR
#define NEAR		__near
#undef FASTCALL
#define FASTCALL	__fastcall
#define AFASTCALL	FASTCALL
#undef INLINED
#define INLINED		__inline

#ifndef C_MAIN
#define C_MAIN		Fly8Main
#endif

#undef KF_UP

#define NOSTDERR

#define NEED_STRICMP	1
#define NEED_STRNICMP	1
#define NEED_STRDUP	1

#define HAVE_JOYSTICK	1
#define HAVE_MIDI	1
#define HAVE_WAVE	1

#define STRICT

#define BPTR		huge

#if !SYS_ASM
#define VGASET(b, o, c, n)	m_set ((short)((Ulong)(b)>>16), (o), (c), (n))
extern void	FAR m_set (short seg, int to, int value, int n);
#endif

#define SYS_MSWIN_16	1

#define SYS_WINSOCK	1

#endif
