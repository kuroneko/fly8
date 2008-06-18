/* --------------------------------- console.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Amiga keyboard and mouse handler. 
 * by Michael Taylor
*/

#include "fly.h"

#include "amigainc.h"


#define PO	p->opt

typedef struct {
	short rw_code;		/* normal keycode to generate */
	short rw_scode;		/* shifted  "  */
	short rw_ccode;		/* control  "  */
	short rw_acode;		/* alt      "  */
} RKEY;

/* raw keycode scan code to Fly8 keycode translation table */

RKEY keytrans[0x60] = {

/*      CODE    NORM    SHIFT   CTRL    ALT */
/*      0x00,*/ '`',    '~',    0,      0,     
/*      0x01,*/ '1',    '!',    0,      0,     
/*      0x02,*/ '2',    '@',    0,      0,     
/*      0x03,*/ '3',    '#',    0,      0,     
/*      0x04,*/ '4',    '$',    0,      0,     
/*      0x05,*/ '5',    '%',    0,      0,     
/*      0x06,*/ '6',    '^',    0,      0,     
/*      0x07,*/ '7',    '&',    0,      0,     
/*      0x08,*/ '8',    '*',    0,      0,        
/*      0x09,*/ '9',    '(',    0,      0,     
/*      0x0a,*/ '0',    ')',    0,      0,     
/*      0x0b,*/ '-',    '_',    0,      0,     
/*      0x0c,*/ '=',    '+',    0,      0,     
/*      0x0d,*/ '\\',   '|',    0,      0,     
/*      0x0e,*/ 0,      0,      0,      0,     
/*      0x0f,*/ '0',    '0',    '0',    '0',   
/*      0x10,*/ 'q',    'Q',    'Q',    'Q',   
/*      0x11,*/ 'w',    'W',    'W',    'W',   
/*      0x12,*/ 'e',    'E',    'E',    'E',   
/*      0x13,*/ 'r',    'R',    'R',    'R',   
/*      0x14,*/ 't',    'T',    'T',    'T',   
/*      0x15,*/ 'y',    'Y',    'Y',    'Y',   
/*      0x16,*/ 'u',    'U',    'U',    'U',   
/*      0x17,*/ 'i',    'I',    'I',    'I',   
/*      0x18,*/ 'o',    'O',    'O',    'O',   
/*      0x19,*/ 'p',    'P',    'P',    'P',   
/*      0x1a,*/ '[',    '{',    0,      0,     
/*      0x1b,*/ ']',    '}',    0,      0,     
/*      0x1c,*/ 0,      0,      0,      0,     
/*      0x1d,*/ '1',    '1',    '1',    '1',   
/*      0x1e,*/ '2',    '2',    '2',    '2',   
/*      0x1f,*/ '3',    '3',    '3',    '3',   
/*      0x20,*/ 'a',    'A',    'A',    'A',   
/*      0x21,*/ 's',    'S',    'S',    'S',   
/*      0x22,*/ 'd',    'D',    'D',    'D',   
/*      0x23,*/ 'f',    'F',    'F',    'F',   
/*      0x24,*/ 'g',    'G',    'G',    'G',   
/*      0x25,*/ 'h',    'H',    'H',    'H',   
/*      0x26,*/ 'j',    'J',    'J',    'J',   
/*      0x27,*/ 'k',    'K',    'K',    'K',   
/*      0x28,*/ 'l',    'L',    'L',    'L',   
/*      0x29,*/ ';',    ':',    0,      0,     
/*      0x2a,*/ '"',    '\'',   0,      0,     
/*      0x2b,*/ 0,      0,      0,      0,     
/*      0x2c,*/ 0,      0,      0,      0,     
/*      0x2d,*/ '4',    '4',    '4',    '4',   
/*      0x2e,*/ '5',    '5',    '5',    '5',   
/*      0x2f,*/ '6',    '6',    '6',    '6',   
/*      0x30,*/ 0,      0,      0,      0,     
/*      0x31,*/ 'z',    'Z',    'Z',    'Z',   
/*      0x32,*/ 'x',    'X',    'X',    'X',   
/*      0x33,*/ 'c',    'C',    'C',    'C',   
/*      0x34,*/ 'v',    'V',    'V',    'V',   
/*      0x35,*/ 'b',    'B',    'B',    'B',   
/*      0x36,*/ 'n',    'N',    'N',    'N',   
/*      0x37,*/ 'm',    'M',    'M',    'M',   
/*      0x38,*/ ',',    '<',    0,      0,     
/*      0x39,*/ '.',    '>',    0,      0,     
/*      0x3a,*/ '/',    '?',    0,      0,     
/*      0x3b,*/ 0,      0,      0,      0,     
/*      0x3c,*/ '.',    '.',    '.',    '.',   
/*      0x3d,*/ '7',    '7',    '7',    '7',   
/*      0x3e,*/ '8',    '8',    '8',    '8',   
/*      0x3f,*/ '9',    '9',    '9',    '9',   
/*      0x40,*/ ' ',    ' ',    0,      0,     
/*      0x41,*/ 'H',    'D',    0,      0,     
/*      0x42,*/ 'I',    'I',    0,      0,     
/*      0x43,*/ K_ENTER,K_ENTER,K_ENTER,K_ENTER,
/*      0x44,*/ K_ENTER,K_ENTER,K_ENTER,K_ENTER,
/*      0x45,*/ K_ESC,  K_ESC,  K_ESC,  K_ESC,  
/*      0x46,*/ K_DEL,  K_DEL,  K_DEL,  K_DEL,  
/*      0x47,*/ 0,      0,      0,      0,     
/*      0x48,*/ 0,      0,      0,      0,     
/*      0x49,*/ 0,      0,      0,      0,     
/*      0x4a,*/ '-',    '-',    '-',    '-',   
/*      0x4b,*/ 0,      0,      0,      0,     
/*      0x4c,*/ KF_XUP, K_UP,   KF_ZUP, KF_YUP,  
/*      0x4d,*/ KF_XDOWN,K_DOWN,KF_ZDOWN,KF_YDOWN,
/*      0x4e,*/ KF_XRIGHT,K_RIGHT,KF_ZRIGHT,KF_YRIGHT,
/*      0x4f,*/ KF_XLEFT,K_LEFT,KF_ZLEFT,KF_YLEFT,
/*      0x50,*/ KF_FIRE,0, 0 , 0, 
/*      0x51,*/ K_F2,  0, 0 , 0, 
/*      0x52,*/ K_F3,  0, 0 , 0, 
/*      0x53,*/ K_F4,  0, 0 , 0, 
/*      0x54,*/ K_F5,   0, 0 , 0, 
/*      0x55,*/ K_F6,   0, 0 , 0, 
/*      0x56,*/ K_F7,   0, 0 , 0, 
/*      0x57,*/ K_F8,   0, 0 , 0, 
/*      0x58,*/ K_F9,   0, 0 , 0, 
/*      0x59,*/ K_F10,  0, 0 , 0,
/*      0x5a,*/ '(',    0,      0,      0,     
/*      0x5b,*/ ')',    0,      0,      0,     
/*      0x5c,*/ '/',    0,      0,      0,     
/*      0x5d,*/ '*',    0,      0,      0,     
/*      0x5e,*/ '+',    0,      0,      0,     
/*      0x5f,*/ KF_LEVEL,KF_ORIGIN,      0,      0
};

/* some keyboard keys current states */

static int r_shiftflag=0;	/* right shift key */
static int l_shiftflag=0;	/* left shift key */
static int r_altflag=0;		/* right alt key */
static int l_altflag=0;		/* left alt key */
static int r_amiflag=0;		/* right amiga key */
static int l_amiflag=0;		/* left amiga key */
static int ctrlflag=0;		/* control key */
static int lockflag=0;		/* shift lock key */

static struct  IntuiMessage  *msg=NULL;  /* for the Intuition message */

/* process an incoming keyboard code */

static int
dokey(code)
int code;	/* raw keycode to convert */
{
	register int ekey;	/* translate emacs key */
	register int dir;	/* key direction (up/down) */

	/* decode the direction of the key */
	dir = 1;
	if (code > 127) {
		code = code & 127;
		dir = 0;
	}

	/* process various shift keys */
	if (code >= 0x60) {
		switch (code) {

			case 0x60:	l_shiftflag = dir;	break;
			case 0x61:	r_shiftflag = dir;	break;
			case 0x62:	lockflag    = dir;	break;
			case 0x63:	ctrlflag    = dir;	break;
			case 0x64:	l_altflag   = dir;	break;
			case 0x65:	r_altflag   = dir;	break;
			case 0x66:	l_amiflag   = dir;	break;
			case 0x67:	r_amiflag   = dir;	break;

		}
		return (-2);
	}

	/* up keystrokes are ignored for the rest of these */
	if (dir == 0)
		return (-2);

	/* first apply the shift, alt  and control modifiers */
	if (ctrlflag)
		ekey = keytrans[code].rw_ccode;
	else if (l_shiftflag || r_shiftflag || lockflag)
		ekey = keytrans[code].rw_scode;
	else if (l_altflag || r_altflag)
		ekey = keytrans[code].rw_acode;
	else
		ekey = keytrans[code].rw_code;
		
	return (ekey);
}

extern struct Window *window1;

static int FAR
kinit (void)
{
        return (0);
}
        
static void FAR
kterm (void)
{
        return (0);
}

static int FAR kread (void);


static int FAR
kgetch (void)
{
	int     c;

	while ((c = kread ()) < 0);
	return (c);
}

struct SpriteImage {
	unsigned long posdata[2];
	unsigned long sprdata[4][2];
	unsigned long reserved[2];
};

#define SHIFTED (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)

static int FAR
kwait (void)
{
	int	esc;
	int     c;
	ULONG	class;               /* event class */
	ULONG	code;                /* event code */

	if (window1 == NULL)
		return (1);
nextchar:	
        Wait((1L<<window1->UserPort->mp_SigBit));
        msg = (struct IntuiMessage *)GetMsg(window1->UserPort);
        ReplyMsg((struct Message *)msg);
        
        class=msg->Class;  /* Get the event class */
        code = msg->Code;
        switch(class)      /* handle our events */
        {
             case CLOSEWINDOW:
			c = K_ESC;
			break;
             case RAWKEY:
			c = dokey(code);
			if (c < 0)	/* up keystroke or ctrl/shift/alt */
				goto nextchar;
			break;
	     default:
			c = -1;
			break;
        }
        esc = 0;
	if (K_ESC == c)
		esc = 1;
        return (esc);
}

static	ULONG	class;               /* event class */
static	ULONG	code;                /* event code */
static  int	mouse=0;	     /* flag for mouse routines */

static int FAR
kread (void)
{
	int	c;

nextchar:	
        msg = (struct IntuiMessage *)GetMsg(window1->UserPort);
        if (!msg) {
        	mouse = 0;
        	return (-1);
        }
        mouse = 1;		/* set flag so mouse routines check msg */
        class = msg->Class;  	/* Save the event class and code */
        code  = msg->Code;
        ReplyMsg((struct Message *)msg);
        switch(class)      	/* handle our events */
        {
             case CLOSEWINDOW:
		c = K_ESC;
		break;
             case RAWKEY:
		c = dokey(code);
		if (c < 0)
			goto nextchar; /* up keystroke or shift,alt,ctrl msg */
		break;
             default:
		c = -1;
		break; /* probably a mouse button */
        }
	return (c);
}

extern void FAR
beep (void)
{
#if 0
	putchar ('\a');
#endif
}

/*
 * Mouse routines.
 */

static struct SpriteImage *Fly8Sprite=NULL;

static int FAR
mcal (POINTER *p)
{
	p->a[PO[1]] = p->a[PO[3]] = 0;
}

#define BLUE   0x00f

static int FAR
minit (POINTER *p)
{
	p->flags = PF_PRESENT | PF_INITED;

	mcal (p);

	Fly8Sprite = (struct SpriteImage *)
			AllocMem (sizeof (struct SpriteImage), MEMF_CHIP);
	if (!Fly8Sprite)
		return (1);
	
	Fly8Sprite->posdata[0] = 0;
	Fly8Sprite->posdata[1] = 0;
	Fly8Sprite->sprdata[0][0] = 0;
	Fly8Sprite->sprdata[0][1] = 0x3c;
	Fly8Sprite->sprdata[1][0] = 0;
	Fly8Sprite->sprdata[1][1] = 0x3c;
	Fly8Sprite->sprdata[2][0] = 0;
	Fly8Sprite->sprdata[2][1] = 0x3c;
	Fly8Sprite->sprdata[3][0] = 0;
	Fly8Sprite->sprdata[3][1] = 0x3c;
	Fly8Sprite->reserved[0] = 0;
	Fly8Sprite->reserved[1] = 0;
	
	SetPointer (window1, (void *)Fly8Sprite, 4, 4 , -13, -2);
	
	return (0);
}

static void FAR
mterm (POINTER *p)
{
	ClearPointer (window1);
	FreeMem (Fly8Sprite, sizeof (struct SpriteImage));
	p->flags = 0;
}

static int FAR
mouse_read (POINTER *p)
{
	p->a[PO[3]] = (window1->MouseY - window1->Height / 2); /* y */
	p->a[PO[3]] *=  PO[2];

	if (p->a[PO[3]] < -100)	/* scale to -100..100 */
		p->a[PO[3]] = -100;
	else if (p->a[PO[3]] > 100)
		p->a[PO[3]] = 100;

	p->a[PO[1]] = (window1->MouseX - window1->Width / 2);	   /* x */
	p->a[PO[1]] *=  PO[0];

	if (p->a[PO[1]] < -100)	/* scale to -100..100 */
		p->a[PO[1]] = -100;
	else if (p->a[PO[1]] > 100)
		p->a[PO[1]] = 100;

	if (!mouse)
		return;
		
        switch(class)      /* handle our events */
        {
             case MOUSEBUTTONS:
    		switch(code) {
       			case SELECTDOWN:
				p->b[PO[4]] = 1;	/* left button */
		          	break;
       			case SELECTUP:
          			break;
       			case MENUDOWN:
				p->b[PO[5]] = 1;	/* right button */
          			break;
       			case MENUUP:
          			break;
    		}
		break;
             default:
		break;
        }
	mouse = 0;	/* clear flag as we have used the saved msg */
	return;
}

struct KbdDriver KbdConsole = {
	"CONSOLE",
	0,
	kinit,
	kterm,
	kread,
	kgetch,
	kwait
};

struct PtrDriver PtrMouse = {
	"MOUSE",
	0,
	minit,
	mterm,
	mcal,
	mcal,			/* center */
	mouse_read,
	std_key
};
