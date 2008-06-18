/* --------------------------------- ptrmgr.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Pointing devices manager.
*/

#include "fly.h"

#define USELOG		0x0001		/* log scale on x/y (default) */

static int def_opt[NOPTS] = {
	1,		/* +ve */
	0,		/* x axis is channel 0 */
	1,		/* +ve */
	1,		/* y axis is channel 1 */
	1,		/* +ve */
	2,		/* rudder is channel 2 */
	1,		/* +ve */
	3,		/* throttle is channel 3 */
	1,		/* +ve */
	3,		/* hat is channel 3 */
	USELOG		/* default is log mode */
};


LOCAL_FUNC void NEAR btn_init (void);
LOCAL_FUNC void	NEAR get_btns (POINTER *p, char *options);

extern int FAR
pointers_init (void)
{
	btn_init ();
	return (0);
}

extern void FAR
pointers_term (void)
{}

static int
get_axis (char *options, char *name, int *opt, int direction, int channel)
{
	long	l;

	if (get_narg (options, name, &l)) {
		return (0);
	}
	if (l < 0) {
		opt[direction] = -1;
		l = -l;
	}
	if (l < 1 || l > 6) {
		LogPrintf ("ptrmgr: bad axis %s\n", name);
		return (1);
	}
	opt[channel] = (int)--l;

	return (0);
}

extern POINTER * FAR
pointer_select (char *name)
{
	int			l;
	int			n;
	char			*options;
	int			opt[NOPTS];
	POINTER			*ptr;
	struct PtrDriver	NEAR* FAR* p;

	memcpy (opt, def_opt, sizeof (opt));

	if (!name) {
		p = PtrDrivers;				/* use default */
		options = "";
		goto ret;
	}

/* separate name and options
*/
	if (T(options = strchr (name, ':')))
		n = options - name;
	else {
		n = strlen (name);
		options = "";
	}

/* get axes assignement
*/
	if (get_axis (options, "sx=", opt, IFA1D, IFA1F))
		return (NULL);
	if (get_axis (options, "sy=", opt, IFA2D, IFA2F))
		return (NULL);
	if (get_axis (options, "sr=", opt, IFA3D, IFA3F))
		return (NULL);
	if (get_axis (options, "st=", opt, IFA4D, IFA4F))
		return (NULL);
	if (get_axis (options, "sh=", opt, IFA5D, IFA5F))
		return (NULL);

	if (get_parg (options, "linear"))
		opt[IGOPTS] &= ~USELOG;

	for (p = PtrDrivers; *p; ++p) {
		if (!strnicmp ((*p)->name, name, n) && !(*p)->name[n])
			break;
	}
ret:
	if (!*p)
		return (NULL);

	if (F(NEW (ptr)))
		return (NULL);

	ptr->control = *p;
	ptr->name    = (*p)->name;
	memcpy (ptr->opt, opt, sizeof (ptr->opt));

	get_btns (ptr, options);

	for (l = 0; l < NANALOG; ++l) {
		ptr->a[l] = 0;
		ptr->l[l] = 0;
		ptr->low[l]  = 0x7fff;
		ptr->c[l]    = 0x4000;
		ptr->high[l] = 0x0000;
		ptr->play[l] = 0;
	}

	if (ptr->control->Init (ptr, options))
		DEL0 (ptr);

	return (ptr);
}

extern POINTER * FAR
pointer_release (POINTER *ptr)
{
	ptr->control->Term (ptr);
	DEL (ptr);
	return (0);
}

extern int FAR
pointer_read (POINTER *ptr, int transfer)
{
	int	i;

	if (0 != (i = (*ptr->control->Read)(ptr)))
		return (i);

	if (transfer) {
		for (i = 0; i < 2; ++i)
			ptr->l[i] = (ptr->opt[IGOPTS] & USELOG)
				? (short)lin2log ((xshort)ptr->a[i])
				: ptr->a[i];
		for (i = 2; i <= 9; ++i)
			ptr->l[i] = ptr->a[i];
	}
	return (0);
}

extern void FAR
std_key (POINTER *p, int key)
{
	int	i;

	switch (key) {
	case KF_POWER_AB:
		if (p->a[3] < 75)
			p->a[3] = 75;		/* full power first */
		else {
			p->a[3] += 5;
			if (p->a[3] > 100)
				p->a[3] = 100;
		}
		break;

	case KF_POWER_UP:
		i = (p->a[3] > 75) ? 100 : 75;
		p->a[3] += 5;
		if (p->a[3] > i)
			p->a[3] = (xshort)i;
		break;

	case KF_POWER_DOWN:
		p->a[3] -= 5;
		if (p->a[3] < 0)
			p->a[3] = 0;
		break;

	case KF_POWER_0:
		p->a[3] = 0;
		break;

	case KF_POWER_100:
		p->a[3] = 75;
		break;

	case KF_LEVEL:
		++p->b[0];
		break;

	case KF_ORIGIN:
		++p->b[1];
		break;

	case KF_FIRE:
		++p->b[2];
		break;

	case KF_RESET_ROLL:
		++p->b[4];
		break;

	case KF_STABLE:
		++p->b[3];
		break;

	case KF_RUDDER_LEFT:
		p->a[2] += 10;
		if (p->a[2] > 100)
			p->a[2] = 100;
		break;
	case KF_RUDDER_RIGHT:
		p->a[2] -= 10;
		if (p->a[2] < -100)
			p->a[2] = -100;
		break;
	case KF_RUDDER_CENTER:
		p->a[2] = 0;
		break;

	case KF_FLAPS_MORE:
		p->a[6] += 10;
		if (p->a[6] > 100)
			p->a[6] = 100;
		break;
	case KF_FLAPS_LESS:
		p->a[6] -= 10;
		if (p->a[6] < 0)
			p->a[6] = 0;
		break;

	case KF_SPOILERS_MORE:
		p->a[7] += 10;
		if (p->a[7] > 100)
			p->a[7] = 100;
		break;
	case KF_SPOILERS_LESS:
		p->a[7] -= 10;
		if (p->a[7] < 0)
			p->a[7] = 0;
		break;

	case KF_BRAKES_MORE:
		p->a[9] += 10;
		if (p->a[9] > 100)
			p->a[9] = 100;
		break;
	case KF_BRAKES_LESS:
		p->a[9] -= 10;
		if (p->a[9] < 0)
			p->a[9] = 0;
		break;

	case KF_SPEED_BRAKES_MORE:
		p->a[8] += 25;
		if (p->a[8] > 100)
			p->a[8] = 100;
		break;
	case KF_SPEED_BRAKES_LESS:
		p->a[8] -= 25;
		if (p->a[8] < 0)
			p->a[8] = 0;
		break;

	case KF_SPEED_BRAKES:
		if (p->a[8])
			p->a[8] = 0;
		else
			p->a[8] = 100;
		break;

	case KF_BRAKES:
		if (p->a[9])
			p->a[9] = 0;
		else
			p->a[9] = 100;
		break;

	case KF_GEAR:
		++p->b[5];
		break;

	case KF_RADAR_SELECT:
		++p->b[6];
		break;

/* Ctl Arrows used for trim.
*/
	case KF_TRIM_RIGHT:
		++p->a[4];
		if (p->a[4] > 100)
			p->a[4] = 100;
		break;
	case KF_TRIM_LEFT:
		--p->a[4];
		if (p->a[4] < -100)
			p->a[4] = -100;
		break;
	case KF_TRIM_DOWN:
		--p->a[5];
		if (p->a[5] < -100)
			p->a[5] = -100;
		break;
	case KF_TRIM_UP:
		++p->a[5];
		if (p->a[5] > 100)
			p->a[5] = 100;
		break;
	case KF_TRIM_IDLE:
		p->a[4] = p->a[5] = 0;
		break;
	}
}

/* Select Pointing Device
*/

extern int FAR
menu_ptrs (void)
{
	MENU	*MenuPtr;
	int	sel, i, nEntries, EntrySize;
	char	*oldptr, newptr[256], *p;
	POINTER	*ptr;

	for (nEntries = 0; PtrDrivers[nEntries]; ++nEntries)
		;
		;
	EntrySize = 20;

	if (F(MenuPtr = (MENU *) memory_calloc (nEntries+1,
							sizeof (*MenuPtr))))
		return (1);

	sel = MENU_FAILED;
	for (i = 0; i < nEntries; ++i)
		if (F(MenuPtr[i].text = (char *) memory_alloc (EntrySize)))
			goto ret;

	oldptr = st.ptrname;
	sel = 0;
	for (i = 0; i < nEntries; ++i) {
		MenuPtr[i].letter = (Uchar)menuch[i];
		strcpy (MenuPtr[i].text, PtrDrivers[i]->name);
		if (!stricmp (PtrDrivers[i]->name, oldptr))
			sel = i;
	}

	sel = menu_open (MenuPtr, sel);

	oldptr = st.ptrname;

	switch (sel) {
	case MENU_ABORTED:
	case MENU_FAILED:
		break;
	default:
		strcpy (newptr, PtrDrivers[sel]->name);
		strcat (newptr, ":");
		i = strlen (newptr);
		if (NULL != (p = strchr (oldptr, ':')))
			strcpy (newptr+i, p+1);
		edit_str ("pointer options:", newptr + i, sizeof (newptr) - i);

		sim_set ();
		if (T(ptr = CV->pointer))
			CV->pointer = pointer_release (ptr);
		for (;;) {
			if (T(ptr = pointer_select (newptr))) {
				CV->pointer = ptr;
				st.ptrname = STRfree (st.ptrname);
				st.ptrname = STRdup (newptr);
				break;
			}
			MsgEPrintf (-100, "pointer init failed");
			if (T(ptr = pointer_select (oldptr))) {
				CV->pointer = ptr;
				break;
			}
			MsgEPrintf (-100, "old pointer init failed");
			if (oldptr) {
				if (T(ptr = pointer_select (NULL))) {
					CV->pointer = ptr;
					st.ptrname = STRfree (st.ptrname);
					st.ptrname = STRdup (ptr->name);
					break;
				}
			}
			LogPrintf ("default pointer init failed\n");
			die ();
		}
		sim_reset ();
		break;
	}

ret:
	for (i = 0; i < nEntries; ++i)
		if (MenuPtr[i].text)
			memory_free (MenuPtr[i].text, EntrySize);

	MenuPtr = memory_cfree (MenuPtr, nEntries+1, sizeof (*MenuPtr));

	if (MENU_FAILED == sel)
		return (1);

	menu_close ();
	return (0);
}


/* From here on: buttons stuff.
*/

#define BTN_POSITION	0x0001
#define BTN_DEBOUNCE	0x0080

static char	btn_order[] =
	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static Uchar	btn_decode[256] = {0};

LOCAL_FUNC void NEAR
btn_init (void)
{
	int	i;

	for (i = 0; i < rangeof (btn_decode); ++i)
		btn_decode[i] = (Uchar)255;
	for (i = 0; i < rangeof (btn_order) - 1; ++i)
		btn_decode[((Uchar *)btn_order)[i]] = (Uchar)i;
}

LOCAL_FUNC int NEAR
btn_code (int c)
{
	if (c < 0 || c >= rangeof (btn_decode))
		c = -1;
	else if ((c = btn_decode[c]) == 255)
		c = -1;
	return (c);
}

LOCAL_FUNC void NEAR
get_btn (POINTER *p, char *options, char *opt, Ushort mode)
{
	char	*s;
	int	n;
	int	prev;
	int	quit;

	if (F(s = get_parg (options, opt)))
		return;

	for (prev = 0, quit = 0; !quit;) {
		if ('-' == (n = *s++)) {
			if ((n = btn_code (*s++)) < 0) {
				quit = 1;
				n = NBTNS - 1;
			}
			while (prev <= n)
				p->btn[prev++] |= mode;
		} else if ((n = btn_code (n)) >= 0)
			p->btn[n] |= mode;
		else
			quit = 1;
		prev = n;
	}
}

/* read button debounce definition.
*/
LOCAL_FUNC void NEAR
get_btns (POINTER *p, char *options)
{
	int	n;

	for (n = 0; n < NBTNS; ++n)
		p->btn[n] = 0;

	get_btn (p, options, "d=", BTN_DEBOUNCE);
	get_btn (p, options, "r=", K_RLS);

	get_btn (p, options, "a=", K_ALT);
	get_btn (p, options, "c=", K_CTRL);
	get_btn (p, options, "p=", K_SPECIAL);
	get_btn (p, options, "s=", K_SHIFT);

/* remember that debounce and release have a negative logic.
*/
	for (n = 0; n < NBTNS; ++n)
		p->btn[n] ^= (BTN_DEBOUNCE | K_RLS);
}

/* handle a button press with debouncing.
*/
LOCAL_FUNC void NEAR
do_btn (POINTER *p, int button, int state)
{
	Ushort	cmd[1];

	if (state)
		state = BTN_POSITION;

	if ((int)(BTN_POSITION & p->btn[button]) != state)	/* toggle */
		p->btn[button] ^= BTN_POSITION;
	else if (BTN_DEBOUNCE & p->btn[button])
		return;					/* debounce */

/* do not interfere with user keystrokes.
*/
	if (st.flags & SF_INTERACTIVE)
		return;

	if (state)
		cmd[0] = 0;
	else if (K_RLS & p->btn[button])
		cmd[0] = K_RLS;
	else
		return;

	cmd[0] |= K_BTN | (st.btnmode & p->btn[button] & K_MODES)
							| menuch[button];
	mac_interpret (cmd, rangeof (cmd));
}

extern void FAR
do_btns (POINTER *p, char *btn, int size)
{
	int	i;

/* button releases
*/
	for (i = 0; i < size; ++i)
		if (!btn[i])
			do_btn (p, i, 0);
	for (; i < NBTNS; ++i)
		if (!(p->btn[i] & BTN_POSITION))
			do_btn (p, i, 0);

/* button presses
*/
	for (i = 0; i < size; ++i)
		if (btn[i])
			do_btn (p, i, 1);
	for (; i < NBTNS; ++i)
		if (p->btn[i] & BTN_POSITION)
			do_btn (p, i, 1);
}

extern void FAR
do_bchar (POINTER *p, int bchar, int state)
{
	int	button;

	if (NULL == CC->pointer)
		return;
	if ((button = btn_code (bchar)) < 0)
		return;
	do_btn (CC->pointer, button, state);
}

/* Set buttons mode.
*/
static MENU FAR MenuBtn[] = {
	{'0', "off"},
	{'1', "on"},
	{'2', "toggle"},
	{'a', "Alt"},		/*  3 */
	{'c', "Ctrl"},		/*  4 */
	{'s', "Shift"},		/*  5 */
	{'p', "sPecial"},	/*  6 */
	{'d', "Debounce"},	/*  7 */
	{'r', "Release"},	/*  8 */
	{'x', "Clear"},		/*  9 */
	{'*', "Cancel"},	/* 10 */
{'\0', 0}};

extern int FAR
menu_btn (void)
{
	int	sel, quit, ch;
	Ushort	i;
	int	j;
	HMSG	*m;
	POINTER *p;

	if (F(p = CC->pointer))
		MsgWPrintf (50, "No pointer!");

	m = MsgEPrintf (0, "enter button name:");
	do {
		ch = mgetch ();
		j = btn_code (ch);
	} while (j < 0);
	msg_del (m);

	m = MsgEPrintf (0, "defining button %c", ch);
	i = p ? p->btn[j] : 0;

	SetOption (NULL, 2);		/* default mode: toggle */
	sel = 2;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuBtn, sel);
		switch (sel) {
		case MENU_ABORTED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&i, K_ALT);
			break;
		case 4:
			SetOption (&i, K_CTRL);
			break;
		case 5:
			SetOption (&i, K_SHIFT);
			break;
		case 6:
			SetOption (&i, K_SPECIAL);
			break;
		case 7:
			SetOption (&i, K_RLS);
			break;
		case 8:
			SetOption (&i, BTN_DEBOUNCE);
			break;
		case 9:
			i = 0;
			SetOption (NULL, 1);
			break;
		case 10:
			i = p ? p->btn[j] : 0;
			quit = 1;
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	if (p)
		p->btn[j] = i;
	msg_del (m);
	return (0);
}
#undef NO_DEBOUNCE

