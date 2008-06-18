/* --------------------------------- djgpp.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* general djgpp header.
*/

#ifndef FLY8_DJGPP_DJGPP_H
#define FLY8_DJGPP_DJGPP_H

#include <dpmi.h>
#include <go32.h>
#include <dos.h>

#if 1 == SYS_DJGPP
#define __dpmi_regs	_go32_dpmi_registers
#define __dpmi_int	_go32_dpmi_simulate_int
#define __dpmi_meminfo	_go32_dpmi_meminfo
#define __dpmi_simulate_real_mode_procedure_retf \
			_go32_dpmi_simulate_fcall
#endif

#define clear_dpmi_regs(r)	memset ((r), 0, sizeof(__dpmi_regs))


/* djgpp.c
*/
extern Ulong		tb_size;
extern Ushort		tb_seg;
extern Ulong		tb_real;

#endif
