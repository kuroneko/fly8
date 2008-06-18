/* --------------------------------- pcmouse.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the mouse as a pointing device.
*/

#include "fly.h"

#include <dos.h>


#define PO		p->opt
#define FA1D		PO[IFA1D]
#define FA1F		PO[IFA1F]
#define FA2D		PO[IFA2D]
#define FA2F		PO[IFA2F]

#define FSPEEDX		PO[IPFREE+0]
#define FSPEEDY		PO[IPFREE+1]

static int	nbuttons = 2;

LOCAL_FUNC int FAR
MikCal (POINTER *p)
{
	union REGS	rg;		/* cpu register for use of DOS calls */

	rg.x.ax = 4;			/* set mouse cursor position */
	rg.x.cx = 100 << 3;		/* middle col */
	rg.x.dx = 100 << 3;		/* middle row */
	int86(0x33, &rg, &rg);
	p->a[FA1F] = p->a[FA2F] = 0;
	p->l[FA1F] = p->l[FA2F] = 0;
	return (0);
}

LOCAL_FUNC int FAR
MikInit (POINTER *p, char *options)
{
	union REGS	rg;		/* cpu register for use of DOS calls */
	long		l;

	p->flags = 0;

#if !SYS_DJGPP
/* check if the mouse drive exists first
*/
{
	struct SREGS	segreg;		/* cpu segment registers	     */
	long		miaddr;		/* mouse interupt routine address */

	rg.x.ax = 0x3533;		/* look at the interrupt 33 address */
	int86x(0x21, &rg, &rg, &segreg);
	miaddr = (((long)segreg.es) << 16) + (long)rg.x.bx;
	if (0L == miaddr || (Uchar)0x0cf == *(Uchar FAR *)miaddr)
		return (1);
}
#endif

/* check for mouse present
*/
	rg.x.ax = 0;			/* mouse status flag */
	int86(0x33, &rg, &rg);		/* check for the mouse interupt */
	if (rg.x.ax == 0)
		return (2);
	p->flags |= PF_PRESENT;
	nbuttons = rg.x.bx;

	if (get_narg (options, "smx=", &l))
		FSPEEDX = 2;
	else
		FSPEEDX = (int)l;

	if (get_narg (options, "smy=", &l))
		FSPEEDY = 2;
	else
		FSPEEDY = (int)l;

#if 0
/* set mouse attributes
*/
	rg.x.ax = 10;			/* set text cursor */
	rg.x.bx = 0;			/* software text cursor please */
	rg.x.cx = 0x77ff;		/* screen mask */
	rg.x.dx = 0x7700;		/* cursor mask */
	int86(0x33, &rg, &rg);
#endif

/* set number of columns for mouse
*/
	rg.x.ax = 7;			/* set min/max horizontal position */
	rg.x.cx = 0;			/* start at 0 */
	rg.x.dx = 200 << 3;	/* end at the end */
	int86(0x33, &rg, &rg);

/* set number of vertical rows for mouse
*/
	rg.x.ax = 8;			/* set min/max vertical position */
	rg.x.cx = 0;			/* start at 0 */
	rg.x.dx = 200 << 3;	/* end at the end */
	int86(0x33, &rg, &rg);

/* set mouse speed
*/
	rg.x.ax = 15;
	rg.x.cx = FSPEEDX;
	rg.x.dx = FSPEEDY;
	int86(0x33, &rg, &rg);

#if 0
/* turn the mouse cursor on
*/
	rg.x.ax = 1;			/* Show Cursor */
	int86(0x33, &rg, &rg);

/* turn the mouse cursor back off
*/
	rg.x.ax = 2;			/* Hide Cursor */
	int86(0x33, &rg, &rg);
#endif
	p->flags |= PF_INITED;

/* get it in the middle of the screen
*/
	MikCal (p);

	return (0);
}

LOCAL_FUNC void FAR
MikTerm (POINTER *p)
{
	p->flags = 0;
}

LOCAL_FUNC int FAR
MikRead (POINTER *p)
{
	union REGS	rg;
	char		btn[3];
	int		reading;

	rg.x.ax = 3;		/* Get button status and mouse position */
	int86(0x33, &rg, &rg);

	reading = (rg.x.dx >> 3) - 100;		/* x */
	reading *=  FA2D;
	p->a[FA2F] = reading;

	reading = (rg.x.cx >> 3) - 100;		/* y */
	reading *=  -FA1D;
	p->a[FA1F] = reading;

	btn[0] = (char)T(rg.x.bx & 0x02);	/* right button */
	btn[1] = (char)T(rg.x.bx & 0x01);	/* left button */
	if (nbuttons > 2) {
		btn[2] = (char)T(rg.x.bx & 0x04); /* middle button ? */
		reading = 3;
	} else
		reading = 2;
	do_btns (p, btn, reading);

	return (0);
}

struct PtrDriver NEAR PtrMouse = {
	"MOUSE",
	0,
	NULL,	/* extra */
	MikInit,
	MikTerm,
	MikCal,
	MikCal,			/* center */
	MikRead,
	std_key
};
#undef PO
#undef FA1D
#undef FA1F
#undef FA2D
#undef FA2F
#undef FSPEEDX
#undef FSPEEDY
