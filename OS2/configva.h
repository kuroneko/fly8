/* --------------------------------- config.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: OS/2 VAC++
 * OS/2  by Michael Taylor (miket@pcug.org.au)
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

#define	RTMODE	"r"		/* fopen file mode: read  text */
#define	RBMODE	"rb"		/* fopen file mode: read  binary */
#define	WTMODE	"w"		/* fopen file mode: write text  */
#define	WBMODE	"wb"		/* fopen file mode: write binary */
#define	ATMODE	"a"		/* fopen file mode: read/write text */
#define	ABMODE	"ab"		/* fopen file mode: read/write binary */

#define PATHSEP	';'		/* path separator */

#undef FAR
#define FAR
#undef NEAR
#define NEAR
#undef FASTCALL
#define FASTCALL
#define AFASTCALL	FASTCALL
#undef INLINED
#define INLINED

#ifndef C_MAIN
#define C_MAIN		main
#endif

#undef KF_UP

#define NOSTDERR

#define HAVE_JOYSTICK	1
#define FLY8_NONBLOCK	O_NONBLOCK

#define BPTR

#ifdef DEBUG_OS2 && !SYS_ASM
extern void OS2Baderr (char *msg);
#endif

#endif
