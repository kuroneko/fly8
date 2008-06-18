/* --------------------------------- s3.c ----------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* S3 accelerated line drawing (testing).
 *
 * NOTES: THIS DOES NOT WORK YET!!!!!!
*/

#include "fly.h"
#include "regs3.h"

#include <conio.h>

#define outb(p,n)		outp(p,n)
#define outw(p,n)		outpw(p,n)
#define inb(p)			inp(p)
#define inw(p)			inpw(p)

#define WaitQueue16_32(n16,n32)	WaitQueue(n16)
#define outw32(p,n)		outw(p,n)


extern Ulong	FAR GrStats[2048];
extern Ulong	FAR _BankSwitches;

static Uint	x1 = 0, y1 = 0, FirstTime = 1;

/* This segment is for testing the S3 accelerated line drawing.
 *
 * This code is lifted wholesale from XFree86-3.1.
*/

static void NEAR
s3Unlock (void)
{
   unsigned char tmp;

   outb(vgaCRIndex, 0x38);
   outb(vgaCRReg, 0x48);

   outb(vgaCRIndex, 0x31);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp | 0x09);

   outb(vgaCRIndex, 0x39);
   outb(vgaCRReg, 0xa5);

   outb(vgaCRIndex, 0x35);		/* select segment 0 */
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp & 0xf0);

   outb(vgaCRIndex, 0x51);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, (tmp & 0xf3));

   outb(vgaCRIndex, 0x11);		/* allow writting? */
   outb(vgaCRReg, 0x00);
}

static void NEAR
s3InitEnvironment (void)
{
LogPrintf ("s3InitEnvironment> start (GO_STAT 0x%02x)\n", inb(GP_STAT));
 /* Current mixes, src, foreground active */

   WaitQueue(6);
LogPrintf ("s3InitEnvironment> WaitQueue(6) done \n");
   outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);

 /* Clipping rectangle to full drawable space */
   outw(MULTIFUNC_CNTL, SCISSORS_T | 0x000);
   outw(MULTIFUNC_CNTL, SCISSORS_L | 0x000);
   outw(MULTIFUNC_CNTL, SCISSORS_R | (1024-1));
   outw(MULTIFUNC_CNTL, SCISSORS_B | ( 768-1));

 /* Enable writes to all planes and reset color compare */
   WaitQueue16_32(2,3);
LogPrintf ("s3InitEnvironment> WaitQueue16_32(2,3) done \n");
   outw32(WRT_MASK, ~0);
   outw(MULTIFUNC_CNTL, PIX_CNTL | 0x0000);

 /*
  * Clear the display.  Need to set the color, origin, and size. Then draw.
  */
   WaitQueue16_32(6,7);
LogPrintf ("s3InitEnvironment> WaitQueue16_32(6,7) done \n");
   outw32(FRGD_COLOR, 1);
   outw(CUR_X, 0);
   outw(CUR_Y, 0);
   outw(MAJ_AXIS_PCNT,  (1024-1));
   outw(MULTIFUNC_CNTL, ( 768-1) | MIN_AXIS_PCNT);
   outw(CMD, CMD_RECT | INC_Y | INC_X | DRAW | PLANAR | WRTDATA);

   WaitQueue16_32(4,6);
LogPrintf ("s3InitEnvironment> WaitQueue16_32(4,6) done \n");
 /* Reset current draw position */
   outw(CUR_X, 0);
   outw(CUR_Y, 0);

 /* Reset current colors, foreground is all on, background is 0. */
   outw32(FRGD_COLOR, ~0);
   outw32(BKGD_COLOR,  0);
LogPrintf ("s3InitEnvironment> all done \n");
}

static void NEAR
s3Init (void)
{
	unsigned char	tmp;
	int		i;

	FirstTime = 0;

LogPrintf ("s3Init> start \n");
	s3Unlock ();
LogPrintf ("s3Init> s3Unlock() done \n");
#if 1
	outw(ADVFUNC_CNTL, 0x0007);

	outb(0x3C4, 1);
	tmp = inb(0x3C5);
	outb(0x3C5, tmp | 0x20); /* blank the screen */
	outb(DAC_MASK, 0);

/* Reset the 8514/A, and disable all interrupts.
*/
	outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
	outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL);
	i = inw(SUBSYS_STAT);

	outb(0x3C4, 1);
	outb(0x3C5, tmp);        /* unblank the screen */
	outb(DAC_MASK, 0xff);
LogPrintf ("s3Init> init sequence done \n");
#endif
	s3InitEnvironment();
LogPrintf ("s3Init> all done \n");
}

extern void FAR
GrfMoveS3 (int xx1, int yy1)
{
	x1 = xx1;
	y1 = yy1;
}

extern void FAR
GrfDrawS3 (int x2, int y2, Uint c)
{
	int	adx;		/* abs values of dx and dy */
	int	ady;
	int	signdx;		/* sign of dx and dy */
	int	signdy;
	int	e, e1, e2;	/* bresenham error and increments */
	int	len;		/* length of segment */
	short	cmd = CMD_LINE | DRAW | PLANAR | WRTDATA | LASTPIX;
	short	fix;

	if (FirstTime)
		s3Init ();

	if ((adx = x2 - x1) < 0) {
	    adx = -adx;
	    signdx = -1;
	    fix = 0;
	} else {
	    signdx = 1;
	    cmd |= INC_X;
	    fix = -1;
	}
	if ((ady = y2 - y1) < 0) {
	    ady = -ady;
	    signdy = -1;
	} else {
	    signdy = 1;
	    cmd |= INC_Y;
	}

	if (adx > ady) {
	    e1 = ady << 1;
	    e2 = e1 - (adx << 1);
	    e = e1 - adx;
	    len = adx;
	} else {
	    e1 = adx << 1;
	    e2 = e1 - (ady << 1);
	    e = e1 - ady;
	    cmd |= YMAJAXIS;
	    len = ady;
	}
	++GrStats[len];

	if (len) {
	     /*
	      * Here is a problem, the unwound error terms could be
	      * upto 16bit now. The poor S3 is only 12 or 13 bit.
	      * The rounding error is probably small I favor scaling
	      * the error terms, although re-evaluation is also an
	      * option I think it might give visable errors
	      * - Jon 12/9/93.
	      */

	     if (abs(e) > 4096  || abs(e1) > 4096 || abs(e2) > 4096) {
		int div;

		if (abs(e) > abs(e1))
		    div = (abs(e) > abs(e2)) ?
		    (abs(e) + 4095)/ 4096 : (abs(e2) + 4095)/ 4096;
		else
		    div = (abs(e1) > abs(e2)) ?
		    (abs(e1) + 4095)/ 4096 : (abs(e2) + 4095)/ 4096;

		e  /= div;
		e1 /= div;
		e2 /= div;
	     }
	     WaitQueue(7);
	     S3_OUTW(CUR_X, (short)x1);
	     S3_OUTW(CUR_Y, (short)y1);
	     S3_OUTW(ERR_TERM, (short)(e + fix));
	     S3_OUTW(DESTY_AXSTP, (short)e1);
	     S3_OUTW(DESTX_DIASTP, (short)e2);
	     S3_OUTW(MAJ_AXIS_PCNT, (short)len);
	     S3_OUTW(CMD, cmd);
	}
}
