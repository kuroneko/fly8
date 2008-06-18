/*----------------------------- vbeasm.x ------------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Some assembly support for protected mode VBE 2.0.
 *
 * void vbeCallFunc5 (int bank);
 * void vbeCallFunc7 (int pixel, int line);
*/

	.file	"vbeasm.x"

	.text
	.align	4

/* void far vbeCallFunc5 (long bank);
 *
 * Call the "set Window" protected mode function.
*/

#define P(n)	4*(7+n)(%esp)

	.globl	_vbeCallFunc5
_vbeCallFunc5:

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushw	%ds
	pushw	%es

	movw	$0,%bx
	movw	P(1),%dx

	movw	$0x4f05,%ax		/* needed? */
	call	*_vbeFunc5

	popw	%es
	popw	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret

#undef P


/* void far vbeCallFunc7 (int pixel, int line)
 *
 * Call the "set Display Start" protected mode function.
*/

#define P(n)	4*(7+n)(%esp)

	.globl	_vbeCallFunc7
_vbeCallFunc7:

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushw	%ds
	pushw	%es

	movw	$0x0000,%bx		/* set display start */
	movl	P(1),%ecx		/* first pixel */
	movl	P(2),%edx		/* first scan line */

	movw	%ds,%ax			/* maybe? */
	movw	%ax,%es

	movw	$0x4f07,%ax		/* needed? */
	call	*_vbeFunc7

	popw	%es
	popw	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret

#undef P


/* void far vbeCallFunc9 (int color, char *rgb)
 *
 * Call the "set Primary Palette data" protected mode function.
*/

#define P(n)	4*(7+n)(%esp)

	.globl	_vbeCallFunc9
_vbeCallFunc9:

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushw	%ds
	pushw	%es

	xorw	%bx,%bx			/* set palette data */
	movl	$1,%ecx			/* one register */
	movl	P(1),%edx		/* first register (color) */
	movw	%ds,%ax
	movw	%ax,%es
	movl	P(2),%edi		/* rgb table address */

	movw	$0x4f09,%ax		/* needed? */
	call	*_vbeFunc9

	popw	%es
	popw	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret

#undef P
