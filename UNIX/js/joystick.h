#ifndef LINUX_JOYSTICK_H
#define LINUX_JOYSTICK_H

/* timeout can either be in usecs or in hware timer0 counts (if JS_READTIMER
 * is set 
*/
#define JS_DEF_TIMEOUT	5000 	/* default timeout value (counts) */
#define JS_DEF_CORR	0	/* default correction factor */
#define JS_DEF_TIMELIMIT (HZ/10) /* default data valid time 100ms */

/* ioctl's
*/
#define JS_SET_CAL		0x01	/* joystick correction shift factor */
#define JS_GET_CAL		0x02
#define JS_SET_TIMEOUT		0x03	/* max wait for a timeout () */
#define JS_GET_TIMEOUT		0x04
#define JS_SET_TIMELIMIT	0x05	/* data retention time (jiffies) */
#define JS_GET_TIMELIMIT	0x06

/* This struct is used for the ioctl to set the scaling factor and to return
 * the current values for a joystick. 'buttons' returns the compiled-in options
 * and axes present for a GET_CAL.
*/
struct JS_DATA_TYPE {
	int buttons;
#define JS_STATUS_X1		0x0001	/* 1st stick */
#define JS_STATUS_Y1		0x0002
#define JS_STATUS_X2		0x0004	/* 2nd stick */
#define JS_STATUS_Y2		0x0008
#define JS_STATUS_READTIMER	0x0010	/* using h'ware timer */
#define JS_STATUS_CLISTI	0x0020	/* using cli/sti */
#define JS_STATUS_BLOCK		0x0040	/* blocking */
#define JS_BUTTON_1		0x0001
#define JS_BUTTON_2		0x0002
#define JS_BUTTON_3		0x0004
#define JS_BUTTON_4		0x0008
	int x;
	int y;
};
#define JS_RETURN	sizeof (struct JS_DATA_TYPE)

/* This struct is used for the long reads.
*/
struct JS_DATA_TYPE_LONG {
	int buttons;
	int x;
	int y;
	int x2;
	int y2;
};
#define JS_RETURN_LONG	sizeof (struct JS_DATA_TYPE_LONG)

#endif /* ifndef LINUX_JOYSTICK_H */
