/* --------------------------------- plsound.h ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Header for plsound.c
*/

#ifndef FLY8_PLSOUND_H
#define FLY8_PLSOUND_H

struct beep {
	int	freq;
	int	milli;
	int	*list;		/* running list address */
	int	*list_id;	/* head of list address */
};

typedef struct beep BEEP;


struct plsextra {
	void	(FAR* Start) (int eff);
	void	(FAR* Stop) (void);
	BEEP	*beeps;
	Ulong	lasttime;
	Ulong	nexttime;
	int	playing;
	int	nbeeps;
};

extern void	FAR PlsPoll (int force);
extern int	FAR PlsBeep (int f, int milli);
extern int	FAR PlsList (int *list, int command);
extern int	FAR PlsEffect (int eff, int command, ...);
extern int	FAR PlsInit (char *options);
extern void	FAR PlsTerm (void);

#endif
