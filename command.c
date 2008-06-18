/* --------------------------------- command.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* process user commands.
*/

#include "plane.h"


extern Ushort FAR
set_lists (Ushort list)
{
	Ushort	old;

	old = st.flags & SF_LISTS;
	st.flags &= ~SF_LISTS;
	if (list && list != old)
		st.flags |= list;
	return (old);
}

LOCAL_FUNC int NEAR
one_command (int ch)
{
	int		i, j;
	char		msg[80];
	char		m[80];
	int		cv;
	OBJECT		*p;
	E_PLANE		*ecv;
	VIEWPORT	*vp;
	int		ret;
	Ushort		temp;
	ANGLE		a;

	ecv = IS_PLANE(CV) ? EE(CV) : 0;

	ret = 0;
	switch (ch) {

#define WSTEP	FONE/64		/* ++++++++++++ start debug keys ++++++++++ */

/* arrows */	case KF_XRIGHT:					/* move */
		case KF_XLEFT:
		case KF_XUP:
		case KF_XDOWN:
			if (CP->zoom > 0) {
				i = 1 << (CP->zoom / 3);
				j = CP->zoom % 3;
				if (1 == j)
					i += fmul (i, FCON(0.26));
				else if (2 == j)
					i += fmul (i, FCON(0.59));
			} else
				i = 1;
			a = (ANGLE)(D90/(18*i));
			if (KF_XRIGHT == ch)
				CP->rotz += a;
			else if (KF_XLEFT == ch)
				CP->rotz -= a;
			else if (KF_XUP == ch)
				CP->rotx += a;
			else
				CP->rotx -= a;
			if (iabs (CP->rotz) < (Uint)a/2)
				CP->rotz = 0;
			if (iabs (CP->rotx) < (Uint)a/2)
				CP->rotx = 0;
			break;
#if 0
		case KF_RIGHT:
			if (CW->orgx + CW->maxx < FONE-WSTEP) {
				CW->orgx += WSTEP;
				repaint ();
			}
			break;
		case KF_XLEFT:
			if (CW->orgx - CW->maxx > WSTEP) {
				CW->orgx -= WSTEP;
				repaint ();
			}
			break;
		case KF_XUP:
			if (CW->orgy - CW->maxy > WSTEP) {
				CW->orgy -= WSTEP;
				repaint ();
			}
			break;
		case KF_XDOWN:
			if (CW->orgy + CW->maxy < FONE-WSTEP) {
				CW->orgy += WSTEP;
				repaint ();
			}
			break;
#endif

/* AltArrows */	case KF_YRIGHT:					/* resize x */
			CW->maxx += WSTEP;
			if (CW->orgx + CW->maxx > FONE)
				CW->maxx = FONE - CW->orgx;
			if (CW->orgx - CW->maxx < 0)
				CW->maxx = CW->orgx;
			repaint ();
			break;
		case KF_YLEFT:
			CW->maxx -= WSTEP;
			if (CW->maxx < FONE/128)
				CW->maxx = FONE/128;
			repaint ();
			break;

		case KF_YUP:					/* resize y */
			CW->maxy += WSTEP;
			if (CW->orgy + CW->maxy > FONE)
				CW->maxy = FONE - CW->orgy;
			if (CW->orgy - CW->maxy < 0)
				CW->maxy = CW->orgy;
			repaint ();
			break;
		case KF_YDOWN:
			CW->maxy -= WSTEP;
			if (CW->maxy < FONE/128)
				CW->maxy = FONE/128;
			repaint ();
			break;

#undef WSTEP		/* ++++++++++++++ end   debug keys ++++++++++++++ */

		case KF_ZOOMIN:
			i = 1;
			goto main_zoom;
		case KF_ZOOMOUT:
			i = -1;
		main_zoom:
			if (T(vp = CP))
				goto zoomit;
			break;

		case KF_VZOOMIN:
			i = 1;
			goto ext_zoom;
		case KF_VZOOMOUT:
			i = -1;
		ext_zoom:
			if (st.flags1 & SF_EXTVIEW)
				goto main_zoom;
			if (scenery (st.extview)
					&& T(p = get_viewer (st.extview))
					&& T(vp = p->viewport))
				goto zoomit;
			break;

		zoomit:
			zoom (vp, i);
			update_viewer ();
			break;

		case KF_MENU:
			ret = menu_top ();
			break;

		case 'B':
			menu_btn ();
			break;
		case 'c':
			msg_clear (0);
			set_lists (0);
			break;
		case 'C':
			if (ecv)
				SetOption (&ecv->flags, PF_CHASE);
			break;
		case 'd':
			if (ecv)
				SetOption (&ecv->hudmode, HM_DECLUTTER);
			break;
		case 'D':
			if (O_CHUTE == CV->name) {
				i = 5*2*VONE;		/* 2 seconds worth */
				if (CV->R[Z] > i)
					CV->R[Z] = i;
				if (CV->V[Z] > 0)
					CV->V[Z] = 0;
			}
			break;
		case 'E':
			if (CC->gpflags & GPF_PILOT)
				eject (CC);
			break;
		case 'f':
			if (ecv) {
				if (ecv->radar & R_SELECT3)
					ecv->radar ^= R_SELECT3|R_SELECT20;
				else if (ecv->radar & R_SELECT20)
					ecv->radar ^= R_SELECT20|R_SELECT5;
				else if (ecv->radar & R_SELECT5)
					ecv->radar ^= R_SELECT5;
				else
					ecv->radar ^= R_SELECT3;
				ecv->target = 0;	/* re-select target */
			}
			break;
		case 'h':
		case '?':
			set_lists (SF_HELP);
			break;
		case 'i':
			if (ecv)
				SetOption (&ecv->radar, R_INTEL);
			break;
		case 'j':
			if (ecv)
				SetOption (&ecv->radar, R_INTELCC);
			break;
		case 'k':
			if (ecv)
				SetOption (&ecv->flags, PF_KILL);
			break;
		case 'l':
			if (ecv)
				SetOption (&ecv->radar, R_LOCK);
			break;
		case 'm':
			set_lists (SF_MODES);
			break;
		case 'M':
			temp = st.btnmode;
			SetOption (NULL, 2);
			for (ch = 0; ch >= 0;) {
				ch = mgetch ();
				switch (ch) {
				case '0':
				case '1':
				case '2':
					SetOption (NULL, (Ushort)(ch - '0'));
					break;
				case 'a':
					SetOption (&temp, K_ALT);
					break;
				case 'c':
					SetOption (&temp, K_CTRL);
					break;
				case 's':
					SetOption (&temp, K_SHIFT);
					break;
				case 'p':
					SetOption (&temp, K_SPECIAL);
					break;
				case 'x':
					temp = 0;
					SetOption (NULL, 1);
					break;
				case '*':
					ch = -1;	/* cancel */
					break;
				default:
					st.btnmode = temp;
					ch = -1;	/* quit */
					break;
				}
			}
			break;
		case 'n':
			set_lists (SF_NET);
			break;
		case 'o':
		case 'O':
			if (st.network & NET_INITED) {
				MsgWPrintf (50, "Net active!");
				break;
			}
#if 0
			flags = st.flags;
			st.flags |= SF_PAUSED;
#endif
			cv = 0;
			for (i = 1, p = CO; p; p = p->next, ++i) {
				if (ch == 'o' && (p->name == O_M61 ||
						  p->name == O_MK82 ||
						  p->name == O_BROKEN))
					continue;
				if (p == CV) {
					j = 'V';
					cv = i;
				} else if (p->flags & F_CC)
					j = 'C';
				else if (IS_PLANE(CC) && p == EE(CC)->target)
					j = 'L';
				else
					j = ' ';
				sprintf (msg, "%c%3u ", j, i);
				sprintf (m, "%-6s", TITLE(p));
				strcat (msg, m);
				if (p->flags & F_IMPORTED) {
					if (p->rplayer)
						sprintf (m, " %s",
							p->rplayer->name);
					else
						sprintf (m, " I");
				} else if (p->flags & F_EXPORTED)
					sprintf (m, " ***");
				else
					m[0] = '\0';
				strcat (msg, m);
				sprintf (m, " (%ld,%ld,%ld)",
					vuscale (p->R[X]),
					vuscale (p->R[Y]),
					vuscale (p->R[Z]));
				strcat (msg, m);
				MsgPrintf (200, msg);
			}
			do {
				if (cv)
					sprintf (msg, "%d", cv);
				else
					msg[0] = '\0';
				edit_str ("choose viewer", msg, sizeof (msg));
				if ('\0' == msg[0]) {
					j = 0;
					break;
				}
				if ('c' == msg[0]) {
					j = -1;
					break;
				} else if ('l' == msg[0]) {
					j = -2;
					break;
				}
			} while (1 != sscanf (msg, "%u", &j) ||
					j < 0 || j >= i);
			if (0 == j)
				p = 0;
			else if (-1 == j)
				p = CC;
			else if (-2 == j) {
				if (IS_PLANE(CC))
					p = EE(CC)->target;
			} else {
				for (i = 1, p = CO; p && i != j; ++i)
					p = p->next;
			}
			if (p) {
#if 0
				if (!IS_PLANE(p))
					goto again;
#endif
				save_viewport (CV);
				get_viewport (p);
				CV = p;
			}
#if 0
			st.flags = flags;
#endif
			break;
		case 'p':
			temp = st.flags;
			SetOption (&temp, SF_PAUSED);
			pause_set (temp);
			break;
		case 'P':
			i = mgetch ();
			st.flags &= ~SF_INTERACTIVE;
			do_bchar (CC->pointer, i, 1);
			break;
		case 'q':
			st.quiet = (st.quiet+1)%3;
			if (2 == st.quiet) {
				Snd->Effect (EFF_ENGINE, SND_ON);
				MsgPrintf (20, "Sound on");
			} else {
				Snd->Effect (EFF_ENGINE, SND_OFF);
				if (0 == st.quiet) {
					Snd->List (NULL, SND_OFF);
					st.sounds = 0;
					MsgPrintf (20, "Sound off");
				} else
					MsgPrintf (20, "Sound on, no engine");
			}
			break;
		case 'r':
			if (ecv)
				SetOption (&ecv->radar, R_ON);
			break;
		case 'R':
			i = mgetch ();
			st.flags &= ~SF_INTERACTIVE;
			do_bchar (CC->pointer, i, 0);
			break;
		case 's':
			set_lists (SF_STATS);
			break;
		case 'S':
			if (ecv)
				supply (CV, 1);
			break;
		case 'u':
			if (ecv)
				menu_hud ();
			break;
		case 'v':
			temp = st.flags1;
			SetOption (&temp, SF_EXTVIEW);
			if ((temp ^ st.flags1) & SF_EXTVIEW)
				swap_extview ();
			break;
		case 'w':
			if (ecv)
				ecv->weapon = (ecv->weapon+1)%3;
			break;
		case 'W':
			if (ecv)
				memset (ecv->stores, 0, sizeof (ecv->stores));
			break;
		case 'x':
			(*CC->pointer->control->Cal)(CC->pointer);
			break;
		case '-':
			CP->rotz += D90/2;
			break;
		case '*':
			CP->rotx = 0;
			CP->rotz = 0;
			CP->rotz = 0;
			break;
		case '/':
			CP->rotz -= D90/2;
			break;
		case '!':
			if (!Sys->Shell) {
				MsgWPrintf (5, "shell not supported\n");
				break;
			}
			Gr->Term (CD);
			Sys->Shell ();
			if (Gr->Init (CD, st.grname)) {
				LogPrintf ("device init failed after shell\n");
				die ();
			}
			set_palette ();
			repaint ();
			break;
		default:
			(*CC->pointer->control->Key)(CC->pointer, ch);
			break;
	}
	return (ret);
}

LOCAL_FUNC void NEAR
misc_actions (void)
{
	int	n, r;
	OBJECT	*p;
	HMSG	*m;

	if (st.drones) {			/* top up planes */
		for (n = r = 0, p = CO; p; p = p->next) {
			if (p->name == O_PLANE && !(p->flags & F_CC) &&
			    !(p->flags & F_IMPORTED)) {
				++n;
				if (EX->radar & R_ON)
					++r;
			}
		}
		if (n < st.drones && TIMEOUT (st.Drone_next) >= 0) {
			emit_drone ();
			st.Drone_next = st.present + DRONE_RATE;
		}
		if (r < st.killers) {
			for (p = CO; p; p = p->next) {
				if (p->name == O_PLANE &&
				    !(p->flags & F_CC) &&
				    !(p->flags & F_IMPORTED) &&
				    !(EX->radar & R_ON)) {
					EX->radar |= R_ON|R_LOCK|R_INTELCC;
					EX->flags |= PF_CHASE | PF_KILL;
					EX->weapon = WE_M61;
					if (++r >= st.killers)
						break;
				}
			}
		}
	}

	if (st.ShutdownTime && TIMEOUT (st.ShutdownTime) >= 0) {
		static Ushort	keys[] = {KF_MENU, 'x', 'y'};

		mac_interpret (keys, rangeof (keys));
	}

	if ((st.flags1 & SF_TESTING) && !st.ntargets) {	/* score result */
		if (st.quiet)
			Snd->Effect (EFF_GONE, SND_ON);

		MsgPrintf (100, "All gone");
		m = MsgWPrintf (0, "Hit Any key");

		Kbd->Wait ();

		msg_del (m);

		if (st.quiet)
			Snd->Effect (EFF_GONE, SND_OFF);
		st.flags1 &= ~(SF_TESTING|SF_INFO);
	}

	mem_check ();			/* top up memory */

	if (st.network & NET_AUTOCONNECT) {
		if (TIMEOUT (st.AutoConnect_next) >= 0) {
			st.AutoConnect_next = st.present + st.AutoConnect_rate;
			remote_ping ();
		}
	}
}

extern int FAR
user_command (void)
{
	int	i, ch;

	do {
		sys_poll (0);
		misc_actions ();
		for (i = 0; -1 != (ch = mread ());) {
			SetOption (NULL, 2);
			st.flags |= SF_INTERACTIVE;
			i = one_command (ch);
			st.flags &= ~SF_INTERACTIVE;
			if (i)
				break;
			if (T(i = st.flags1 & SF_TERM))
				break;
		}
	} while (!i);
	return (i);
}


/* screen menu
*/

static MENU FAR MenuCmd[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
{'\0', 0}};

extern int FAR
menu_cmd (void)
{
	int	sel, ret;
	HMSG	*m;

	ret = 0;
	SetOption (NULL, 2);
	sel = menu_open (MenuCmd, 2);
	switch (sel) {
	case MENU_ABORTED:
	default:
		break;
	case 0:
	case 1:
	case 2:
		SetOption (NULL, (Ushort)sel);
		m = MsgWPrintf (0, "Enter any command");
		ret = one_command (mgetch ());
		msg_del (m);
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	return (ret);
}
