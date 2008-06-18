/* --------------------------------- stick.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the joy-stick as a pointing device.
 * Amiga joy-stick routines by Michael Taylor
*/

#include "fly.h"


#define PO		p->opt
#define FA1D		PO[IFA1D]
#define FA1F		PO[IFA1F]
#define FA2D		PO[IFA2D]
#define FA2F		PO[IFA2F]

/* the following is the header for the joystick routines
 *	
 * JOY.asm
 *
 * subroutine for checking Amiga's gameports
 * this source for a68k
 *
 * by Oliver Wagner, Landsberge 5, 4322 Sprockhövel, West Germany
 *
 * This is Public Domain, Enjoy!
 *
*/

#define JOY_LEFT	1
#define JOY_RIGHT	2
#define JOY_UP		4
#define JOY_DOWN	8
#define JOY_FIRE	16

extern short	joy0 (), joy1 ();

/* Calibrate joy-stick. Paddle must be at center!
*/
static int FAR
cal (POINTER *p)
{
	p->c[FA1F] = p->c[FA2F] = 0;
	p->a[FA1F] = p->a[FA2F] = 0;
	p->l[FA1F] = p->l[FA2F] = 0;

	return (0);  /* this function always works */
}

static int
init (POINTER *p, char *options)
{
	return (cal (p));
}

/* Read joy-stick. Values are adjusted to 0...200.
*/
static int FAR
read (POINTER *p)
{
	int		px, py;
	unsigned int	x, y;
	short		codeval;
	char		btn[1];

	codeval = joy1 ();
	
	btn[0] = (codeval&JOY_FIRE && JOY_FIRE);	/* left button */

	x = -1*((codeval&JOY_LEFT) && JOY_LEFT) +
		1*((codeval&JOY_RIGHT) && JOY_RIGHT);	/* range is -1 to 1 */
		 
	y = -1*((codeval&JOY_DOWN) && JOY_DOWN) +
		1*((codeval&JOY_UP) && JOY_UP);		/* range is -1 to 1 */

calcpos:
	px = FA1F;
	py = FA2F;

#define	REF	100		/* expected full range */
#define	EDGE	10		/* movement increment  */

	p->a[px] += x * EDGE * FA1D;
	if (p->a[px] > REF)
		p->a[px] = REF;
	else if (p->a[px] < -REF)
		p->a[px] = -REF;
	p->a[py] += y * EDGE * FA2D;
	if (p->a[py] > REF)
		p->a[py] = REF;
	else if (p->a[py] < -REF)
		p->a[py] = -REF;

	do_btns (p, btn, rangeof (btn));

	return (0);
}

static void FAR
term (POINTER *p)
{}

static int FAR
center (POINTER *p)
{
	p->a[FA1F] = p->a[FA2F] = 0;
	p->l[FA1F] = p->l[FA2F] = 0;

	return (0);
}

struct PtrDriver PtrAstick = {
	"ASTICK",
	0,
	cal,			/* init */
	term,
	cal,
	center,
	read,
	std_key
};

struct PtrDriver PtrBstick = {
	"BSTICK",
	0,
	cal,			/* init */
	term,
	cal,
	center,
	read,
	std_key
};

#undef PO
#undef FA1D
#undef FA1F
#undef FA2D
#undef FA2F
