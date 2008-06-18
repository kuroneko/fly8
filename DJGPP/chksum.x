/*----------------------------- chksum.x ------------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* short chksum (short *buf, int wordcnt)
 *
 * A 32-bit version.
*/

#define P(n)	4*(2+n)(%esp)

	.file	"chksum.x"
	.text
	.align	4
	.globl	_chksum
_chksum:
	push	%esi
	push	%ebx
	
        movl	P(1),%esi	/* fetch parameters */
	xorl	%ecx,%ecx
	movw	P(2),%cx

	movb	%cl,%bl		/* save for later odd-word processing */
	shrl	$1,%ecx		/* group into dword [was $2] */
	xorl	%edx,%edx	/* set checksum to 0, clear CF */
	jcxz	short		/* if nothing to do */
	cld
deloop:
	lodsl
	adcl	%eax,%edx
	loop	deloop

	adcl	$0,%edx		/* clean up from loop */
short:
	xorl	%eax,%eax	/* %eax is odd word (if any) */
	testb	$1,%bl
	jz	done
	lodsw			/* fetch odd word */
done:
	movl	%edx,%ecx	/* %dx is low word of sum */
	shrl	$16,%ecx	/* %cx is top word of sum */
	addw	%cx,%dx		/* fold long sum to short */
	adcw	%dx,%ax		/* add to result */
 	adcw	$0,%ax		/* and clean last carry */

	pop	%ebx
	pop	%esi
	ret

#undef P
