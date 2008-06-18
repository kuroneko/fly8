/* --------------------------------- keypad.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the keyboard as a pointing device.
*/

#include "fly.h"


LOCAL_FUNC void FAR
Kkey (POINTER *ptr, int key)
{
	switch (key) {
	case KF_UP:			/* counter clockwise */
		if (ptr->a[1] < 100)
			ptr->a[1] += 10;
		else
			ptr->a[1] = 100;
		break;
	case KF_DOWN:			/* clockwise */
		if (ptr->a[1] > -100)
			ptr->a[1] -= 10;
		else
			ptr->a[1] = -100;
		break;

	case KF_RIGHT_TURN:		/* clockwise */
		if (ptr->a[0] > -100)
			ptr->a[0] -= 10;
		else
			ptr->a[0] = -100;
		break;
	case KF_LEFT_TURN:		/* counter clockwise */
		if (ptr->a[0] < 100)
			ptr->a[0] += 10;
		else
			ptr->a[0] = 100;
		break;

	default:
		std_key (ptr, key);
		break;
	}
}

LOCAL_FUNC int FAR
Kcal (POINTER *ptr)
{
	ptr->a[0] = ptr->a[1] = 0;
	ptr->l[0] = ptr->l[1] = 0;
	return (0);
}

LOCAL_FUNC int FAR
Kread (POINTER *ptr)
{
	do_btns (ptr, NULL, 0);
	return (0);
}

LOCAL_FUNC int FAR
Kinit (POINTER *ptr, char *options)
{
	return (Kcal (ptr));
}

LOCAL_FUNC void FAR
Kterm (POINTER *ptr)
{}

struct PtrDriver NEAR PtrKeypad = {
	"KEYPAD",
	0,
	NULL,	/* extra */
	Kinit,
	Kterm,
	Kcal,
	Kcal,			/* center */
	Kread,
	Kkey
};
