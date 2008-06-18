/* --------------------------------- menus.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Menus.
*/

#include "plane.h"


static MENU FAR MenuYN[] = {
	{'y', "Yes"},
	{'n', "No"},
{'\0', 0}};

/* Select info level
*/

static MENU FAR MenuInfo[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "none"},		/* keep these in this exact order! */
	{'3', "timing"},
	{'4', "stats"},
	{'5', "game"},
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_info (void)
{
	int	sel;

	sel = menu_open (MenuInfo, !(st.flags1 & SF_INFO));

	switch (sel) {
	case MENU_ABORTED:
	case MENU_FAILED:
		break;
	case 0:
		st.flags1 &= ~SF_INFO;
		break;
	case 1:
		st.flags1 |= SF_INFO;
		break;
	default:
		st.info = (short)(sel - 2);
		st.flags1 |= SF_INFO;
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	return (0);
}

/* Remove all objects of type 'name'.
*/

LOCAL_FUNC void NEAR
clear_object (int name)
{
	OBJECT	*p;

	for (p = CO; p;) {
		if (p->name == name) {
			if (O_PLANE != name ||
			    (EE(p)->flags & PF_AUTO))
				p->flags |= F_DEL|F_MOD;
		}
		p = p->next;
	}
}

static MENU FAR MenuEmit[] = {
	{'t', "target"},	/*  0 */
	{'T', "  del"}, 	/*  1 */
	{'g', "gtarget"},	/*  2 */
	{'G', "  del"}, 	/*  3 */
	{'b', "box"},		/*  4 */
	{'B', "  del"}, 	/*  5 */
	{'-', "dell tgts"},	/*  6 */
	{'d', "drone"}, 	/*  7 */
	{'D', "  del"}, 	/*  8 */
	{'y', "drones"},	/*  9 */
	{'z', "killers"},	/* 10 */
	{'r', "game"},		/* 11 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_emit (void)
{
	int	sel, quit, i;
	char	msg[80], prompt[80];
	OBJECT	*p;

	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuEmit, sel);
		switch (sel) {
		default:
			quit = 1;
			break;
		case 0:
			create_object (O_TARGET, 1);
			break;
		case 1:
			clear_object (O_TARGET);
			st.flags1 &= ~SF_TESTING;
			break;
		case 2:
			create_object (O_GTARGET, 1);
			break;
		case 3:
			clear_object (O_GTARGET);
			st.flags1 &= ~SF_TESTING;
			break;
		case 4:
			create_object (O_BOX, 1);
			break;
		case 5:
			clear_object (O_BOX);
			break;
		case 6:
			clear_object (O_TARGET);
			clear_object (O_GTARGET);
			clear_object (O_BOX);
			st.flags1 &= ~SF_TESTING;
			break;
		case 7:
			emit_drone ();
			break;
		case 8:
			clear_object (O_PLANE);
			st.drones = 0;	 	/* no more drones */
			break;
		case 9:
			for (;;) {
				sprintf (prompt, "planes number(%d)",
					(int)st.drones);
				sprintf (msg, "%d", (int)st.drones);
				edit_str (prompt, msg, sizeof (msg));
				if ('\0' == msg[0])
					break;
				if (1 == sscanf (msg, "%u", &i) && i >= 0) {
					st.drones = i;
					break;
				}
			}
			break;
		case 10:
			for (;;) {
				sprintf (prompt, "killers number(%d)",
					(int)st.killers);
				sprintf (msg, "%d", (int)st.killers);
				edit_str (prompt, msg, sizeof (msg));
				if ('\0' == msg[0])
					break;
				if (1 == sscanf (msg, "%u", &i) &&
				    i >= 0 && i <= st.drones) {
					st.killers = i;
					break;
				}
			}
			break;
		case 11:
			st.info = 3;			/* show time & count */
			st.flags1 |= SF_TESTING|SF_INFO;
			st.ntargets = 10;
			st.nbullets = 0;
			st.test_start = st.present;
			for (p = CO; p;) {	/* kill boxes and targets */
				if (p->name == O_GTARGET || p->name == O_BOX ||
				    p->name == O_TARGET || p->name == O_M61 ||
				    p->name == O_MK82)
					p->flags |= F_DEL|F_MOD;
				p = p->next;
			}
			st.mscore = 0;
			for (i = 0; i < st.ntargets; ++i) {
				if (!create_object (O_TARGET, 1))
					break;
				/* 15 seconds + 25 bullets */
				st.mscore += 15*10 + 25*2;
			}
			(*CC->pointer->control->Key)(CC->pointer, KF_ORIGIN);
			(*CC->pointer->control->Key)(CC->pointer, KF_POWER_0);
			(*CC->pointer->control->Key)(CC->pointer, KF_LEVEL);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* debug menu
*/

static MENU FAR MenuDebug[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "toggle"},
	{'d', "debug"},		/*  3 */
	{'t', "trace"},		/*  4 */
	{'w', "gp w"},	 	/*  5 */
	{'x', "gp x"}, 		/*  6 */
	{'y', "gp y"},		/*  7 */
	{'z', "gp z"}, 		/*  8 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_debug (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 3;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuDebug, sel);
		switch (sel) {
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&st.flags, SF_DEBUG);
			break;
		case 4:
			SetOption (&st.debug, DF_TRACE);
			break;
		case 5:
			SetOption (&st.debug, DF_GPW);
			break;
		case 6:
			SetOption (&st.debug, DF_GPX);
			break;
		case 7:
			SetOption (&st.debug, DF_GPY);
			break;
		case 8:
			SetOption (&st.debug, DF_GPZ);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* options menu
*/

static MENU FAR MenuOpts[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "toggle"},
	{'v', "Version"},	/*  3 */
	{'s', "Smoke"}, 	/*  4 */
	{'f', "Font"},		/*  5 */
	{'c', "Colors"},	/*  6 */
	{'m', "Modes"}, 	/*  7 */
	{'k', "Sky"},		/*  8 */
	{'g', "Gravity"},	/*  9 */
	{'b', "Play Blues"},	/* 10 */
	{'V', "Verbose"},	/* 11 */
	{'n', "Net Stats"},	/* 12 */
	{'l', "Limited"},	/* 13 */
	{'N', "no stall"},	/* 14 */
	{'p', "paused msg"},	/* 15 */
	{'w', "win ident"},	/* 16 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_opts (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuOpts, sel);
		switch (sel) {
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			welcome (0);
			MsgPrintf (50, "%s at %dx%d", Gr->name,
				CD->sizex, CD->sizey);
			break;
		case 4:
			SetOption (&st.flags1, SF_SMOKE);
			break;
		case 5:
			set_lists (SF_FONT);
			break;
		case 6:
			set_lists (SF_COLORS);
			break;
		case 7:
			set_lists (SF_MODES);
			break;
		case 8:
			SetOption (&st.flags, SF_SKY);
			if (st.flags & SF_SKY)
				sky_init ();
			else
				sky_term ();
			repaint ();
			break;
		case 9:
			SetOption (&st.flags1, SF_USEG);
			break;
		case 10:
			if (st.quiet)
				Snd->List (TnBlues, SND_ON);
			break;
		case 11:
			SetOption (&st.flags, SF_VERBOSE);
			break;
		case 12:
			set_lists (SF_NET);
			break;
		case 13:
			if (IS_PLANE(CV))
				SetOption (&EE(CV)->flags, PF_LIMITED);
			break;
		case 14:
			if (IS_PLANE(CV))
				SetOption (&EE(CV)->flags, PF_NOSTALL);
			break;
		case 15:
			SetOption (&st.flags, SF_PAUSEMSG);
			break;
		case 16:
			SetOption (&st.flags1, SF_WIDENT);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* net menu
*/

LOCAL_FUNC int FAR
match_player (PLAYER *pl, char *name, int nlen, char *team, int tlen)
{
	if (name && strnicmp (pl->name, name, nlen))
		return (0);
	if (team && strnicmp (pl->team, team, tlen))
		return (0);
	return (1);
}

LOCAL_FUNC PLAYER * NEAR
match_players (int mode, char *fullname)
{
	char    *name, *team;
	int     nlen, tlen;
	PLAYER  *pl;

	if (T(team = strchr (fullname, ':'))) {
		nlen = (int)(team - fullname);
		if (!nlen)
			name = 0;
		else
			name = fullname;
		++team;
		tlen = strlen (team);
		if (!tlen)
			team = 0;
	} else {
		tlen = 0;
		name = fullname;
		nlen = strlen (name);
		if (!nlen)
			name = 0;
	}
	for (pl = 0; T(pl = player_next (pl));) {
		if ((pl->flags & mode) &&
		    match_player (pl, name, nlen, team, tlen))
			return (pl);
	}
	return (0);
}

/* returns: selected player, 0 (abort), no_players, all_team or all_players.
*/
LOCAL_FUNC PLAYER * NEAR
choose_player (int mode)
{
	int     i;
	PLAYER  *pl;
	char    msg[2*LNAME];

	for (;;) {
		for (i = 0, pl = 0; T(pl = player_next (pl));) {
			if (pl->flags & mode) {
				++i;
				MsgWPrintf (100, "%s:%s", pl->name, pl->team);
			}
		}
		if (!i)
			return (st.no_players);
		msg[0] = '\0';
		edit_str ("choose player", msg, sizeof (msg));
		if ('*' == msg[0])
			return (st.all_players);
		if ('+' == msg[0])
			return (st.all_team);
		if ('\0' == msg[0])
			return (0);
		if (T(pl = match_players (mode, msg)))
			return (pl);
	}
}

static MENU FAR MenuNet[] = {
	{'p', "ping"},	  /*  0 */
	{'y', "play"},	  /*  1 */
	{'q', "quit"},	  /*  2 */
	{'m', "message"},       /*  3 */
	{'a', "accept"},	/*  4 */
	{'d', "decline"},       /*  5 */
	{'A', "auto accept"},   /*  6 */
	{'D', "auto decline"},  /*  7 */
	{'r', "manual reply"},  /*  8 */
	{'c', "auto connect"},  /*  9 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_net (void)
{
	int     sel, i;
	char    msg[80];
	PLAYER  *pl, *pl1;

/* If no net, still go through the menu. This makes macros predictable.
*/
	if (!(st.network & NET_ON))
		MsgPrintf (50, "no net");

	sel = menu_open (MenuNet, 0);

	if (!(st.network & NET_ON)) {
		MsgPrintf (50, "no net");
		sel = MENU_ABORTED;
	}

	switch (sel) {
	default:
		break;
	case 0:		 /* ping */
		remote_ping ();
		break;
	case 1:		 /* play */
		pl = choose_player (~PL_PLAYING);
		if (pl == st.no_players)
			MsgPrintf (50, "no players");
		else if (pl == st.all_players || pl == st.all_team) {
			for (pl1 = 0; T(pl1 = player_next (pl1));) {
				if (!(pl1->flags & PL_PLAYING)) {
					if (pl == st.all_team &&
							stricmp (pl1->team,
							st.teamname))
						continue;
					remote_request (pl1);
				}
			}
		} else if (pl)
			remote_request (pl);
		break;
	case 2:		 /* quit */
		pl = choose_player (PL_PLAYING);
		if (pl == st.no_players)
			MsgPrintf (50, "no players");
		else if (pl == st.all_players) {
			MsgPrintf (50, "Quitting all");
			remote_noplay (st.all_active);
			players_remove (st.all_known);
		} else if (pl == st.all_team) {
			MsgPrintf (50, "Quitting team");
			remote_noplay (st.all_team);
			players_remove (st.all_team);
		} else if (pl) {
			MsgPrintf (50, "Quitting %s:%s", pl->name, pl->team);
			remote_noplay (pl);
			player_remove (pl);
		}
		break;
	case 3:		 /* message */
		pl = choose_player (~0);
		if (pl == st.no_players)
			MsgPrintf (50, "no players");
		else if (pl) {
			msg[0] = '\0';
			edit_str ("message text", msg, sizeof (msg));
			if (pl == st.all_players) {
				remote_time (st.all_active);
				remote_msg (msg, st.all_active);
			} else if (pl == st.all_team) {
				remote_time (st.all_team);
				remote_msg (msg, st.all_team);
			} else {
				remote_time (pl);
				remote_msg (msg, pl);
			}
		}
		break;
	case 4:		 /* accept */
	case 5:		 /* decline */
		i = (4 == sel);
		pl = choose_player (PL_PENDBOSS);
		if (pl == st.no_players)
			MsgPrintf (50, "no players");
		else if (pl == st.all_players || pl == st.all_team) {
			for (pl1 = 0; T(pl1 = player_next (pl1));) {
				if (pl1->flags & PL_PENDBOSS) {
					if (pl == st.all_team &&
							stricmp (pl1->team,
								st.teamname))
						continue;
					remote_reply (pl1, i);
				}
			}
		} else if (pl)
			remote_reply (pl, i);
		break;
	case 6:		 /* auto accept */
		st.network &= ~NET_AUTOREPLY;
		st.network |= NET_AUTOACCEPT;
		break;
	case 7:		 /* auto decline */
		st.network &= ~NET_AUTOREPLY;
		st.network |= NET_AUTODECLINE;
		break;
	case 8:		 /* manual reply */
		st.network &= ~NET_AUTOREPLY;
		break;
	case 9:		 /* auto connect */
		st.network &= ~NET_AUTOREPLY;
		st.network |= (NET_AUTOACCEPT|NET_AUTOCONNECT);
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	return (0);
}

/* stereo menu
*/

static MENU FAR MenuStr[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "toggle"},
	{'m', "Mono"},		/*  3 */
	{'s', "S'Scopic"},	/*  4 */
	{'b', "RedBlue"},	/*  5 */
	{'a', "Alternate"},	/*  6 */
	{'r', "Reverse"},	/*  7 */
	{'p', "Paralax"},	/*  8 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_stereo (void)
{
	int     sel, quit, ch;
	HMSG    *m;

	SetOption (NULL, 2);
	sel = st.stereo;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuStr, sel);
		switch (sel) {
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			sel = VIS_MONO;
			goto set_stereo_mode;
		case 4:
			sel = VIS_STEREOSCOPIC;
			goto set_stereo_mode;
		case 5:
			if (!Gr->SetWriteMode) {
				MsgPrintf (50, "Cannot do red/blue");
				break;
			}
			if (st.flags1 & SF_SOLIDSKY) {
				MsgPrintf (50, "Not with Solid Sky");
				break;
			}
			sel = VIS_REDBLUE;
			goto set_stereo_mode;
		case 6:
			if (!Gr->Shutters || Gr->Shutters (-3)) {
				MsgPrintf (50, "No shutters");
				quit = 1;
				break;
			}
			sel = VIS_ALTERNATING;
			goto set_stereo_mode;
set_stereo_mode:
			sim_set ();
			if (Gr->Shutters) {
				if (VIS_ALTERNATING == sel)
					Gr->Shutters (-1); /* shutters on */
				else
					Gr->Shutters (-2); /* shutters off */
			}
			if (VIS_STEREOSCOPIC == sel) {
				if (VIS_STEREOSCOPIC != st.stereo)
					set_small_frame ();
			} else if (VIS_STEREOSCOPIC == st.stereo)
				set_large_frame ();
			st.stereo = (short)sel;
			repaint ();
			sim_reset ();
			quit = 1;
			break;
		case 7:
			SetOption (&st.flags1, SF_STEREOREV);
			break;
		case 8:
			for (;;) {
				if (F(m = MsgPrintf (0, "paralx(%d) ?[+-]",
							(int)st.paralax)))
					break;
				ch = mgetch ();
				m = msg_del (m);
				if ('+' == ch)
					++st.paralax;
				else if ('-' == ch)
					--st.paralax;
				else if (KF_ESC == ch)
					break;
			}
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* screen menu
*/

static MENU FAR MenuScr[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'p', "Palette"},	/*  3 */
	{'c', "Colors"},	/*  4 */
	{'s', "Stereo"},	/*  5 */
	{'d', "Dbl Buff"},	/*  6 */
	{'b', "Blanker"},	/*  7 */
	{'h', "hudpos"},	/*  8 */
	{'S', "Solid Sky"},	/*  9 */
	{'g', "Graded Sky"},	/* 10 */
	{'C', "clear"},		/* 11 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_screen (void)
{
	int     sel, quit;
	Ushort  temp;

	SetOption (NULL, 2);
	sel = 0;
	do {
		quit = 1;
		sel = menu_open (MenuScr, sel);
		switch (sel) {
		default:
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			quit = 0;
			break;
		case 3:
			menu_palette ();
			break;
		case 4:
			menu_colors ();
			break;
		case 5:
			menu_stereo ();
			break;
		case 6:
			temp = st.flags1;
			SetOption (&temp, SF_DBUFFERING);
			if (temp != st.flags1)
				double_buffer (temp);
			break;
		case 7:
			temp = st.flags;
			SetOption (&temp, SF_BLANKER);
			if (temp != st.flags) {
				st.flags ^= SF_BLANKER;
				repaint ();
			}
			break;
		case 8:
			SetOption (&st.flags1, SF_HUDINFRONT);
			break;
		case 9:
			if (VIS_REDBLUE == st.stereo) {
				MsgPrintf (50, "Not with RedBlue stereo");
				break;
			}
			SetOption (&st.flags1, SF_SOLIDSKY);
			repaint ();
			break;
		case 10:
			if (Gr->SetPalette) {
				MsgPrintf (50, "Cannot set palette");
				break;
			}
			SetOption (&st.flags1, SF_GRADEDSKY);
			break;
		case 11:
			repaint ();
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	} while (!quit);

	return (0);
}

/* autopilot menu
*/

static MENU FAR MenuAuto[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "toggle"},
	{'f', "Flaps"},		/*  3 */
	{'e', "Elevators"},	/*  4 */
	{'d', "Rudder"},	/*  5 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_auto (void)
{
	int	sel, quit;

	if (!IS_PLANE(CV))
		return (0);

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuAuto, sel);
		switch (sel) {
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->flags, PF_AUTOFLAP);
			break;
		case 4:
			SetOption (&EE(CV)->flags, PF_AUTOELEVATOR);
			break;
		case 5:
			SetOption (&EE(CV)->flags, PF_AUTORUDDER);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* Top menu
*/

static MENU FAR MenuTop[] = {
	{'x', "Exit"},		/*  0 */
	{'h', "Help"},		/*  1 */
	{'p', "Pointer"},	/*  2 */
	{'s', "Screen"},	/*  3 */
	{'w', "Windows"},	/*  4 */
	{'i', "Info"},		/*  5 */
	{'e', "Emit"},		/*  6 */
	{'u', "Hud"},		/*  7 */
	{'n', "Net"},		/*  8 */
	{'o', "Options"},	/*  9 */
	{'a', "Auto"},		/* 10 */
	{'d', "Debug"}, 	/* 11 */
	{'b', "Buttons"}, 	/* 12 */
	{'c', "Command"}, 	/* 13 */
{'\0', 0}};

extern int FAR
menu_top (void)
{
	int	sel, ret, i;
	Ushort	list = 0;

	if (st.flags & SF_VERBOSE) {
		list = set_lists (0);
		set_lists (SF_MODES);
	}

	ret = 0;
	sel = menu_open (MenuTop, 0);

	switch (sel) {
	default:
		break;
	case 0:
		if (MENU_FAILED != (i = menu_open (MenuYN, 1))) {
			if (0 == i)	/* "Yes" */
				ret = 1;
			menu_close ();
		}
		break;
	case 1:
		set_lists (SF_HELP);
		break;
	case 2:
		menu_ptrs ();
		break;
	case 3:
		menu_screen ();
		break;
	case 4:
		menu_windows ();
		break;
	case 5:
		menu_info ();
		break;
	case 6:
		menu_emit ();
		break;
	case 7:
		if (!IS_PLANE(CV))
			break;
		menu_hud ();
		break;
	case 8:
		menu_net ();
		break;
	case 9:
		menu_opts ();
		break;
	case 10:
		menu_auto ();
		break;
	case 11:
		menu_debug ();
		break;
	case 12:
		menu_btn ();
		break;
	case 13:
		ret = menu_cmd ();
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	if (st.flags & SF_VERBOSE)
		set_lists (list);

	return (ret);
}
