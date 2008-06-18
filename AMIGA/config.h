/* --------------------------------- config.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: AmigaDOS
*/

#if !SYS_ASM
#include <string.h>
#endif

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
#define	RBMODE	"r"		/* fopen file mode: read  binary */
#define	WTMODE	"w"		/* fopen file mode: write text  */
#define	WBMODE	"w"		/* fopen file mode: write binary */
#define	ATMODE	"a"		/* fopen file mode: read/write text */
#define	ABMODE	"a"		/* fopen file mode: read/write binary */

#define PATHSEP	';'		/* path separator */

#define	FAR
#define NEAR
#define FASTCALL

#define C_MAIN	main

#define INLINED
