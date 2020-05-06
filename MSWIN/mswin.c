/* --------------------------------- mswin.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general system-specific stuff for Microsoft WINDOWS.
 * Windows support by Michael Taylor miket@pcug.org.au
*/

#include <windows.h>
#include <SDL.h>

#include "fly.h"

#if SYS_MSWIN_16
#include <toolhelp.h>
#endif

#undef DEBUG_MSWIN

/* define USETOOLHELP if MS ever fox WinG to work with it! 
*/

HANDLE		Fly8Instance;
char		FAR Fly8AppName[10]  = "Fly8";
char		FAR Fly8Message[15]  = "Fly8";

static FARPROC	procInst;
static char	_Gbuf[1024];

#if USETOOLHELP
WORD __export _cdecl
FaultHandler(WORD wES, WORD wDS, WORD wDI, WORD wSI, 
        WORD wBP, WORD wSP, WORD wBX, WORD wDX, WORD wCX, WORD wOldAX, 
        WORD wOldBP, WORD wRetIP, WORD wRetCS, WORD wRealAX,
        WORD wNumber, WORD wHandle, WORD wIP, WORD wCS, WORD wFlags)
{
	static WORD wReentry;

/* See if we're already here.  If so, tell routine to chain on
*/
	if (wReentry)
		return 2;
	wReentry = 1;

/* If this was a CtlAltSysRq interrupt, just restart the instr.
*/
	if (wNumber == INT_CTLALTSYSRQ) {
		wsprintf(_Gbuf, "CtlAltSysRq at %04X:%04X\r\n", wCS, wIP);
		LogPrintf (_Gbuf);
		wReentry = 0;
		return 1;
	}
	if (wNumber == INT_DIV0) {
		wsprintf (_Gbuf, "integer divide by zero at %04X:%04X\n", 
						wCS, wIP);
		LogPrintf (_Gbuf);
	} else {
		wsprintf (_Gbuf, "Interrupt %d at %04X:%04X\n", 
						wNumber, wCS, wIP);
		LogPrintf (_Gbuf);
	}
	LogPrintf ("Fly8 terminating...\n");

/* We're getting out now, so undo reentry flag
*/
	wReentry = 0;
#if 0
	TerminateApp (NULL, NO_UAE_BOX);
#endif
	die ();	 /* do as it says ... exits gracefully */
	return (0);
}
#endif

static int FAR
MswInit (char * options)
{
#if USETOOLHELP
	procInst = MakeProcInstance ((FARPROC)FaultHandler, Fly8Instance);
	if (!InterruptRegister (NULL, procInst))
		MessageBox((HWND)NULL, (LPCSTR)"Unable to set div by zero trap",
				       (LPCSTR)"Fly8 Message", MB_OK);
	SetErrorMode (SEM_FAILCRITICALERRORS);
#endif

	return (0);
}

static void FAR
MswTerm (void)
{
#if USETOOLHELP
	InterruptUnRegister (NULL);
	FreeProcInstance (procInst);
#endif
}

static Ulong FAR
MswDisable (void)
{return (0);}

static void FAR
MswEnable (Ulong flags)
{flags=flags;}

/* Build file name from parts.
 * path is NULL for "current directory".
 * path is ""   for "root directory".
*/
static void FAR
MswBuildFileName (char *FullName, char *path, char *name, char *ext)
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
	"WINDOWS",
	0,
	NULL,	/* extra */
	MswInit,
	MswTerm,
	0,	/* Poll */
	MswDisable,
	MswEnable,
	0, 	/* Shell */
	MswBuildFileName
};

extern long FAR PASCAL Fly8WndProc (HWND, Uint, WORD, LONG);
extern int C_MAIN (int argc, char *argv[]);

static char	badmsg[1024];

static void
MswBaderr (char *msg)
{
	FILE	*errs;

	errs = fopen ("c:\\temp\\fly8w.err", ATMODE);
	if (errs) {
		fprintf (errs, msg);
		fclose (errs);
	}
}

/* Procedure called when the application is loaded for the first time
*/
static BOOL
WinInit ( HANDLE hInstance, LPSTR  lpszAppName, LPSTR  lpszMessage)
{
	WNDCLASS   pFly8Class;

#if 0
/* Check pointer values
*/
	if (lpszAppName[0] != '\0'  || lpszMessage != '\0')
		return (FALSE);

/* Load strings from resource
*/
	LoadString ( hInstance, IDSTR_NAME, (LPSTR)lpszAppName, 10 );
	LoadString ( hInstance, IDSTR_TITLE, (LPSTR)lpszMessage, 15 );
#endif
/* define aspects common to both classes
*/
	memset (&pFly8Class, 0, sizeof (pFly8Class));

	pFly8Class.style	 = CS_OWNDC | CS_BYTEALIGNWINDOW;
	pFly8Class.hCursor	 = LoadCursor ((HINSTANCE)NULL, IDC_ICON);
	pFly8Class.hIcon	 = (HICON)NULL;
	pFly8Class.lpszMenuName	 = (LPSTR)NULL;
	                
	pFly8Class.hInstance	 = hInstance;
	
	pFly8Class.lpszClassName = (LPSTR)Fly8AppName;
	pFly8Class.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	pFly8Class.lpfnWndProc	 = (WNDPROC)Fly8WndProc;

	if (!RegisterClass (&pFly8Class)) {
		MswBaderr ("Fly8: RegisterClass failed\n");

/* Initialization failed.
 * Windows will automatically deallocate all allocated memory.
*/
		return FALSE;
	}

	return TRUE;	/* Initialization succeeded */
}


/* Procedure called when the application is unloaded for the last time
*/
static BOOL
WinTerm (HANDLE hInstance, LPSTR lpszAppName)
{
	if (!UnregisterClass ((LPCSTR)lpszAppName, hInstance)) {
		MswBaderr ("Fly8: unregisterClass failed\n");
		return FALSE;
	}
	return TRUE;
}


static char	args[1024], *argv[30];

int PASCAL
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine,
	int nCmdShow)
{
	MSG    	msg;
	HMENU  	hMenu  = NULL;
	char	*p;
	int	i;

	SDL_Init(SDL_INIT_VIDEO);

#ifdef DEBUG_MSWIN
MswBaderr ("Fly8 starting\n");
sprintf (badmsg, "Fly8 %shInstance\n", hInstance ? "" : "!"); MswBaderr (badmsg);
sprintf (badmsg, "Fly8 %shPrevInstance\n", hPrevInstance ? "" : "!"); MswBaderr (badmsg);
sprintf (badmsg, "Fly8 lpszCmdLine=\"%s\"\n", lpszCmdLine); MswBaderr (badmsg);
sprintf (badmsg, "Fly8 nCmdShow=%d\n", nCmdShow); MswBaderr (badmsg);
#endif
	if (!hPrevInstance) {

/* Call initialization procedure if this is the first instance
*/
		if (!WinInit (hInstance, (LPSTR)Fly8AppName,
						(LPSTR)Fly8Message)) {
			MswBaderr ("Fly8: WinInit failed\n");
			return FALSE;
		}
	} else {

/* Copy data from previous instance
*/
#if SYS_MSWIN_16
		GetInstanceData (hPrevInstance, (BYTE *)Fly8AppName, 10);
		GetInstanceData (hPrevInstance, (BYTE *)Fly8Message, 15);
#endif
	}
	
	Fly8Instance = hInstance;

/* prepare command line arguments for C main()
*/

/* can we handle this?
*/
	if (strlen (lpszCmdLine) > sizeof (args) - 1) {
		MswBaderr ("Fly8: args longer than 1024\n");
		return FALSE;
	}

/* get our own copy, to be carved into separae args
*/
	strcpy (args, lpszCmdLine);

/* scan 'args' and tokenize of whitespace
*/
#if SYS_GNUWIN_B18

	for (p = args, i = 0; i < rangeof (argv);) {
#else
	argv[0] = Fly8AppName;
	for (p = args, i = 1; i < rangeof (argv);) {
#endif

/* find arg start
*/
		while (isspace (*p))
			++p;
		if (!*p)
			break;		/* no more argv */
		argv[i++] = p++;	/* set argv start */

/* find arg end
*/
		while (*p && !isspace (*p))
			++p;
		if (!*p)
			break;		/* last argv already trimmed */
		*p++ = '\0';		/* trim argv tail */
	}

	argv[i] = NULL;

	C_MAIN (i, argv);	/* pass control to Fly8 */	

	if (!hPrevInstance) {
		if (!WinTerm (hInstance, (LPSTR)Fly8AppName)) {
			MswBaderr ("Fly8: WinTerm failed\n");
			return FALSE;
		}
	}

#ifdef DEBUG_MSWIN
MswBaderr ("Fly8 ended\n");
#endif
	return ((int)msg.wParam);
}
