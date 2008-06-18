/* --------------------------------- pc8254.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Definitions for the timer chip 8254. Used for timekeeping and speaker
 * control.
*/

#ifndef FLY8_PC8254_H
#define FLY8_PC8254_H

#define CHANNEL_0	0x0040		/* system tick */
#define	CHANNEL_2	0x0042		/* speaker tone */
#define	COMMAND_REG	0x0043
#define WRITE_CH0	0x0036
#define WRITE_CH2	0x00b6
#define READ_SPECIAL	0x00c2
#define	PORT_B		0x0061		/* speaker on/off control */
#define	XTAL		1193182L	/* some call it 1,193,000 */
#define TIMER_MODES	0x000e
#define TIMER_MODE	0x0002
#define TIMER_OUT	0x0080
#define TIMER_PERIOD	0x0000ffffL

#endif
