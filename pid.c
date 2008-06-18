/* --------------------------------- pid.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Automatic control. The PID method calculates the system response (output)
 * to an error signal (input) as the sum of three components which are termed
 * Proportional, Integral and Derivative. It is used by the auto-pilot system.
*/

#include "fly.h"


extern long FAR
pid_calc (F8PID *pid, long P, int interval)
{
	long	D, s;

	if (P < pid->Iband && P > -pid->Iband)
		pid->I += P*interval;
	else
		pid->I = 0;

	if (P < pid->Dband && P > -pid->Dband) {
		D = P - pid->Pprev;
		D = D*1000L/interval;
	} else
		D = 0;
	pid->Pprev = P;

	s = (P*pid->Kp + pid->I/pid->Ki + D*pid->Kd)/pid->factor;
#if 0
if (pid == EE(CC)->PIDroll) {
	STATS_TEMP2[0] = P;
	STATS_TEMP2[1] = P*pid->Kp/pid->factor;
	STATS_TEMP2[2] = pid->I/pid->Ki/pid->factor;
	STATS_TEMP2[3] = D*pid->Kd/pid->factor;
}
#endif
	if (s > pid->range)
		s = pid->range;
	else if (s < -pid->range)
		s = -pid->range;
	return (s);
}
