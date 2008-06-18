/* ---------------------------- ints.x -------------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* interrupt on/off support.
*/
	.file	"ints.x"
	.text
	.align	4
	.globl	_IntOff		/* Ulong IntOff (void) */
_IntOff:
	pushf
	pop	%eax
	cli
	ret

	.globl	_IntOn		/* void IntOn (Ulong flags) */
_IntOn:
	pushl	4(%esp)
	popf
	ret
