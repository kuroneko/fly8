/*
linux/kernel/chr_drv/joystick.c
   Copyright (C) 1992, 1993 Arthur C. Smith
   Joystick driver for Linux running on an IBM/PC compatible computer.

VERSION INFO:
01/08/93	ACS	0.1: Works but needs multi-joystick support
01/13/93	ACS	0.2: Added multi-joysitck support (minor 0 and 1)
		  	     Added delay between measuring joystick axis
		   	     Added scaling ioctl
02/16/93	ACS	0.3: Modified scaling to use ints to prevent kernel
			     panics 8-)
02/28/93	ACS	0.4: Linux99.6 and fixed race condition in js_read.
			     After looking at a schmatic of a joystick card
                             it became apparent that any write to the joystick
			     port started ALL the joystick one shots. If the one
			     that we are reading is short enough and the first
		 	     one to be read, the second one will return bad data
			     if it's one shot has not expired when the joystick
			     port is written for the second time. Thus solves
			     the mystery delay problem in 0.2!
04/22/93	ACS/Eyal 0.5:Upgraded the driver to the 99.8 kernel, added
			     joystick support to the make config options,
			     updated the driver to return the buttons as
			     positive logic, and read both axis at once
			     (thanks Eyal)!
12 Feb 94	Eyal	Allow timelimit of zero.
			DO read first time!
			Return all 4 buttons for port 0
			DO return correct buttons for port 1
			Slight cleaning of analog reading loop.
			TIMEOUT is now in timer count units.
			Rename fields to lower case, remove js_ prefix.
			Use a private internal data structure.
			Allow long reads for both sticks at once.
13 Feb 94	Eyal	Fix header "linux/joystick.h".
			For compatibility, I still access 'corr' using the same
			  structure. I'd rather use the struct only for reads.
			js_init() now recognizes complex joysticks.
			Check timeout incrementally, to allow large timeouts
			  without locking the machine up if the stick is
			  disconnected.
		?	Should js_open() enforce exclusion? Just checking the
			  'busy' flag is not enough, I think.
			If js_read() times out, DO release the semaphore.
		?	Is there a need for a better 'corr' logic, like setting
			  a low/high/range value such that:
				reading = the_usual_timer_loop;
				if (reading > high)
					reading = high;
				else if (reading < low)
					reading = low;
				return (reading - low) * range / (high - low);
25 Apr 94	Eyal	Merge in the loadable stuff from 0.7 public release
			(maintained by Carlos Puchol?)
14 Oct 94	Eyal	minor cosmetics.
 4 Feb 95	Eyal	add <linux/mm.h> 1.1.88 needs it.
16 May 95	Eyal	drop local version.h, use <linux/version.h>, support
			new modules style.
23 May 95	Eyal	Register IO port, nicer messages.
26 May 95	Eyal	Remove cli/sti.
23 Jun 95	Eyal	Restructure driver:
			- set js_exist at open time.
			- redo the semaphore/locking.
			- use do_gettimeofday().
25 Apr 96	Eyal	Add /proc/joystick entry (as /proc/net/joystick),
			 (proper dynamic proc not ready it seems).
			Turn off JS_CLISTI (reduce interrupt latency).
			Do use do_gettimeofday() (was disabled).
			Some cleanup.
			move kernel stuff out of joystick.h
			use save/restore_flags()
 3 May 96	Eyal	Make it build on 1.2.13 again.
11 May 96	Eyal	Maintain persistent data.
13 May 96	Eyal	Accept 'io=' address.
14 May 96	Eyal	Use proc_register_dynamic() now that 1.99.4 allows it.
18 May 96	Eyal	No need to check MOD_IN_USE anymore.
16 Mar  7	Eyal	Cleanup. Return more for GET_CAL.
*/


/* If you are running an old (1.2.x) kernel then define this:
*/
/*#define JS_V12		1*/

/* Define this if you want js to read the h/w timer. The default is to use
 * gettimeofday().
*/
#define JS_READTIMER	0		/* directly read hw timer 0? */

/* Define this if you want the js read to happen with interrupts off - this
 * gives more accurate results but disturbs the kernel and may cause
 * problems.
*/
#define JS_CLISTI	0		/* need to cli/sti for reads? */

/* Define this if you need to serialise read requests. I am not sure if
 * this is really necessary.
*/
#define JS_BLOCK	1		/* need to serialize access? */

/* This is the default i/o port for the games port. Note that you can
 * override it with the 'io=' option.
*/
#define JS_PORT		0x201	/* io port for joystick operations */

/* -------------- end of compile options --------------- */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/ioport.h>	/* register_region() */

#if JS_V12
#undef CONFIG_PROC_FS
#include <linux/version.h>
#include <linux/mm.h>

#else	/* v1.3 and later */

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#endif
#include <linux/major.h>	/* JOYSTICK_MAJOR */
#ifdef KERNELD_GET_PERSIST
#include <linux/kerneld.h>	/* for PERSIST */
#endif
#endif	/* v1.3 and later */

#if 1	/* for 2.2 */
#define verify_area(x, y, z)
#endif

#include <asm/io.h>		/* inb(), outb() */

#if 0
#include <linux/joystick.h>
#else
#include "joystick.h"
#endif

#ifndef JOYSTICK_MAJOR
#define JOYSTICK_MAJOR	15	/* major device number for joysticks */
#endif

#define JS_X_0		0x01	/* bit mask for x-axis js0 */
#define JS_Y_0		0x02	/* bit mask for y-axis js0 */
#define JS_0		(JS_X_0 | JS_Y_0)
#define JS_X_1		0x04	/* bit mask for x-axis js1 */
#define JS_Y_1		0x08	/* bit mask for y-axis js1 */
#define JS_1		(JS_X_1 | JS_Y_1)
#define JS_ALL		(JS_0 | JS_1)

#define JS_MAX		2	/* Max number of joysticks */

#define PIT_MODE	0x43	/* io port for timer 0 */
#define PIT_COUNTER_0	0x40	/* io port for timer 0 */

/* This struct is used for misc data about the joystick.
*/
struct JS_DATA_SAVE_TYPE {
	int	io;
	int	timeout;	/* max timer counts for joystick port read */
	int	busy;		/* joystick is in use */
	long	expiretime;	/* Time after which stick must be re-read (jf) */
	long	timelimit;	/* Min time before re-reads (jf) */
	int	buttons;
	int	x;
	int	y;
	struct JS_DATA_TYPE corr;	/* correction factor */
};

#define LATCH		(1193180L/HZ)	/* timer 0 period */
#define DELTA_TIME(X,Y) ((X) - (Y) + ((X) >= (Y) ? 0 : LATCH))
#define CURRENT_JIFFIES	(jiffies /*+ jiffies_offset*/)

#define JS_VERSION	"0.7f (Eyal)"
#define JS_NAME		"joystick"

#if JS_V12
#undef JS_READTIMER
#define JS_READTIMER	1		/* directly read hw timer 0? */
#endif

#if !JS_READTIMER
#undef JS_CLISTI
#define JS_CLISTI	0		/* not with do_gettimeofday() */
#endif

#define GET_USERMEM(kernel,user) \
	get_usermem ((char *)&kernel, (char *)user, sizeof(kernel), 1)

#define PUT_USERMEM(kernel,user) \
	put_usermem ((char *)&kernel, (char *)user, sizeof(kernel), 1)

#if JS_V12
static char kernel_version[] = UTS_RELEASE;
#endif

#ifdef CONFIG_PROC_FS
static int	js_get_info (char *, char **, off_t, int, int);

static struct proc_dir_entry	js_proc_entry = {
	0, sizeof(JS_NAME)-1, JS_NAME,
	S_IFREG|S_IRUGO, 1, 0, 0, 0,
	NULL, js_get_info
};
#endif

static struct JS_DATA_SAVE_TYPE js_data[JS_MAX];	/* misc data */
static int	io = 0;
static int	js_exist = 0;		/* which joysticks' axis exist? */
static int	js_num = 0;		/* number of full sticks */
static int	js_part = 0;		/* number of extra partial sticks */
static int	js_read_semaphore = 0;	/* to prevent two processes */
					/* from trying to read different */
					/* joysticks at the same time */

#if JS_READTIMER
/* Return the current value of timer 0. This is a 16 bit counter that starts
 * at LATCH and counts down to 0 then repeats.
 *
 * NOTE: we do not register the timer i/o ports!
*/
inline int 
get_timer0 (void)
{
	int	t;
#if !JS_CLISTI
	unsigned long	flags;

	save_flags (flags);
	cli ();
#endif
	outb (0, PIT_MODE);
	t  = (int)inb (PIT_COUNTER_0);
	t += (int)inb (PIT_COUNTER_0) << 8;
#if !JS_CLISTI
	restore_flags (flags);
#endif

	return (t);
}
#endif

static void
js_lock ()
{
	int		t;
#if JS_BLOCK
	unsigned long	flags;
#endif

	for (t = 1; t;) {
#if JS_BLOCK
		save_flags (flags);
		cli ();
#endif
		if (!(t = js_read_semaphore))
			js_read_semaphore = 1;
#if JS_BLOCK
		restore_flags (flags);
#endif
	}
}

static void
js_unlock ()
{
	js_read_semaphore = 0;
}

static int
js_status ()
{
	return (JS_STATUS_X1        * !!(js_exist & JS_X_0) +
		JS_STATUS_Y1        * !!(js_exist & JS_Y_0) +
		JS_STATUS_X2        * !!(js_exist & JS_X_1) +
		JS_STATUS_Y2        * !!(js_exist & JS_Y_1) +
		JS_STATUS_READTIMER * JS_READTIMER +
		JS_STATUS_CLISTI    * JS_CLISTI +
		JS_STATUS_BLOCK     * JS_BLOCK);
}

static void
get_usermem (char *kernel, char *user, int size, int lock)
{
	verify_area (VERIFY_READ, (void *)user, size);
	if (lock)
		js_lock ();
	while (size-- > 0)
		*kernel++ = get_fs_byte (user++);
	if (lock)
		js_unlock ();
}

static void
put_usermem (char *kernel, char *user, int size, int lock)
{
	verify_area (VERIFY_WRITE, (void *)user, size);
	if (lock)
		js_lock ();
	while (size-- > 0)
		put_fs_byte (*kernel++, user++);
	if (lock)
		js_unlock ();
}

static int 
js_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	unsigned long arg)
{
	unsigned int	minor = MINOR(inode->i_rdev);

	if (MAJOR(inode->i_rdev) != JOYSTICK_MAJOR)
		return -EINVAL;
	if (minor >= JS_MAX)
		return -ENODEV;

	switch (cmd) {
	case JS_SET_CAL:
		GET_USERMEM (js_data[minor].corr, arg);
		if (js_data[minor].corr.x < 0)
			js_data[minor].corr.x = 0;
		else if (js_data[minor].corr.x > 32)
			js_data[minor].corr.x = 32;
		if (js_data[minor].corr.y < 0)
			js_data[minor].corr.y = 0;
		else if (js_data[minor].corr.y > 32)
			js_data[minor].corr.y = 32;
		break;
	case JS_GET_CAL:
		js_data[minor].corr.buttons = js_status ();
		PUT_USERMEM (js_data[minor].corr, arg);
		break;
	case JS_SET_TIMEOUT:
		GET_USERMEM (js_data[minor].timeout, arg);
		break;
	case JS_GET_TIMEOUT:
		PUT_USERMEM (js_data[minor].timeout, arg);
		break;
	case JS_SET_TIMELIMIT:
		GET_USERMEM (js_data[minor].timelimit, arg);
		if (js_data[minor].timelimit < 0)
			js_data[minor].timelimit = 0;
		break;
	case JS_GET_TIMELIMIT:
		PUT_USERMEM (js_data[minor].timelimit, arg);
		break;
	default:
		return -EINVAL;
	}
	return (0);
}

/* This is the actual joystick reading. It reads all channels always. It is
 * up to the caller to only use what they want.
*/
static int
js_update (int mask, long timeout)
{
	long		t;
	int		chk;
	long		t_x0, t_y0, t_x1, t_y1;
#if JS_READTIMER
	int		t0, t1;
#else
	struct timeval	t0, t1;
#endif
#if JS_CLISTI
	unsigned long	flags;
#endif

	t = t_x0 = t_y0 = t_x1 = t_y1 = 0;

#if JS_CLISTI
	save_flags (flags);
	cli ();
#endif
#if JS_READTIMER
	t0 = get_timer0 ();
#else
	do_gettimeofday (&t0);
#endif
	outb (0, io);			/* trigger one-shots */
	while (0 != ((chk = inb (io)) & mask) && t < timeout) {
#if JS_READTIMER
		t1 = get_timer0 ();
		t += DELTA_TIME (t0, t1);
		t0 = t1;
#else
		do_gettimeofday (&t1);
		t = (t1.tv_sec - t0.tv_sec) * 1000000L +
			(t1.tv_usec - t0.tv_usec);
#endif
		if (chk & JS_X_0)
			t_x0 = t;
		if (chk & JS_Y_0)
			t_y0 = t;
		if (chk & JS_X_1)
			t_x1 = t;
		if (chk & JS_Y_1)
			t_y1 = t;
	}
#if JS_CLISTI
	restore_flags (flags);
#endif

/* Update local data register.
*/
	js_data[0].expiretime = CURRENT_JIFFIES + js_data[0].timelimit;
	js_data[0].x = (int)(t_x0 >> js_data[0].corr.x);
	js_data[0].y = (int)(t_y0 >> js_data[0].corr.y);
	js_data[0].buttons = (~chk >> 4) & 0x0f;	/* all four buttons */

	js_data[1].expiretime = CURRENT_JIFFIES + js_data[1].timelimit;
	js_data[1].x = (int)(t_x1 >> js_data[1].corr.x);
	js_data[1].y = (int)(t_y1 >> js_data[1].corr.y);
	js_data[1].buttons = js_data[0].buttons >> 2;	/* only two buttons*/

	return (chk);
}

static void
js_reset (unsigned int minor)
{
	js_data[minor].timeout    = JS_DEF_TIMEOUT;
	js_data[minor].expiretime = CURRENT_JIFFIES;
	js_data[minor].timelimit  = JS_DEF_TIMELIMIT;
	js_data[minor].corr.x     = JS_DEF_CORR;
	js_data[minor].corr.y     = JS_DEF_CORR;
}

/* 'js_num' is the number of joysticks found, 'js_part' means we also found 
 * a partial joystick (like rudder, FlightStick hat, etc.).
*/
static void
get_exist (void)
{
	js_exist = JS_ALL & ~js_update (JS_ALL, JS_DEF_TIMEOUT);
	js_part = 0;
	switch (js_exist) {
	case JS_ALL:		/* Two complete */
		js_num = 2;
		break;

	case JS_0 | JS_X_1:	/* One complete + one partial */
	case JS_0 | JS_Y_1:
	case JS_1 | JS_X_0:
	case JS_1 | JS_Y_0:
		js_part = 1;
	case JS_0:		/* One complete */
	case JS_1:
		js_num = 1;
		break;

	default:		/* One partial */
		js_part = 1;
	case 0x00:		/* None at all */
		js_num = 0;
		break;
	}
}

/* The open will verify that this joystick exists even if it was missing at
 * init time. If the stick is disconnected later then timeouts will happen
 * on every read until the next open. This allows for intermittent failures.
*/
static int
js_open (struct inode *inode, struct file *file)
{
	unsigned int	minor = MINOR (inode->i_rdev);
	int		mask;

	if (minor >= JS_MAX)
		return -ENODEV;

	MOD_INC_USE_COUNT;

	js_lock ();

	if (js_data[minor].busy) {
		js_unlock ();
		MOD_DEC_USE_COUNT;
		return -EBUSY;
	}

	js_reset (minor);

/* We read all channels to set the global 'js_exist'.
*/
	get_exist ();

	mask = minor ? (JS_X_1 | JS_Y_1) : (JS_X_0 | JS_Y_0);
	if ((js_exist & mask) != mask) {
#if 0
		printk (KERN_ERR "%s: open failed js_exist=%x mask=%x\n",
			JS_NAME, js_exist, mask);
#endif
		js_unlock ();
		MOD_DEC_USE_COUNT;
		return -ENODEV; 
	}

	js_data[minor].busy = 1;

	js_unlock ();

	return (0);
}

static void
js_release (struct inode *inode, struct file *file)
{
	unsigned int	minor = MINOR(inode->i_rdev);

	js_lock ();
	inode->i_atime = CURRENT_TIME;
	js_data[minor].busy = 0;
	js_unlock ();

	MOD_DEC_USE_COUNT;
}

/* We support two kinds of read:
 *	- a single joystick read (js0 or js1) which reads two analogue channels
 *	and two buttons. Use JS_DATA_TYPE for this.
 *	- a combined read of all channels (on js0 only). Use JS_DATA_TYPE_LONG
 *	for this.
*/
static int
js_read (struct inode *inode, struct file *file, char *buf, int count)
{
	char		*c;
	unsigned int	minor = MINOR(inode->i_rdev);
	struct JS_DATA_TYPE		js_short;
	struct JS_DATA_TYPE_LONG	js_long;

	if (!(count == JS_RETURN || (count == JS_RETURN_LONG && !minor)))
		return -EOVERFLOW;

	js_lock ();

	inode->i_atime = CURRENT_TIME;

/* Only read if the old data is stale, but then read ALL channels.
*/
	if (CURRENT_JIFFIES >= js_data[minor].expiretime)
		js_update (js_exist, js_data[minor].timeout);

/* Deliver data to user memory.
*/
	if (JS_RETURN_LONG == count) {
		js_long.buttons = js_data[minor].buttons;
		js_long.x = js_data[0].x;
		js_long.y = js_data[0].y;
		js_long.x2 = js_data[1].x;
		js_long.y2 = js_data[1].y;
		c = (char *)&js_long;
	} else {
		js_short.buttons = js_data[minor].buttons;
		js_short.x = js_data[minor].x;
		js_short.y = js_data[minor].y;
		c = (char *)&js_short;
	}

	js_unlock ();

	put_usermem (c, buf, count, 0);

	return (count);
}


static struct file_operations js_fops = {
	NULL,			/* js_lseek	*/
	js_read,		/* js_read	*/
	NULL,			/* js_write	*/
	NULL,			/* js_readaddr	*/
	NULL,			/* js_select	*/
	js_ioctl,		/* js_ioctl	*/
	NULL,			/* js_mmap	*/
	js_open,		/* js_open	*/
	js_release,		/* js_release	*/
	NULL			/* js_sync	*/
};

#ifdef __cplusplus
extern "C" {
#endif

int
init_module (void)
{
	int	t;

	if (register_chrdev (JOYSTICK_MAJOR, JS_NAME, &js_fops)) {
		printk (KERN_ERR "%s: unable to get MAJOR=%d\n", JS_NAME,
			JOYSTICK_MAJOR);
		return (1);
	}

	for (t = 0; t < JS_MAX; t++) {
		js_reset ((unsigned int)t);
		js_data[t].io = 0;
#ifdef KERNELD_GET_PERSIST
{
		char	js_key[] = "joystick-" "0";
		struct JS_DATA_SAVE_TYPE js_per;

		js_key[sizeof(js_key)-2] = '0' + t;
		if (sizeof (js_per) == get_persist (js_key, &js_per, 
							sizeof (js_per))) {
			js_data[t] = js_per;
			js_data[t].expiretime = CURRENT_JIFFIES;
		} else
			printk (KERN_ERR "%s: get_persist(\"%s\") failed\n",
				JS_NAME, js_key);
}
#endif
		js_data[t].busy = 0;
	}

	if (0 == io) {
		if (0 != js_data[0].io)
			io = js_data[0].io;
		else
			io = JS_PORT;
	}
	js_data[0].io = io;

	printk (KERN_INFO "%s: %s MAJOR=%d BASE=0x%x%s%s%s\n",
		JS_NAME, JS_VERSION, JOYSTICK_MAJOR, io,
		JS_READTIMER ? " READTIMER" : "",
		JS_CLISTI    ? " CLISTI"    : "",
		JS_BLOCK     ? " BLOCK"     : "");

	request_region (io, 1, JS_NAME);

	js_read_semaphore = 0;

/* The rest of this function is done for information only and is not used
 * to block 'open()' of missing devices.
*/
	get_exist ();

	printk (KERN_INFO "%s: found %d%s device%s\n", JS_NAME,
		js_num, js_part ? "+" : "", (js_num == 1) ? "" : "s");

#ifdef CONFIG_PROC_FS
	proc_register_dynamic (&proc_root, &js_proc_entry);
#endif
	return (0);
}

void
cleanup_module (void)
{
#ifdef KERNELD_GET_PERSIST
{
	int	t;
	char	js_key[] = "joystick-" "0";

	for (t = 0; t < JS_MAX; t++) {
		js_key[sizeof(js_key)-2] = '0' + t;
		if (set_persist (js_key, &js_data[t], sizeof (js_data[t])) < 0)
			printk (KERN_WARNING "%s: set_persist(\"%s\") failed.\n",
				JS_NAME, js_key);
	}
}
#endif

#if JS_V12
	if (MOD_IN_USE) {
		printk (KERN_WARNING "%s: device busy, remove delayed.\n",
			JS_NAME);
		return;
	}
#endif

#ifdef CONFIG_PROC_FS
	proc_unregister (&proc_root, js_proc_entry.low_ino);
#endif

	release_region (io, 1);
	if (unregister_chrdev (JOYSTICK_MAJOR, JS_NAME) != 0)
		printk (KERN_ERR "%s: cleanup_module failed.\n",
			JS_NAME);
	else
		printk (KERN_INFO "%s: cleanup_module succeeded.\n",
			JS_NAME);
}

#ifdef CONFIG_PROC_FS
#define BSHOW \
	if ((len += sprintf (buf+len,
#define BEND \
			)) <= fpos) {	\
		fpos -= len;		\
		len = 0;		\
	} else if (len >= length)	\
		break

static int
js_get_info (char *buf, char **start, off_t fpos, int length, int dummy)
{
	int	len;
	int	i;
#define JS_TLEN	3
	char	title[JS_TLEN+1];

	len = 0;

	do {
		BSHOW "%-9s %s\n", "version",
			JS_VERSION BEND;
		BSHOW "%-9s %s %s %s\n", "compiled",
			__DATE__, __TIME__, UTS_RELEASE BEND;
		BSHOW "%-9s 0x%x\n", "io",
			io BEND;
		BSHOW "%-9s 0x%x\n", "status",
			js_status () BEND;
		BSHOW "%-9s %s%s\n", "title",
			"   timeout bz    expire  timelimit",
			" cx cy          x          y btns"
			BEND;

		strncpy (title, "js0", sizeof (title));
		for (i = 0; i < 2; ++i) {
			title[JS_TLEN-1] = '0'+i;
			BSHOW
"%-9s %.10u %1u %.10lu %.10lu %.2u %.2u %.10u %.10u %u%u%u%u\n", title,
				js_data[i].timeout,
				js_data[i].busy,
				js_data[i].expiretime,
				js_data[i].timelimit,
				js_data[i].corr.x,
				js_data[i].corr.y,
				js_data[i].x,
				js_data[i].y,
				!!(js_data[i].buttons & 0x08),
				!!(js_data[i].buttons & 0x04),
				!!(js_data[i].buttons & 0x02),
				!!(js_data[i].buttons & 0x01) BEND;
		}
	} while (0);

	*start = buf + fpos;
	len -= fpos;
	if (len > length)
		len = length;
	else if (len < 0)
		len = 0;	/* EOF */

	return (len);
}
#endif

#ifdef __cplusplus
}
#endif
