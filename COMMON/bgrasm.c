/*----------------------------- bgrasm.c ------------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Some assembly rewrites of line.c functions.
 *
 * void FAR vgaset (char BPTR *to, int color, int nbytes);
 * char BPTR * FAR ILoop (char BPTR *va, int dy, int dx, int dvx, int dvy,
 *	int color);
 * char BPTR * FAR SLoop (char BPTR *va, int count, int dva, int color);
 *
 * Note: vc2/4 will push/pop to save/restore the necessary registers, so
 * you must inspect the generated code (use -Fc) to know how to set the
 * P(n) macro. The first number is how many pushes we do ourselves,
 * the second is how many pushes vc did. Normally we only need to save
 * 'ebp' if we modify it.
*/

#ifdef USE_BGRASM

#include "fly.h"
#include "bgr.h"

extern char	_bWriteMode;

#define GrSET	000H
#define GrXOR	001H
#define GrOR	002H
#define GrXXX	003H


/* The subroutine will set 'nbytes' bytes at 'to' to 'color'.
*/
void FAR
vgaset (char BPTR *to, int color, int nbytes)
{

#define P(n)	esp+4*(0+2+(n))		/* ebx, edi */
#define PD(n)	DWORD PTR[P(n)]
#define PW(n)	WORD PTR[P(n)]
#define PB(n)	BYTE PTR[P(n)]

#define _to	edi
#define _color	eax
#define _nbytes	ebx
#define _count	ecx

    _asm {
	mov	_to,PD(1)
	mov	_color,PD(2)
	mov	_nbytes,PD(3)
	cld
	cmp	_nbytes,6		/* minimal length; is 6 ok? */
	jl	vgasetx			/*    MUST be 3 or more! */

	mov	ah,al			/* for long set */
	mov	cx,ax
	shl	eax,16
	mov	ax,cx	

	mov	_count,_to
	neg	_count
	and	_count,3		/* 'bytes' in head */
	sub	_nbytes,_count		/* update length */
	rep	stosb			/* set head */

#ifdef UNROLL_VGASET
	mov	_count,_nbytes
	shr	_count,6		/* '16*longs' in body */
	jz	_vgam2
	and	_nbytes,0ffH
_vgam1:
	mov	0[_to],eax
	mov	4[_to],eax
	mov	8[_to],eax
	mov	12[_to],eax
	mov	16[_to],eax
	mov	20[_to],eax
	mov	24[_to],eax
	mov	28[_to],eax
	mov	32[_to],eax
	mov	36[_to],eax
	mov	40[_to],eax
	mov	44[_to],eax
	mov	48[_to],eax
	mov	52[_to],eax
	mov	56[_to],eax
	mov	60[_to],eax
	add	_to,64
	dec	_count
	jnz	_vgam1
_vgam2:
#endif

	mov	_count,_nbytes
	shr	_count,2		/* 'longs' in body */
	and	_nbytes,003H		/* 'bytes' in tail */
	rep	stosd			/* set body */

vgasetx:
	mov	_count,_nbytes
	rep	stosb			/* set tail */
    }
}

#undef P
#undef PD
#undef PW
#undef PB
#undef _to
#undef _color
#undef _nbytes
#undef _count


/* The inner loop of the Bresenham line drawing algorithm for the general
 * case. Shorten the loop by sacrificing setup time.
*/
char BPTR * FAR
ILoop (char BPTR *va, int dy, int dx, int dvx, int dvy, int color)
{

#define P(n)	esp+4*(1+4+(n))		/* ebp, ebx, esi, edi */
#define PD(n)	DWORD PTR[P(n)]
#define PW(n)	WORD PTR[P(n)]
#define PB(n)	BYTE PTR[P(n)]

#define _va	esi
#define _dy	di
#define _dx	bx
#define _dvx	edx
#define _dvy	ebp
#define _color	cl
#define _error	ax

    _asm {
#if _MSC_VER > 1000	/* vc 5.0 */
	push	ebp
#endif

	mov	_dx,PW(3)			/* ebx = dx, count */

	mov	dx,PW(2)			/* %dx = dy */
	mov	ax,0				/* %dx:%ax = 0x10000*dy */
	div	bx				/* %ax = (0x10000*dy)/dx */
	mov	_dy,ax				/* dy = (0x10000*dy)/dx */

	mov	_va,PD(1)
	mov	_dvx,PD(4)
	mov	_dvy,PD(5)
	mov	_color,PB(6)
	mov	_error,08000H			/* err = 0x8000 */
	add	_dvy,_dvx

	test	_bWriteMode,GrXXX
	jz	iloops2				/* Set write mode */
	test	_bWriteMode,GrOR
	jnz	iloopo2				/* OR  write mode */
	jmp	iloopx2				/* XOR write mode */

iloops1:
	add	_va,_dvx			/* va += dvx */
	mov	[_va],_color			/* *va = c */
iloops2:
	dec	_dx				/* --count */
	js	iloopr
iloops3:
	add	_error,_dy			/* err += dy */
	jnc	iloops1
	add	_va,_dvy			/* va += dvy */
	mov	[_va],_color			/* *va = c */
	dec	_dx				/* --count */
	jns	iloops3
	jmp	iloopr

iloopx1:
	add	_va,_dvx			/* va += dvx */
	xor	[_va],_color			/* *va ^= c */
iloopx2:
	dec	_dx				/* --count */
	js	iloopr
iloopx3:
	add	_error,_dy			/* err += dy */
	jnc	iloopx1
	add	_va,_dvy			/* va += dvy */
	xor	[_va],_color			/* *va ^= c */
	dec	_dx				/* --count */
	jns	iloopx3
	jmp	iloopr

iloopo1:
	add	_va,_dvx			/* va += dvx */
	or	[_va],_color			/* *va |= c */
iloopo2:
	dec	_dx				/* --count */
	js	iloopr
iloopo3:
	add	_error,_dy			/* err += dy */
	jnc	iloopo1
	add	_va,_dvy			/* va += dvy */
	or	[_va],_color			/* *va |= c */
	dec	_dx				/* --count */
	jns	iloopo3
	jmp	iloopr

iloopr:
	mov	eax,_va				/* return (va) */
#if _MSC_VER > 1000	/* vc 5.0 */
	pop	ebp
#endif
	mov	va,eax				/* return (va) */
    }
    return (va);
}
#undef P
#undef PD
#undef PW
#undef PB
#undef _va
#undef _dy
#undef _dx
#undef _dvx
#undef _dvy
#undef _color
#undef _error


/* Draw simple 8 directions (except horizontal).
*/
char BPTR * FAR
SLoop (char BPTR *va, int count, int dva, int color)
{

#if _MSC_VER > 1000	/* vc 5.0 */
#define P(n)	esp+4*(0+3+(n))		/* ebp, ebx, esi */
#else
#define P(n)	esp+4*(0+2+(n))		/* ebx, esi */
#endif
#define PD(n)	DWORD PTR[P(n)]
#define PW(n)	WORD PTR[P(n)]
#define PB(n)	BYTE PTR[P(n)]

#define _va	esi
#define _count	ebx
#define _dva	edx
#define _color	cl

    _asm {
	mov	_va,PD(1)
	mov	_count,PD(2)			/* ebx = dx, count */
	dec	_count				/* --count */
	js	sloopr
	mov	_dva,PD(3)
	mov	_color,PB(4)

	test	_bWriteMode,GrXXX		/* [X]OR write mode? */
	jnz	sloopt

sloops:
	add	_va,_dva			/* va += dva */
	mov	[_va],_color			/* *va = c */
	dec	_count				/* --count */
	jns	sloops
	jmp	sloopr

sloopt:
	test	_bWriteMode,GrXOR		/* XOR  write mode? */
	jnz	sloopx

sloopo:
	add	_va,_dva			/* va += dva */
	or	[_va],_color			/* *va |= c */
	dec	_count				/* --count */
	jns	sloopo
	jmp	sloopr

sloopx:
	add	_va,_dva			/* va += dva */
	xor	[_va],_color			/* *va ^= c */
	dec	_count				/* --count */
	jns	sloopx
	jmp	sloopr

sloopr:
	mov	va,_va				/* return (va) */
    }
    return (va);
}
#undef _va
#undef _count
#undef _dva
#undef _color
#undef P
#undef PD

#undef GrSET
#undef GrXOR
#undef GrOR
#undef GrXXX

#endif /* ifdef USE_BGRASM */
