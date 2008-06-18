/*----------------------------- csum.x -------------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* short chksum (short *buf, int wordcnt)
 *
 * A 16-bit version.
*/

#define P(n)	4*(1+n)(%esp)

	.file	"csum.x"
	.text
	.align	4
	.globl	_chksum
_chksum:
	push	%esi

	movl	P(1),%esi	/* fetch parameters */
	xorl	%ecx,%ecx
	movw	P(2),%cx

	cld
	xorl	%dx,%dx		/* initial value for xsum, clear carry */
lpchk:	lodsw			/* get next word	  1 byte, 16 cycles */
	adcw	%ax,%dx		/* overlap adding last CY 2 bytes, 3 c */
	loop	lpchk		/* next word		  2 bytes, 17 c */

	adcw	$0,%dx		/* add final carry bit */
	movl	%edx,%eax

	pop	%esi
	ret

#undef P
