/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Console keyboard handler (MSDOS).
*/

#include "fly.h"

#include <dos.h>

#if SYS_DJGPP
#include <pc.h>
#if 2 == SYS_DJGPP
#include <go32.h>
#include <sys/farptr.h>
#define peekmemb(a)	_farpeekb (_dos_ds, (a))
#define pokememb(a,b)	_farpokeb (_dos_ds, (a), (b))
#else
#define membyte(a)	((Uchar *)(0xe0000000 + (a)))
#define peekmemb(a)	*membyte (a)
#define pokememb(a,b)	(*membyte (a) = (b))
#endif
#else
#include <conio.h>
#define membyte(a)	((Uchar *)((((a) & 0x0fff0L) << 12) | (a)&0x0f))
#define peekmemb(a)	*membyte (a)
#define pokememb(a,b)	(*membyte (a) = (b))
#endif


#define PUSH_SIZE	256

static int	FAR push_buf[PUSH_SIZE] = {0};
static int	push_head = 0, push_tail = 0, push_size = 0;

extern int FAR
PcPush (int c)
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
PcPop (void)
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

static Uchar	NEAR tab16[] = "qwertyuiop[]\r";
static Uchar	NEAR tab30[] = "asdfghjkl;";
static Uchar	NEAR tab43[] = "\\zxcvbnm,./";

LOCAL_FUNC int NEAR
kSpecial (int c)
{
	int k;

	switch (c) {
	default:
		k = -1;
		break;
	case 1:
		k = K_ESC + K_ALT;
		break;
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
		k = tab16[c-16] | + K_ALT;
		break;
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
		k = tab30[c-30] | + K_ALT;
		break;
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
		k = tab43[c-43] | + K_ALT;
		break;
	case 59:
	case 60:
	case 61:
	case 62:
	case 63:
	case 64:
	case 65:
	case 66:
	case 67:
	case 68:
		k = K_F1 + (c - 59);
		break;
	case 71:
		k = K_HOME;
		break;
	case 72:
		k = K_UP;
		break;
	case 73:
		k = K_PGUP;
		break;
	case 75:
		k = K_LEFT;
		break;
	case 76:
		k = K_CENTER;
		break;
	case 77:
		k = K_RIGHT;
		break;
	case 79:
		k = K_END;
		break;
	case 80:
		k = K_DOWN;
		break;
	case 81:
		k = K_PGDN;
		break;
	case 82:
		k = K_INS;
		break;
	case 83:
		k = K_DEL;
		break;
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
		k = K_F1 + (c - 84) + K_SHIFT;
		break;
	case 94:
	case 95:
	case 96:
	case 97:
	case 98:
	case 99:
	case 100:
	case 101:
	case 102:
	case 103:
		k = K_F1 + (c - 94) + K_CTRL;
		break;
	case 104:
	case 105:
	case 106:
	case 107:
	case 108:
	case 109:
	case 110:
	case 111:
	case 112:
	case 113:
		k = K_F1 + (c - 104) + K_ALT;
		break;
	case 115:
		k = K_LEFT | K_CTRL;
		break;
	case 116:
		k = K_RIGHT | K_CTRL;
		break;
	case 117:
		k = K_END | K_CTRL;
		break;
	case 118:
		k = K_PGDN | K_CTRL;
		break;
	case 119:
		k = K_HOME | K_CTRL;
		break;
	case 120:
	case 121:
	case 122:
	case 123:
	case 124:
	case 125:
	case 126:
	case 127:
	case 128:
		k = '1' + (c - 120) + K_ALT;
		break;
	case 129:
		k = '0' | K_ALT;
		break;
	case 132:
		k = K_PGUP | K_CTRL;
		break;
	case 133:
		k = K_F11;
		break;
	case 134:
		k = K_F12;
		break;
	case 135:
		k = K_F11 | K_SHIFT;
		break;
	case 136:
		k = K_F12 | K_SHIFT;
		break;
	case 137:
		k = K_F11 | K_CTRL;
		break;
	case 138:
		k = K_F12 | K_CTRL;
		break;
	case 139:
		k = K_F11 | K_ALT;
		break;
	case 140:
		k = K_F12 | K_ALT;
		break;
	case 141:
		k = K_UP | K_CTRL;
		break;
	case 143:
		k = K_CENTER | K_CTRL;
		break;
	case 145:
		k = K_DOWN | K_CTRL;
		break;
	case 146:
		k = K_INS | K_CTRL;
		break;
	case 147:
		k = K_DEL | K_CTRL;
		break;
	case 151:
		k = K_HOME | K_ALT;
		break;
	case 152:
		k = K_UP | K_ALT;
		break;
	case 153:
		k = K_PGUP | K_ALT;
		break;
	case 155:
		k = K_LEFT | K_ALT;
		break;
	case 157:
		k = K_RIGHT | K_ALT;
		break;
	case 159:
		k = K_END | K_ALT;
		break;
	case 160:
		k = K_DOWN | K_ALT;
		break;
	case 161:
		k = K_PGDN | K_ALT;
		break;
	case 162:
		k = K_INS | K_ALT;
		break;
	case 163:
		k = K_DEL | K_ALT;
		break;
	}
	return (k);
}

static Uchar	NEAR tabCTRL[] = "@abcdefghijklmnopqrstuvwxyz[\\]^_";

LOCAL_FUNC int FAR
kread (void)
{
	int	c;

	if (-1 != (c = PcPop ()))
		return (c);

	if (!kbhit ())
		return (-1);

#if SYS_DJGPP
	c = getkey ();
	if (c & 0x100)
		c = kSpecial (c & 0x0ff);
#else
	c = getch ();
	if (c == 0)
		c = kSpecial (getch ());
#endif
	else if (c < 32)
		c = tabCTRL[c]| K_CTRL;

	return (c);
}

LOCAL_FUNC int FAR
kwait (void)
{
	int	esc, c;

	while (-1 == (c = kread ()))
		sys_poll (29);
	for (esc = 0; -1 != c; c = kread ())
		if (K_ESC == c)
			esc = 1;
	return (esc);
}

LOCAL_FUNC int FAR
kgetch (void)
{
	int	c;

	while ((c = kread ()) == -1)
		sys_poll (30);
	return (c);
}

static char	kdelay = 0, krate = 0;

LOCAL_FUNC int FAR
kinit (char *options)
{
	union REGS	rg;

	rg.h.ah = 0x03;		/* get typamatic delay and rate */
	rg.h.al = 0x06;
	int86 (0x16, &rg, &rg);
	kdelay = rg.h.bh;
	krate = rg.h.bl;

	rg.h.ah = 0x03;		/* set typamatic delay and rate */
	rg.h.al = 0x05;
	rg.h.bh = 0x00;		/* delay:  [0..3] = [250..1000]ms  */
	rg.h.bl = 0x06;		/* repeat: [00-1f] = [30-2]/sec */
	int86 (0x16, &rg, &rg);

	pokememb (0x417, peekmemb (0x417) | 0x20);	/* Turn NumLock on */
	return (0);
}

LOCAL_FUNC void FAR
kterm (void)
{
	union REGS	rg;

	rg.h.ah = 0x03;		/* set typamatic delay and rate */
	rg.h.al = 0x05;
	rg.h.bh = kdelay;
	rg.h.bl = krate;
	int86 (0x16, &rg, &rg);

	pokememb (0x417, peekmemb (0x417) & ~0x20);	/* Turn NumLock off */
}

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
