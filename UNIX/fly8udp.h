/* --------------------------------- fly8udp.h ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common header for Fly8 udp programs.
*/

#ifndef FLY8_FLY8UDP_H
#define FLY8_FLY8UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>


#if SYS_WATTCP
#include "tcp.h"
#define SOCKET		int
#define INVALID_SOCKET	-1
#define FLY8_SOCKWOULDBLOCK	EWOULDBLOCK
#endif


#if SYS_WINSOCK
#if SYS_GCC
#define Win32_Winsock
#include <windows.h>
#else
#include <winsock.h>
#include <io.h>
#endif
#define WINSOCK_VERSION	0x0101
#define FLY8_SOCKWOULDBLOCK	WSAEWOULDBLOCK
#endif


#if SYS_UNIX || SYS_OS2
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define SOCKET		int
#define INVALID_SOCKET	-1
#define FLY8_SOCKWOULDBLOCK	EWOULDBLOCK
#endif


/* These are from fly.h.
*/
#ifndef LADDRESS
#define LADDRESS	6
#endif

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

struct svr_stats {
	char	hdr[14 /*PHSIZE*/];
	char	msgid[2];
#define	SVM_STATS	0x001
#define	SVM_MSG		0x002
	long	nclients;
	long	nin;
	long	ninb;
	long	nout;
	long	noutb;
	long	errs[10];
};

#endif
