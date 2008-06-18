/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Console keyboard handler: Microsoft WINDOWS
 * Windows support by Michael Taylor miket@pcug.org.au
*/

#include <windows.h>

#include "fly.h"
#include "mouse.h"

#include "common.h"


#ifdef USE_WING
extern int	usingWinG;
#endif
extern int	resetSSize;

static MSG	msg;
static int	firsttime = 1;
static int	win_x = 0, win_y = 0, button1 = 0, button2 = 0;
static int	shift = 0, ctrl = 0, alt = 0;

#ifdef USE_WING
extern void FAR ResetPalette (void);	/* in GrWinG.c */
#endif
extern void FAR MSResetPalette (void);	/* in GrMSWin.c */

#define PUSH_SIZE	256

static int	FAR push_buf[PUSH_SIZE] = {0};
static int	push_head = 0, push_tail = 0, push_size = 0;

LOCAL_FUNC int NEAR
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

LOCAL_FUNC int NEAR
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

LOCAL_FUNC int FAR
kread (void)
{
	int	key;

	if (!ghWndMain)
		return (-1);

	do {
		if (-1 != (key = MswPop ()))
			break;

		if (F(PeekMessage (&msg, ghWndMain, 0, 0, PM_REMOVE)))
			break;

		switch (msg.message) {
		case WM_KEYUP:
			switch (msg.wParam) {
			case VK_CONTROL:
				ctrl = 0;
				break;
			case VK_SHIFT:
				shift = 0;
				break;
			case VK_MENU:
				alt = 0;
				break;
			default:

/* if not a key we need let system handle it
*/
				TranslateMessage ((LPMSG)&msg);
				DispatchMessage ((LPMSG)&msg);
				break;
			}
			break;
		case WM_KEYDOWN:

/* catch special keys eg F1 - F12
*/
			switch (msg.wParam) {
			case VK_CONTROL:
				ctrl = K_CTRL;
				break;
			case VK_SHIFT:
				shift = 32;
				break;
			case VK_MENU:
				alt = K_ALT;
				break;
			case VK_ESCAPE:
				key = K_ESC;
				break;
			case VK_SPACE:
				key = ' ';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
				key = (msg.wParam - shift + 32);
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				key = msg.wParam;
				break;
			case VK_NUMPAD0:
			case VK_NUMPAD1:
			case VK_NUMPAD2:
			case VK_NUMPAD3:
			case VK_NUMPAD4:
/*			case VK_NUMPAD5:*/
			case VK_NUMPAD6:
			case VK_NUMPAD7:
			case VK_NUMPAD8:
			case VK_NUMPAD9:
				key = '0' + msg.wParam - VK_NUMPAD0;
				break;
			case VK_NUMPAD5:
				key = K_CENTER;
				break;
			case VK_DELETE:
				key = K_DEL;
				break;
			case VK_BACK:
				key = K_RUBOUT;
				break;
			case VK_TAB:
				key = K_TAB;
				break;
			case VK_RETURN:
				key = K_ENTER;
				break;
			case VK_DECIMAL:
				key = '.';
				break;
			case VK_ADD:
				key = '+';
				break;
			case VK_SUBTRACT:
				key = '-';
				break;
			case VK_DIVIDE:
				key = '/';
				break;
#ifdef VK_SEPARATOR
			case VK_SEPARATOR:
				key = '\\';
				break;
#endif
			case VK_MULTIPLY:
				key = '*';
				break;
			case VK_F1:
				key = K_F1;
				break;
			case VK_F2:
				key = K_F2;
				break;
			case VK_F3:
				key = K_F3;
				break;
			case VK_F4:
				key = K_F4;
				break;
			case VK_F5:
				key = K_F5;
				break;
			case VK_F6:
				key = K_F6;
				break;
			case VK_F7:
				key = K_F7;
				break;
			case VK_F8:
				key = K_F8;
				break;
			case VK_F9:
				key = K_F9;
				break;
			case VK_F10:
				key = K_F10;
				break;
			case VK_F11:
				key = K_F11;
				break;
			case VK_F12:
				key = K_F12;
				break;
			case VK_UP:
				key = K_UP;
				break;
			case VK_DOWN:
				key = K_DOWN;
				break;
			case VK_LEFT:
				key = K_LEFT;
				break;
			case VK_RIGHT:
				key = K_RIGHT;
				break;
			case VK_END:
				key = K_END;
				break;
			case VK_HOME:
				key = K_HOME;
				break;
			case VK_INSERT:
				key = K_INS;
				break;
#ifdef VK_LBUTTON
			case VK_LBUTTON:
				button1 = 1;
				break;
#endif
#ifdef VK_RBUTTON
			case VK_RBUTTON:
				button2 = 1;
				break;
#endif
			default:

/* if not a key we need let system handle it
*/
				TranslateMessage ((LPMSG)&msg);
				DispatchMessage ((LPMSG)&msg);
				break;
			}
			break;	/* Eyal */
		default:

/* other messages Dispatch
*/
			TranslateMessage ((LPMSG)&msg);
			DispatchMessage ((LPMSG)&msg);
			break;
		}
	} while (key < 0);

/* remove messages for the Text window
*/
	if (ghWndText) {
		if (PeekMessage (&msg, ghWndText,   0, 0, PM_REMOVE)) {
			TranslateMessage ((LPMSG)&msg);
			DispatchMessage ((LPMSG)&msg);
		}
	}

	if (key >= 0) {
		if (alt)
			key |= K_ALT;
		if (ctrl)
			key |= K_CTRL;
	}

	return (key);

}

LOCAL_FUNC int FAR
kwait (void)
{
	MSG	msg;
	int	esc, c;

	if (!ghWndMain)
		return (1);

/* Polling messages from event queue
*/
	GetMessage((LPMSG)&msg, ghWndMain, 0, 0);
	TranslateMessage ((LPMSG)&msg);
	DispatchMessage ((LPMSG)&msg);

	for (esc = 0; -1 != (c = kread ());)
		if (K_ESC == c)
			esc = 1;
	return (esc);
}

LOCAL_FUNC int FAR
kgetch (void)
{
	int	c;

	while ((c = kread ()) == -1)
		sys_poll (20);
	return (c);
}

LOCAL_FUNC int FAR
kinit (char *options)
{options = options; return (0);}

LOCAL_FUNC void FAR
kterm (void)
{}

struct KbdDriver NEAR KbdConsole = {
	"CONSOLE",
	0,
	NULL,	/* extra */
	kinit,
	kterm,
	kread,
	kgetch,
	kwait
};

extern int FAR
GetMouse (int *x, int *y, char *btn, int *nbtn)
{
	*x = win_x;
	*y = win_y;
	btn[0] = (char)button1;			/* right button */
	btn[1] = (char)button2;			/* left button */
	*nbtn = 2;

	return (0);
}

/* procedure called by WINDOWS when an event occurs in the fly8 display window
*/
long FAR PASCAL
Fly8WndProc (HWND hWnd, unsigned message, WORD wParam, LONG lParam)
{
	PAINTSTRUCT	ps;
	POINT		pt;
	RECT		rect;

	switch (message) {

	case WM_DESTROY:		/* quit the Fly8 application */
		PostQuitMessage (0);
		die ();
		break;

	case WM_PAINT:			/* paint the window */
		BeginPaint (hWnd, &ps);
		GetClientRect (hWnd, &rect);
		EndPaint (hWnd, &ps);
		break;

	case WM_SETFOCUS:		/* just got the input focus */
		if (firsttime) {
			firsttime =0;
			break;
		}
#ifdef USE_WING
		if (usingWinG)
			ResetPalette ();
		else
#endif
			MSResetPalette ();
		break;

	case WM_SIZE:			/* resize the window */
		resetSSize = 1;
		break;

	case WM_MOUSEMOVE:
		button1 = MK_LBUTTON & wParam;
		button2 = MK_RBUTTON & wParam;
		pt.x = LOWORD (lParam);
		pt.y = HIWORD (lParam);
		ScreenToClient (hWnd, &pt);
		GetWindowRect (hWnd, &rect);
		win_x = pt.x + rect.left;
		win_y = pt.y + rect.top;
		break;

	case WM_LBUTTONDOWN:
		button1 = 1;
		break;

	case WM_RBUTTONDOWN:
		button2 = 1;
		break;

	case WM_LBUTTONUP:
		button1 = 0;
		break;

	case WM_RBUTTONUP:
		button2 = 0;
		break;

	case WM_CHAR:		/* all normal alphanumeric keys */
		MswPush (wParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return(0L);
}
