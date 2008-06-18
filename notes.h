/* --------------------------------- notes.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Define notes stuff for tunes.c.
*/

#ifndef FLY8_NOTES_H
#define FLY8_NOTES_H

/* pitch */

#define	RR_	0		/* rest */

#define	CN_	2093		/* note pitch */
#define	CSH_	2217
#define	DFL_	2217
#define	DN_	2349
#define	DSH_	2489
#define	EFL_	2489
#define	EN_	2637
#define	FN_	2794
#define	FSH_	2960
#define	GFL_	2960
#define	GN_	3136
#define	GSH_	3322
#define	AFL_	3322
#define	AN_	3520
#define	ASH_	3729
#define	BFL_	3729
#define	BN_	3951

#define	H_	1L		/* C5=32.7Hz */
#define	I_	2L		/* C6=65.4Hz */
#define	J_	4L		/* C7=130.8Hz */
#define	K_	8L		/* C8=261.6Hz [A8=440Hz]*/
#define	L_	16L		/* C9=523.3Hz */
#define	M_	32L		/* C10=1046.5Hz */
#define	N_	64L		/* C11=2093Hz */


/* duration */

#define	T1_		(16*64*4*3)		/* 1 = 4*millisecs */
#define	T2_		(T1_/2)
#define	T4_		(T1_/4)
#define	T8_		(T1_/8)
#define	T16_		(T1_/16)
#define	T32_		(T1_/32)
#define	T64_		(T1_/64)
#define	TT1_		(T1_*2/3)
#define	TT2_		(TT1_/2)
#define	TT4_		(TT1_/4)
#define	TT8_		(TT1_/8)
#define	TT16_		(TT1_/16)
#define	TT32_		(TT1_/32)
#define	TT64_		(TT1_/64)
#define	D1_		(T1_*3/2)
#define	D2_		(D1_/T2)
#define	D4_		(D1_/T4)
#define	D8_		(D1_/T8)
#define	D16_		(D1_/T16)
#define	D32_		(D1_/T32)
#define	D64_		(D1_/T64)
#define	DD1_		(T1_*7/4)
#define	DD2_		(DD1_/2)
#define	DD4_		(DD1_/4)
#define	DD8_		(DD1_/8)
#define	DD16_		(DD1_/16)
#define	DD32_		(DD1_/32)
#define	DD64_		(DD1_/64)


/* articulation */

#define	SLUR_	0	/* macro P */
#define	LEGATO_	1/16	/* macro PL */
#define	SLEG_	1/8	/* macro PSL */
#define	SSLEGA_	1/4	/* macro PSSL */
#define	STACO_	1/2	/* macro PST */
#define	SSTACO_	5/8	/* macro PSST */


/* macros */

#define	P_(o,n,d)	(int)(n*o/N_),(int)(d)
#define	R_(d)		RR_,(int)(d)
#define	PL_(o,n,d)	(int)(n*o/N_),(int)((d)-(d)*LEGATO_), \
			RR_,(int)((d)*LEGATO_)
#define	PSL_(o,n,d)	(int)(n*o/N_),(int)((d)-(d)*SLEG_), \
			RR_,(int)((d)*SLEG_)
#define	PSSL_(o,n,d)	(int)(n*o/N_),(int)((d)-(d)*SSLEG_), \
			RR_,(int)((d)*SSLEG_)
#define	PST_(o,n,d)	(int)(n*o/N_),(int)((d)-(d)*STACO_), \
			RR_,(int)((d)*STACO_)
#define	PSST_(o,n,d)	(int)(n*o/N_),(int)((d)-(d)*SSTACO_), \
			RR_,(int)((d)*SSTACO_)

#define	CEND_		-1
#define	CGO_		-2
#define	CRPT_		-3

#define	GOTO_(a)	CGO_,a
#define	REPEAT_(a,n)	CRPT_,a,n,0
#define	END_		CEND_,0

#endif
