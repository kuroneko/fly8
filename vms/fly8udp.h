/* --------------------------------- fly8udp.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common header for Fly8 udp programs.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef WATTCP
#include "tcp.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


/* These are from fly.h.
*/
#ifndef LADDRESS
#define LADDRESS	6
#endif

/* The following definitions are shared with the server programs fly8srv.c
 * and udpcli.c.
*/
#define SOCK_FLY8	SOCK_DGRAM
#define IPPORT_FLY8	0xf8f8

#ifdef PHDATA

#define PHLEN		(PHDATA-2)
#define PHFROM		(PHLEN-LADDRESS)
#define PHTO		(PHFROM-LADDRESS)
#define PHEAD		PHTO

#else

#define PHTO		PHEAD
#define PHFROM		(PHTO+LADDRESS)
#define PHLEN		(PHFROM+LADDRESS)
#define PHDATA		(PHLEN+2)

#endif

#define PHSIZE		(PHDATA-PHEAD)

#define ADMIN_ADDR	{0, 0, 0, 0, 0x12, 0x34}
