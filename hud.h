/* --------------------------------- hud.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Head Up Display header
*/

#ifndef FLY8_HUD_H
#define FLY8_HUD_H

#include "fly.h"


#ifndef EX
#define EX	EE(p)
#endif

#define	VVPERIOD	2000			/* vv delay milliseconds */
#define VVDELAY		EX->misc[9]		/* vv temp storage */

/* For HUDs, the following are sizes measured in fraction of HUD size
 * (length from center to edge).
*/

#define MAPBOX	((int)(FONE*22L/140))		/* intel mode box size */

/* These are for all
*/

#define RSELECT3	FCON( 22/140.0)		/* target selection radius */
#define RSELECT20	FCON(140/140.0)		/* target selection radius */
#define RSELECT5	FCON( 35/140.0)		/* target selection radius */

#define SVV		FCON(15/140.0)		/* vv size */
#define RVV		FCON( 9/280.0)		/* vv radius */

#define PULLUPCUE	SVV
#define PULLUPCUEGAP	(SVV/3)
#define PULLUPCUETIP	(SVV+SVV/4)

#define IPDIAMOND	FCON(0.03)

/* These are for the fa18
*/

#define F18HEAD		FCON(45/140.0)		/* heading scale height */
#define F18HEADG	FCON(25/140.0)		/* -"- when gear down */
#define F18HEADS	FCON(61/140.0)		/* -"- size */

#define F18ALT		FCON(52/140.0)		/* altitude box position */
#define F18SPD		FCON(62/140.0)		/* speed box position */

#define F18RAIM		FCON(22/140.0)		/* aiming reticule radius */
#define F18RAIMC	FCON(50/60.0)		/* -"- circle radius ratio */
#define F18TBOX		FCON( 8/140.0)		/* target designator size */

#define F18GPSIZE	FCON(145/140.0)		/* groung pointer radius */
#define F18GPTICK	-FCON(0.03)		/* tick size */

#define F18WEAPON	FCON(100/140.0)		/* weapon name position */

#define F18LINE0	FCON(55/140.0)		/* base of first text line */
#define F18GPPOS	FCON(0)			/* groung pointer center */
#define F18LINE		FCON(12/140.0)		/* text line spacing */

/* These are for the f16
*/

#define F16CNTRG	FCON( 75/232.0)		/* alt/spd center (land) */
#define F16CNTR		FCON(125/232.0)		/* alt/spd center */
#define F16CNTRDG	FCON(160/232.0)		/* alt/spd center (dgft) */

#define F16HEAD		FCON( 55/232.0)		/* heading scale height */
#define F16HEADS	FCON( 50/232.0)		/* -"-  -"- size */
#define F16HEADTOP	FCON( 60/232.0)		/* max heading above vv */

#define F16ALT		FCON(132/232.0)		/* altitude scale position */
#define F16SPD		FCON(132/232.0)		/* speed scale position */
#define F16RDR		FCON(105/232.0)		/* radar scale position */

#define F16GPSIZE	FCON( 95/232.0)		/* groung pointer radius */
#define F16GPPOS	FCON(175/232.0)		/* groung pointer center */
#define F16GPTICK	FCON(-0.08)		/* tick size */

#define F16RAIM		FCON(31/232.0)		/* aiming reticule radius */
#define F16RAIMC	FCON(0.88)		/* -"- circle radius ratio */
#define F16RASPECT	FCON(0.1)		/* aspect angle marker size */
#define F16RPIP		FCON(0.16)		/* piper radius ratio */
#define F16RBUL		FCON(3.5/232.0)		/* bullet piper radius */
#define F16TBOX		FCON(13.5/232.0)	/* target designator size */

/* These are for the f15
*/

#define F15CNTR		FCON( 55/295.0)		/* alt/spd center */

#define F15HEADL 	FCON( 54/295.0)		/* heading scale height */
#define F15HEADH 	FCON(120/295.0)		/* -"- in high position */
#define F15HEADS 	FCON(210/295.0)		/* -"-  -"- size */

#define F15ALT		FCON(180/295.0)		/* altitude scale position */
#define F15SPD		FCON(180/295.0)		/* speed scale position */
#define F15RDR		FCON(140/295.0)		/* radar scale position */

#define F15RDRS		FCON(170/295.0)		/* radar scale size */

#define F15RAIM		FCON(40/250.0)		/* aiming reticule radius */
#define F15RAIMC	FCON(80/90.0)		/* -"- circle radius ratio */
#define F15TBOX		FCON(16/250.0)		/* target designator size */

/* These are for full-frame ether.
*/

#define ETHERALT	FCON(0.6)
#define ETHERSPD	FCON(0.6)

#endif
