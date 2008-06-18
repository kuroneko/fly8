/* --------------------------------- editstr.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* User input with editing. This is built around C code from a mag (forget
 * which one) but the original author's note follows.
*/

/* Get input string, with VMS-style input line editing
 * and previous-command scrolling.
 *
 * Written for Turbo C 2.0 / Borland C++ 2.0
 * Bob Bybee, 2/91
*/

/* Modified for usual msdos keys,
 * Eyal Lebedinsky, Feb 1992
 *
 * Use microsoft C. Original program compiled and run without any changes
 *   with this compiler.
 * Show insert/overtype mode as cursor size[sys_show].
 * Initial mode is overwrite!
 * User can select length for shortest string kept [shortest_saved].
 * If an earlier line is selected (with up/down arrows) and entered then it
 *   is not saved again.
 * Provide history search with PAGE_UP key: Previous entry that matches up
 *   to the cursor is retrieved.
 * Added keys:
 *   HOME		move to start of line (CTRL-B)
 *   END		move to end   of line (CTRL-E)
 *   ESC		clear line (CTRL-X)
 *   INSERT		toggle mode (CTRL-A)
 * Added keys with new functions:
 *   C_RIGHT_ARROW	move to next word start
 *   C_LEFT_ARROW	move to prev word start
 *   DEL		delete char at right of cursor
 *   C_END		delete right of cursor to end of line
 *   C_HOME		delete left of cursor to start of line
 *   PAGE_UP		search line prefix
 *   Ctrl-Y		Recall original line
 *   Ctrl-Q		Abort
*/

/* Modified for fly8
 * Eyal Lebedinsky, Nov 1992
 *
 * Removed escape-sequences for input.
 * Removed init function.
 * Modified to use fly8 key names.
 * Modified to interact with fly8 input/output.
 *
 * Eyal Lebedinsky, Aug 1994
 *
 * Split Fly8 stuff from the generic code. However, to simplify things, the
 * keyboard mapping is still shared.
*/

#include "fly.h"


/* ASCII key definitions
*/
#define ESC_KEY			K_ESC
#define DELETE_KEY		K_DEL
#define BACKSPACE_KEY		K_RUBOUT
#define RETURN_KEY 		K_ENTER
#define CTRL(x)			((x) | K_CTRL)

/* Values for tracking cursor key entry
*/
#define UP_ARROW	K_UP
#define DOWN_ARROW	K_DOWN
#define RIGHT_ARROW	K_RIGHT
#define LEFT_ARROW	K_LEFT
#define HOME		K_HOME
#define END		K_END
#define INSERT		K_INS
#define C_LEFT_ARROW	(K_LEFT  | K_CTRL)
#define C_RIGHT_ARROW	(K_RIGHT | K_CTRL)
#define C_END		(K_END   | K_CTRL)	
#define C_HOME		(K_HOME  | K_CTRL)	
#define PAGE_UP		K_PGUP	

#define RECSUB(x)	(((x) + max_recall) % max_recall)
#define RECALL(i)	recalls[i].s
#define RECLEN(i)	recalls[i].len
#define WSTR		RECALL(max_recall)
#define WLEN		RECLEN(max_recall)

struct recall {
	char	*s;
	int	len;
};


/* These three is all that is needed to define a history queue between
 * calls to editstr().
*/
static struct recall	FAR *recalls = 0;
static int		historyptr = 0;	/* ptr to last line entered */
static int		max_recall = 0;
static int		shortest_saved = 0;


#define INSERT_MODE	0x0001
#define INIT_MODE	INSERT_MODE	/* 1=insert 0=overwrite */

static int		num_got = 0;	/* # chars in input buffer */
static int		cursor_pos = 0;	/* cursor position on line */
static int		edit_mode = INIT_MODE;


/* prototypes for this file
*/
LOCAL_FUNC int	NEAR sys_term (void);
LOCAL_FUNC int	NEAR sys_init (void);
LOCAL_FUNC void	NEAR clear_line (void);
LOCAL_FUNC int	NEAR cursor_right (void);
LOCAL_FUNC int	NEAR cursor_left (void);
LOCAL_FUNC int	NEAR get_char_esc (void);
LOCAL_FUNC void	NEAR put_str (char FAR* str);

static void *	(FAR *sys_malloc) (int n) = NULL;
static void *	(FAR *sys_free) (void *p, int n) = NULL;
static void	(FAR *sys_put) (int ch) = NULL;
static int	(FAR *sys_get) (void) = NULL;
static void	(FAR *sys_show) (int edit_mode) = NULL;

/* Set up the editstr API.
*/
extern int FAR
editset (void *	(FAR *s_malloc) (int n),
	void *	(FAR *s_free) (void *p, int n),
	void	(FAR *s_put) (int ch),
	int	(FAR *s_get) (void),
	void	(FAR *s_show) (int edit_mode),
	int	s_saved,
	int	m_recall)
{
	sys_malloc = s_malloc;
	sys_free   = s_free;
	sys_put    = s_put;
	sys_get    = s_get;
	sys_show   = s_show;
	shortest_saved = s_saved;
	if (m_recall)
		max_recall = m_recall;

	return (0);
}

/* Get edit status.
*/
extern int FAR
editget (char **str, int *len, int *pos, int *mode)
{
	*str = WSTR;
	*len = num_got;
	*pos = cursor_pos;
	*mode = edit_mode & INSERT_MODE;
	return (0);
}

/* Do termination processing.
*/
LOCAL_FUNC int NEAR
sys_term (void)
{
	int	i;

	if (NULL == recalls)
		return (0);

	for (i = 0; i < (int)max_recall; ++i) {
		if (NULL != RECALL(i)) {
			RECALL(i) = (*sys_free) (RECALL(i), RECLEN(i));
			RECLEN(i) = 0;
		}
	}

	recalls = (*sys_free) (recalls, (max_recall+1) * sizeof (*recalls));

	return (0);
}

/* Do initial processing.
*/
LOCAL_FUNC int NEAR
sys_init (void)
{
	int	i;

	if (NULL != recalls)
		return (0);

	recalls = (struct recall *) (*sys_malloc) ((max_recall+1)
						* sizeof (*recalls));
	if (NULL == recalls)
		return (1);

	for (i = 0; i <= (int)max_recall; ++i) {
		RECALL(i) = NULL;
		RECLEN(i) = 0;
	}

	return (0);
}

/* Erase all characters on the current line.
*/
LOCAL_FUNC void NEAR
clear_line (void)
{
	int	i;

	while (cursor_left())		/* move to begining of line */
		;

	for (i = 0; i < num_got; ++i)
		(*sys_put)(' ');
	for (i = 0; i < num_got; ++i)
		(*sys_put)('\b');
	cursor_pos = num_got = 0;
}

/* Move the cursor right one position, by echoing the
 * character it's currently over.
 * Return 1-OK, 0-can't move.
*/
LOCAL_FUNC int NEAR
cursor_right (void)
{
	if (cursor_pos < num_got) {
		(*sys_put)(WSTR[cursor_pos]);
		++cursor_pos;
		return (1);
	}
	return (0);
}

/* Move the cursor left one position, by echoing
 * a backspace.  Return 1-OK, 0-can't move.
*/
LOCAL_FUNC int NEAR
cursor_left (void)
{
	if (cursor_pos > 0) {
		(*sys_put)('\b');
		--cursor_pos;
		return (1);
	}
	return (0);
}

/* Get a character, with escape processing.
 * Handles special sequences like "ESC [ A" for up-arrow.
 * This function would need to be modified to handle
 * keyboards that are neither PC's nor VT-100's.
*/
LOCAL_FUNC int NEAR
get_char_esc (void)
{
	int	ch;

	ch = (*sys_get)();
	if (ch != ESC_KEY)
		return (ch);

	return (ch);	/* no escapes! */
}

/* Put a string to (*sys_put)().
*/
LOCAL_FUNC void NEAR
put_str (char FAR* s)
{
	while (*s != '\0')
		(*sys_put)(*s++);
}


/* editstr() is called to get a line of input.
 * The input line is placed in "str" and will be no
 * more than "len" characters.
*/
extern int FAR
editstr (char FAR *str, int len)
{
	int	i, c;
	int	curptr;			/* current visible entry */
	int	GoOn;

	if (NULL == str && 0 == len)
		return (sys_term ());
	if (!recalls) {
		if (sys_init ())
			return (-1);
	}

	if (len < 1)
		return (-2);

	(*sys_show) (edit_mode = INIT_MODE);
	cursor_pos = 0;
	curptr = historyptr;

	WSTR = (char *) (*sys_malloc) (len);
	if (NULL == WSTR)
		return (-3);
	WLEN = len;
	strcpy (WSTR, str);			/* keep it */

	put_str (WSTR);
	cursor_pos = num_got = strlen (WSTR);

	for (GoOn = 1; GoOn;) {
		switch (c = get_char_esc()) {
		case RETURN_KEY:		/* done */
			GoOn = 0;
			break;
		case CTRL('q'):			/* abort */
			clear_line();
			return (1);
		case DELETE_KEY:		/* del char on right */
		case BACKSPACE_KEY:		/* del char on left */
			if ((c == DELETE_KEY && cursor_pos < num_got) ||
			    (c != DELETE_KEY && cursor_left())) {
				++cursor_pos;
				for (i = cursor_pos; i < num_got; ++i) {
					WSTR[i - 1] = WSTR[i];
					(*sys_put)(WSTR[i]);
				}
				(*sys_put)(' ');
				for (i = cursor_pos; i <= num_got; ++i)
					(*sys_put)('\b');
				--num_got;
				--cursor_pos;
			}
			break;
		case C_END:			/* erase to end-of-line */
			if (cursor_pos < num_got) {
				for (i = cursor_pos; i < num_got; ++i) {
					WSTR[i] = ' ';
					(*sys_put)(' ');
				}
				for (i = cursor_pos; i < num_got; ++i)
					(*sys_put)('\b');
				num_got = cursor_pos;
			}
			break;
		case C_HOME:			/* erase to beg-of-line */
			if (cursor_pos > 0) {
				for (i = 0; i < cursor_pos; ++i)
					(*sys_put)('\b');
				num_got -= cursor_pos;
				for (i = 0; i < num_got; ++i) {
					WSTR[i] = WSTR[cursor_pos+i];
					(*sys_put)(WSTR[i]);
				}
				for (i = 0; i < cursor_pos; ++i)
					(*sys_put)(' ');
				for (i = cursor_pos + num_got; i-- > 0;)
					(*sys_put)('\b');
				cursor_pos = 0;
			}
			break;
		case CTRL('x') :		/* erase all line */
		case ESC_KEY:
			clear_line();
			break;
		case CTRL('a'):			/* insert/overtype */
		case INSERT:
			(*sys_show) (edit_mode ^= INSERT_MODE);
			break;
		case CTRL('b'):			/* cursor to beg-of-line */
		case HOME:
			while (cursor_left())
				;
			break;
		case CTRL('e'):			/* cursor to end-of-line */
		case END:
			while (cursor_right())
				;
			break;
		case CTRL('r'):			/* recall current line */
			i = curptr;
			goto recall;
		case CTRL('y'):			/* recall original line */
			strncpy (WSTR, str, len);
			curptr = historyptr;
			goto reshow;
		case UP_ARROW:			/* recall prev line */
			for (i = curptr; (i = RECSUB(i-1)) != curptr;) {
				if (NULL != RECALL(i))
					goto recall;
			}
			break;
		case DOWN_ARROW:		/* recall next line */
			for (i = curptr; (i = RECSUB(i+1)) != curptr;) {
				if (NULL != RECALL(i))
					goto recall;
			}
			break;
		case PAGE_UP:			/* find by prefix */
			for (i = curptr; (i = RECSUB(i-1)) != curptr;) {
				if (!memcmp (WSTR, RECALL(i), cursor_pos))
					goto recall;
			}
			(*sys_put) ('\a');
			break;
		recall:
			strncpy (WSTR, RECALL(i), len);
			curptr = i;
		reshow:
			c = cursor_pos;
			clear_line();
			put_str (WSTR);
			cursor_pos = num_got = strlen (WSTR);
			while (cursor_pos > c)
				cursor_left ();
			break;
		case LEFT_ARROW:		/* cursor left */
			if (cursor_pos > 0) {
				(*sys_put)('\b');
				--cursor_pos;
			}
			break;
		case RIGHT_ARROW:		/* cursor right */
			cursor_right();
			break;
		case C_LEFT_ARROW:		/* find left space */
			while (cursor_pos > 0 && WSTR[cursor_pos-1] == ' ') {
				(*sys_put)('\b');
				--cursor_pos;
			}
			while (cursor_pos > 0 && WSTR[cursor_pos-1] != ' ') {
				(*sys_put)('\b');
				--cursor_pos;
			}
			break;
		case C_RIGHT_ARROW:		/* find right space */
			while (cursor_pos < num_got && WSTR[cursor_pos] != ' ')
				cursor_right();
			while (cursor_pos < num_got && WSTR[cursor_pos] == ' ')
				cursor_right();
			break;
		default:
			if (' ' <= c && c < 0x7f) {
			    if (edit_mode & INSERT_MODE) {
				if (num_got < len - 1) {
/* Move right, all the characters
 * to the right of cursor_pos.
*/
					for (i = num_got; i > cursor_pos; --i)
						WSTR[i] = WSTR[i - 1];
					WSTR[cursor_pos] = (char)c;
					for (i = cursor_pos; i <= num_got; ++i)
						(*sys_put)(WSTR[i]);
					for (i = cursor_pos; i < num_got; ++i)
						(*sys_put)('\b');
					++num_got;
					++cursor_pos;
				}
			    } else {
				if (cursor_pos < len - 1) {
					WSTR[cursor_pos] = (char)c;
					(*sys_put)(c);
					if (cursor_pos == num_got)
						++num_got;
					++cursor_pos;
				}
			    }
			}
			break;
		} /* switch */
	} /* for */

	WSTR[num_got] = '\0';
	(*sys_put)('\n');

	strcpy (str, WSTR);		/* copy to user space */

/* If this line is non-empty, and different
 * from the last one accepted, store it into
 * the recall buffer.
*/

	if (num_got < shortest_saved)
		i = 0;		/* too short */
	else if (RECALL(curptr) && !strcmp (WSTR, RECALL(curptr)))
		i = 0;		/* same as current */
	else if (RECALL(historyptr) && !strcmp (WSTR, RECALL(historyptr)))
		i = 0;		/* same as prev */
	else
		i = 1;
	if (i) {
		historyptr = RECSUB(historyptr + 1);

		if (RECALL(historyptr)) {
			RECALL(historyptr) = (*sys_free) (RECALL(historyptr),
							RECLEN(historyptr));
			RECLEN(historyptr) = 0;
		}

		len = strlen (WSTR) + 1;
		RECALL(historyptr) = (char *) (*sys_malloc) (len);
		if (NULL != RECALL(historyptr)) {
			RECLEN(historyptr) = len;
			strcpy (RECALL(historyptr), WSTR);
		}
	}

	WSTR = (*sys_free) (WSTR, WLEN);
	WLEN = 0;

	return (0);
}

#undef ESC_KEY
#undef DELETE_KEY
#undef BACKSPACE_KEY
#undef RETURN_KEY
#undef CTRL
#undef UP_ARROW
#undef DOWN_ARROW
#undef RIGHT_ARROW
#undef LEFT_ARROW
#undef HOME
#undef END
#undef INSERT
#undef C_LEFT_ARROW
#undef C_RIGHT_ARROW
#undef C_END
#undef C_HOME
#undef PAGE_UP
#undef RECSUB
#undef RECALL
#undef RECLEN
#undef WSTR
#undef WLEN
#undef INSERT_MODE
#undef INIT_MODE
