/* --------------------------------- plane.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common header for plane dynamics models.
*/

#ifndef FLY8_PLANE_H
#define FLY8_PLANE_H

#include "hud.h"

typedef struct btrail	BTRAIL;
struct btrail {
	BTRAIL		*next;
	LVECT		R;
	VECT		V;
	Uint		ms;
};

typedef struct e_plane E_PLANE;
struct e_plane {
	struct e_parms	*parms;		/* plane parameters */
	void		*private;	/* private model data */
	Ushort	flags;
#define	PF_AUTO		0x0001
#define	PF_ONGROUND	0x0002
#define PF_CHASE	0x0004
#define PF_KILL		0x0008
#define PF_FLAMEOUT	0x0010
#define PF_GLIMIT	0x0020
#define PF_STALL	0x0040
#define PF_AUTOFLAP	0x0080
#define PF_AUTORUDDER	0x0100
#define PF_AUTOELEVATOR	0x0200
#define PF_NOSTALL	0x0400
#define PF_LIMITED	0x0800
	Ushort	hud;
#define	HUD_ON		0x0001
#define	HUD_LADDER	0x0002
#define	HUD_BIG		0x0004
#define	HUD_FINE	0x0008
#define	HUD_XFINE	0x0010
#define	HUD_PLUS	0x0020
#define	HUD_CURSOR	0x0040
#define	HUD_VV		0x0080
#define	HUD_TARGET	0x0100
#define	HUD_DATA	0x0200
#define	HUD_RETICLE	0x0400
#define	HUD_ROSS	0x0800
#define	HUD_INFO	0x1000
#define	HUD_INFOM	0x7000
#define	HUD_DEFAULT	(HUD_TARGET|HUD_RETICLE)
#define	HUD_FULLHEADING	0x8000
	Ushort	hud1;
#define	HUD_TOP		0x0001
#define	HUD_LIMIT	0x0002
#define	HUD_CORNER	0x0004
#define	HUD_AALARM	0x0008
#define	HUD_VALARM	0x0010
#define	HUD_KNOTS	0x0020
/*#define	HUD_PANEL	0x0040*/
#define	HUD_ACCVECT	0x0080
#define	HUD_TYPE	0x0100
#define	HUD_TYPES	0x0700
#define	HUD_CLASSIC	(0*HUD_TYPE)
#define	HUD_FA18	(1*HUD_TYPE)
#define	HUD_F16		(2*HUD_TYPE)
#define	HUD_F15		(3*HUD_TYPE)
#define	HUD_ETHER	(4*HUD_TYPE)
#define	HUD_FLIR	(5*HUD_TYPE)
#define	HUD_PENDULUM	0x0800
#define	HUD_INAME	0x1000
#define	HUD_IDIST	0x2000
#define	HUD_THICK	0x4000
#define	HUD_BORDER	0x8000
	Ushort	hud2;
#define	HUD_ILS		0x0001
#define	HUD_VW 		0x0002
#define	HUD_ALTITUDE	0x0004
#define	HUD_SPEED	0x0008
#define	HUD_HEADING	0x0010
#define HUD_REALHEADING	0x0020		/* show real heding scale */
#define	HUD_HIDETGT	0x0040
#define	HUD_BETA	0x0080
#define HUD_XBREAK	0x0100
#define HUD_XGRID	0x0200
#define HUD_DIRECTOR	0x0400		/* show flight director */
#define HUD_WAYPOINT	0x0800
#define HUD_BTRAIL	0x1000		/* bullet trail history */
#define HUD_CALIBRATED	0x2000		/* calibrated airspeed */
#define HUD_TPOINTER	0x4000		/* target pointer */
#define HUD_VPOINTER	0x8000		/* variable size target pointer */
	Ushort	hud3;
#define	HUD_GVV		0x0001		/* show ghost VV */
#define	HUD_CUE		0x0002
#define	HUD_XVAR	0x0004
#define	HUD_TRUEHEADING	0x0008
	Ushort	hudmode;
#define	HM_LAND		0x0001
#define	HM_NAV		0x0002
#define	HM_DGFT		0x0004
#define	HM_CDIP		0x0008
#define	HM_DECLUTTER	0x0100
	Ushort	hdd;
#define HDD_INSTRUMENTS	0x0001
#define	HDD_NAV		0x0002		/* show nav data */
#define	HDD_COMPASS	0x0004
#define	HDD_SQRCOMPASS	0x0008
#define	HDD_ORTCOMPASS	0x0010
#define	HDD_PANEL	0x0020
	Ushort	ladder;
#define	LD_FIXED	0x0001		/* ladder fixed on waterline */
#define	LD_RIGHT	0x0002		/* numerals on the right only */
#define	LD_ERECT	0x0004		/* numerals do not roll */
#define	LD_COLOR	0x0008		/* steps are colored */
#define	LD_FUNNEL	0x0010		/* tips are inside */
#define	LD_SLANT	0x0020		/* steps are slanted */
#define	LD_ZENITH	0x0040		/* zenith/nadir symbols */
#define	LD_UNDER	0x0080		/* numerals under the step */
#define	LD_TIP0		0x0100		/* show tip on horizon */
#define	LD_HOLD		0x0200		/* hold ladder  */
#define	LD_HOLDROLL	0x0400		/* hold on heading */
#define	LD_SUN		0x0800		/* show sun symbol */
#define	LD_NEGTIP	0x1000		/* -ve pitch tip inverted */
	Ushort	radar;
#define	R_ON		0x0001
#define	R_LOCK		0x0002
#define	R_INTEL		0x0004
#define	R_INTELCC	0x0008
#define	R_MODE		0x0010
#define	R_MODES		0x0070
#define	R_SHOOT		0x0080
#define	R_SELECT3	0x0100
#define	R_SELECT20	0x0200
#define	R_SELECT5	0x0400
	Ushort	equip;
#define	EQ_GEAR		0x0001		/* gear deployed */
#define	EQ_GEAR1	0x0002		/* true if gear1 has ground contact */
#define	EQ_GEAR2	0x0004
#define	EQ_GEAR3	0x0008
#define	EQ_GEAR4	0x0010
#define	EQ_GEAR5	0x0020
	short	weapon;			/* weapon type */
#define	WE_M61		0x0001
#define	WE_MK82		0x0002
#define	WE_LAST		0x0002
#define	N_WE		5
	short	stores[N_WE];		/* weapons available */
#define MODEL_BASIC	0		/* opt[0] values */
#define MODEL_CLASSIC	1
#define MODEL_XPLANE	2
#define MODEL_YPLANE	3
#define MODEL_FPLANE	4
	Uchar	lamps[25];
#define LAMP_ON		0x01
#define LAMP_BLINK	0x02
#define LAMP_MASK	(LAMP_ON | LAMP_BLINK)
#define LAMP_OK		0x04

#define LAMP_GREEN	LAMP_ON
#define LAMP_BGREEN	LAMP_BLINK
#define LAMP_OKGREEN	LAMP_OK
#define LAMP_MGREEN	LAMP_MASK
#define LAMP_REDSHIFT	4
#define LAMP_RED	(LAMP_ON    << LAMP_REDSHIFT)
#define LAMP_BRED	(LAMP_BLINK << LAMP_REDSHIFT)
#define LAMP_OKRED	(LAMP_OK    << LAMP_REDSHIFT)
#define LAMP_MRED	(LAMP_MASK  << LAMP_REDSHIFT)
#define LAMP_ALL	(LAMP_MGREEN | LAMP_MRED)

#define LAMP_GLIMIT	0
#define LAMP_STALL	1
#define LAMP_FUEL	2
#define LAMP_GEAR	3
#define LAMP_ALT	4
#define LAMP_PULLUP	5
#define LAMP_DAMAGE	6
#define LAMP_EJECT	7

#define LAMP_SET_GREEN(n,s) \
	(EX->lamps[n] = (Uchar)((EX->lamps[n] & ~LAMP_MGREEN) | (s)))
#define LAMP_SET_RED(n,s) \
	(EX->lamps[n] = (Uchar)((EX->lamps[n] & ~LAMP_MRED) \
			| ((s) << LAMP_REDSHIFT)))
#define LAMP_SET_OFF(n) \
	(EX->lamps[n] &= ~LAMP_ALL)

	xshort		hudarea;	/* degrees from center to edge */
	xshort		hudFontSize;
	xshort		ils;		/* ils beacon id */
	Uxshort		ilsRange;
	ANGLE		ilsHeading;
	xshort		tapelen;	/* speed/alt scale range */
	xshort		hudshift;	/* hud y shift */
	xshort		ldgap;		/* ladder gap size */
	xshort		ldstep;		/* ladder step size */
	xshort		ldstep0;	/* ladder step 00 size */
	xshort		ldstepg;	/* ladder step 00 (gear down) size */
	xshort		ldtip;		/* ladder tip size */
	xshort		ldndash;	/* ladder -ve step n-dashes */
	long		fuel;		/* fuel left [*100] */
	xshort		fuelRate;	/* [*100] */
	xshort		throttle;	/* position [-100..100]*/
	xshort		afterburner;	/* position [0..100] */
	xshort		elevators;	/* position [0..100] */
	xshort		ailerons;	/* position [-100..100] */
	xshort		rudder;		/* position [-70..70] */
	xshort		flaps;		/* position [0..100] */
	xshort		leFlaps;	/* position [0..100] */
	xshort		spoilers;	/* position [0..100] */
	xshort		tElevators;	/* trim position [0..100] */
	xshort		tRudder;	/* trim position [0..100] */
	xshort		airbrake;	/* position [0..100] */
	xshort		brake;		/* position [0..100] */
	xshort		gear[5];	/* position [0..100] */
	xshort		power;		/* percent*100 [0..10000] */
	xshort		thrust;		/* lb_thrust/10 */
	xshort		Gforce;		/* for display */
	xshort		maxG;		/* for display */
	ANGLE		aoa;		/* for display */
	xshort		mach;		/* for display */
	short		StFont;
	Uchar		NEAR*  NEAR* StFontPtr;	/* StFonts[StFont] */
	xshort		StFontSize;
	OBJECT		*target;	/* aqcuired target */
	long		tid;		/* target id */
	F8PID		*PIDthrottle;
	F8PID		*PIDpitch;
	F8PID		*PIDroll;
	VECT		taccel;		/* target average acceleration */
	VECT		tspeed;		/* target average velocity */
	BTRAIL		*btrail;	/* bullet trail history */
	short		misc[20];	/* autopilot, history etc. */
#define EE(p)		(*(E_PLANE **)&(p)->extra)
#define	LIFETIME(p)	EE(p)->misc[0]	/* millisecs to next randomization */
#define	SPEED(p)	EE(p)->misc[1]	/* desired speed */
#define	HEADING(p)	EE(p)->misc[2]	/* desired heading */
#define	ALTITUDE(p)	EE(p)->misc[3]	/* desired altitude */
#define	NEWTGT(p)	EE(p)->misc[4]
};


typedef struct e_bomb E_BOMB;
struct e_bomb {
	LVECT		IP;
	long		timpact;
};

#define EBM(p)		(*(E_BOMB **)&(p)->extra)


struct e_parms {
	struct e_parms	*next;
	char	name[10];

	long	weight;			/* empty weight [lb] */
	xshort	Ixx;			/* Ixx as I/Weight in kg-m2/kg */
	xshort	Iyy;			/* Iyy */
	xshort	Izz;			/* Izz */
	xshort	Izx;			/* Izx */

	xshort	liftoff_speed;		/* [kts] */
	xshort	max_lift;		/* [vm] */
	xshort	min_lift;		/* [vm] */
	ANGLE	pitch_rate;		/* at 300 knots full elev. [ang/s] */
	ANGLE	roll_rate;		/* at 300 knots full ail. [ang/s] */
	xshort	eyez;			/* pilot eye above cg [vm] */
	xshort	eyey;			/* pilot eye forwad of cg [vm] */
	ANGLE	gpitch;			/* pitch on ground [ang] */
	long	ceiling;		/* operational ceiling [ft] */
	xshort	stores[N_WE];		/* count */

	xshort	brake_mu;		/* friction with brakes applied */
	xshort	wheel_mu;		/* friction when freewheeling */

	xshort	mil_thrust;		/* [lbf] */
	xshort	mil_sfc;		/* lb_fuel/(lb_thrust*hour) */
	xshort	ab_thrust;		/* [lbf] */
	xshort	ab_sfc;			/* lb_fuel/(lb_thrust*hour) */
	Ushort	fuel_capacity;		/* internal fuel [lb] */
	ANGLE	Ea;			/* Engine rig ang vs. plane [ang] */
	ANGLE	Eb;			/* Engine offset ang vs. plane [ang] */
	xshort	Er;			/* Engine offset vs. cg [vm] */

	xshort	wing_area;		/* wings area [vm] */
	xshort	wing_span;		/* tip to tip [vm] */
	xshort	wing_cord;		/* mean aerodynamic cord [vm] */
	xshort	ACy;			/* wing ac forward of cg [vm] */
	xshort	ACz;			/* wing ac upward of cg [vm] */
	ANGLE	Aoffset;		/* Wing rigging angle vs. plane [ang] */
	ANGLE	Cl0;			/* alpha where Cl=0 [ang] */
	xshort	maxCl;			/* max Cl for wing foil */
	xshort	minCl;			/* min Cl for wing foil */
	xshort	FEff;			/* flaps effective alpha/flaps */
	xshort	FEffCl;			/* flaps Clmax rate/DegFlaps */
	xshort	lefEffCl;		/* Clmax/LE flaps deg */
	xshort	efficiency_factor;	/* Oswald's efficiency factor */
	xshort	Cm0w;			/* wing foil Cm at 0 alpha */

	xshort	tail_area;		/* [vm] */
	xshort	tail_span;		/* [vm] */
	xshort	TACy;			/* tail ac forward of cg [vm] */
	xshort	TACz;			/* tail ac upward of cg [vm] */
	ANGLE	Toffset;		/* Tail rigging angle vs. wing [ang] */
	xshort	Tvol;			/* tail volume */
	xshort	TmaxCl;			/* max Cl for tail foil */
	xshort	TminCl;			/* min Cl for tail foil */

	xshort	rudd_area;		/* [vm] */
	xshort	rudd_span;		/* [vm] */
	xshort	RACy;			/* rudder ac forward of cg [vm] */
	xshort	RACz;			/* rudder ac upward of cg [vm] */
	xshort	RmaxCl;			/* max Cl for rudder foil */

	xshort	Cdp0;			/* parasitic drag: profile */
	xshort	Cds;			/* parasitic drag: speed brakes */
	xshort	Cdg;			/* parasitic drag: gear */
	xshort	CdMK82;			/* parasitic drag: each MK82 */

	ANGLE	MaxFlaps;		/* max flaps [ang] */
	ANGLE	MaxLEFlaps;		/* max LE flaps [ang] */
	ANGLE	MaxSpoilers;		/* max spoilers [ang] */
	ANGLE	MaxElevators;		/* max elevators [ang] */
	ANGLE	MaxAilerons;		/* max ailerons [ang] */
	ANGLE	MaxRudder;		/* max rudder [ang] */

	ANGLE	AFamin;			/* AutoFlaps: min aoa to engage */
	short	AFrate;			/* AutoFlaps: flaps/aoa rate */
	ANGLE	AFmax;			/* AutoFlaps: max flaps authority [%] */

	ANGLE	ALEFamin;		/* AutoLEFlaps: min aoa to engage */
	short	ALEFrate;		/* AutoLEFlaps: leFlaps/aoa rate */

	xshort	AErate;			/* AutoElevators: speed factor */

	xshort	APrate;			/* AutoPedals: speed factor */

	xshort	Cydr;			/* rudder sideforce */
	xshort	Cybeta;			/* vx damping */

	xshort	Cm0;			/* total Cm at 0 alpha */
	xshort	Cmde;			/* elevators effectiveness */
	xshort	Cmq;			/* pitch damping */
	xshort	Cmalpha;		/* alpha (stabilizer) induced pitch */
	xshort	Cmalphadot;		/* alpha rate induced pitch */

	xshort	Clda;			/* aileron effectiveness */
	xshort	Clp;			/* roll damping */
	xshort	Clbeta;			/* dihedral effect */
	xshort	Cldr;			/* roll from rudder */

	xshort	Cndr;			/* rudder effectiveness */
	xshort	Cnr;			/* yaw damping */
	xshort	Cnbeta;			/* weathercock stability */
	xshort	Cnda;			/* ailerons induced yaw */
	xshort	Cnp;			/* roll induced yaw */

	short	hudtype;		/* HUD_F16 etc. */

	short	opt[10];		/* options */
	F8PID	PIDthrottle;
	F8PID	PIDpitch;
	F8PID	PIDroll;
	struct gear {		/* gear tip position relative to cg */
		xshort	x;		/* + means right */
		xshort	y;		/* + means forward */
		xshort	z;		/* - means down */
		xshort	dgmax;		/* max strut deflection */
		xshort	dtp;		/* tyre deflection at P */
		xshort	P;		/* strut precharge force */
		xshort	Cv;		/* strut damping coeff. */
		ANGLE	emax;		/* Steering angle at full pedals. */
		xshort	ur;		/* rolling friction coeff. */
		xshort	ub;		/* full brake friction coeff. */
		xshort	us;		/* sideslip friction coeff. */
		xshort	rateup;		/* percent/sec raise rate */
		xshort	ratedn;		/* percent/sec lower rate */
	} gear[5];
};

#define EEP(p)	EE(p)->parms
#define EP	EEP(p)

#ifndef EX
#define EX	EE(p)
#endif

/* autop.c */
extern void	FAR SetKillCorrection (OBJECT *p, OBJECT *target, VECT R,
	int *tti);
extern void	FAR dynamics_auto (OBJECT *p);

/* engine.c */
extern void	FAR f16engine (OBJECT *p, xshort sos);

/* gear.c */
extern void	FAR LandGear (OBJECT *p, VECT F, VECT MM);
extern void	FAR LandGearDamp (OBJECT *p);

/* oplane.c */
#define DDshow(p,n,t,v)	if (CC==(p)) fDDshow (n, t, v)
#define CCshow(p,n,t,v)	if (CC==(p)&&(st.flags&SF_DEBUG)) fCCshow (n, t, v)
#define CFshow(p,t,v)	if (CC==(p)&&(st.flags&SF_DEBUG)) fCFshow (t, v)
#define CAshow(p,t,v)	if (CC==(p)&&(st.flags&SF_DEBUG)) fCAshow (t, v)
#define CVshow(p,t,v)	if (CC==(p)&&(st.flags&SF_DEBUG)) fCVshow (t, v)

extern void	FASTCALL FAR fDDshow (int frac, char *title, long value);
extern void	FASTCALL FAR fCCshow (int frac, char *title, long value);
extern void	FASTCALL FAR fCFshow (char *title, int value);
extern void	FASTCALL FAR fCAshow (char *title, int value);
extern void	FASTCALL FAR fCVshow (char *title, int value);

extern void	FAR CCnote (OBJECT *p, char *note);
extern void	FAR CCland (OBJECT *p);
extern void	FAR CCfly  (OBJECT *p);
extern int	FAR check_land (OBJECT *p);
extern int	FAR check_takeoff (OBJECT *p);
extern void	FAR supply (OBJECT *p, int mode);
extern int	FAR dampen (int old, int new, int factor);
extern int	FAR on_runway (OBJECT *p);
extern int	FAR dynamics_input (OBJECT *p);

#define DAMPEN(x, y, z)	(x = dampen ((x), (y), (z)))

/* om61.c */
#define	BULLETV		(1000*VONE)
#define	BULLETSCATTER	(D90/180)
extern int	FAR BulletSpeed (OBJECT *p, VECT V);

/* omk82.c */
extern int	FAR BombSpeed (OBJECT *p, VECT V);
extern int	FAR BombIP (LVECT R, VECT V, long tz, LVECT IP);

/* prm.c */
extern void 	(FAR* FAR flight_models[])(OBJECT *p, int action);
extern struct e_parms * FAR parms_get (char *pname);
extern void	FAR parms_free (void);

/* sixdof.c */
extern void	FAR SixDOF (OBJECT *p, VECT F, VECT MM, MAT I);

#endif
