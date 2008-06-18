/* --------------------------------- init.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* startup code.
*/

#include "fly.h"

LOCAL_FUNC int NEAR
get_option (char *e)
{
	char	t, *tp, *p;
	int	errs = 0, itemp;
	long	ltemp;

	while (isspace (e[0]))
		++e;
	if (!e[0])
		return (0);

/* Find first space and replace it with a string terminator.
*/
	for (t = 0, tp = e; *tp; ++tp) {
		if (isspace (*tp)) {
			t = *tp;
			*tp = '\0';
			break;
		}
	}

	LogPrintf (" %s\n", e);
	switch (e[0]) {
	case 'b':
		if (e[1] >= '0' && e[1] <= '9')
			st.windows = (int)(e[1] - '0');
		else if ('e' == e[1])
			st.windows = WIN_ETHER;
		else
			st.windows = WIN_FULL;
		break;
	case 'c':
		if (get_long (e+2, &ltemp) ||
		    set_rrggbb (e[1], (Ulong)ltemp)) {
			LogPrintf ("bad color %s\n", e);
			++errs;
		}
		break;
	case 'd':			/* devices */
		switch (e[1]) {
		case 'k':		/* keyboard */
			st.kbdname = STRfree (st.kbdname);
			st.kbdname = STRdup (e+2);
			break;
		case 'n':		/* netport */
			{
				struct netname	*nn;

				if (F(NEW (nn))) {
					LogPrintf ("no mem for netname\n");
					++errs;
					break;
				}
				nn->name = STRdup (e+2);
				nn->next = st.netnames;
				st.netnames = nn;
			}
			break;
		case 'p':		/* pointer */
			st.ptrname = STRfree (st.ptrname);
			st.ptrname = STRdup (e+2);
			break;
		case 's':		/* sound */
			st.sndname = STRfree (st.sndname);
			st.sndname = STRdup (e+2);
			break;
		case 't':		/* timer */
			st.timeropts = STRfree (st.timeropts);
			st.timeropts = STRdup (e+2);
			break;
		case 'v':		/* video */
			st.grname = STRfree (st.grname);
			st.grname = STRdup (e+2);
			break;
		default:
			goto too_bad;
		}
		break;
	case 'D':
		st.dtype = STRfree (st.dtype);
		st.dtype = STRdup (e+1);
		break;
	case 'F':		/* already processed */
		break;
	case 'h':
	case '?':
help:
		welcome (2);
#ifdef NOSTDERR
#define SHOW(x)	LogPrintf (x)
#else
#define SHOW(x)	fprintf (stderr, x)
#endif
		SHOW ("usage: fly8 [options]\n");
		SHOW ("       see ini.doc for options\n");
#undef SHOW
		++errs;
		break;
	case 'H':
		st.homename = STRfree (st.homename);
		st.homename = STRdup (e+1);
		break;
	case 'i':
		st.initkeys = STRfree (st.initkeys);
		st.initkeys = STRdup (e+1);
		break;
	case 'I':		/* already processed */
		break;
	case 'L':		/* already processed */
		break;
	case 'm':
		st.grmname = STRfree (st.grmname);
		st.grmname = STRdup (e+1);
		break;
	case 'M':
		st.mname = STRfree (st.mname);
		st.mname = STRdup (e+1);
		break;
	case 'n':
		if (get_long (e+2, &ltemp)) {
bad_num:
			LogPrintf ("bad number in '%s'\n", e);
			++errs;
			break;
		}
		switch (e[1]) {
		case 'a':		/* autoconnect rate */
			if (ltemp < 1000)
				goto bad_num;
			st.AutoConnect_rate = (Ulong)ltemp;
			break;
		case 'b':		/* max num of display list lines */
			itemp = (int)(ltemp/BUFLEN*2+1);
			if (itemp < 20)
				goto bad_num;
			st.maxbuffers = (Ushort)itemp;
			break;
		case 'f':		/* limit frame rate */
			if (ltemp < MIN_FRAME_RATE)
				goto bad_num;
			st.frame_rate = (Ulong)ltemp;
			break;
		case 'i':		/* plane dynamics minimal interval */
			if (ltemp < 1 || ltemp > 1000)
				goto bad_num;
			st.interval_max = (int)ltemp;
			break;
		case 'k':		/* number of sky lines */
			if (ltemp < 1)
				goto bad_num;
			st.SkyLines = (int)ltemp;
			break;
		case 'l':		/* log flush rate */
			if (ltemp < 0)
				goto bad_num;
			st.LogFlush_rate = (Ulong)ltemp;
			break;
		case 'm':		/* num of macros */
			if (ltemp < 1)
				goto bad_num;
			st.nMacros = (Ushort)ltemp;
			break;
		case 'r':		/* history recall queue size */
			if (ltemp < 1)
				goto bad_num;
			st.maxrecall = (Ushort)ltemp;
			break;
		case 't':		/* num of secs till shutdown */
			if (ltemp < 0)
				goto bad_num;
			st.ShutdownTime = ltemp*1000L;
			break;
		case 'u':		/* limit maint update rate */
			if (ltemp < 0)
				goto bad_num;
			st.maint_rate = (Ulong)ltemp;
			break;
		default:
			goto too_bad;
		}
		break;
	case 'N':
		st.nikname = STRfree (st.nikname);
		if (e[1] && T(st.nikname = STRdup (e+1)) &&
				strlen (st.nikname) >LNAME-1) /* too long? */
			st.nikname[LNAME-1] = '\0';
		break;
	case 'o':
		SetOption (NULL, (Ushort)(itemp = 1));
		for (p = e + 1; *p; ++p) {
			switch (*p) {
			case '-':
				SetOption (NULL, (Ushort)(itemp = 0));
				break;
			case '+':
				SetOption (NULL, (Ushort)(itemp = 1));
				break;
			case '^':
				SetOption (NULL, (Ushort)(itemp = 2));
				break;
			case 'c':
				SetOption (&st.flags, SF_USECLEAR);
				break;
			case 'l':
				SetOption (&st.flags, SF_LANDSCAPE);
				break;
			case 'q':
				if (0 == itemp)
					st.quiet = 1;	/* quiet off */
				else if (1 == itemp)
					st.quiet = 0;	/* quiet on */
				else
					st.quiet = (st.quiet + 1) % 3;
				break;
			case 's':
				SetOption (&st.flags1, SF_SOLIDSKY);
				break;
			case 't':
				SetOption (&st.debug, DF_TRACE);
				break;
			case 'v':
				SetOption (&st.flags, SF_VERBOSE);
				break;
			default:
				goto too_bad;
			}
		}
		break;
	case 'P':
		st.ptype = STRfree (st.ptype);
		st.ptype = STRdup (e+1);
		break;
	case 'r':
		itemp = 1;
		if ('0' == e[itemp]) {
			st.network &= ~NET_ON;
			++itemp;
		} else
			st.network |= NET_ON;
		if ('l' == e[itemp]) {
			++itemp;
			if ('0' == e[itemp]) {
				st.network &= ~NET_NOBCAST;
				++itemp;
			} else
				st.network |= NET_NOBCAST;
		}
		break;
	case 'T':
		st.teamname = STRfree (st.teamname);
		if (e[1] && T(st.teamname = STRdup (e+1)) &&
				strlen (st.teamname) >LNAME-1) /* too long? */
			st.teamname[LNAME-1] = '\0';
		break;
	case 'V':
		if (!e[1]) {
			++errs;
			goto help;
		}
		st.vmdname = STRfree (st.vmdname);
		st.vmdname = STRdup (e+1);
		break;
	case 'X':
		st.navname = STRfree (st.navname);
		st.navname = STRdup (e+1);
		break;
	case 'Y':
		st.lndname = STRfree (st.lndname);
		st.lndname = STRdup (e+1);
		break;
	case 'z':
		st.flags ^= SF_BLANKER;
		if (e[1]) {
			if (get_int (e+1, &itemp) ||
				itemp < 0) {
				LogPrintf ("use -znn\n");
				++errs;
			}
			st.drones = itemp;
		} else
			st.drones = 0;
		break;
	default:
too_bad:
		LogPrintf ("unknown option %s\n", e);
		++errs;
		break;
	}
	if (t)
		*tp = t;
	return (errs);
}

LOCAL_FUNC int NEAR
get_argopts (char **a)
{
	int	errs = 0;
	char	*p;

	if (*a)
		LogPrintf ("command line args:\n");

	while (T(p = *a++)) {
		if (T(p = STRdup (p + ('-' == p[0])))) {
			errs += get_option (p);
			p = STRfree (p);
		}
	}
	return (errs);
}

LOCAL_FUNC int NEAR
get_envopts (void)
{
	char	*e, *p, *q;
	int	errs = 0;

	if (T(e = getenv ("FLY8"))) {
		LogPrintf ("env args:\n");
		if (T(e = STRdup (e))) {
			for (p = e; p && *p; p = q) {
				while (isspace (*p))	/* find start */
					++p;
				if (!*p)
					break;
				if (T(q = strchr (p, ';'))) /* find end */
					*q = '\0';	/* end string */
				errs += get_option (p);
				if (q)
					*q++ = ';';	/* restore string */
			}
			e = STRfree (e);
		}
	}
	return (errs);
}

LOCAL_FUNC FILE * NEAR
find_ini (void)
{
	FILE	*ini;
	char	*e, *p, *q;

	if (!st.iname  && F(st.iname = STRdup (INIFILE))) {
		LogPrintf ("no mem: iname\n");
		die ();
	}

	if (T(ini = fopen (st.iname, RTMODE)))
		return (ini);

	if (st.fdir) {
		Sys->BuildFileName (st.filename, st.fdir, st.iname, "");
		if (T(ini = fopen (st.filename, RTMODE))) {
			st.iname = STRfree (st.iname);
			if (F(st.iname = STRdup (st.filename))) {
				LogPrintf ("no mem: iname\n");
				die ();
			}
			return (ini);
		}
	}

	if (T(e = getenv ("HOME"))) {
		Sys->BuildFileName (st.filename, e, st.iname, "");
		if (T(ini = fopen (st.filename, RTMODE))) {
			st.iname = STRfree (st.iname);
			if (F(st.iname = STRdup (st.filename))) {
				LogPrintf ("no mem: iname\n");
				die ();
			}
			return (ini);
		}
	}

	if (T(e = getenv ("PATH"))) {
		e = STRdup (e);
		for (p = e; p && *p; p = q) {
			if (T(q = strchr (p, PATHSEP)))	/* find end */
				*q++ = '\0';	/* end string */
			Sys->BuildFileName (st.filename, p, st.iname, "");
			if (T(ini = fopen (st.filename, RTMODE))) {
				st.iname = STRfree (st.iname);
				if (F(st.iname = STRdup (st.filename))) {
					LogPrintf ("no mem: iname\n");
					die ();
				}
				return (ini);
			}
		}
		e = STRfree (e);
	}
	return (ini);
}

LOCAL_FUNC int NEAR
get_iniopts (void)
{
	int	errs = 0, l;
	FILE	*ini;
	char	opt[256], *p;

	if (F(ini = find_ini ()))
		return (0);

	LogPrintf ("%s args:\n", st.iname);
	while (fgets (opt, sizeof (opt), ini)) {
		for (p = opt; isspace (*p); ++p)
			;
		if ('\n' == p[0] || !p[0] || '#' == p[0])
			continue;
		l = strlen(p);
		p[l-1] = '\0';		/* remove '\n' */
		errs += get_option (p);
	}
	if (ferror (ini)) {
		perror ("error reading init file");
		++errs;
	}

	fclose (ini);
	return (errs);
}

LOCAL_FUNC void NEAR
get_first_opts (char **argv)
{
	char	**pp, *p;

	for (pp = argv; T(p = *pp); ++pp) {
		if ('-' == p[0])
			++p;
		switch (p[0]) {
		case 'I':
			st.iname = STRfree (st.iname);
			st.iname = STRdup (p+1);
			break;
		case 'F':
			st.fdir = STRfree (st.fdir);
			st.fdir = STRdup (p+1);
			break;
		case 'L':
			st.lname = STRfree (st.lname);
			st.lname = STRdup (p+1);
			break;
		default:
			break;
		}
	}
}

LOCAL_FUNC OBJECT * NEAR
create_viewer (int type, int nzoom)
{
	OBJECT	*p;

	if (T(p = create_object (O_VIEWER, 1))) {
		p->misc[0] = type;
		save_viewport (p);
		zoom (p->viewport, nzoom);
	} else
		MsgEPrintf (-50, "no viewer %u", type);
	return (p);
}

extern void FAR
initialize (char *argv[])
{
	POINTER	*ptr;
	char	*p;
	int	gotit;
	int	i;

	st.object_id = 1000;			/* reserve 1-999 */
	st.paralax = 12;
	st.focus = VMAX;
	st.gap = 64;
	st.quiet = 1;
	st.gravity = (int) (C_G*VONE);
	st.windows = WIN_FULL;
	st.flags  |= SF_PAUSEMSG;
	st.flags1 |= SF_USEG;
	st.info = 1;
	st.extview = HDT_RADAR;
	st.maxbuffers = 200;
	st.maxrecall = 20;
	st.SkyLines = 50;			/* sky lines */
	st.network |= NET_AUTOACCEPT;
	st.interval_max = 100;
	st.LogFlush_rate = 1000;
	st.nMacros = 256;
	st.frame_rate = MIN_FRAME_RATE;
	st.maint_rate = 60;
	st.AutoConnect_rate = 5000;

/* initial palette settings
*/
	st.palette[CC_BLACK]   = C_BLACK;
	st.palette[CC_RED]     = C_RED;
	st.palette[CC_BLUE]    = C_BLUE;
	st.palette[CC_MAGENTA] = C_MAGENTA;
	st.palette[CC_GREEN]   = C_GREEN;
	st.palette[CC_BROWN]   = C_BROWN;
	st.palette[CC_GRAY]    = C_GRAY;
	st.palette[CC_DYELLOW] = C_DYELLOW;
	st.palette[CC_YELLOW]  = C_YELLOW;
	st.palette[CC_LRED]    = C_LIGHTRED;
	st.palette[CC_LBLUE]   = C_LIGHTBLUE;
	st.palette[CC_LGRAY]   = C_LIGHTGRAY;
	st.palette[CC_DBROWN]  = C_DARKBROWN;
	st.palette[CC_SKYBLUE] = C_SKYBLUE;
	st.palette[CC_DGREEN]  = C_DGREEN;
	st.palette[CC_WHITE]   = C_WHITE;

/* default pixel settings
*/
	for (i = 0; i < rangeof (st.colors); ++i)
		st.colors[i] = -1;

/* default colors assignements
*/
	ST_IFG     = CC_WHITE;
	ST_MFG     = CC_WHITE;
	ST_WFG     = CC_GREEN;
	ST_CFG     = CC_RED;
	ST_HFG     = CC_DYELLOW;
	ST_HFGI    = CC_YELLOW;
	ST_HBO     = CC_GRAY;
	ST_SLEFT   = CC_RED;
	ST_SRIGHT  = CC_BLUE;
	ST_SBOTH   = CC_MAGENTA;
	ST_GROUND  = CC_GRAY;
	ST_DULL    = CC_GRAY;
	ST_FAINT   = CC_LGRAY;
	ST_SKY     = CC_SKYBLUE;
	ST_FRIEND  = CC_LBLUE;
	ST_FOE     = CC_LRED;
	ST_HELP    = CC_GREEN;
	ST_FIRE1   = CC_WHITE;
	ST_FIRE2   = CC_RED;
	ST_MENU    = CC_LGRAY;
	ST_MENUH   = CC_WHITE;
	ST_SUN     = CC_WHITE;
	ST_EARTH   = CC_DBROWN;
	ST_COMPASS = CC_GREEN;
	ST_MSGERR  = CC_RED;
	ST_MSGWARN = CC_MAGENTA;
	ST_LAMPOFF = CC_GRAY;
	ST_LAMPOK  = CC_GREEN;
	ST_LAMPERR = CC_RED;

	sim_set ();

	Sys = &SysNone;
	Tm = &TmNone;
	Snd = &SndNone;
	Kbd = &KbdNone;

/* First we bring up the system and time drivers, these are essential.
*/

	if (mem_init ()) {
		LogPrintf ("memory mgr init failed\n");
		die ();
	}

	get_first_opts (argv+1);

	if (F(Sys = &SysDriver) || (Sys->Init && Sys->Init (NULL))) {
		Sys = &SysNone;
		LogPrintf ("system mgr init failed\n");
		die ();
	}

	if (F(Tm = &TmDriver) || (Tm->Init && Tm->Init (NULL))) {
		Tm = &TmNone;
		LogPrintf ("timer mgr init failed\n");
		die ();
	}
	st.big_bang = Tm->Milli ();

	Frandomize ();

	if (log_init ())
		die ();
TRACE ();

	if (msg_init ()) {
		LogPrintf ("no messages\n");
		die ();
	}
TRACE ();

	LogPrintf ("Fly8 start: %s\n", Tm->Ctime ());
	LogPrintf ("Program  %s\n", argv[0]);
	welcome (1);

	if (get_iniopts () || get_envopts () || get_argopts (argv+1))
		die ();
TRACE ();

	if (st.timeropts && Tm->Init (st.timeropts)) {
		Tm = &TmNone;
		LogPrintf ("timer re-init failed\n");
		die ();
	}
TRACE ();
	if (!st.nikname && F(st.nikname = STRdup ("JohnDoe"))) {
		LogPrintf ("no mem: nikname\n");
		die ();
	}
	if (!st.teamname && F(st.teamname = STRdup ("[*]"))) {
		LogPrintf ("no mem: teamname\n");
		die ();
	}

	Fsrand ((int)Tm->Milli ());		/* don't be boring */

	if (funcs_init ()) {
		LogPrintf ("funcs init failed\n");
		die ();
	}
TRACE ();

	Kbd = kbrd_init (st.kbdname);
	if (!Kbd || Kbd->Init (st.kbdname)) {
		Kbd = &KbdNone;
		LogPrintf ("keyboard init failed\n");
		die ();
	}
TRACE ();

/* In the next section 'Snd' is temporarily set to zero. Handle this
 * carefully since many programs use it blindly.
*/
	gotit = 0;
	if (F(Snd = sound_init (st.sndname)) || Snd->Init (st.sndname)) {
TRACE ();
		Snd = &SndNone;		/* always safe */
		LogPrintf ("sound init failed\n");
	} else
		gotit = 1;
TRACE ();
	if (!gotit && st.sndname &&
				(F(Snd = sound_init (0)) || Snd->Init (0))) {
TRACE ();
		Snd = &SndNone;		/* always safe */
		LogPrintf ("default sound init failed\n");
	} else
		gotit = 1;

TRACE ();
	if (!gotit && (F(Snd = sound_init ("")) || Snd->Init (""))) {
TRACE ();
		Snd = &SndNone;		/* always safe */
		LogPrintf ("'%s' sound init failed\n", Snd->name);
		die ();
	}
TRACE ();
	st.sndname = STRfree (st.sndname);
TRACE ();
	st.sndname = STRdup (Snd->name);
TRACE ();

	if (F(NEW (CS))) {
		LogPrintf ("out of memory [%s(%u)]\n", __FILE__, __LINE__);
		die ();
	}

	if (F(NEW (CW))) {
		LogPrintf ("out of memory [%s(%u)]\n", __FILE__, __LINE__);
		die ();
	}

	if (F(NEW (CP))) {
		LogPrintf ("out of memory [%s(%u)]\n", __FILE__, __LINE__);
		die ();
	}

	if (F(Gr = devices_init (st.grname))) {
		LogPrintf ("devices init failed\n");
		die ();
	}
TRACE ();

	CD = devices_select (st.grmname);
TRACE ();
	if (CD == 0 || Gr->Init (CD, st.grname)) {
TRACE ();
		if (st.grmname) {
			LogPrintf ("no device: %s\n", st.grname);
			LogPrintf ("trying default\n");
			devices_release ();
			CD = devices_select (NULL);
		} else
			CD = 0;
TRACE ();
		if (CD == 0 || Gr->Init (CD, st.grname)) {
			devices_release ();
			LogPrintf ("no device\n");
			die ();
		}
	}
TRACE ();
	if (!Gr->SetVisual || !	Gr->SetActive)
		CD->npages = 1;

/* Default color assignments.
*/
	CS->FgColor = CC_WHITE;
	CS->BgColor = CC_BLACK;
	CS->BoColor = CC_LGRAY;

	for (i = 0; i < rangeof (st.hdd); ++i) {
		st.hdd[i].FgColor = CC_WHITE;
		st.hdd[i].BgColor = CC_BLACK;
		st.hdd[i].BoColor = CC_LGRAY;
	}

	set_palette ();
TRACE ();
	set_main ();
TRACE ();

/* Now we are in graphics mode!
*/
	st.grmname = STRfree (st.grmname);
	st.grmname = STRdup (CD->name);

	font_set (0);
	st.StFontSize = 8;

	if (mac_init ()) {
		LogPrintf ("no macros");
		die ();
	}
TRACE ();

	MsgPrintf (-100, "System   %s", Sys->name);
	MsgPrintf (-100, "Timer    %s", Tm->name);
	MsgPrintf (-100, "Graphics %s", Gr->name);
	MsgPrintf (-100, " vmodes  %s", st.vmdname);
	MsgPrintf (-100, " mode    %s", st.grmname);
	MsgPrintf (-100, "Sound    %s", Snd->name);
	MsgPrintf (-100, "Keyboard %s", Kbd->name);

	MsgPrintf (-100, "nBuffers %u", st.maxbuffers);

	if (edit_init ()) {
		LogPrintf ("no edit_str\n");
		die ();
	}
TRACE ();

	if (pointers_init ()) {
		LogPrintf ("no pointers\n");
		die ();
	}
TRACE ();

	if (bodies_init ()) {
		LogPrintf ("no bodies\n");
		die ();
	}
TRACE ();

	if (land_init ()) {
		LogPrintf ("no land\n");
		die ();
	}
TRACE ();

	if (nav_init ()) {
		LogPrintf ("no nav\n");
		die ();
	}
TRACE ();
	if ((st.home = nav_find (st.homename)) < 0) {
		LogPrintf ("bad home name \"%s\"\n", st.homename);
		st.home = 0;
	}

/* We are initialized, now get us a plane. First find the pointer.
*/
	if (F(ptr = pointer_select (st.ptrname))) {
		if (st.ptrname) {
			MsgEPrintf (-100, "no ptr: %s", st.ptrname);
			st.ptrname = STRfree (st.ptrname);
		}
		MsgPrintf (-100, "trying default");
		if (F(ptr = pointer_select (NULL))) {
			LogPrintf ("no pointer\n");
			die ();
		}
		st.ptrname = STRdup (ptr->name);
	}
TRACE ();
	MsgPrintf (-100, "Pointer  %s", ptr->name);

	CO = COT = 0;

	if (NULL == st.ptype)
		st.ptype = STRdup ("plane:basic");
	if ((st.p_name = body_name (p = get_siarg (st.ptype, 0))) < 0)
		LogPrintf ("unknown CC object \"%s\"\n", st.ptype);
	else
		MsgPrintf (-100, "CC is    %s", st.ptype);
	STRfree (p);

	if (NULL == st.dtype)
		st.dtype = STRdup (st.ptype);
	if ((st.d_name = body_name (p = get_siarg (st.dtype, 0))) < 0)
		LogPrintf ("unknown Drone object \"%s\"\n", st.dtype);
	else
		MsgPrintf (-100, "Drone is %s", st.ptype);
	STRfree (p);

	if (st.d_name < 0 || st.d_name < 0) {
		pointer_release (ptr);
		die ();
	}

	st.options = st.ptype;
	if (F(CC = create_object (st.p_name, 1))) {
		LogPrintf ("no CC %s\n", st.ptype);
		pointer_release (ptr);
		die ();
	}
TRACE ();
	CC->pointer = ptr;
	CC->flags |= F_CC | F_FRIEND;
	CC->color = CC_LBLUE;
	if (IS_PLANE (CC)) {
		CC->gpflags |= GPF_PILOT;
		ptr->l[9] = ptr->a[9] = 100;	/* brakes */
		place_plane (CC, st.home);
	}

	CV = CC;

/* Establish the virtual viewers.
*/
	create_viewer (HDT_FRONT, 0);
	create_viewer (HDT_REAR, 0);
	create_viewer (HDT_MAP, 0);
	create_viewer (HDT_RADAR, 0);
	create_viewer (HDT_TARGET, 12);
	create_viewer (HDT_PAN, 12);
	create_viewer (HDT_GAZE, 0);
	create_viewer (HDT_CHASE, 0);
	create_viewer (HDT_FOLLOW, 0);

	create_viewer (HDT_RIGHT, 0);
	create_viewer (HDT_LEFT,  0);

	create_viewer (HDT_MIRROR, -3);		/* somewhat wide angle */
TRACE ();

/* Now get the net going.
*/
	remote_init ();
TRACE ();

/* We made it, tell the world.
*/
	welcome (0);			/* welcome everybody */
TRACE ();
	if (st.quiet) {
		Snd->Effect (EFF_HELLO, SND_ON);
		if (st.quiet >= 2)
			Snd->Effect (EFF_ENGINE, SND_ON);
TRACE ();
	}

	cc_setup ();
TRACE ();

	double_buffer (SF_DBUFFERING);
TRACE ();

	st.flags |= SF_INITED;
	if (Gr->Shutters) {
		Gr->Shutters (-1);		/* turn on */
TRACE ();
	}

	st.Drone_next = st.present;

	{
		if (NULL == st.initkeys) {
			Ushort		keys[1];

			keys[0] = KF_INIT;
			mac_interpret (keys, rangeof (keys));
TRACE ();
		} else {
			Ushort		key, shift;
			int		i;
			int		l;
			Ushort		*keys;
			Ushort		*k;

			l = strlen (st.initkeys) * sizeof (*keys);
			keys = mem_alloc (l);
			if (F(keys)) {
				LogPrintf ("initkeys no mem\n");
				die ();
			}
			k = keys;
			shift = K_CTRL;
			for (i = 0; T(key = st.initkeys[i]); ++i) {
				if ('C' == key)
					shift = K_CTRL;
				else if ('A' == key)
					shift = K_ALT;
				else if (isalpha (key))
					*k++ = shift + tolower (key);
				else {
					MsgEPrintf (-100, "bad initkeys");
					break;
				}
			}
			mac_interpret (keys, (int)(k - keys));
			mem_free (p, l);
TRACE ();
		}
	}

TRACE ();
	log_flush (1);

	sim_reset ();
}
