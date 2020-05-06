/* --------------------------------- config.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
* VMS port: Patrick Moreau (pmoreau@cena.dgac.fr, moreau_p@decus.fr)
* (this one includes IP networkin support under OpenVMS)
*/

/* System dependent configuration information: VMS
*/

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

#define PATHSEP	':'		/* path separator */

#define	FAR
#define NEAR
#define FASTCALL
#define INLINED

#define C_MAIN		main

#define NEED_STRICMP    1
#define NEED_STRNICMP   1
#define NEED_STRDUP     1

#define FLY8_NONBLOCK   O_NONBLOCK
#define NO_XSHM  1  
#define HAVE_UDP 1
