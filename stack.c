/* --------------------------------- stack.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* measure stack usage.
*/

#include "fly.h"


#define	STACK_CHAR ((Uchar)0x5a)

extern int FAR
check_stack (int func)		/* 0=set, 1=check */
{
	Uchar	chunk[3000], *p;

	if (func == 0) {
		for (p = chunk; p < &chunk[sizeof (chunk)]; *p++ = STACK_CHAR)
			;
		return (0);
	}

/* If the first bytes of chunk[] are untouched then we assume that the stack
 * grown to lower addresses (so chunk[] was used from its end).
*/
	for (p = &chunk[16]; --p > chunk && *p == STACK_CHAR;)
		;

	if (p > chunk) {			/* head was used */
		for (p = &chunk[sizeof (chunk)]; p > chunk;)
			if (*--p != STACK_CHAR)
				return ((int)(p - chunk));
	} else {				/* tail was used */
		for (p = chunk; p < &chunk[sizeof (chunk)];)
			if (*p++ != STACK_CHAR)
				return (sizeof (chunk) - (int)(p - chunk));
	}

	return (0);
}

#undef STACK_CHAR
