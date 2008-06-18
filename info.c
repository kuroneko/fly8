/* --------------------------------- info.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Show text on the screen
*/

#include "plane.h"


#define y0	Y0	/* avoid clash with math.h */
#define y1	Y1

#define EV	EE(CV)

static int	x0 = 0, x1 = 0, xs = 0, xm = 0, y0 = 0, y1 = 0, x = 0, y = 0,
		xd = 0, xw = 0, dx = 0, dy = 0;

LOCAL_FUNC void NEAR
ScrNewline (void)
{
	y += dy;
	x = x0;
	if (y > y1) {
		x0 += xs;
		if (xs > 0 && x0 > x1)
			x0 = 2;
		if (xs < 0 && x0 < 0)
			x0 = x1;
		xm = x0 + xw;
		x = x0;
		y = y0 + dy;
	}
}

LOCAL_FUNC int NEAR
ScrPrintf (const char *fmt, ...)
{
	va_list	ap;
	int	i, c;
	char	*p;
	static char _Gbuf[256];

	va_start (ap, fmt);
	i = vsprintf (_Gbuf, fmt, ap);
	va_end (ap);

	for (p = _Gbuf; T(c = *p); ++p) {
		switch (c) {
		default:
			if (x+xd > xm)
				ScrNewline ();
			x += stroke_char (x, y, c, dx, ST_HELP);
			break;
		case '\n':
			ScrNewline ();
			break;
		case '\r':
			x = x0;
			break;
		case '\f':
			y = y0 + dy;
			x = x0;
			break;
		}
	}
	return (i);
}

#define	NOVALUE	((int)0x8000)

LOCAL_FUNC void NEAR
ScrOpt (char *title, int state)
{
	if (state)
		ScrPrintf ("%s\n", title);
}

LOCAL_FUNC void NEAR
ScrOnOff (char *title, int state, int value)
{
	ScrPrintf ("%-9s %s", title, state ? "On" : "Off");
	if (value != NOVALUE)
		ScrPrintf (" %d\n", value);
	else
		ScrPrintf ("\n");
}

LOCAL_FUNC void NEAR
ScrValue (char *title, int value)
{
	ScrPrintf ("%-7s %d\n", title, value);
}

LOCAL_FUNC int NEAR
show_ang (char *line, ANGLE a)
{
	int	i, s, f;

	if (a < 0) {
		s = -1;
		a = -a;
	} else
		s = 1;

	i = ANG2DEG (a);
	a -= DEG2ANG (i);
	f = ANG2DEG (10*a);
	sprintf (line, "%4d.%u", s*i, f);
	return (strlen (line));
}

LOCAL_FUNC void NEAR
right_margin (int orgx, int orgy, int maxx, int maxy, int bss)
{
	dx = bss;
	dy = bss;

	xs = -19*xd;
	x0 = orgx + maxx + xs;
	x1 = orgx - maxx + 2;
	xm = x0 + xw;
	y0 = orgy - maxy;
	y1 = orgy + maxy;

	x = x0;
	y = y0 + dy;
}

LOCAL_FUNC void NEAR
left_margin (int orgx, int orgy, int maxx, int maxy, int bss)
{
	dx = bss;
	dy = bss;

	xs = -xw;
	x0 = orgx - maxx + 2;
	x1 = orgx + maxx - xs;
	xm = x0 + xw;
	y0 = orgy - maxy + dy;
	y1 = orgy + maxy;

	x = x0;
	y = y0 + dy;
}

static char *HudTypeNames[] = {
	"Classic", "FA18", "F16", "F15",
	"Ether", "Flir", "Classic", "Classic"
};

LOCAL_FUNC void NEAR
screen_hud (void)
{
	int	i;

	if (!IS_PLANE(CV))
		return;

	ScrOnOff ("hud",	EV->hud&HUD_ON,			NOVALUE);
	i = (EV->hud1&HUD_TYPES)/HUD_TYPE;
	ScrPrintf("type %u %s\n", i, HudTypeNames[i]);
	ScrOnOff ("ILS",	EV->hud2&HUD_ILS,		EV->ils);

	ScrPrintf("parts:\n");
	ScrOnOff (" altitude",	EV->hud2&HUD_ALTITUDE,		NOVALUE);
	ScrOnOff (" speed",	EV->hud2&HUD_SPEED,		NOVALUE);
	ScrOnOff (" heading",	EV->hud2&HUD_HEADING,		NOVALUE);
	ScrOnOff (" border",	EV->hud1&HUD_BORDER,		NOVALUE);
	ScrOnOff (" v vector",	EV->hud&HUD_VV,			NOVALUE);
	ScrOnOff (" w line",	EV->hud2&HUD_VW,		NOVALUE);
	ScrOnOff (" cross",	EV->hud&HUD_PLUS,		NOVALUE);
	ScrOnOff (" pointer",	EV->hud&HUD_CURSOR,		NOVALUE);
	ScrOnOff (" beta",	EV->hud2&HUD_BETA,		NOVALUE);
	ScrOnOff (" director",	EV->hud2&HUD_DIRECTOR,		NOVALUE);
	ScrOnOff (" waypoint",	EV->hud2&HUD_WAYPOINT,		NOVALUE);
	ScrOnOff (" tracers",	EV->hud2&HUD_BTRAIL,		NOVALUE);
	ScrOnOff (" ghost",	EV->hud3&HUD_GVV,		NOVALUE);
	ScrOnOff (" truehead",	EV->hud3&HUD_TRUEHEADING,	NOVALUE);

	ScrPrintf("ground:\n");
	ScrOnOff (" gnd ptr",	EV->hud1&HUD_PENDULUM,		NOVALUE);
	ScrOnOff (" xbreak",	EV->hud2&HUD_XBREAK,		NOVALUE);
	ScrOnOff (" xvar",	EV->hud3&HUD_XVAR,		NOVALUE);
	ScrOnOff (" xgrid",	EV->hud2&HUD_XGRID,		NOVALUE);
	ScrOnOff (" pullup",	EV->hud3&HUD_CUE,		NOVALUE);

	ScrOnOff ("ladder:",	EV->hud&HUD_LADDER,		NOVALUE);
	ScrOnOff (" pinnd",	EV->ladder&LD_FIXED,		NOVALUE);
	ScrOnOff (" right",	EV->ladder&LD_RIGHT,		NOVALUE);
	ScrOnOff (" erect",	EV->ladder&LD_ERECT,		NOVALUE);
	ScrOnOff (" color",	EV->ladder&LD_COLOR,		NOVALUE);
	ScrOnOff (" funel",	EV->ladder&LD_FUNNEL,		NOVALUE);
	ScrOnOff (" slant",	EV->ladder&LD_SLANT,		NOVALUE);
	ScrOnOff (" zenit",	EV->ladder&LD_ZENITH,		NOVALUE);
	ScrOnOff (" under",	EV->ladder&LD_UNDER,		NOVALUE);
	ScrOnOff (" tip0",	EV->ladder&LD_TIP0,		NOVALUE);
	ScrOnOff (" negtip",	EV->ladder&LD_NEGTIP,		NOVALUE);
	ScrOnOff (" hold",	EV->ladder&LD_HOLD,		NOVALUE);
	ScrOnOff ("  roll",	EV->ladder&LD_HOLDROLL,		NOVALUE);
	ScrOnOff (" sun",	EV->ladder&LD_SUN,		NOVALUE);
	ScrPrintf(" gap   %d\n", fmul (100, EV->ldgap));
	ScrPrintf(" step  %d\n", fmul (100, EV->ldstep));
	ScrPrintf(" step0 %d\n", fmul (100, EV->ldstep0));
	ScrPrintf(" stepg %d\n", fmul (100, EV->ldstepg));
	ScrPrintf(" tip   %d\n", fmul (100, EV->ldtip));
	ScrPrintf(" ndash %d\n", EV->ldndash);

	ScrPrintf("options 1:\n");
	ScrOnOff (" heading",	EV->hud2&HUD_FULLHEADING,	NOVALUE);
	ScrOnOff (" knots",	EV->hud1&HUD_KNOTS,		NOVALUE);
	ScrOnOff (" top",	EV->hud1&HUD_TOP,		NOVALUE);
	ScrOnOff (" fine",	EV->hud&HUD_FINE,		NOVALUE);
	ScrOnOff (" exfine",	EV->hud&HUD_XFINE,		NOVALUE);
	ScrOnOff (" big",	EV->hud&HUD_BIG,		NOVALUE);
	ScrPrintf(" scale len %d\n", EV->tapelen);
	ScrPrintf(" area %dDeg\n", (int)EV->hudarea);
	ScrOnOff (" cas",	EV->hud2&HUD_CALIBRATED,	NOVALUE);

	ScrPrintf("options 2:\n");
	ScrOnOff (" aural",	EV->hud1&HUD_AALARM,		NOVALUE);
	ScrOnOff (" visual",	EV->hud1&HUD_VALARM,		NOVALUE);
	ScrPrintf(" font %d\n", (int)st.StFont);
	ScrPrintf(" size %d/%d\n", (int)st.StFontSize, (int)dx);

	ScrPrintf("radar:\n");
	ScrOnOff (" corner",	EV->hud1&HUD_CORNER,		NOVALUE);
	ScrOnOff (" data",	EV->hud&HUD_DATA,		NOVALUE);
	ScrOnOff (" dist",	EV->hud1&HUD_IDIST,		NOVALUE);
	ScrOnOff (" name",	EV->hud1&HUD_INAME,		NOVALUE);
	ScrOnOff (" acc vect",	EV->hud1&HUD_ACCVECT,		NOVALUE);
	ScrOnOff (" reticle",	EV->hud&HUD_RETICLE,		NOVALUE);
	ScrOnOff (" target",	EV->hud&HUD_TARGET,		NOVALUE);
	ScrPrintf(" type %s\n",	EV->hud&HUD_ROSS ? "Ross" : "Eyal");
	ScrOnOff (" limit",	EV->hud1&HUD_LIMIT,		NOVALUE);
	ScrOnOff (" thick",	EV->hud1&HUD_THICK,		NOVALUE);
	ScrOnOff (" hide",	EV->hud2&HUD_HIDETGT,		NOVALUE);
	ScrOnOff (" tpointer",	EV->hud2&HUD_TPOINTER,		NOVALUE);
	ScrOnOff (" vpointer",	EV->hud2&HUD_VPOINTER,		NOVALUE);

	ScrPrintf("hdd:\n");
	ScrOnOff (" instr",	EV->hdd&HDD_INSTRUMENTS,	NOVALUE);
	ScrOnOff (" nav",	EV->hdd&HDD_NAV,		NOVALUE);
	ScrOnOff (" compass",	EV->hdd&HDD_COMPASS,		NOVALUE);
	ScrOnOff ("  square",	EV->hdd&HDD_SQRCOMPASS,		NOVALUE);
	ScrOnOff ("  ortho",	EV->hdd&HDD_ORTCOMPASS,		NOVALUE);
	ScrOnOff (" panel",	EV->hdd&HDD_PANEL,		NOVALUE);
}

static struct {
	Uchar	name[4];
	int	index;
} StatsInfo[] = {
	{"sndB", STAT_NETSENTBYTES},
	{"sndL", STAT_NETSENTP},
	{"sndP", STAT_NETSENTPACKETS},

	{"rcvB", STAT_NETRECEIVEDBYTES},
	{"rcvL", STAT_NETRECEIVEDP},
	{"rcvP", STAT_NETRECEIVEDPACKETS},

	{"sERs", STAT_NETERRSEND},
	{"sERa", STAT_NETERRSENDA},
	{"sERr", STAT_NETERRSENDSOFT},	/* send retry count exceeded */

	{"rERd", STAT_NETERRD},
	{"rERc", STAT_NETERRCRC},
	{"NoPl", STAT_NETERRNOPLAYER},
	{"rERb", STAT_NETERRL},
	{"Nois", STAT_NETERRNOISE},
	{"rERl", STAT_NETERRPACKED},

	{"Fond", STAT_NETERRFOUND},
	{"Lost", STAT_NETERRLOST},

	{"rOOO", STAT_NETERROOO},

	{"Debg", STAT_DEBUG},

	{"Fix1", STAT_FIX1},

	{"Mem ", STAT_MEMTOTAL},
	{"MemM", STAT_MEMMAXUSED},
	{"MemA", STAT_MEMALLOCED},
	{"MemL", STAT_MEMLOW},
	{"MemN", STAT_MEMNO},

	{"Mx<0", STAT_GRERRMOVEXLOW},
	{"Mx>w", STAT_GRERRMOVEXHIGH},
	{"My<0", STAT_GRERRMOVEYLOW},
	{"My>h", STAT_GRERRMOVEYHIGH},
	{"Dx<0", STAT_GRERRDRAWXLOW},
	{"Dx>w", STAT_GRERRDRAWXHIGH},
	{"Dy<0", STAT_GRERRDRAWYLOW},
	{"Dy>h", STAT_GRERRDRAWYHIGH},
	{"Bufl", STAT_ERRBUFLINE},

	{"CLot", STAT_TCLIPOUT},
	{"CLin", STAT_TCLIPIN},
	{"CLxx", STAT_TCLIPINOUT},
	{"CLho", STAT_TCLIPOUTHARD},
	{"CLhf", STAT_TCLIPFAILED},
	{"CLhi", STAT_TCLIPINHARD},
	{"CLtt", STAT_TCLIPCOUNT},
	{"Move", STAT_MOVECOUNT},
	{"Draw", STAT_DRAWCOUNT},

	{"T1-0", STAT_TEMP1+0},
	{"T1-1", STAT_TEMP1+1},
	{"T1-2", STAT_TEMP1+2},
	{"T1-3", STAT_TEMP1+3},
	{"T1-4", STAT_TEMP1+4},
	{"T1-5", STAT_TEMP1+5},
	{"T1-6", STAT_TEMP1+6},
	{"T1-7", STAT_TEMP1+7},
	{"T1-8", STAT_TEMP1+8},
	{"T1-9", STAT_TEMP1+9},

	{"T2-0", STAT_TEMP2+0},
	{"T2-1", STAT_TEMP2+1},
	{"T2-2", STAT_TEMP2+2},
	{"T2-3", STAT_TEMP2+3},
	{"T2-4", STAT_TEMP2+4},
	{"T2-5", STAT_TEMP2+5},
	{"T2-6", STAT_TEMP2+6},
	{"T2-7", STAT_TEMP2+7},
	{"T2-8", STAT_TEMP2+8},
	{"T2-9", STAT_TEMP2+9},

{"", 0}};

LOCAL_FUNC void NEAR
screen_net (void)
{
	PLAYER	*pl;

	ScrPrintf ("Time %s\n", show_time ("", st.present));
	ScrPrintf ("Flags  %.4x\n", st.network);
	if (st.network & NET_INITED) {
		ScrPrintf ("V %u", (int)st.ComVersion);
		if (st.network & NET_ON)
			ScrPrintf (" On");
		if (st.network & NET_NOBCAST)
			ScrPrintf (" Nb");
		if (st.network & NET_AUTOACCEPT)
			ScrPrintf (" Aa");
		if (st.network & NET_AUTODECLINE)
			ScrPrintf (" Ad");
		if (st.network & NET_AUTOCONNECT)
			ScrPrintf (" Ac");
		ScrPrintf ("\n");
		ScrPrintf ("Remote players:\n");
		for (pl = 0; T(pl = player_next (pl));) {
			strncpy (st.filename, pl->name, 20);
			strncat (st.filename, ":", 20);
			strncat (st.filename, pl->team, 20);
			if (pl->flags & PL_ACTIVE)
				ScrPrintf ("Act");
			else if (pl->flags & PL_PENDREQUEST)
				ScrPrintf ("PnR");
			else if (pl->flags & PL_PENDCONFIRM)
				ScrPrintf ("PnC");
			else if (pl->flags & PL_PLAYING)
				ScrPrintf ("Ply");
			else
				ScrPrintf ("Idl");
			ScrPrintf (" %s\n", st.filename);
			ScrPrintf ("  port %s\n",
				netport_name (pl->netport));
			ScrPrintf ("  addr %s\n",
				netport_addr (pl->netport, pl->address));
		}
	} else
		ScrPrintf ("Net not active\n");
}

extern void FAR
stats_show (void)
{
	int	i, j;
	Ulong	lt;

	for (i = 0; StatsInfo[i].name[0]; ++i) {
		if (T(lt = (Ulong)st.stats[j = StatsInfo[i].index]))
			LogPrintf ("%2u) %-4.4s %13s\n",
				j, StatsInfo[i].name, show_ul (lt));
	}
}

LOCAL_FUNC void NEAR
screen_modes (VIEW *view)
{
	int	i, n;

	ScrPrintf ("%s\n", show_time ("Time  ", st.present));
	if (st.ShutdownTime)
		ScrPrintf ("%s\n", show_time ("Limit ", st.ShutdownTime));

	ScrPrintf ("Flags  %.4x %.4x\n", st.flags, st.flags1);
	ScrPrintf ("Net    %.4x\n", st.network);
	ScrOpt   ("Blanker", st.flags  & SF_BLANKER);
	ScrOpt   ("Land",    st.flags  & SF_LANDSCAPE);

	ScrOpt   ((st.flags1 & SF_SOLIDSKY) ? "Solid Sky" : "Sky",
				st.flags  & SF_SKY);

	ScrOpt   ("Verbose", st.flags  & SF_VERBOSE);
	ScrOpt   ("Smoke",   st.flags1 & SF_SMOKE);
	ScrOpt   ("G",       st.flags1 & SF_USEG);

	ScrPrintf ("Stereo %u%+d ",  st.stereo, st.paralax);
	if (st.flags1 & SF_STEREOREV)
		ScrPrintf ("R");
	if (st.flags1 & SF_HUDINFRONT)
		ScrPrintf ("F");
	ScrPrintf ("\n");

	ScrOpt   ("Dbl Buff",st.flags1&SF_DBUFFERING);
	ScrValue ("Drones",  st.drones);
	ScrValue ("Killers", st.killers);
	ScrOnOff ("Zviews",  st.flags1&SF_EXTVIEW, st.extview);
	ScrOnOff ("Zoom",    VP->zoom, VP->zoom);
	ScrOnOff ("Gaze",    VP->rotz, ANG2DEG(VP->rotz));
	ScrOnOff ("Info",    st.flags1&SF_INFO, st.info);

	ScrPrintf ("Sound %d %.4x", st.quiet, st.sounds);
	if (st.sounds & SS_WARN)
		ScrPrintf (" W");
	if (st.sounds & SS_ALARM)
		ScrPrintf (" A");
	ScrPrintf ("\n");

	ScrPrintf ("last obj %s\n", show_l (st.object_id));
	if (IS_PLANE(CV)) {
		ScrOnOff ("OnGround", EV->flags & PF_ONGROUND,	NOVALUE);
		ScrOnOff ("No Stall", EV->flags & PF_NOSTALL,	NOVALUE);
		ScrPrintf ("Auto ");
		if (EV->flags & PF_AUTOFLAP)
			ScrPrintf ("F");
		if (EV->flags & PF_AUTOELEVATOR)
			ScrPrintf ("E");
		if (EV->flags & PF_AUTORUDDER)
			ScrPrintf ("R");
		ScrPrintf ("\n");
		ScrPrintf ("Wpn ");
		ScrPrintf ("%s %d", get_wname (EE(CV)->weapon),
				EV->stores[EV->weapon-1]);
		if (EV->flags & PF_LIMITED)
			ScrPrintf ("L");
		ScrPrintf ("\n");
		ScrPrintf ("Radar");
		ScrPrintf ("%s", EV->radar&R_ON ? " On" : "");
		ScrPrintf ("%s", EV->radar&R_LOCK ? " Lck" : "");
		ScrPrintf ("%s", EV->radar&R_INTELCC ? " P" : "");
		ScrPrintf ("\n");
		n = 0;
		if (EV->radar & R_SELECT3)
			ScrPrintf (" Sbr"), ++n;
		else if (EV->radar & R_SELECT20)
			ScrPrintf (" Shd"), ++n;
		else if (EV->radar & R_SELECT5)
			ScrPrintf (" Svt"), ++n;
		if (EV->radar&R_INTEL)
			ScrPrintf (" Itl"), ++n;
		i = (EV->radar&R_MODES) / R_MODE;
		ScrPrintf (" M%d", i), ++n;
		if (EV->flags&PF_AUTO)
			ScrPrintf (" Aut"), ++n;
		if (EV->flags&PF_CHASE)
			ScrPrintf (" Chs"), ++n;
		if (EV->flags&PF_KILL)
			ScrPrintf (" Kil"), ++n;
		if (n)
			ScrPrintf ("\n"), n = 0;
		if (EV->hudmode & HM_DECLUTTER)
			ScrPrintf ("Declutter\n");
	}

	ScrPrintf ("Debug %s ",
		(st.flags & SF_DEBUG) ? "On " : "Off");
	ScrPrintf ("%c ",
		(st.debug & DF_TRACE) ? 'T' : ' ');
	if (st.debug & DF_GPW)
		ScrPrintf ("W");
	if (st.debug & DF_GPX)
		ScrPrintf ("X");
	if (st.debug & DF_GPY)
		ScrPrintf ("Y");
	if (st.debug & DF_GPZ)
		ScrPrintf ("Z");
	ScrPrintf ("\n");
}

#undef NOVALUE

static void NEAR
screen_help (void)
{
	ScrPrintf ("A   reticle mode[ob]n");
	ScrPrintf ("b   wheel brakes   \n");
	ScrPrintf ("c   clear text     \n");
	ScrPrintf ("C   chase target   \n");
	ScrPrintf ("d   declutter HUD  \n");
	ScrPrintf ("D   descend chute  \n");
	ScrPrintf ("E   eject          \n");
	ScrPrintf ("f   radar acq mode \n");
	ScrPrintf ("g   landing gear   \n");
	ScrPrintf ("h   help (also ?)  \n");
	ScrPrintf ("i   intel mode     \n");
	ScrPrintf ("j   radar on pilots\n");
	ScrPrintf ("k   auto shoot     \n");
	ScrPrintf ("l   lock radar     \n");
	ScrPrintf ("m   show modes     \n");
	ScrPrintf ("n   net stats      \n");
	ScrPrintf ("o   observer select\n");
	ScrPrintf ("O    +show all objs\n");
	ScrPrintf ("p   pause          \n");
	ScrPrintf ("q   quiet modes    \n");
	ScrPrintf ("r   activate radar \n");
	ScrPrintf ("s   show obj stats \n");
	ScrPrintf ("S   reSupply plane \n");
	ScrPrintf ("u   hud menu       \n");
	ScrPrintf ("v   alternate view \n");
	ScrPrintf ("w   weapon select  \n");
	ScrPrintf ("W   remove weapons \n");
	ScrPrintf ("x   calib. pointer \n");
	ScrPrintf ("/*- view L/C/R     \n");
	ScrPrintf ("]/[ flaps +/-      \n");
	ScrPrintf ("}/{ spoilers +/-   \n");
	ScrPrintf ("+   air brakes     \n");
	ScrPrintf ("!   shell          \n");
	ScrPrintf ("F1  shoot          \n");
	ScrPrintf ("F2/3/4 rudder L/C/R\n");
	ScrPrintf ("F5/6 zoom in/out   \n");
	ScrPrintf ("F7/8 macro rec/play\n");
	ScrPrintf ("ESC main menu      \n");
}


LOCAL_FUNC void NEAR
screen_stats (void)
{
	OBJECT	*p;
	int	i, j;
	Ulong	lt;

	ScrPrintf ("Time %s\n", show_time ("", st.present));
	for (i = 0; StatsInfo[i].name[0]; ++i) {
		if (T(lt = (Ulong)st.stats[j = StatsInfo[i].index]))
			ScrPrintf ("%2u %-4.4s %s\n",
				j, StatsInfo[i].name, show_ul (lt));
	}

	if (STATS_NETERRLOW[1])
		ScrPrintf ("in  av %lu max %lu\n",
			STATS_NETERRLOW[2]/STATS_NETERRLOW[0],
			STATS_NETERRLOW[1]);
	if (STATS_NETERRLOW[4])
		ScrPrintf ("out av %lu max %lu\n",
			STATS_NETERRLOW[5]/STATS_NETERRLOW[3],
			STATS_NETERRLOW[4]);

	for (i = 0; i < O_INT; ++i) {
		j = 0;
		for (p = CO; p; p = p->next) {
			if (p->name == i)
				++j;
		}
		if (j)
			ScrPrintf ("%4u %s\n", j, st.bodies[i]->title);
	}
}

LOCAL_FUNC void NEAR
screen_debug (void)
{
	OBJECT	*target;

	if (CO)
		ScrPrintf ("CO %04lu %04x %-6.6s\n",
			CO->id%10000, (int)CO->flags, TITLE(CO));
	else
		ScrPrintf ("CO (null)\n");
	ScrPrintf ("CC %04lu %04x %-6.6s\n",
		CC->id%10000, (int)CC->flags, TITLE(CC));
	ScrPrintf ("CV %04lu %04x %-6.6s\n",
		CV->id%10000, (int)CV->flags, TITLE(CV));
	if (IS_PLANE(CV) && T(target = EV->target)) {
		if (target->id == EV->tid)
			ScrPrintf ("TG %04lu %04x %-6.6s\n",
				target->id%10000, (int)target->flags,
				TITLE(target));
		else
			ScrPrintf ("TG %lu?%lu\n",
				target->id, EV->tid);
	}
	if (STATS_DEBUG)
		ScrPrintf ("ERRS %s\n", show_ul ((Ulong)STATS_DEBUG));
}

LOCAL_FUNC void NEAR
screen_font (int orgx, int orgy, int dx, int dy)
{
	int	x, y;

	x = orgx-dx*6;				/* test: show font */
	y = orgy-dy*3;

	y += dy;
	stroke_str (x, y, " !\"#$%&'()*+,-./", dx, ST_IFG);
	y += dy;
	stroke_str (x, y, "0123456789:;<=>?", dx, ST_IFG);
	y += dy;
	stroke_str (x, y, "@ABCDEFGHIJKLMNO", dx, ST_IFG);
	y += dy;
	stroke_str (x, y, "PQRSTUVWXYZ[\\]^_", dx, ST_IFG);
	y += dy;
	stroke_str (x, y, "`abcdefghijklmno", dx, ST_IFG);
	y += dy;
	stroke_str (x, y, "pqrstuvwxyz{|}~\x7f", dx, ST_IFG);
}

LOCAL_FUNC void NEAR
screen_colors (void)
{
	int	i;
	int	x, y;

	for (y = y0, i = 0; i < rangeof (st.colors); ++i) {
		x = x0;
		y += dy;
		x += stroke_str (x, y, color_name (i), dx, ST_IFG);
		stroke_str (x+dx,   y, color_rgb  (i), dx, st.colors[i]);
	}
}

LOCAL_FUNC void NEAR
screen_alarm (int orgx, int orgy, int ss, int color)
{
	int	blink, dx;
/*
 * Put high priority alarms last to activate the most urgent aural warning.
*/
	blink = ((int)st.present)&0x0080;
	dx = stroke_size ("A", ss);

	if (O_CHUTE == CV->name && CV->R[Z] <= 0L) {
		if ((st.flags & SF_MAIN) && blink)
			stroke_str (orgx-dx*8, orgy, "WAIT", ss*4, color);
	}
}

extern void FAR
screen_info (VIEW *view, int orgx, int orgy, int maxx, int maxy, int ss,
	int mode)
{
	int	dd, x, y, mg, info, i, savefont, list;
	Ulong	tt, tscore;
	OBJECT	*p;
	char	line[80], *l;

	if (F(p = CV))
		return;

	savefont = font_set (0);

	dd = num_size (9L, ss);

	if (st.flags & SF_MAIN) {
		xd = dd;
		xw = 19*xd;

		right_margin (orgx, orgy, maxx, maxy, ss);

		switch (list = (st.flags & SF_LISTS)) {
		case SF_MODES:
			screen_modes (view);
			break;
		case SF_NET:
			screen_net ();
			break;
		case SF_HUD:
			screen_hud ();
			break;
		case SF_STATS:
			screen_stats ();
			break;
		case SF_COLORS:
			screen_colors ();
			break;
		default:
			break;
		}

		if (st.flags & SF_DEBUG)
			screen_debug ();

		left_margin (orgx, orgy, maxx, maxy, ss);

		switch (list) {
		case SF_HELP:
			screen_help ();
			break;
		case SF_FONT:
			font_set (savefont);
			screen_font (orgx, orgy, ss, ss);
			savefont = font_set (0);
			break;
		default:
			break;
		}

		for (x = 0; x < NHDD; ++x) {
			if ((st.hdd[x].flags & HDF_ON) &&
			    HDT_UPFRONT == st.hdd[x].type)
				break;
		}
		if (x >= NHDD)
			show_menu (view, orgx, orgy, maxx, maxy, ss);

		msg_show (orgx, orgy, maxx, maxy, ss);

		edit_show (view, orgx, orgy, maxx, maxy, ss);
	}

	y = orgy - maxy;
	mg = 0;				/* avoid compiler warning */
	if (F(info = st.info))
		{}
	else if (HDT_UPFRONT == mode)
		mg = orgx - maxx + 2;
	else if ((st.flags & SF_MAIN) && (st.flags1 & SF_INFO))
		mg = orgx - maxx + 4*ss;
	else
		info = 0;

	if (info) {						/* times */
		i = st.misc[0] - st.misc[1] - st.misc[2]
			- st.misc[3] - st.misc[4] - st.misc[5];

		sprintf (line, "%3u %3u %3u %3u %3u %3u %3d ",
			st.misc[0]/10, st.misc[1]/10, st.misc[2]/10,
			st.misc[5]/10, st.misc[3]/10, st.misc[4]/10, i/10);
		y += ss;
		x = stroke_str (mg, y, line, ss, ST_IFG);
		i = st.misc[0] > 100 ? (int)(100000L/st.misc[0]) : 999;
		stroke_frac (mg+x, y, (long)i, 3, 1, ss, ST_IFG);
		y += ss;
		stroke_str (mg, y, "tot vid  3d  2d sim flp bal fps",
				ss, ST_HFG);
	}

	if (2 == info) {
		y += ss;
		stroke_str (mg, y, (char *)show_time ("", st.present), ss,
			ST_WFG);

		sprintf (line, "S%4u %4lu %4lu",
			(int)st.nobjects, STATS_CLIPCOUNT,
			STATS_DISPLAYLISTSIZE);
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "C%4lu%4lu%4lu%4lu",
			STATS_CLIPOUT, STATS_CLIPIN, STATS_CLIPINOUT,
			STATS_CLIPOUTHARD + STATS_CLIPFAILED
							+ STATS_CLIPINHARD);
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "R%6ld%6ld%6ld",		/* position */
			vuscale (CV->R[X]),
			vuscale (CV->R[Y]),
			vuscale (CV->R[Z]));
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "V%6d%6d%6d",		/* velocity (earth) */
			(int)vuscale (CV->V[X]),
			(int)vuscale (CV->V[Y]),
			(int)vuscale (CV->V[Z]));
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "A%6d%6d%6d",		/* accel    (earth) */
			(int)vuscale (CV->A[X]),
			(int)vuscale (CV->A[Y]),
			(int)vuscale (CV->A[Z]));
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "v%6d%6d%6d",		/* velocity (body) */
			(int)vuscale (CV->vb[X]),
			(int)vuscale (CV->vb[Y]),
			(int)vuscale (CV->vb[Z]));
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		sprintf (line, "a%6d%6d%6d",		/* accel    (body) */
			(int)vuscale (CV->ab[X]),
			(int)vuscale (CV->ab[Y]),
			(int)vuscale (CV->ab[Z]));
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		l = line;
		sprintf (l, "e");			/* Euler    (earth) */
		l += 1;
		l += show_ang (l, CV->a[X]);
		l += show_ang (l, CV->a[Y]);
		l += show_ang (l, CV->a[Z]);
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);

		l = line;
		sprintf (l, "r");			/* Euler rates */
		l += 1;
		l += show_ang (l, CV->da[X]*VONE);
		l += show_ang (l, CV->da[Y]*VONE);
		l += show_ang (l, CV->da[Z]*VONE);
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);
	} else if (3 == info) {
		if (st.flags1 & SF_TESTING)
			tt = (Ulong)TIMEOUT (st.test_start);
		else
			tt = st.present;
		sprintf (line, "%s %u %lu %u", show_time ("", tt),
			(int)st.ntargets, st.nbullets, (int)p->score);
		y += ss;
		stroke_str (mg, y, line, ss, ST_IFG);
		y += ss;
		x = mg;
		if (st.flags1 & SF_TESTING) {
			tscore = st.mscore - (tt/1000)*10L - st.nbullets*2L;
			sprintf (line, "%ld ", tscore);
			x += stroke_str (x, y, line, ss, ST_WFG);
		}
		sprintf (line, "%d", (int)(p->speed/VONE));
		x += stroke_str (x, y, line, ss, ST_WFG);
	}

	if (VP && !(st.flags & SF_BLANKER) && scenery (mode)) {
		y = orgy - maxy;
		if (VP->rotz) {
			y += ss;
			x = orgx-maxx+2;
			x += stroke_char (x, y, 'p', ss, ST_HFG);
			if (VP->rotz > 0)
				x += stroke_char (x, y, ' ', ss, ST_HFG);
			x += stroke_num (x, y, ANG2DEG(VP->rotz), ss, ST_HFG);
		}

		if (VP->rotx) {
			y += ss;
			x = orgx-maxx+2;
			x += stroke_char (x, y, 'e', ss, ST_HFG);
			if (VP->rotx > 0)
				x += stroke_char (x, y, ' ', ss, ST_HFG);
			x += stroke_num (x, y, ANG2DEG(VP->rotx), ss, ST_HFG);
		}

		if (VP->zoom) {
			y += ss;
			x = orgx-maxx+2;
			x += stroke_str (x, y, "z ", ss, ST_HFG);
			stroke_num (x, y, VP->zoom, ss, ST_HFG);
		}
	}

	screen_alarm (orgx, orgy, ss, ST_HFGI);
	font_set (savefont);
}

#undef y0
#undef y1
#undef EV
