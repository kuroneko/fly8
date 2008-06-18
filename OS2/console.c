/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Console keyboard handler: OS/2
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>

#include "fly.h"
#include "mouse.h"
#include "common.h"


extern HAB	hab;		/* anchor-block handle  */
extern HMQ	hmq;		/* Message queue handle */
extern HWND	hwndClient;	/* Client window handle */
extern HWND 	ghWndMain;
extern int	usingDive;
extern int	resetSSize;

extern void	DiveResetPalette (void);	/* in Grbitmap.c */
extern void	GPIResetPalette (void);		/* in GrOS2.c */

static QMSG	qmsg;		/* message              */
static int	firsttime = 1;
static int	win_x = 0, win_y = 0, button1 = 0, button2 = 0;
#define PUSH_SIZE	256
static int	push_buf[PUSH_SIZE] = {0};
static int	push_head = 0, push_tail = 0, push_size = 0;


LOCAL_FUNC int
MswPush (int c)
{
	if (push_size == PUSH_SIZE)
		return (1);
	push_buf[push_tail++] = c;
	if (push_tail == PUSH_SIZE)
		push_tail = 0;
	++push_size;
	return (0);
}

LOCAL_FUNC int
MswPop (void)
{
	int	c;

	if (!push_size)
		return (-1);
	c = push_buf[push_head++];
	if (push_head == PUSH_SIZE)
		push_head = 0;
	--push_size;
	return (c);
}

LOCAL_FUNC int
kread (void)
{
#if 1
	if (!hab)
		return (-1);

	if (TRUE == WinPeekMsg (hab, &qmsg, (HWND)0, 0, 0, PM_REMOVE)) {
		if (qmsg.msg == WM_QUIT)
			die ();
		else
			WinDispatchMsg (hab, &qmsg);
	}
#endif
	return MswPop ();
}

LOCAL_FUNC int
kwait (void)
{
	int	esc, c;

	if (!hab)
		return (1);

	for (esc = 0; -1 != (c = kread ());) {
		if (K_ESC == c)
			esc = 1;
	}
	return (esc);
}

LOCAL_FUNC int
kgetch (void)
{
	int	c;

	while ((c = kread ()) == -1)
		sys_poll (20);
	return (c);
}

#if	0
static TID	tidThread;		/* Thread ID for Message processing */
static ULONG	dummy;
static int	endThread = 0;

LOCAL_FUNC VOID APIENTRY
processmsgs (ULONG parm1)
{
	HMQ	hmq;			/* Message queue handle */

/* create a message queue.
*/
	hmq = WinCreateMsgQueue (hab, 0);

	while (!hab && !endThread)
		{}

/* While there are still messages, dispatch them.
*/
	while (WinGetMsg (hab, &qmsg, 0, 0, 0) && !endThread)
		WinDispatchMsg (hab, &qmsg);

	WinDestroyMsgQueue (hmq);
}
#endif

LOCAL_FUNC int
kinit (char *options)
{
	options = options;
#if	0
	if (DosCreateThread (&tidThread, (PFNTHREAD) processmsgs,
			dummy, CREATE_READY, 8192L)) {
		LogPrintf ("console - init failed to create message ");
		LogPrintf ("processing thread - terminating\n");
		return (1);
	}

/* Set the proiroty of the message processing thread
*/
	DosSetPriority (PRTYS_THREAD, PRTYC_IDLETIME, 0, tidThread);
#endif
	if (FALSE != WinSetAccelTable (hab, 0, ghWndMain))
		LogPrintf ("console - failed to clear accel table\n");

	LogPrintf ("console - init: ok\n");
	return (0);
}

LOCAL_FUNC void
kterm (void)
{
#if 0
        endThread = 1;
   	DosWaitThread (&tidThread, DCWW_WAIT);
#endif
	LogPrintf ("console - term: ok\n");
}

struct KbdDriver  KbdConsole = {
	"CONSOLE",
	0,
	NULL, /* extra */
	kinit,
	kterm,
	kread,
	kgetch,
	kwait
};

extern int
GetMouse (int *x, int *y, char *btn, int *nbtn)
{
	*x = win_x;
	*y = win_y;
	btn[0] = (char)button1;       /* right button */
	btn[1] = (char)button2;       /* left button */
	*nbtn = 2;
	return (0);
}

/* procedure called by WINDOWS when an event occurs in the fly8 display window
*/
MRESULT EXPENTRY 
Fly8WndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	POINTL		pt;
	SWP		swp;
	static int	alt = 0, ctrl = 0, shift = 0;

	switch (msg) {
	case WM_CLOSE:     	/* quit the Fly8 application */
	case WM_DESTROY:
	case WM_SAVEAPPLICATION:
		WinPostQueueMsg (hmq, WM_QUIT, 0 ,0);
		break;

	case WM_QUIT:     	/* really quit the Fly8 application */
		die ();
		break;

	case WM_PAINT:       	/* paint the window */
		break;

	case WM_SETFOCUS:    	/* just got the input focus */
		if (firsttime) {
			firsttime =0;
			break;
		}
		if (usingDive)
			DiveResetPalette ();
		else
			GPIResetPalette ();
		break;

	case WM_MOUSEMOVE:
		button1 = WinGetKeyState (HWND_DESKTOP, VK_BUTTON1) & 0x8000;
		button2 = WinGetKeyState (HWND_DESKTOP, VK_BUTTON2) & 0x8000;
		pt.x = SHORT1FROMMP (mp1);
		pt.y = SHORT2FROMMP (mp1);

/* Find the window position and size, relative to parent.
*/
		WinQueryWindowPos (hwndClient, &swp );
		win_x = pt.x - swp.x;
		win_y = pt.y - swp.y;
		break;

	case WM_BUTTON1DOWN:
		button1 = 1;
		break;

	case WM_BUTTON2DOWN:
		button2 = 1;
		break;

	case WM_BUTTON1UP:
		button1 = 0;
		break;

	case WM_BUTTON2UP:
		button2 = 0;
		break;

	case WM_CHAR:     /* all normal alphanumeric keys and virtual keys */
		if (SHORT1FROMMP (mp1) & KC_KEYUP) {
			alt = ctrl = shift = 0;
			break;
		}
#if 0
LogPrintf ("---------------------------------------------\n");
LogPrintf ("console - s1mp1=%d\n", SHORT1FROMMP(mp1));
LogPrintf ("console - s2mp1=%d\n", SHORT2FROMMP(mp1));
LogPrintf ("console - s1mp2=%d\n", SHORT1FROMMP(mp2));
LogPrintf ("console - s2mp2=%d\n", SHORT2FROMMP(mp2));
LogPrintf ("console - KC_CHAR=%d\n", KC_CHAR);
LogPrintf ("console - KC_ALT=%d\n", KC_ALT);
LogPrintf ("console - KC_CTRL=%d\n", KC_CTRL);
LogPrintf ("console - KC_SHIFT=%d\n", KC_SHIFT);
LogPrintf ("console - KC_VIRTUALKEY=%d\n", KC_VIRTUALKEY);
LogPrintf ("console - c1mp1=%d\n", CHAR1FROMMP (mp1));
LogPrintf ("console - c2mp1=%d\n", CHAR2FROMMP (mp1));
LogPrintf ("console - c1mp2=%d\n", CHAR1FROMMP (mp2));
LogPrintf ("console - c2mp2=%d\n", CHAR2FROMMP (mp2));
LogPrintf ("---------------------------------------------\n");
#endif
		if (SHORT1FROMMP(mp1) & KC_CHAR) {
			switch (CHAR1FROMMP (mp2)) {
			case 13:
				MswPush (K_ENTER);
				break;
			case 7:
				MswPush (K_BELL);
				break;
			case 8:
				MswPush (K_RUBOUT);
				break;
			case 9:
				MswPush (K_TAB);
				break;
			case 10:
				MswPush (K_NL);
				break;
			case 11:
				MswPush (K_VTAB);
				break;
			case 12:
				MswPush (K_FF);
				break;
			default:
				MswPush (CHAR1FROMMP (mp2) | (alt|ctrl));
				break;
			}
			alt = ctrl = shift = 0;
			break;
		}

		if (SHORT1FROMMP(mp1) & KC_ALT) {
			alt = K_ALT;
/*			MswPush (CHAR1FROMMP (mp2) | K_ALT);*/
			break;
		}

		if (SHORT1FROMMP(mp1) & KC_CTRL) {
			ctrl = K_CTRL;
/*			MswPush (CHAR1FROMMP (mp2) | K_CTRL);*/
			break;
		}

		if (SHORT1FROMMP(mp1) & KC_SHIFT) {
			shift = K_SHIFT;
#if 0
			if (CHAR1FROMMP (mp2) < '0')
				MswPush (CHAR1FROMMP (mp2));
			else
				MswPush (CHAR1FROMMP (mp2) - 32);
#endif
			break;
		}

		if (!(SHORT1FROMMP(mp1) & KC_VIRTUALKEY))
			break;

#define PUSHRAW(c) \
	(MswPush (c), alt = ctrl = shift = 0)
#define PUSHIFT(c) \
	(MswPush ((c)|alt|ctrl|shift), alt = ctrl = shift = 0)

		switch (SHORT2FROMMP(mp2)) {
        	case VK_ESC:				/* SM$:M1 */
      			PUSHRAW (K_ESC);
			break;
		case VK_SPACE:
       			PUSHRAW (' ');
       			break;
		case VK_DELETE:
                        PUSHRAW (K_DEL);
                        break;
		case VK_BACKSPACE:			/* SM$:M1 */
			PUSHRAW (K_RUBOUT);
              		break;
		case VK_TAB:
               		PUSHRAW (K_TAB);
             		break;
		case VK_ENTER:				/* SM$:M1 */
			PUSHRAW (K_ENTER);
	      		break;
		case VK_NEWLINE:			/* SM$:M1 */
	      		MswPush (K_ENTER);
	      		break;
		case VK_F1:
			PUSHIFT (K_F1);
			break;
		case VK_F2:
			PUSHIFT (K_F2);
		   	break;
		case VK_F3:
			PUSHIFT (K_F3);
			break;
		case VK_F4:
			PUSHIFT (K_F4);
			break;
		case VK_F5:
			PUSHIFT (K_F5);
		   	break;
		case VK_F6:
			PUSHIFT (K_F6);
			break;
		case VK_F7:
	     		PUSHIFT (K_F7);
	      		break;
		case VK_F8:
			PUSHIFT (K_F8);
			break;
		case VK_F9:
			PUSHIFT (K_F9);
			break;
		case VK_F10:
			PUSHIFT (K_F1);
			break;
		case VK_F11:
			PUSHIFT (K_F11);
		   	break;
		case VK_F12:
			PUSHIFT (K_F12);
	   		break;
		case VK_UP:
			PUSHIFT (K_UP);
		   	break;
		case VK_DOWN:
			PUSHIFT (K_DOWN);
			break;
		case VK_LEFT:
			PUSHIFT (K_LEFT);
		   	break;
		case VK_RIGHT:
			PUSHIFT (K_RIGHT);
		   	break;
		case VK_END:
			PUSHIFT (K_END);
		   	break;
		case VK_HOME:
			PUSHIFT (K_HOME);
		   	break;
		case VK_INSERT:
			PUSHIFT (K_INS);
		   	break;
		case VK_BUTTON1:
			button1 = 1;
		   	break;
		case VK_BUTTON2:
			button2 = 1;
		   	break;
		default:	/* let system have this keypress */
			return (WinDefWindowProc(hwnd, msg, mp1, mp2)); /* SM$:C */
		}

	case WM_REALIZEPALETTE:
		if (usingDive)
			DiveResetPalette ();
		else
			GPIResetPalette ();
		break;

	case WM_SIZE:		/* resize or move the window */
	case WM_MOVE:
		resetSSize = 1;
		break;

	case WM_VRNDISABLED:
		resetSSize = -1;
		break;

	case WM_VRNENABLED:
		resetSSize = 1;
		break;

	default:
		return (WinDefWindowProc (hwnd, msg, mp1, mp2)); /* SM$:C */
	}

	return (FALSE);
}
