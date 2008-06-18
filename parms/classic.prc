/* --------------------------------- classic.prc ---------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* plane parameters: Classic spaceship...
*/

#include "parms.h"

15586,		/* weight [lb]						*/
F10(1.38),	/* Ixx as I/Weight in kg-m2/kg				*/
F10(8.14),	/* Iyy							*/
F10(9.20),	/* Izz							*/
F10(0.14),	/* Izx							*/

120,		/* liftoff [kts]					*/
V(9*G),		/* max positive vertical acceleration			*/
V(-6*G),	/* max negative vertical acceleration			*/
DV(34),		/* pitch rate at 300kts full elevators [ang/s]		*/
DV(180),	/* roll rate at 300 kts full ailerons [ang/s]		*/
V(1.00),	/* height   (eye above cg)				*/
V(3.00),	/* position (eye forward of cg) 			*/
D(-1),		/* onground pitch [ang]					*/
50000,		/* ceiling [ft]						*/
511,		/* stores: M61						*/
0,		/* stores: MK82						*/
0,		/* stores: spare					*/
0,		/* stores: spare					*/
0,		/* stores: spare					*/

F(0.70),	/* friction with brakes applied				*/
F(0.02),	/* friction when freewheeling				*/

I10(18000),	/* thrust (mil) [lbf]					*/
71,		/* sfc (mil)						*/
I10(29588),	/* thrust (ab) [lbf]					*/
217,		/* sfc (ab)						*/
6846,		/* fuel capacity [lb]					*/
D(0),		/* Engine rigging angle vs. plane			*/
D(0),		/* Engine offset angle vs. plane			*/
V(0),		/* Engine offset vs. cg					*/

V(27.87),	/* wings area [vm]					*/
V(10),		/* tip to tip [vm]					*/
V(3.5),		/* cbar, MAC, mean aerodynamic cord [vm]		*/
V( 0.00),	/* wing ac offset forward of cg				*/
V( 0.00),	/* wing ac offset above cg				*/
D(0),		/* Wing rigging angle vs. plane (+=leading edge high)	*/
D(0),		/* alpha where Cl=0					*/
F10( 2.0),	/* max Cl for wing foil					*/
F10(-1.5),	/* min Cl for wing foil					*/
F(-8.0/30.0),	/* flaps effective alpha/flaps				*/
F(0.4/D(20)),	/* flaps ClMax rate/Flaps [1/ang]			*/
F(1.0/D(25)),	/* LE flaps ClMax rate/Flaps [1/ang]			*/
93,		/* efficiency						*/
F(-0.004),	/* Cm0w of wing						*/

V(5.92),	/* tail area						*/
V(5.58),	/* tail span						*/
V(-3.00),	/* tail a.c. offset forward of c.g.			*/
V( 0.00),	/* tail a.c. offset above c.g.				*/
D(-2.5),	/* Tail rigging angle vs. wing (+=leading edge high)	*/
F(0.60),	/* tail volume						*/
F10(1.5),	/* tail max Cl						*/
F10(-1.5),	/* tail min Cl						*/

V(5.09),	/* rudder area						*/
V(3.0),		/* rudder span						*/
V(-3.00),	/* rudder a.c. offset forward of c.g.			*/
V( 0.00),	/* rudder a.c. offset above c.g.			*/
F10(1.5),	/* rudder max Cl (symmetrical)				*/

F(0.05),	/* parasitic drag: profile				*/
F(0.50),	/* parasitic drag: speed brakes				*/
F(0.10),	/* parasitic drag: gear					*/
F(0.002),	/* parasitic drag: each MK82				*/

D(20),		/* max flaps [ang]					*/
D(25),		/* max leFlaps [ang]					*/
D(40),		/* max spoilers [ang]					*/
D(25),		/* max elevators [ang]					*/
D(21.5),	/* max ailerons [ang]					*/
D(30),		/* max rudder [ang]					*/

D(10),		/* AutoFlaps: min aoa to engage				*/
F(100.0/D(15)),	/* AutoFlaps: flaps/aoa rate				*/
50,		/* AutoFlaps: max flaps authority (%)			*/

D(15),		/* AutoLEFlaps: min aoa to engage			*/
F(100.0/D(15)),	/* AutoLEFlaps: leFlaps/aoa rate			*/

V(120.0),	/* AutoElevators: speed factor.				*/

V(10.0),	/* AutoPedals: speed factor.				*/

F( 0.20),	/* Cydr	rudder sideforce				*/
F10(-8.00),	/* Cybeta vx damping					*/

F( 0.04),	/* Cm0	of whole plane					*/
F(-0.50),	/* Cmde	elevators effectiveness				*/
F10(-16.00),	/* Cmq	pitch damping					*/
F(-0.70),	/* Cmalpha wing/stabilizer induced pitch		*/
F10(-2.0),	/* Cmalphadot alpha rate induced pitch			*/

F( 0.15),	/* Clda	ailerons effectiveness				*/
F(-0.60),	/* Clp	roll damping					*/
F(-0.10),	/* Clbeta dihedral effect				*/
F( 0.00),	/* Cldr	roll from rudder				*/

F(-0.05),	/* Cndr	rudder effectiveness				*/
F(-1.00),	/* Cnr	yaw damping					*/
F( 0.10),	/* Cnbeta weathercock stability				*/
F(-0.01),	/* Cnda	ailerons induced yaw (negative = adverse)	*/
F( 0.00),	/* Cnp	roll induced yaw 				*/

HUD_CLASSIC,	/* HUD style						*/

/* these 10 options are for future use.
*/
	MODEL_CLASSIC,	/* flight dynamics model			*/
	0,		/* response (larger means slower)		*/
	0,		/* reserved					*/
	4,		/* turn speed (classic), lift (basic)		*/
	0,		/* reserved					*/
	-1,		/* c.g. height (calculated)			*/
	0,		/* reserved					*/
	0,		/* reserved					*/
	0,		/* reserved					*/
	0,		/* reserved					*/


/* these are the three autopilot PID parameter sets.
 *
 *		P,	Iband, Ki,	Dband, Kd,	 factor,	limit
*/
/* throttle */	40,	V(5), 10,	V(50), 400,	 1000*VONE,	 20,
/* pitch    */	40,	D(3), 100,	D(90), 100,	 100*D90/50,	 40,
/* roll     */	400,	D(10), 10,	D(30), 400,	-100*D90/50,	100,


/* Landing gear parameters. Up to 5 gear assemblies can be defined.
 *
 * x	gear tip position right of cg
 * y	gear tip position forward of cg
 * z	gear tip position above cg
 * dgmax max gear depression.
 * dtp	type depression at load P
 * P	gear precharge
 * Cv	gear damping
 * Steer angle at max pedals
 * ur	friction coeff when free rolling
 * ub	friction coeff when braking
 * us	friction coeff of side sliding
 *
 *	     x          y          z         dgmax     dtp       P
 *		Cv        Steer  ur       ub       us       raise lower
*/
/* nose  */  VV(0.00),  VV(3.00), -VV(2.30), VV(1.00), VV(0.10), V(G/10),
		V(   G),  D(40), F(0.02), F(0.00), F(0.60),  20,   25,

/* left  */ -VV(1.58), -VV(1.00), -VV(2.00), VV(0.75), VV(0.10), V(G/5),
		V(   G),  D(0),  F(0.02), F(0.65), F(0.60),  20,   25,

/* right */  VV(1.58), -VV(1.00), -VV(2.00), VV(0.75), VV(0.10), V(G/5), 
		V(   G),  D(0),  F(0.02), F(0.65), F(0.60),  20,   25,

/*       */  VV(0.00),  VV(0.00),  VV(0.00), VV(0.00), VV(0000), V(0.00),
		V(0.00),  D(0),  F(0.00), F(0.00), F(0.00),   0,    0,

/*       */  VV(0.00),  VV(0.00),  VV(0.00), VV(0.00), VV(0000), V(0.00),
		V(0.00),  D(0),  F(0.00), F(0.00), F(0.00),   0,    0,

0 /* dummy EOF */
