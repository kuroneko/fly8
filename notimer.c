/* --------------------------------- notimer.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dummy timer driver.
*/

#include "fly.h"

#include <time.h>


LOCAL_FUNC int FAR
ntm_hires (void)
{return (0);}

LOCAL_FUNC Ulong FAR
ntm_milli (void)
{return (0);}

LOCAL_FUNC Ulong FAR
ntm_interval (int mode, Ulong res)
{return (0);}

LOCAL_FUNC int FAR
ntm_init (char *options)
{return (0);}

LOCAL_FUNC void FAR
ntm_term (void)
{}

LOCAL_FUNC char * FAR
ntm_ctime (void)
{
	time_t	tm;
	char	*t;
	int	len;

	tm = time (0);
	t = ctime (&tm);
	len = strlen (t) - 1;
	if ('\n' == t[len])
		t[len] = '\0';		/* kill NewLine */
	return (t);
}

struct TmDriver NEAR TmNone = {
	"NoTimer",
	0,
	NULL,
	ntm_init,
	ntm_term,
	ntm_milli,
	ntm_hires,
	ntm_ctime,
	ntm_interval
};
