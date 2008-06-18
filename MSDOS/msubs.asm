;------------------------------ msubs.asm ------------------------------------
;
; Math routines to support fly8.c.
;
; int far fmula (int x, int y);
;
; int far fdiva (int x, int y);
;
; int far muldiva (int x, int y, int z);
;
; int far dithadj (int x, int dither, int interval);
;
; void far Vmula (int R[3], int V[3], int M[3][3]);
;
; void far matyxz (int far T[3][3], int sx, int cx, int sy, int cy,
;	int sz, int cz)
;
; void far matxyz (int far T[3][3], int spitch, int cpitch, int sroll, 
;	int croll, int syaw, int cyaw) 				***OBSOLETE***
;
; int chksum (int *buf, int len);
;
; void far m_set (int seg, int to, int color, int nbytes);
;
;-----------------------------------------------------------------------------
;
	.MODEL	LARGE
	.286
;
	EXTRN	_sin_tab:WORD
;
	.CODE
;
fmulm	MACRO	a,b
	mov	ax,a
	imul	WORD PTR b
	shl	ax,1
	rcl	dx,1
	shl	ax,1
	rcl	dx,1
	ENDM
;
	PUBLIC @fmula
@fmula	PROC  far
;
; int far _fastcall fmula (int x, int y); [ax, dx]
;
;    compute (x*y) >> 14
;
	imul	dx
	shl	ax,1
	rcl	dx,1
	shl	ax,1
	rcl	dx,1
	mov	ax,dx
;
	ret
@fmula	ENDP
;
	PUBLIC _fmula
_fmula	PROC  far
;
; int far fmula (int x, int y);
;
;    compute (x*y) >> 14
;
px	EQU	ss:WORD PTR[BX+4]
py	EQU	ss:WORD PTR[BX+6]
;
	mov	bx,sp
;
	fmulm	px,py
	mov	ax,dx
;
	ret
_fmula	ENDP
;
	PUBLIC @fdiva
@fdiva	PROC  far
;
; int far _fastcall fdiva (int x, int y); [ax, dx]
;
;    compute (x/y) as a fraction: (x<<14)/y.
;
	mov	bx,dx
	mov	dx,ax
	sub	ax,ax
	sar	dx,1
	rcr	ax,1
	sar	dx,1
	rcr	ax,1
	idiv	bx
;
	ret
@fdiva	ENDP
;
	PUBLIC _fdiva
_fdiva	PROC  far
;
; int far fdiva (int x, int y);
;
;    compute (x/y) as a fraction: (a<<14)/y.
;
px	EQU	ss:WORD PTR[BX+4]
py	EQU	ss:WORD PTR[BX+6]
;
	mov	bx,sp
;
	mov	dx,px
	sub	ax,ax
	sar	dx,1
	rcr	ax,1
	sar	dx,1
	rcr	ax,1
	idiv	py
;
	ret
_fdiva	ENDP
;
	PUBLIC @muldiva
@muldiva	PROC  far
;
; int far _fastcall muldiva (int x, int y, int z); [ax, dx, bx]
;
;    compute (x*y)/z
;
	imul	dx
	idiv	bx
;
	ret
@muldiva	ENDP
;
	PUBLIC _muldiva
_muldiva	PROC  far
;
; int far muldiva (int x, int y, int z);
;
;    compute (x*y)/z
;
px	EQU	ss:WORD PTR[BX+4]
py	EQU	ss:WORD PTR[BX+6]
pz	EQU	ss:WORD PTR[BX+8]
;
	mov	bx,sp
;
	mov	ax,px
	imul	py
	idiv	pz
;
	ret
_muldiva	ENDP
;
	PUBLIC @dithadja
@dithadja	PROC  far
;
; int far _fastcall dithadja (int x, int dither, int interval);
;
;    compute (x*interval+-dither)/1000
;
	mov	cx,dx
	imul	bx
	or	dx,dx
	js	ditha1
	add	ax,cx
	adc	dx,0
	mov	bx,1000
	idiv	bx
	ret
ditha1:
	sub	ax,cx
	sbb	dx,0
	mov	bx,1000
	idiv	bx
	ret
@dithadja	ENDP
;
	PUBLIC _dithadja
_dithadja	PROC  far
;
; int far dithadja (int x, int dither, int interval);
;
;    compute (x*interval+-dither)/1000
;
px	EQU	ss:WORD PTR[BX+4]
pd	EQU	ss:WORD PTR[BX+6]
pi	EQU	ss:WORD PTR[BX+8]
;
	mov	bx,sp
;
	mov	ax,px		; x
	imul	pi		; x*interval
	or	dx,dx
	js	ditha
	add	ax,pd		; x*interval+dither
	adc	dx,0
	mov	bx,1000
	idiv	bx		; (x*interval+dither)/1000
	ret
ditha:
	sub	ax,pd		; x*interval-dither
	sbb	dx,0
	mov	bx,1000
	idiv	bx		; (x*interval-dither)/1000
	ret
_dithadja	ENDP
;
	PUBLIC _Vmula
_Vmula	PROC  far
;
; void far Vmula (int R[3], int V[3], int M[3][3]);
;
;	R[X]=fmul(V[X],M[0][X])+fmul(V[Y],M[1][X])+fmul(V[Z],M[2][X]);
;	R[Y]=fmul(V[X],M[0][Y])+fmul(V[Y],M[1][Y])+fmul(V[Z],M[2][Y]);
;	R[Z]=fmul(V[X],M[0][Z])+fmul(V[Y],M[1][Z])+fmul(V[Z],M[2][Z]);
;
;
pr	EQU	WORD PTR[BP+6]
prb	EQU	WORD PTR[BP+8]
pv	EQU	WORD PTR[BP+10]
pvb	EQU	WORD PTR[BP+12]
pm	EQU	WORD PTR[BP+14]
pmb	EQU	WORD PTR[BP+16]
;
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	ds
	push	es
;
	mov	bx,pr		; r: ds:bx
	mov	si,pv		; v: es:si
	mov	ax,pvb
	mov	es,ax		; v base
	mov	di,pm		; m: ds:di
	mov	ax,pmb
	mov	ds,ax		; m base
;
	fmulm	es:0[si],0[di]	; v0*m00
	mov     cx,dx
	fmulm	es:2[si],6[di]	; v1*m10
	add     cx,dx
	fmulm	es:4[si],12[di]	; v2*m20
	add     cx,dx
;
	mov	dx,ds		; save ds
	mov	ax,prb
	mov	ds,ax		; r base
	mov	0[bx],cx	; r0
	mov	ds,dx		; restore ds
;
	fmulm	es:0[si],2[di]	; v0*m01
	mov     cx,dx
	fmulm	es:2[si],8[di]	; v1*m11
	add     cx,dx
	fmulm	es:4[si],14[di]	; v2*m21
	add     cx,dx
;
	mov	dx,ds		; save ds
	mov	ax,prb
	mov	ds,ax		; r base
	mov	2[bx],cx	; r1
	mov	ds,dx		; restore ds
;
	fmulm	es:0[si],4[di]	; v0*m02
	mov     cx,dx
	fmulm	es:2[si],10[di]	; v1*m12
	add     cx,dx
	fmulm	es:4[si],16[di]	; v2*m22
	add     cx,dx
;
	mov	ax,prb
	mov	ds,ax		; r base
	mov	4[bx],cx	; r2
;
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	ret
_Vmula	ENDP
;
	PUBLIC _matyxz
_matyxz	PROC  far
;
; void far matyxz (int far T[3][3], int sx, int cx, int sy, int cy,
;	int sz, int cz)
;
;	Build the orientation matrix for the three basic angles
;
pt	EQU	WORD PTR[BP+6]
ptb	EQU	WORD PTR[BP+8]
psx	EQU	WORD PTR[BP+10]
pcx	EQU	WORD PTR[BP+12]
psy	EQU	WORD PTR[BP+14]
pcy	EQU	WORD PTR[BP+16]
psz	EQU	WORD PTR[BP+18]
pcz	EQU	WORD PTR[BP+20]
;
	push	bp
	mov	bp,sp
	push	di
	push	ds
;
	mov	ax,ptb
	mov	ds,ax
	mov	di,pt		; T: ds:di
;
; T[0][2] = -fmul (sy, cx);		0*3+2=2
;
	fmulm	psy,pcx
	neg	dx
	mov	2*2[di],dx
;
; T[1][0] = -fmul (cx, sz);		1*3+0=3
;
	fmulm	pcx,psz
	neg	dx
	mov	2*3[di],dx
;
; T[1][1] = fmul (cx, cz);		1*3+1=4
;
	fmulm	pcx,pcz
	mov	2*4[di],dx
;
; T[1][2] = sx;				1*3+2=5
;
	mov	ax,psx
	mov	2*5[di],ax
;
; T[2][2] = fmul (cy, cx);		2*3+2=8
;
	fmulm	pcy,pcx
	mov	2*8[di],dx
;
; tt1 = fmul (cy, sz);
;
	fmulm	pcy,psz
	mov	cx,dx		; tt1: cx
;
; tt2 = fmul (sy, cz);
;
	fmulm	psy,pcz
	mov	bx,dx		; tt2: bx
;
; T[0][1] = tt1 + fmul (tt2, sx);	0*3+1=1
;
	fmulm	bx,psx
	add	dx,cx
	mov	2*1[di],dx
;
; T[2][0] = tt2 + fmul (tt1, sx);	2*3+0=6
;
	fmulm	cx,psx
	add	dx,bx
	mov	2*6[di],dx
;
; tt3 = fmul (cy, cz);
;
	fmulm	pcy,pcz
	mov	cx,dx		; tt3: cx
;
; tt4 = fmul (sy, sz);
;
	fmulm	psy,psz
	mov	bx,dx		; tt4: bx
;
; T[0][0] = tt3 - fmul (tt4, sx);	0*3+0=0
;
	fmulm	bx,psx
	neg	dx
	add	dx,cx
	mov	2*0[di],dx
;
; T[2][1] = tt4 - fmul (tt3, sx);	2*3+1=7
;
	fmulm	cx,psx
	sub	bx,dx
	mov	2*7[di],bx
;
	pop	ds
	pop	di
	pop	bp
	ret
_matyxz	ENDP
;
	PUBLIC _matxyz
_matxyz	PROC  far
;
; void far matxyz (int far T[3][3], int spitch, int cpitch, int sroll, 
;	int croll, int syaw, int cyaw)
;
;	Build the orientation matrix for the three basic angles
;
pt	EQU	WORD PTR[BP+6]
ptb	EQU	WORD PTR[BP+8]
psp	EQU	WORD PTR[BP+10]
pcp	EQU	WORD PTR[BP+12]
psr	EQU	WORD PTR[BP+14]
pcr	EQU	WORD PTR[BP+16]
psy	EQU	WORD PTR[BP+18]
pcy	EQU	WORD PTR[BP+20]
;
	push	bp
	mov	bp,sp
	push	di
	push	ds
;
	mov	ax,ptb
	mov	ds,ax
	mov	di,pt		; T: ds:di
;
; T[0][0] = fmul (cyaw, croll);		0*3+0=0
;
	fmulm	pcy,pcr
	mov	0[di],dx
;
; T[0][1] = fmul (syaw, croll);		0*3+1=1
;
	fmulm	psy,pcr
	mov	2[di],dx
;
; T[0][2] = -sroll;			0*3+2=2
;
	mov	ax,psr
	neg	ax
	mov	4[di],ax
;
; tt1 = fmul (syaw, cpitch);
;
	fmulm	psy,pcp
	mov	cx,dx		; tt1: cx
;
; tt2 = fmul (cyaw, spitch);
;
	fmulm	pcy,psp
	mov	bx,dx		; tt2: bx
;
; T[1][0] = fmul (tt2, sroll) - tt1;	1*3+0=3
;
	fmulm	bx,psr
	sub	dx,cx
	mov	6[di],dx
;
; T[2][1] = fmul (tt1, sroll) - tt2;	2*3+1=7
;
	fmulm	cx,psr
	sub	dx,bx
	mov	14[di],dx
;
; tt3 = fmul (cyaw, cpitch);
;
	fmulm	pcy,pcp
	mov	cx,dx		; tt3: cx
;
; tt4 = fmul (syaw, spitch);
;
	fmulm	psy,psp
	mov	bx,dx		; tt4: bx
;
; T[1][1] = fmul (tt4, sroll) + tt3;	1*3+1=4
;
	fmulm	bx,psr
	add	dx,cx
	mov	8[di],dx
;
; T[2][0] = fmul (tt3, sroll) + tt4;	2*3+0=6
;
	fmulm	cx,psr
	add	dx,bx
	mov	12[di],dx
;
;
; T[1][2] = fmul (croll, spitch);	1*3+2=5
;
	fmulm	pcr,psp
	mov	10[di],dx
;
; T[2][2] = fmul (cpitch, croll);	2*3+2=8
;
	fmulm	pcp,pcr
	mov	16[di],dx
;
	pop	ds
	pop	di
	pop	bp
	ret
_matxyz	ENDP
;
	PUBLIC	_my_sina
	PUBLIC	@my_sina
_my_sina PROC  far
;
; short far my_sina (short d);
;
dangle	EQU	ss:WORD PTR[BX+4]
;
	mov	bx,sp
	mov	ax,dangle
;
@my_sina:
;
; short far _fastcall my_sina (short d); [ax]
;
;    compute sin(d). All conditionals replaced with linear logic.
;
	mov	cx,ax
	add	cx,cx			; get sign into carry
	sbb	cx,cx			; fill reg with carry
	xor	ax,cx			; negate if -ve
	sub	ax,cx			;
	jo 	l2			; was D180
;
; Now 'ax' is in the positive half (0...180)
;
	cmp	ax,4000H
	jl 	l1
	sub	ax,8000H
	neg	ax
;
; Now 'ax' is in the first quarter (0...90).
;
l1:	push	si
	mov	si,ax
	mov	dx,si			; f = d&15;
	and	dx,0FH			;
	sar	si,3			; d >>= 4;
	and	si,0FFFEH			;
	mov	bx,_sin_tab[si]		; l = sin_tab[d];
	mov	ax,_sin_tab[si+2]	; h = sin_tab[d+1] - l;
	sub	ax,bx			;
	imul	dx			; l += (8+h*f)>>4;
	add	ax,8			;
	sar	ax,4			;
	add	ax,bx			;
;
; Adjust for sign and return.
;
	xor	ax,cx			; return ((short)((l^ind)-ind));
	sub	ax,cx			;
	pop	si			;
	ret				;
l2:	xor	ax,ax			; return (0);
	ret				;
_my_sina ENDP
;

;SRI-UNIX:~billw/cksum.a86 16-nov-84 0400 edit by William Westfield
; improved checksum algorithm (untested)
;  Copyright 1984 by the Massachusetts Institute of Technology  
;  See permission and disclaimer notice in file "notice.h"  

	PUBLIC	_chksum
_chksum	PROC  far

; _chksum(buf, len, 0) performs an Internet type checksum. buf points to where
; 	the checksumming should begin, len is the number of 16 bit words
;	to be summed. Returns the checksum. This is the Unix compatible
;       version

	push	bp
	mov	bp,sp
	push	si
	push	ds

;	mov	bx,4[bp]	; get buf
;	mov	cx,6[bp]	; get len
;	xor	ax,ax		; initial value for xsum, clear carry
;
;lpchk:	add	ax,[bx]		; add value and carry	2 bytes, 18 cycles
;	adc	ax,0		;			3 bytes,  8 c
;	inc	bx		; bump pointer		1 byte,   2 c
;	inc	bx		;			1 byte,   2 c
;	loop	lpchk		; do it again		2 bytes, 17 c

;new (faster?) checksum algorithm by Bill Westfield
; The idea is to overlap the carry wrap around with the addition of the
; next word of data.  Thus we must be careful that pointer increments
; and looping instructions do not affect the carry flag...

	mov	si,8[bp]	; SEG(buf)
	mov	ds,si
	mov	si,6[bp]	; OFF(buf)
	mov	cx,10[bp]	; len

	xor	bx,bx		; initial value for xsum, clear carry
	cld
lpchk:	lodsw			;get next word		1 byte, 16 cycles
	adc	bx,ax		;overlap adding last CY	2 bytes, 3 c
	loop	lpchk		;next word		2 bytes, 17 c

	mov	ax,bx		; put where results have to go
	adc	ax,0		; add final carry bit.

	pop	ds
	pop	si
	pop	bp
	ret

_chksum	ENDP


;-----------------------------------------------------------------------------
;
; void far m_set (int seg, int to, int color, int nbytes); [es di ax bx]
;
;    The subroutine will set 'nbytes' bytes at 'to' to 'color'.
;	'to' os an offset into segment 'seg'.
;
;-----------------------------------------------------------------------------

	PUBLIC	_m_set
_m_set	PROC  far

VGASEG	EQU	0a000H

pseg	EQU	WORD PTR[BP+6]
pto	EQU	WORD PTR[BP+8]
pcolor	EQU	WORD PTR[BP+10]
plength EQU	WORD PTR[BP+12]

	push	bp
	mov	bp,sp
	push	es
	push	di

	mov	ax,pseg
	mov	es,ax

	mov	di,pto
	mov	ax,pcolor
	mov	bx,plength

;
; This is a 32 bit version
;
	cmp	bx,6
	jl	msetx

	cld

	mov	ah,al		; for dword set
	mov	cx,ax
	db	66H		; operand size prefix
	shl	ax,16
	mov	ax,cx		; for dword set

	mov	cx,di		; 'to'
	neg	cx
	and	cx,03H		; head size
	sub	bx,cx		; update length
	rep	stosb		; set head

	mov	cx,bx		; body length
	shr	cx,2
	db	66H		; operand size prefix
	rep	stosw		; set body

	and	bx,03H		; tail length
msetx:
	mov	cx,bx
	rep	stosb		; set tail

	pop	di
	pop	es
	pop	bp
	ret

_m_set	ENDP


	END
