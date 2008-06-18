/*----------------------------- bgrasm.x ------------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Some assembly rewrites of line.c functions.
 *
 * void far vgaset (int to, int color, int nbytes);
 * void far ILoop (char *va, int dy, int dx, int dvx, int dvy, int color);
 * void far SLoop (char *va, int count, int dva, int color);
*/

#define SYS_ASM		1
#include "config.h"

#ifdef USE_BGRASM

#ifdef __ELF__
#define _vgaset		vgaset
#define _ILoop		ILoop
#define _SLoop		SLoop
#define __bWriteMode	_bWriteMode
#endif

#if SYS_LCC
	.extern __bWriteMode
#endif

/* void far vgaset (int to, int color, int nbytes);
 *
 * The subroutine will set 'nbytes' bytes at 'to' to 'color'.
*/

#define GrSET	$0x00
#define GrXOR	$0x01
#define GrOR	$0x02
#define GrXXX	$0x03

#define P(n)	4*(3+n)(%esp)

#define _to	%edi
#define _color	%eax
#define _nbytes	%ebx
#define _count	%ecx

	.file	"bgrasm.x"
	.text
	.align	4
	.globl	_vgaset
_vgaset:
	push	%ebx
	push	%ecx
	push	%edi

	movl	P(1),_to
	movl	P(2),_color
	movl	P(3),_nbytes
	cld
	cmpl	$6,_nbytes		/* minimal length; is 6 ok? */
	jl	vgasetx			/*    MUST be 3 or more! */

	movb	%al,%ah			/* for long set */
	movw	%ax,%cx
	shll	$16,%eax
	movw	%cx,%ax	

	movl	_to,_count
	negl	_count
	andl	$3,_count		/* 'bytes' in head */
	subl	_count,_nbytes		/* update length */
	rep
	stosb				/* set head */

#ifdef UNROLL_VGASET
	movl	_nbytes,_count
	shrl	$6,_count		/* '16*longs' in body */
	jz	_vgam2
	andl	$63,_nbytes
_vgam1:
	movl	%eax,0(_to)
	movl	%eax,4(_to)
	movl	%eax,8(_to)
	movl	%eax,12(_to)
	movl	%eax,16(_to)
	movl	%eax,20(_to)
	movl	%eax,24(_to)
	movl	%eax,28(_to)
	movl	%eax,32(_to)
	movl	%eax,36(_to)
	movl	%eax,40(_to)
	movl	%eax,44(_to)
	movl	%eax,48(_to)
	movl	%eax,52(_to)
	movl	%eax,56(_to)
	movl	%eax,60(_to)
	addl	$64,_to
	decl	_count
	jnz	_vgam1
_vgam2:
#endif

	movl	_nbytes,_count
	shrl	$2,_count		/* 'longs' in body */
	andl	$3,_nbytes		/* 'bytes' in tail */
	rep
	stosl				/* set body */

vgasetx:
	movl	_nbytes,_count
	rep
	stosb				/* set tail */
vgasret:

	pop	%edi
	pop	%ecx
	pop	%ebx
	ret

#undef P
#undef _to
#undef _color
#undef _nbytes
#undef _count

/* void far ILoop (char *va, int dy, int dx, int dvx, int dvy, int color);
 *
 * The inner loop of the Bresenham line drawing algorithm for the general
 * case. Shorten the loop by sacrificing setup time.
*/

#define P(n)	4*(6+n)(%esp)

#define _va	%esi
#define _dy	%di
#define _dx	%bx
#define _dvx	%edx
#define _dvy	%ebp
#define _color	%cl
#define _error	%ax

	.globl _ILoop
_ILoop:
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	movw	P(3),_dx			/* %ebx = dx, count */

	movw	P(2),%dx			/* %dx = dy */
	movw	$0,%ax				/* %dx:%ax = 0x10000*dy */
	divw	%bx				/* %ax = (0x10000*dy)/dx */
	movw	%ax,_dy				/* dy = (0x10000*dy)/dx */

	movl	P(1),_va
	movl	P(4),_dvx
	movl	P(5),_dvy
	movb	P(6),_color
	movw	$0x8000,_error			/* err = 0x8000 */
	addl	_dvx,_dvy

	testb	GrXXX,__bWriteMode
	jz	iloops2				/* Set write mode */
	testb	GrOR,__bWriteMode
	jnz	iloopo2				/* OR  write mode */
	jmp	iloopx2				/* XOR write mode */

iloops1:
	addl	_dvx,_va			/* va += dvx */
	movb	_color,(_va)			/* *va = c */
iloops2:
	decw	_dx				/* --count */
	js	iloopr
iloops3:
	addw	_dy,_error			/* err += dy */
	jnc	iloops1
	addl	_dvy,_va			/* va += dvy */
	movb	_color,(_va)			/* *va = c */
	decw	_dx				/* --count */
	jns	iloops3
iloopr:
	movl	_va,%eax			/* return (va) */

	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx

	ret

iloopx1:
	addl	_dvx,_va			/* va += dvx */
	xorb	_color,(_va)			/* *va ^= c */
iloopx2:
	decw	_dx				/* --count */
	js	iloopr
iloopx3:
	addw	_dy,_error			/* err += dy */
	jnc	iloopx1
	addl	_dvy,_va			/* va += dvy */
	xorb	_color,(_va)			/* *va ^= c */
	decw	_dx				/* --count */
	jns	iloopx3
	jmp	iloopr

iloopo1:
	addl	_dvx,_va			/* va += dvx */
	orb	_color,(_va)			/* *va |= c */
iloopo2:
	decw	_dx				/* --count */
	js	iloopr
iloopo3:
	addw	_dy,_error			/* err += dy */
	jnc	iloopo1
	addl	_dvy,_va			/* va += dvy */
	orb	_color,(_va)			/* *va |= c */
	decw	_dx				/* --count */
	jns	iloopo3
	jmp	iloopr

#undef _va
#undef _dy
#undef _dx
#undef _dvx
#undef _dvy
#undef _color
#undef _error
#undef P


/* void far SLoop (char *va, int count, int dva, int color);
 *
 * Draw simple 8 directions (except horizontal).
*/

#define P(n)	4*(4+n)(%esp)

#define _va	%esi
#define _count	%ebx
#define _dva	%edx
#define _color	%cl

	.globl _SLoop
_SLoop:
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi

	movl	P(1),_va
	movl	P(2),_count			/* %ebx = dx, count */
	decl	_count				/* --count */
	js	sloopr
	movl	P(3),_dva
	movb	P(4),_color

	testb	GrXXX,__bWriteMode		/* [X]OR write mode? */
	jnz	sloopt

sloops:
	addl	_dva,_va			/* va += dva */
	movb	_color,(_va)			/* *va = c */
	decl	_count				/* --count */
	jns	sloops

sloopr:
	movl	_va,%eax			/* return (va) */

	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx

	ret

sloopt:
	testb	GrXOR,__bWriteMode		/* XOR  write mode? */
	jnz	sloopx

sloopo:
	addl	_dva,_va			/* va += dva */
	orb	_color,(_va)			/* *va |= c */
	decl	_count				/* --count */
	jns	sloopo
	jmp	sloopr

sloopx:
	addl	_dva,_va			/* va += dva */
	xorb	_color,(_va)			/* *va ^= c */
	decl	_count				/* --count */
	jns	sloopx
	jmp	sloopr

#undef _va
#undef _count
#undef _dva
#undef _color
#undef P


#if 0


/**********************************************************************
 *
 * Unused stuff from here on
 *
 **********************************************************************
*/

/* void far ILoop (char *va, int dy, int dx, int dvx, int dvy, int color);
 *
 * The inner loop of the Bresenham line drawing algorithm for the general
 * case. A Plain Jane inplementation.
*/

#define P(n)	4*(6+n)(%esp)

#define _va	%esi
#define _dy	%di
#define _dx	%bp
#define _count	%bx
#define _dvx	%edx
#define _dvy	P(5)
#define _color	%cl
#define _error	%ax

	.globl _ILoopn
_ILoopn:
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	movl	P(1),_va
	movw	P(2),_dy
	movw	P(3),_dx
	movl	P(4),_dvx
	movb	P(6),_color

	movw	_dx,_count
	movw	_dx,_error			/* err = dx/2 */
	shrw	$1,_error
	jmp	iloopn2
iloopn1:
	movb	_color,(_va)			/* *va = c */
iloopn2:
	decw	_count				/* --count */
	js	iloopn4
iloopn3:
	addl	_dvx,_va			/* va += dvx */
	addw	_dy,_error			/* err += dy */
	cmpw	_dx,_error			/* err >= dx? */
	jb	iloopn1
	subw	_dx,_error			/* err >= dx? */
	addl	_dvy,_va			/* va += dvy */
	movb	_color,(_va)			/* *va = c */
	decw	_count				/* --count */
	jns	iloopn3
iloopn4:
	movl	_va,%eax			/* return (va) */

	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx

	ret

#undef _va
#undef _dy
#undef _dx
#undef _dvx
#undef _dvy
#undef _color
#undef _error

#endif

#undef GrSet
#undef GrXOR
#undef GrOR
#undef GrXXX

#endif /* ifdef USE_BGRASM */
