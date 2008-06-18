/* --------------------------------- amigainc.h ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

#ifdef FOREVER
#undef FOREVER
#endif
#ifdef X
#undef X
#endif
#ifdef Y
#undef Y
#endif
#ifdef Z
#undef Z
#endif
#ifdef SHORT
#undef SHORT
#endif

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/ports.h>
#include <devices/gameport.h>
#include <devices/inputevent.h>
#include <devices/keyboard.h>
#include <libraries/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/gels.h>
#include <libraries/dos.h>
#ifdef _DCC
#include <clib/all_protos.h>
#endif
#ifdef __GNUC__
#include <inline/graphics.h>
#include <inline/intuition.h>
#include <inline/timer.h>
#endif

#ifdef FOREVER
#undef FOREVER
#endif
#ifdef X
#undef X
#endif
#ifdef Y
#undef Y
#endif
#ifdef Z
#undef Z
#endif
#ifdef SHORT
#undef SHORT
#endif

