/* --------------------------------- tunes.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky _(eyal@eyal.emu.id.au).
*/

/* Tunes.
 * NOTE: a tune cannot start with a command. Duration must be greater that 4!
*/

#include "fly.h"
#include "notes.h"
#include "keyofc.h"

int FAR TnHello[] = {
	PL_(K_,C_,T16_),
	PL_(K_,D_,T16_),
	PL_(K_,E_,T16_),
	PL_(K_,F_,T16_),
	PL_(K_,G_,T16_),
	PL_(K_,A_,T16_),
	PL_(K_,B_,T8_),
	PL_(L_,C_,T8_),
	END_
};

int FAR TnGone[] = {
	R_(T64_),
	PL_(L_,C_,32*4),
	PL_(K_,B_,32*4),
	PL_(K_,A_,32*4),
	PL_(K_,G_,32*4),
	PL_(K_,F_,32*4),
	PL_(K_,E_,32*4),
	PL_(K_,D_,32*4),
	PL_(K_,C_,32*4),
	GOTO_(-8*2-1),
	END_
};

int FAR TnHit[] = {
	PL_(L_,C_,32*4),
	PL_(K_,B_,32*4),
	PL_(K_,A_,32*4),
	PL_(K_,G_,32*4),
	PL_(K_,F_,32*4),
	PL_(K_,E_,32*4),
	PL_(K_,D_,32*4),
	PL_(K_,C_,32*4),
	PL_(K_,D_,32*4),
	PL_(K_,E_,32*4),
	PL_(K_,F_,32*4),
	PL_(K_,G_,32*4),
	PL_(K_,A_,32*4),
	PL_(K_,B_,32*4),
	REPEAT_(-14*2,3000/(14*32)),	/* 3 seconds */
	END_
};

int FAR TnEngine[] = {
	R_(10),
	R_(10),
	GOTO_(-2),
	END_
};

int FAR TnAlarm[] = {
	R_(4),
	P_(N_,A_,T32_),
	P_(N_,G_,T32_),
	GOTO_(-3),
	END_
};

int FAR TnWarn[] = {
	R_(4),
	PL_(I_,G_,T32_),
	R_(T32_),
	GOTO_(-(1*2+2)),
	END_
};

int FAR TnNotice[] = {
	P_(M_,G_,T16_),
	END_
};

int FAR TnGear[] = {
	P_(I_,G_,T2_),
	END_
};

int FAR TnMsg[] = {
	P_(J_,G_,T32_),
	END_
};

int FAR TnDamage[] = {
	P_(M_,A_,T64_),
	P_(M_,G_,T64_),
	REPEAT_(-2,1),
	END_
};

#define	BG_ \
	PL_(J_,G_,TT4_), \
	PL_(J_,G_,TT8_), \
	PL_(J_,B_,TT4_), \
	PL_(J_,B_,TT8_), \
	PL_(K_,D_,TT4_), \
	PL_(K_,D_,TT8_), \
	PL_(K_,F_,TT8_), \
	PL_(K_,E_,TT8_), \
	PL_(K_,D_,TT8_) \

#define	BC_ \
	PL_(K_,C_,TT4_), \
	PL_(K_,C_,TT8_), \
	PL_(K_,E_,TT4_), \
	PL_(K_,E_,TT8_), \
	PL_(K_,G_,TT4_), \
	PL_(K_,G_,TT8_), \
	PL_(K_,ASH_,TT8_), \
	PL_(K_,A_,TT8_), \
	PL_(K_,G_,TT8_)

#define	BD_ \
	PL_(K_,D_,TT4_), \
	PL_(K_,D_,TT8_), \
	PL_(K_,FSH_,TT4_), \
	PL_(K_,FSH_,TT8_), \
	PL_(K_,A_,TT4_), \
	PL_(K_,A_,TT8_), \
	PL_(L_,C_,TT8_), \
	PL_(K_,B_,TT8_), \
	PL_(K_,A_,TT8_)

int FAR TnBlues[] = {
	BG_,
	BG_,
	BG_,
	BG_,

	BC_,
	BC_,
	BG_,
	BG_,

	BD_,
	BC_,

	PL_(J_,G_,TT4_),
	PL_(J_,G_,TT8_),
	PL_(J_,B_,TT4_),
	PL_(J_,B_,TT8_),
	PL_(K_,C_,TT4_),
	PL_(K_,C_,TT8_),
	PL_(K_,CSH_,TT4_),
	PL_(K_,CSH_,TT8_),
	PL_(K_,D_,T4_),
	PL_(J_,B_,TT4_),
	PL_(J_,G_,TT4_),
	R_(TT4_),
	R_(T4_),
	END_
};

#undef BG_
#undef BC_
#undef BD_
