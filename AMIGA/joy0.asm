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
* remember:
*   JOY_LEFT	= 1
*   JOY_RIGHT	= 2
*   JOY_UP	= 4
*   JOY_DOWN	= 8
*   JOY_FIRE	= 16
*
*

    CODE
    xdef    _joy0
    xdef    @joy0   ; Lettuce compatibility
    xdef    _joy1
    xdef    @joy1   ; Lettuce compatibility

_joy0:
@joy0:
    sub.l   a1,a1
    move.w  $dff00a,d1
    btst    #6,$bfe001
    bne.s   dojoy
    subq.w  #1,a1
    bra.s   dojoy

_joy1:
@joy1:
    sub.l   a1,a1
    move.w  $dff00c,d1
    btst    #7,$bfe001
    bne.s   dojoy
    subq.w  #1,a1

dojoy:
    and.w   #$303,d1
    lea     jtab(pc),a0
    moveq   #8,d0
*
* this algorith??? (<- shit :-) should give
* best results on 68o1o or higher, but will
* still suffice on the standard 68ooo
*
jlop:
    cmp.w   (a0)+,d1
    dbeq    d0,jlop
    move.w  16(a0),d0
    ext.l   d0
    move.w  a1,d1
    beq.s   return
    or.w    #16,d0
return:
    rts

jtab:
    dc.w    0,$3,$300,$100,$1,$103,$2,$200,$301
    dc.w    0,2,1,4,8,6,10,5,9

    END
