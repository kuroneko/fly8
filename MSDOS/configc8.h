/* --------------------------------- config.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: MSDOS, ms vc1 (c8)
*/

#ifndef FLY8_CONFIG_H
#define FLY8_CONFIG_H

#if !SYS_ASM
#include <string.h>
#endif

#define FLY8_MSC	1

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
#define	WTMODE	"wt"		/* fopen file mode: write text  */
#define	WBMODE	"wb"		/* fopen file mode: write binary */
#define	ATMODE	"at"		/* fopen file mode: read/write text */
#define	ABMODE	"ab"		/* fopen file mode: read/write binary */

#define PATHSEP	';'		/* path separator */

#define	FAR		__far
#define NEAR		__near
#define FASTCALL	__fastcall
#define AFASTCALL	FASTCALL
#define CDECL		__cdecl
#define LOADDS		__loadds
#define INTERRUPT	__interrupt
#define INLINED		__inline

#define C_MAIN		main

#define FAULT_OFFSET	10		/* word count! */
#define MULDIV_OFFSET	(FAULT_OFFSET+3)

#define HAVE_JOYSTICK	1
#define HAVE_MIDI	1

#if !SYS_ASM
#define VGASET(b, o, c, n)	m_set ((short)((Ulong)(b)>>16), (o), (c), (n))
extern void	FAR m_set (short seg, int to, int value, int n);
#endif

#define POLL_RATE	0x080

#endif
