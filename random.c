/* --------------------------------- random.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the random pointing device.
*/

#include "fly.h"


LOCAL_FUNC int FAR
Rread (POINTER *ptr)
{
	int	i;

	if (ptr->c[0] > 40) {
		if (ptr->a[3] < 75)
			ptr->a[3] += 5;		/* throttle up */
		--ptr->c[0];
	} else if (ptr->c[0] > 30) {
		--ptr->c[0];
		ptr->a[1] += 10;			/* pitch up */
	} else if (ptr->c[0] > 10) {
		--ptr->c[0];
		ptr->a[1] -= 10;			/* pitch down */
	} else if (ptr->c[0] > 0) {
		--ptr->c[0];
		ptr->a[1] += 10;			/* pitch up */
	} else {
		ptr->a[1] = 0;
#if 1
		i = ptr->a[0];
		if (i < 0)
			++i;
		else if (i > 0)
			--i;
		i += (Frand () % 10) - 5;
		if (i > 40)
			i = 80 - i;
		else if (i < -40)
			i = -80 - i;
		ptr->a[0] = (xshort)i;
#else
		ptr->a[0] = 0;
#endif
	}

	return (0);
}

LOCAL_FUNC int FAR
Rcal (POINTER *ptr)
{
	ptr->c[0] = 90;		/* set course for 250 m/s */
	ptr->a[0] = 0;
	ptr->a[1] = 0;
	ptr->l[0] = 0;
	ptr->l[1] = 0;
	return (0);
}

LOCAL_FUNC int FAR
Rinit (POINTER *ptr, char *options)
{
	return (Rcal (ptr));
}

LOCAL_FUNC void FAR
Rterm (POINTER *ptr)
{}

LOCAL_FUNC void FAR
Rkey (POINTER *ptr, int key)
{}

struct PtrDriver NEAR PtrRandom = {
	"RANDOM",
	0,
	NULL,	/* extra */
	Rinit,
	Rterm,
	Rcal,
	Rcal,			/* center */
	Rread,
	Rkey
};
