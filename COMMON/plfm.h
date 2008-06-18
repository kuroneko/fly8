/* -------------------------------- plfm.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Include file for FM sound 
 *
 * FM support by Chris Collins (ccollins@pcug.org.au)
*/

#ifndef FLY8_PLFM_H
#define FLY8_PLFM_H

/* Defines for Soundblaster and Soundblaster Pro IO address
*/
#define LEFT_FM_STATUS      0x00    /* Pro only */
#define LEFT_FM_ADDRESS     0x00    /* Pro only */
#define LEFT_FM_DATA        0x01    /* Pro only */
#define RIGHT_FM_STATUS     0x02    /* Pro only */
#define RIGHT_FM_ADDRESS    0x02    /* Pro only */
#define RIGHT_FM_DATA       0x03    /* Pro only */
#define DSP_RESET           0x06
#define FM_STATUS           0x08
#define FM_ADDRESS          0x08
#define FM_DATA             0x09

#define ADLIB_FM_STATUS     0x388
#define ADLIB_FM_ADDRESS    0x388
#define ADLIB_FM_DATA       0x389

/* FM Instrument definition
*/
typedef struct FM_Instrument {
	Uchar	SoundCharacteristic[2];
	Uchar	Level[2];
	Uchar	AttackDecay[2];
	Uchar	SustainRelease[2];
	Uchar	WaveSelect[2];
	Uchar	Feedback[1];
	Uchar	filler[1];
} FM_Instrument;

#endif /* #ifndef FLY8_PLFM_H */
