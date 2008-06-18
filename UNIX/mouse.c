/* --------------------------------- mouse.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the mouse as a pointing device for UNIX/X11 & mswin.
*/

#include "fly.h"
#include "mouse.h"


#define PO		p->opt
#define FA1D		PO[IFA1D]
#define FA1F		PO[IFA1F]
#define FA2D		PO[IFA2D]
#define FA2F		PO[IFA2F]

#define FSPEEDX		PO[IPFREE+0]
#define FSPEEDY		PO[IPFREE+1]

static int FAR
Mcal (POINTER *p)
{return (0);}

static int FAR
Minit (POINTER *p, char *options)
{
	long	l;

	if (get_narg (options, "smx=", &l) || l <= 0)
		FSPEEDX = 10;
	else
		FSPEEDX = (int)l;

	if (get_narg (options, "smy=", &l) || l <= 0)
		FSPEEDY = 10;
	else
		FSPEEDY = (int)l;

	return (0);
}

static void FAR
Mterm (POINTER *p)
{}

static int FAR
Mread (POINTER *p)
{
	int	reading, win_x, win_y, sizex, sizey, nbtn;
	char	btn[NBTNS];

	if (!Gr)
		return (-1);

	if (T(reading = GetMouse (&win_x, &win_y, btn, &nbtn)))
		return (reading);

	win_x -= fmul (CW->orgx, CS->sizex) + CS->left;
	win_y -= fmul (CW->orgy, CS->sizey) + CS->top;

	sizex = fmul (CW->maxx, CS->sizex);
	sizey = fmul (CW->maxy, CS->sizey);

/* y
*/	reading = muldiv (win_y, 100, sizey);
	reading = muldiv (reading*FA2D, 10, FSPEEDY);
	if (reading > 100)
		reading = 100;
	else if (reading < -100)
		reading = -100;
	p->a[FA2F] = (short)reading;

/* x
*/
	reading = muldiv (win_x, 100, sizex);
	reading = muldiv (reading*-FA1D, 10, FSPEEDX);
	if (reading > 100)
		reading = 100;
	else if (reading < -100)
		reading = -100;
	p->a[FA1F] = (short)reading;

	do_btns (p, btn, nbtn);

	return (0);
}

struct PtrDriver NEAR PtrMouse = {
	"MOUSE",
	0,
	NULL,	/* extra */
	Minit,
	Mterm,
	Mcal,
	Mcal,			/* center */
	Mread,
	std_key
};

#undef PO
#undef FA1D
#undef FA1F
#undef FA2D
#undef FA2F
#undef FSPEEDX
#undef FSPEEDY
