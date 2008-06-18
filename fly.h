/* --------------------------------- fly.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* structures used by the system.
*/

#ifndef FLY8_FLY_H
#define FLY8_FLY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#ifndef LOCAL_FUNC
#define LOCAL_FUNC	static
#endif

#include "config.h"
#include "keymap.h"
#include "shape.h"
#include "objects.h" 
#include "stats.h"
#include "colors.h"

#ifndef rangeof
#define rangeof(array)	(sizeof(array)/sizeof(array[0]))
#endif

#define	BUFLEN	256		/* size of a display list block */

#ifndef SHORT_TYPE
#define SHORT_TYPE	short
#endif

#ifndef ANGLE_TYPE
#define ANGLE_TYPE	short
#endif

typedef unsigned char		Uchar;
typedef unsigned int		Uint;
typedef unsigned short		Ushort;
typedef unsigned long		Ulong;
typedef SHORT_TYPE		xshort;
typedef unsigned SHORT_TYPE	Uxshort;

typedef ANGLE_TYPE		ANGLE;
typedef xshort			VECT[3];
typedef long			LVECT[3];
typedef ANGLE			AVECT[3];
typedef xshort			MAT[3][3];

#define X	0
#define Y	1
#define Z	2

#define	HEAD	0
#define	TAIL	1

#define QUEUE(q,m) \
	if ((q)[HEAD])			\
		(q)[TAIL]->next = (m);	\
	else				\
		(q)[HEAD] = (m);	\
	(q)[TAIL] = (m)

typedef struct vertex	VERTEX;
typedef struct shape	SHAPE;
typedef struct object	OBJECT;
typedef Ushort		BUFLINE;
typedef struct buffer	BUFFER;
typedef struct device	DEVICE;
typedef struct pointer	POINTER;
typedef struct screen	SCREEN;
typedef struct view	VIEW;
typedef struct window	WINDOW;
typedef struct viewport	VIEWPORT;
typedef struct player	PLAYER;
typedef struct packet	PACKET;
typedef struct netport	NETPORT;
typedef struct msg	HMSG;
typedef struct hdd	HDD;
typedef struct pid	F8PID;
typedef struct macro	MACRO;
typedef struct menu	MENU;

struct vertex {
	VECT	V;
	Ushort	flags;
};

struct shape {
	VERTEX	*v;
	Uxshort	extent;
	Ushort	flags;
	long	weight;		/* grams */
	xshort	drag;		/* drag factor */
	Ushort	color;
};

#define	SH(p)		(st.bodies[p->name]->shape)
#define TITLE(p)	(st.bodies[p->name]->title)

struct pid {
	long	Kp;
	long	Iband;
	long	Ki;
	long	Dband;
	long	Kd;
	long	factor;
	long	range;
	long	Pprev;
	long	I;
};

struct macro {
	Ushort	name;
#define KEYUNUSED		0
	Ushort	len;
	Ushort	*def;
};

struct menu {
	int	letter;
	char	*text;
};

typedef struct e_imported E_IMPORT;
struct e_imported {
	Ulong	lasttime;
	Ulong	timeout;
	short	misc[5];
};

#define EIM(p)	(*(E_IMPORT **)&(p)->extra)


struct object {
	OBJECT		*next;
	OBJECT		*prev;
	void		*extra;
	OBJECT		*owner;
	PLAYER		*rplayer;
	POINTER		*pointer;
	VIEWPORT	*viewport;
	long		id;		/* local object id */
	long		rid;		/* remote object id */
	long		ownerid;	/* local object id of owner-object */
	Ulong		rtime;		/* remote time */
	Ulong		tob;		/* time of birth */
	Ulong		tom;		/* time of maturity */
	Ulong		maint_rate;	/* update rate */
	Ulong		maint_last;	/* last send_maint() */
	LVECT		R;		/* position		[earth] */
	VECT		V;		/* velocity		[earth] */
	VECT		A;		/* acceleration		[earth] */
	VECT		vb;		/* velocity		[body] */
	VECT		ab;		/* acceleration		[body] */
	AVECT		dda;		/* angular accels	[body] */
	AVECT		da;		/* angular rates	[body] */
	AVECT		dae;		/* Euler rates		[earth] */
	AVECT		a;		/* Euler angles		[earth] */
	xshort		sinx;
	xshort		cosx;
	xshort		siny;
	xshort		cosy;
	xshort		sinz;
	xshort		cosz;
	MAT		T;		/* body->earth xform matrix */
	short		home;		/* home ils beacon id */
	xshort		longitude;
	xshort		longmin;
	xshort		latitude;
	xshort		latmin;
	short		name;
	Ushort		color;
#define	FOREVER		0x7fff
	short		time;
#define	TIMEPSEC	100
	Ushort		flags;
#define F_VISIBLE	0x0001		/* permanent: */
#define F_IMPORTED	0x0002
#define F_EXPORTED	0x0004
#define F_MAINT		0x0008
#define F_STEALTH	0x0010
#define F_CC		0x0020
#define F_LAND		0x0040		/* part of landscape */
#define F_FRIEND	0x0080
#define F_ALIVE		0x0200		/* transitional: */
#define F_NEW		0x0400
#define F_DEL		0x0800
#define F_HIT		0x1000
#define F_KEEPNAV	0x2000
#define F_DONE		0x4000
#define F_MOD		0x8000
	Ushort		gpflags;
#define GPF_PILOT	0x0001
	Ushort		shflags;	/* copy of SH flags */
	xshort		speed;
	short		e_type;
#define ET_IMPORTED	0x0001
#define ET_PLANE	0x0002
#define ET_BOMB		0x0003
#define ET_MAX		0x0003
#define IS_PLANE(p)	(ET_PLANE == (p)->e_type)
	short		score;
	short		damage;
	short		damaging;
	short		misc[5];
};

struct buffer {
	BUFFER		*next;
	BUFLINE		*p;		/* first free */
	BUFLINE		first[1];	/* start of data array */
};

#define	T_MASK		0xe000U
#define	T_BIT		0x2000U
#define	T_COLOR		(0*T_BIT)
#define	T_DRAW		(1*T_BIT)
#define	T_MOVE		(2*T_BIT)
#define	T_ELLIPSE	(3*T_BIT)
#define	T_CLEAR		(4*T_BIT)
#define	T_POLYGON	(5*T_BIT)
#define	T_OP		(7*T_BIT)
#define	T_NOP		0U
#define	T_MSET		1U
#define	T_MOR		2U
#define	T_MXOR		3U
#define	T_MPUSH		4U
#define	T_MPOP		5U
#define	T_NOERASE	6U
#define	T_ERASE		7U


struct pointer {
	char	*name;
	Ushort	flags;
#define PF_PRESENT	0x0001
#define PF_INITED	0x0002
	struct PtrDriver NEAR* control;
#define	NANALOG	30
/* Channel allocation:
 * 0	roll clockwise
 * 1	pitch up
 * 2	rudder right
 * 3	throttle
 * 4	trim pitch up
 * 5	trim rudder right
 * 6	flaps
 * 7	spoilers
 * 8	speed brakes
 * 9	ground brakes
 *10	analog hat
*/
	xshort	a[NANALOG];	/* analog -100...+100 */
	xshort	l[NANALOG];	/* last value of a[] */
	Uxshort	low[NANALOG];	/* calibration */
	Uxshort	c[NANALOG];	/* calibration */
	Uxshort	high[NANALOG];	/* calibration */
	Uxshort	play[NANALOG];	/* calibration */
	short	b[30];		/* digital commands */
#define	NBTNS	10+26+26
	Ushort	btn[NBTNS];	/* buttons flags */
#define	NOPTS	30
	int	opt[NOPTS];
#define IFA1D	0		/* reserved options */
#define IFA1F	1
#define IFA2D	2
#define IFA2F	3
#define IFA3D	4
#define IFA3F	5
#define IFA4D	6
#define IFA4F	7
#define IFA5D	8
#define IFA5F	9
#define IGOPTS	10
#define IPFREE	11		/* available for private use */
};

/* This is the lowest level, the phisical device being used.
*/
struct device {
	DEVICE	*next;
	char	*name;
	void	*pdevice;	/* private device data */
	int	mode;		/* vga bios mode etc. */
	int	colors;		/* number of colors */
	int	left;		/* physical left */
	int	top;		/* physical top */
	int	right;		/* physical right */
	int	bottom;		/* physical bottom */
	int	sizex;		/* physical horizontal pixels */
	int	sizey; 		/* physical vertical pixels */
	int	npages;		/* how many display pages */
	int	lengx;		/* millimeter */
	int	lengy;		/* millimeter */
	int	flags;		/* driver dpivate */
};

/* This defines an area inside the device for drawing the window.
*/
struct screen {
	DEVICE	*device;
	void	*pscreen;	/* private screen data */
	xshort	left;		/* physical left margin offset */
	xshort	top;		/* physical top margin offset */
	xshort	sizex;		/* physical horizontal pixels */
	xshort	sizey;		/* physical vertical pixels */
	Ushort	FgColor;
	Ushort	BgColor;
	Ushort	BoColor;	/* border */
};

/* This is the NDC abstraction that all drawing is done to.
*/
struct window {
	int	orgx;	/* all in normalized coordinates [0...1] */
	int	orgy;
	int	maxx;	/* left=orgx-maxx right =orgx+maxx */
	int	maxy;	/* top =orgy-maxy bottom=orgy+maxy */
};

/* This is the real world rectangle that the viewer is looking through.
 * what is seen inside it will be darwn on the "windowd".
*/
struct viewport {
	Ushort	flags;
#define	VF_MIRROR	0x0001
	int	x;	/* NDC normally 0.0 */
	int	y;	/* NDC normally 0.0 */
	int	z;	/* NDC zoom, normally 0.5, must >= maxx and maxy! */
	int	maxx;	/* NDC vp width normally 0.5 */
	int	maxy;	/* NDC vp height normally 0.5 */
			/* distz and shift in units of VONE*VONE */
	int	distz;	/* eye to vp in viewers coords */
	int	shift;	/* eye x shift (v coords) in stereo */
	int	eyex;	/* viewer coords: */
	int	eyey;	/* eye position relative to viewer object origin */
	int	eyez;
	ANGLE	rotx;	/* eye direction relative to viewer origin */
	ANGLE	roty;
	ANGLE	rotz;
	int	zoom;	/* zoom count */
};

struct	view {
	VIEWPORT	*viewport;	/* camera: world -> 2D */
	WINDOW		*window;	/* camera -> NDC window */
	SCREEN		*screen;	/* window -> physical screen */
};

typedef struct body BODY;
struct body {
	int	name;
	Ushort	flags;
#define BO_DYNSHAPE	0x0001
	char	*title;
	SHAPE	*shape;
	int	(FAR* init) (BODY *b);
	void	(FAR* term) (BODY *b);
	int	(FAR* create) (OBJECT *object);
	void	(FAR* delete) (OBJECT *object);
	void	(FAR* dynamics) (OBJECT *object);
	void	(FAR* hit) (OBJECT *object, int speed, int extent,
			int damaging);
};

#define	LADDRESS	6
#define	LNAME		20

struct player {
	PLAYER	*next;
	Ushort	flags;
#define	PL_ACTIVE	0x0001
#define	PL_PENDREQUEST	0x0002
#define	PL_PENDBOSS	0x0004
#define	PL_PENDCONFIRM	0x0008
#define	PL_PEND	(PL_PENDREQUEST|PL_PENDCONFIRM|PL_PENDBOSS)
#define	PL_PLAYING	0x0010
#define	PL_RECEIVE	(PL_PENDCONFIRM|PL_PLAYING)
#define	PL_SEND	(PL_PLAYING)
#define	PL_NOTIDLE	(PL_ACTIVE|PL_PEND|PL_PLAYING)
#define	PL_FRIEND	0x0020
	short	netport;
	Uchar	address[LADDRESS];
	char	name[LNAME];
	char	team[LNAME];
	short	ComVersion;			/* net version */
	PACKET	*incoming;
	PACKET	*tail;
	Ulong	timeout;
	long	rtime;				/* RemoteTime-LocalTime */
	short	rtimeErr;			/* integrated error */
};

struct netport {
	Ushort			flags;
#define	NP_ON		0x0001
#define	NP_BCAST	0x0002
#define	NP_PACKING	0x0004
	Ushort			packlen;
	short			netport;
	char			unit;
	int			nplayers;	/* playing users */
	struct NetDriver	*NetDriver;
	PACKET			*incoming[2];
	PACKET			*outgoing;	/* async send */
	PACKET			*outpak;	/* packed packet */
};

#define	PAKPACKLEN	1500			/* max pack len */

struct packet {
	PACKET	*next;		/* must be first! */
	Uchar	*data;		/* acquired packet data memory */
	Uchar	*address;	/* where address is in 'data' */
	Uchar	*raw;		/* where real packet starts in 'data'*/
	Ulong	arrived;	/* arrival time */
	Ushort	flags;		/* running length if incomming */
	short	netport;
	short	length;		/* message length */
	Ushort	size;		/* raw data size */
};

struct SysDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (char *options);
	void	(FAR* Term) (void);
	void	(FAR* Poll) (void);
	Ulong	(FAR* Disable) (void);
	void	(FAR* Enable) (Ulong i);
	void	(FAR* Shell) (void);
	void	(FAR* BuildFileName) (char *FullName, char *path, char *name,
					char *ext);
};

struct TmDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (char *options);
	void	(FAR* Term) (void);
	Ulong	(FAR* Milli) (void);
	int	(FAR* Hires) (void);
	char *	(FAR* Ctime) (void);
	Ulong	(FAR* Interval) (int mode, Ulong res);
#define TMR_PUSH	0x0001
#define TMR_POP		0x0002
#define TMR_READ	0x0004
#define TMR_SET		0x0008
#define	TMR_START	(TMR_PUSH|TMR_SET)
#define TMR_STOP	(TMR_READ|TMR_POP)
#define TMR_RESTART	(TMR_READ|TMR_SET)
};

struct KbdDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (char *options);
	void	(FAR* Term) (void);
	int	(FAR* Read) (void);
	int	(FAR* Getch) (void);
	int	(FAR* Wait) (void);
};

struct SndDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (char *options);
	void	(FAR* Term) (void);
	void	(FAR* Poll) (int force);
	int 	(FAR* Beep) (int f, int milli);
	int 	(FAR* Effect) (int eff, int command, ...);
#define EFF_HIT			1	/* eff */
#define EFF_M61_SHOOT		2
#define EFF_MK82_EXPLODE	3
#define EFF_MK82_SHOOT		4
#define EFF_NO_POINTER		5
#define EFF_BEEP		6
#define EFF_MSG			7
#define EFF_ENGINE		8
#define EFF_GONE		9
#define EFF_HELLO		10
#define EFF_NOTICE		11
#define EFF_GEAR		12
#define EFF_ALARM		13
#define EFF_WARN		14
#define EFF_DAMAGE		15
#define SND_OFF			0	/* command */
#define SND_ON			1
#define SND_PARMS		2
	int 	(FAR* List) (int *list, int command);
};

struct GrDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	DEVICE	*devices;
	int	(FAR* Init) (DEVICE *dev, char *options);
	void	(FAR* Term) (DEVICE *dev);
	void	(FAR* MoveTo) (Uint x1, Uint y1);
	void	(FAR* DrawTo) (Uint x2, Uint y2, Uint c);
	int	(FAR* SetVisual) (int page);
	int	(FAR* SetActive) (int page);
	void	(FAR* Clear) (Uint x, Uint y, Uint sx, Uint sy, Uint color);
	int	(FAR* SetWriteMode) (int mode);
	int	(FAR* SetPalette) (int index, long color);
	void	(FAR* Ellipse) (Uint x, Uint y, Uint rx, Uint ry, Uint c);
	void	(FAR* Polygon) (int npoints, BUFLINE *points, Uint color);
	void	(FAR* Flush) (void);
	int	(FAR* Shutters) (int eye);
};

struct PtrDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (POINTER *p, char *options);
	void	(FAR* Term) (POINTER *p);
	int	(FAR* Cal) (POINTER *p);
	int	(FAR* Center) (POINTER *p);
	int	(FAR* Read) (POINTER *p);
	void	(FAR* Key) (POINTER *p, int key);
};

struct NetDriver {
	char	*name;
	Ushort	flags;
	void	*extra;
	int	(FAR* Init) (NETPORT *port, char *options);
	void	(FAR* Term) (NETPORT *port);
	int	(FAR* Send) (NETPORT *port, PACKET *pack);
	int	(FAR* Poll) (NETPORT *port, int poll);
};

struct netname {
	struct netname	*next;
	char		*name;
};

struct msg {
	HMSG		*next;
	char		*text;
	Ulong		timeout;	/* time to delete */
	Uint		flags;
#define MSG_WARN	0x0001
#define MSG_ERR		0x0002
};

#define NBUFS		2		/* must be 2 for now */

struct hdd {
	short		type;
#define	HDT_FRONT	 0
#define	HDT_NONE	 1
#define	HDT_REAR	 2
#define	HDT_MAP		 3
#define	HDT_RADAR	 4
#define	HDT_TARGET	 5
#define	HDT_PAN		 6
#define	HDT_GAZE	 7
#define	HDT_CHASE	 8
#define	HDT_FOLLOW	 9
#define	HDT_HUD		10
#define	HDT_UPFRONT	11
#define	HDT_PANEL	12
#define	HDT_RIGHT	13
#define	HDT_LEFT	14
#define	HDT_STORES	15
#define	HDT_LAMPS	16
#define	HDT_MIRROR	17
	Ushort		flags;
#define HDF_ON		0x0001
#define HDF_MONO	0x0002		/* monochrome */
	Ushort		FgColor;
	Ushort		BgColor;
	Ushort		BoColor;	/* border */
	VIEW		view;
	BUFFER		*bufs[NBUFS];	/* HDD window */
};
#define NVIEWERS	10

struct status {
	struct SysDriver	NEAR* system;
	struct TmDriver		NEAR* time;
	struct GrDriver		NEAR* graphics;
	struct SndDriver	NEAR* sound;
	struct KbdDriver	NEAR* keyboard;
	BODY		**bodies;
	char		*iname;			/* init file */
	char		*mname;			/* macros file */
	char		*vmdname;		/* video modes file */
	char		*lname;			/* log file name */
	char		*navname;		/* nav file name */
	char		*lndname;		/* landscape file name */
	char		*fdir;			/* support files directory */
	char		*ptrname;		/* pointer device name */
	char		*grname;		/* video device name */
	char		*grmname;		/*    and mode */
	char		*sndname;		/* sound device name */
	char		*kbdname;		/* keyboard device name */
	struct netname	*netnames;		/* net drivers */
	char		*ptype;			/* plane type */
	char		*dtype;			/* drone type */
	char		*options;		/* options for 'create_obj' */
	char		*nikname;		/* my handle */
	char		*teamname;		/* my team name */
	char		*homename;		/* my nav home name */
	char		*timeropts;
	char		*initkeys;		/* startup keystrokes */
	Ulong		big_bang;		/* keep relative time */
	Ulong		present;
	Ulong		lasttime;
#define	OBJECT_TIMEOUT	(4*1000L)
	Ulong		ObjectTimeout;
#define	PLAYER_TIMEOUT	(8*1000L)
	Ulong		PlayerTimeout;
#define	DRONE_RATE	(8*1000L)
	Ulong		Drone_next;		/* drone takeoff delay */
#define	OBJECT_REFRESH	(OBJECT_TIMEOUT/4)
	Ulong		Refresh_next;
	Ulong		ShutdownTime;
	Ulong		test_start;
#define MIN_FRAME_RATE	10
	Ulong		frame_rate;		/* ms */
	Ulong		maint_rate;		/* ms */
	Ulong		AutoConnect_rate;	/* ms */
	Ulong		AutoConnect_next;
	Ulong		LogFlush_rate;
	Ulong		LogFlush_next;
	long		mscore;
	Ulong		nbullets;
	Ushort		ComVersion;
	Ushort		nbuffers;
	Ushort		maxbuffers;
	Ushort		maxrecall;
	Ushort		flags;
#define	SF_BLANKER	0x0001
#define	SF_LANDSCAPE	0x0002
#define	SF_SKY		0x0004
#define	SF_VERBOSE	0x0008
#define	SF_INTERACTIVE	0x0010
#define	SF_SIMULATING	0x0020
#define	SF_INITED	0x0040
#define	SF_PAUSED	0x0080
#define	SF_LISTS	0x0f00
#define	SF_MODES	0x0100
#define	SF_HUD		0x0200
#define	SF_HELP		0x0300
#define	SF_NET 		0x0400
#define	SF_STATS	0x0500
#define	SF_COLORS	0x0600
#define SF_FONT		0x0700
#define SF_PAUSEMSG	0x1000
#define SF_DEBUG	0x2000
#define SF_USECLEAR	0x4000
#define	SF_MAIN		0x8000
	Ushort		flags1;
#define	SF_USEG		0x0001
#define	SF_WIDENT	0x0002
#define	SF_DBUFFERING	0x0004
#define	SF_TESTING	0x0008
#define	SF_STEREOREV	0x0010
#define	SF_EXTVIEW	0x0020
#define	SF_IDENT	0x0040
#define	SF_INFO		0x0080
#define	SF_SMOKE	0x0100
#define SF_TERM		0x0200
#define	SF_HUDINFRONT	0x0400
#define	SF_ASYNC	0x0800		/* asynchronous process active */
#define	SF_SOLIDSKY	0x1000
#define	SF_GRADEDSKY	0x2000
	Ushort		flags2;
#define SF_CLEAR1	0x0001		/* multi-buffers Clear requests */
#define SF_CLEAR2	0x0002
#define SF_CLEAR3	0x0004
#define SF_CLEAR4	0x0008
#define SF_CLEAR	0x000f
	Ushort		sounds;
#define	SS_WARN		0x0001
#define	SS_ALARM	0x0002
	Ushort		debug;
#define	DF_TRACE	0x0001
#define DF_GPW		0x1000
#define DF_GPX		0x2000
#define DF_GPY		0x4000
#define DF_GPZ		0x8000
	Ushort		network;
#define	NET_ON		0x0001
#define	NET_NOBCAST	0x0002
#define	NET_INITED	0x0004
#define	NET_AUTOACCEPT	0x0010
#define	NET_AUTODECLINE	0x0020
#define	NET_AUTOCONNECT	0x0040
#define	NET_AUTOREPLY	(NET_AUTOACCEPT|NET_AUTODECLINE|NET_AUTOCONNECT)
	short		ntargets;
	short		extview;
	short		info;
	short		stereo;		/* vision type */
#define VIS_MONO		0
#define VIS_STEREOSCOPIC	1
#define VIS_REDBLUE		2
#define VIS_ALTERNATING		3
	short		windows;	/* windows configuration */
#define WIN_TEXT	0
#define WIN_FULL	1
#define WIN_LANDSCAPE	2
#define WIN_PORTRATE	3
#define WIN_SQUARE	4
#define WIN_WIDE	5
#define WIN_PANORAMA	6
#define WIN_ETHER	7
	xshort		paralax;	/* stereo eye shift. MAX 100*VONE! */
	xshort		focus;		/* stereo eye focus distance */
	xshort		gap;		/* gap between stereo frames: 1/gap */
	short		quiet;
	OBJECT		*owner;
	BUFFER		*bufs[NBUFS];	/* main window */
	BUFFER		*buf[2];	/* current writable buffer */
	BUFLINE		*buf_p;		/* current writable bufline */
	short		buf_avail;	/* available entries in st.buf*/
	short		which_buffer;
	short		nobjects;
	long		object_id;
	OBJECT		*world[2];	/* all objects in world */
#define CO	st.world[0]
#define COT	st.world[1]
	OBJECT		*land[2];	/* all objects in landscape */
#define CL	st.land[0]
#define CLT	st.land[1]
	OBJECT		*viewer;	/* origin of view */
#define CV	st.viewer
	OBJECT		*control;	/* object under user control */
#define CC	st.control
	VIEW		view[1];	/* active view */
#define CVIEW	st.view
#define CP	st.view->viewport
#define CW	st.view->window
#define CS	st.view->screen
#define CD	CS->device
#define NHDD	10
	HDD		hdd[NHDD];	/* HDD windows */
	int		interval_max;
	int		interval;	/* milliseconds of last frame */
	int		dither;		/* rand () % 1000 */
	POINTER		*pointer;	/* kludge for oplane.c */
	int		StFont;
	Uchar	NEAR* NEAR* StFontPtr;	/* StFonts[StFont] */
	int		StFontSize;
	int		landx;		/* land square: x and y */
	int		landy;
	int		gravity;	/* meters/sec/sec */
	HMSG		*msg;		/* message queue */
	int		drones;		/* number of drones */
	int		killers;	/* how many drones are killers */
	int		SkyLines;	/* number of lines in sky */
	short		home;		/* home base nav point */
	Ushort		btnmode;	/* buttons shift mode */
	short		nMacros;	/* max number of keyboard macros */
	long		lineno;
	int		p_name;		/* plane object name */
	int		d_name;		/* drone object name */
#define NCOLORS	16
	int		colors[NCOLORS];
	Ulong		palette[NCOLORS];
	Ushort		assign[64];
#define CA_IFG		0
#define CA_MFG		1
#define CA_WFG		2
#define CA_CFG		3
#define CA_HFG		4
#define CA_HFGI		5
#define CA_HBO		6
#define CA_SLEFT	7
#define CA_SRIGHT	8
#define CA_SBOTH	9
#define CA_GROUND	10
#define CA_DULL		11
#define CA_FAINT	12
#define CA_SKY		13
#define CA_FRIEND	14
#define CA_FOE		15
#define CA_HELP		16
#define CA_FIRE1	17
#define CA_FIRE2	18
#define CA_MENU		19
#define CA_MENUH	20
#define CA_SUN		21
#define CA_EARTH	22
#define CA_COMPASS	23
#define CA_MSGERR	24
#define CA_MSGWARN	25
#define CA_LAMPOFF	26
#define CA_LAMPOK	27
#define CA_LAMPERR	28
#define ST_IFG		st.assign[CA_IFG]
#define ST_MFG		st.assign[CA_MFG]
#define ST_WFG		st.assign[CA_WFG]
#define ST_CFG		st.assign[CA_CFG]
#define ST_HFG		st.assign[CA_HFG]
#define ST_HFGI		st.assign[CA_HFGI]
#define ST_HBO		st.assign[CA_HBO]
#define ST_SLEFT	st.assign[CA_SLEFT]
#define ST_SRIGHT	st.assign[CA_SRIGHT]
#define ST_SBOTH	st.assign[CA_SBOTH]
#define ST_GROUND	st.assign[CA_GROUND]
#define ST_DULL		st.assign[CA_DULL]
#define ST_FAINT	st.assign[CA_FAINT]
#define ST_SKY		st.assign[CA_SKY]
#define ST_FRIEND	st.assign[CA_FRIEND]
#define ST_FOE		st.assign[CA_FOE]
#define ST_HELP		st.assign[CA_HELP]
#define ST_FIRE1	st.assign[CA_FIRE1]
#define ST_FIRE2	st.assign[CA_FIRE2]
#define ST_MENU		st.assign[CA_MENU]
#define ST_MENUH	st.assign[CA_MENUH]
#define ST_SUN		st.assign[CA_SUN]
#define ST_EARTH	st.assign[CA_EARTH]
#define ST_COMPASS	st.assign[CA_COMPASS]
#define ST_MSGERR	st.assign[CA_MSGERR]
#define ST_MSGWARN	st.assign[CA_MSGWARN]
#define ST_LAMPOFF	st.assign[CA_LAMPOFF]
#define ST_LAMPOK	st.assign[CA_LAMPOK]
#define ST_LAMPERR	st.assign[CA_LAMPERR]

	PLAYER NEAR	*all_known;	/* for remote_* () usage */
	PLAYER NEAR	*all_active;	/* -"- */
	PLAYER NEAR	*all_team;	/* -"- */
	PLAYER NEAR	*all_ports;	/* -"- */
	PLAYER NEAR	*all_pports;	/* -"- */
	PLAYER NEAR	*all_players;	/* internal to menus.c */
	PLAYER NEAR	*no_players;	/* -"- */
	int		misc[20];
	long		stats[100];
	char		filename[1024];	/* file name work area */
};

#define Sys	st.system
#define Tm	st.time
#define Gr	st.graphics
#define Snd	st.sound
#define Kbd	st.keyboard

#define GACC	st.gravity

#define VP	view->viewport
#define VW	view->window
#define VS	view->screen
#define VD	VS->device

struct FldTab {
	int	type;
	void	*p;
};

struct hud {
	short	flags;
#define HF_ETHER	0x0001	/* this is an ether display/HUD */
#define HF_ETHERFRAME	0x0002	/* use ether style frame indicators */
	xshort	orgx;		/* window center */
	xshort	orgy;
	xshort	maxx;		/* window size */
	xshort	maxy;
	xshort	shifty;		/* hud down shift */
	xshort	cx;		/* hud center */
	xshort	cy;
	xshort	sx;		/* hud size */
	xshort	sy;
	xshort	clipx;
	xshort	clipy;
	xshort	clipr;		/* bounding rectangle (relative) */
	xshort	clipl;
	xshort	clipt;
	xshort	clipb;
	xshort	right;		/* bounding rectangle (absolute) */
	xshort	left;
	xshort	top;
	xshort	bottom;
	xshort	tx;		/* tick size */
	xshort	ty;
	xshort	ttx;		/* 'big' adjusted tick size */
	xshort	tty;
	xshort	ss;		/* stroke size */
	xshort	dd;		/* digit width */
	ANGLE	width;		/* horizontal aperture */
	ANGLE	height;		/* vertical aperture */
	Ushort	fg;		/* color (normal) */
	Ushort	fgi;		/* color (intensified) */
	int	VV[2];		/* vv position */
	xshort	etherx;		/* ether frame x centerline */
	xshort	ethery;		/* ether frame y centerline */
	xshort	ethertx;	/* ether frame x half-width */
	xshort	etherty;	/* ether frame y half-width */
	short	misc[10];
};
typedef struct hud HUD;

struct ils {
	char	*name;
	LVECT	R;		/* runway position */
	xshort	longitude;
	xshort	latitude;
	xshort	l[2];		/* localizer relative x/y */
	xshort	g[2];		/* glide-path relative x/y */
	ANGLE	localizer;	/* heading of forward beam */
	ANGLE	glidepath;	/* pitch of beam */
};

#include "extern.h"

#endif
