/* --------------------------------- hddmenu.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Set hdd options.
 *
 * We need a bit more abstraction here. The vehicle can be describd as
 * having the following structures:
 *
 * - systems (scattered)
 *	Each has a private data structure to control it and to hold its
 *	status.
 * - visuals (the HDD_ types)
 *	Each has some parameters to control the display contents and
 *	format.
 * - displays (st.hdd[])
 *	Each has a position and dimensions, an assigned visual (maybe more
 *	than one for overlaid image?) and other info (color/mono, intensity,
 *	contrast etc.)
 *
 * todo:
 *
 * Define the new HD*_*** flags.
*/

#include "fly.h"


static MENU FAR	MenuHddFront[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'i', "instruments"},	/*  3 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hdd_front (int hdd)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHddFront, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&st.hdd[hdd].flags, HDf_INSTRUMENTS);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}


static MENU FAR	MenuHddMap[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'c', "compass"},	/*  3 */
	{'s', " square"},	/*  4 */
	{'o', " ortho"},	/*  5 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hdd_map (int hdd)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHddMap, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
			break;
		case 3:
			SetOption (&st.hdd[hdd].flags, HDm_COMPASS);
			break;
		case 4:
			SetOption (&st.hdd[hdd].flags, HDm_SQRCOMPASS);
			break;
		case 5:
			SetOption (&st.hdd[hdd].flags, HDm_ORTCOMPASS);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

static MENU FAR	MenuHddPanel[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'n', "nav"},		/*  3 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hdd_panel (int hdd)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHddPanel, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&st.hdd[hdd].flags, HDP_NAV);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}


static MENU FAR	MenuHddTarget[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'n', "vv"},		/*  3 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hdd_target (int hdd)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHddTarget, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&st.hdd[hdd].flags, HDt_TVV);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}


extern int FAR
hdd_setopts (int sel)
{
	int	sel;

	sel = menu_view (0);

	switch (sel) {
	case MENU_ABORTED:
	case MENU_FAILED:
		break;
	case  0:		/* front  */
		menu_hdd_front (sel);
		break;
	case  3:		/* map    */
	case  4:		/* radar  */
		menu_hdd_map (sel);
		break;
	case 12:		/* panel  */
		menu_hdd_panel (sel);
		break;
	case  5:		/* target */
	case  6:		/* pan    */
		menu_hdd_target (sel);
		break;
	case  1:		/* none   */
	case  2:		/* rear   */
	case  7:		/* gaze   */
	case  8:		/* chase  */
	case  9:		/* follow */
	case 10:		/* hud    */
	case 11:		/* up-front */
	case 13:		/* right  */
	case 14:		/* left   */
	case 15:		/* stores */
	case 16:		/* mirror */
		MasPrintf (50, "No options for this hdd");
		break;
	default:
		break;
	}
}
