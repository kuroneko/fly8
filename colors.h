/* --------------------------------- colors.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* colors definition.
*/

#ifndef FLY8_COLORS_H
#define FLY8_COLORS_H

#define C_RGB(r,g,b)	(((0x0ffL&(b))<<16)+((0x0ffL&(g))<<8)+(0x0ffL&(r)))
#define C_RGB_R(c)	(0x0ff & (Uint)((c)      ))
#define C_RGB_G(c)	(0x0ff & (Uint)((c) >>  8))
#define C_RGB_B(c)	(0x0ff & (Uint)((c) >> 16))
 
#define C_BLACK		C_RGB (0x00, 0x00, 0x00)

#define C_RED		C_RGB (0xff, 0x00, 0x00)
#define C_GREEN		C_RGB (0x00, 0xff, 0x00)
#define C_BLUE		C_RGB (0x00, 0x00, 0xff)

#define C_CYAN		C_RGB (0x00, 0xff, 0xff)
#define C_MAGENTA	C_RGB (0xff, 0x00, 0xff)
#define C_YELLOW	C_RGB (0xff, 0xff, 0x00)

#define C_WHITE		C_RGB (0xff, 0xff, 0xff)
#define C_LIGHTGRAY	C_RGB (0xa0, 0xa0, 0xa0)
#define C_GRAY		C_RGB (0x60, 0x60, 0x60)

#define C_LIGHTRED	C_RGB (0xc0, 0x60, 0x60)
#define C_LIGHTGREEN	C_RGB (0x80, 0xff, 0x80)
#define C_LIGHTBLUE	C_RGB (0x80, 0x80, 0xff)

#define C_LIGHTCYAN	C_RGB (0x80, 0xff, 0xff)
#define C_LIGHTMAGENTA	C_RGB (0xff, 0x80, 0xff)

#define C_DYELLOW	C_RGB (0x80, 0x80, 0x00)
#define C_SKYBLUE	C_RGB (0x00, 0x00, 0x80)

#define C_DGREEN	C_RGB (0x00, 0x20, 0x00)

#define C_BROWN		C_RGB (0x80, 0x40, 0x00)
#define C_DARKBROWN	C_RGB (0x50, 0x28, 0x00)


/* These are the portable color names.
*/

#define CC_DEFAULT	-1
#define CC_BLACK	0
#define CC_RED		1
#define CC_BLUE		2
#define CC_MAGENTA	3
#define CC_GREEN	4
#define CC_BROWN	5
#define CC_GRAY		6
#define CC_DYELLOW	7
#define CC_YELLOW	8
#define CC_LRED		9
#define CC_LBLUE	10
#define CC_LGRAY	11
#define CC_DBROWN	12
#define CC_SKYBLUE	13
#define CC_DGREEN	14
#define CC_WHITE	15

#endif
