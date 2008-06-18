/* --------------------------------- pcudp.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Header for the low level UDP drivers.
*/

#ifndef FLY8_DJGPP_PCUDP_H
#define FLY8_DJGPP_PCUDP_H

#define	FLY8_IP		0x0800U		/* ether type:  IP */
#define	FLY8_ARP	0x0806U		/* ether type:  ARP */
#define	FLY8_ARPQ	0x0001U		/* ARP op:      ARP Query */
#define	FLY8_ARPR	0x0002U		/* ARP op:      ARP Reply */
#define	FLY8_UDP	0x011U		/* IP protocol: UPD */

#define	FLY8_SPORT	0xf8f8U		/* server UDP port */
#define	FLY8_PORT	0xf8f9U		/* my     UDP port */

#define MACADDRESS	6

#define ETHDEST		0		/* ethernet header */
#define ETHSRCE		(ETHDEST   + MACADDRESS)
#define ETHTYPE		(ETHSRCE   + MACADDRESS)
#define ETHNNN		(ETHTYPE   + 2)

#define IPADDRESS	4

#define IPHVER		0		/* IP header */
#define IPHTOS		(IPHVER    + 1)
#define IPHLEN		(IPHTOS    + 1)
#define IPHID		(IPHLEN    + 2)
#define IPHFLAGS	(IPHID     + 2)
#define IPHTTL		(IPHFLAGS  + 2)
#define IPHPROTO	(IPHTTL    + 1)
#define IPHCHECK	(IPHPROTO  + 1)
#define IPHSRCE		(IPHCHECK  + 2)
#define IPHDEST		(IPHSRCE   + IPADDRESS)
#define IPHNNN		(IPHDEST   + IPADDRESS)

#define IP_VER		0x45
#define IP_TOS_LOWDELAY	0x10
#define IP_TTL		64
#define IP_DF		0x4000      /* Don't fragment bit set for FRAG Flags */
#define IP_NF		0x2000      /* Next frag */
#define IP_OF		0x1fff      /* frag off. */

#define ARPHHTYPE	0		/* ARP header */
#define ARPHPTYPE	(ARPHHTYPE + 2)
#define ARPHHSIZE	(ARPHPTYPE + 2)
#define ARPHPSIZE	(ARPHHSIZE + 1)
#define ARPHOP		(ARPHPSIZE + 1)
#define ARPHESRCE	(ARPHOP    + 2)
#define ARPHISRCE	(ARPHESRCE + MACADDRESS)
#define ARPHEDEST	(ARPHISRCE + IPADDRESS)
#define ARPHIDEST	(ARPHEDEST + MACADDRESS)
#define ARPHNNN		(ARPHIDEST + IPADDRESS)

#define UDPADDRESS	2

#define UDPXSRCE	0		/* UDP pseudo header */
#define UDPXDEST	(UDPXSRCE  + IPADDRESS)
#define UDPXZERO	(UDPXDEST  + IPADDRESS)
#define UDPXPROTO	(UDPXZERO  + 1)
#define UDPXLEN		(UDPXPROTO + 1)
#define UDPXNNN		(UDPXLEN   + 2)

#define UDPHSRCE	0		/* UDP header */
#define UDPHDEST	(UDPHSRCE  + UDPADDRESS)
#define UDPHLEN		(UDPHDEST  + UDPADDRESS)
#define UDPHCHECK	(UDPHLEN   + 2)
#define UDPHNNN		(UDPHCHECK + 2)

#define APADDRESS	(IPADDRESS + UDPADDRESS)

#define APHDEST		0		/* Fly8 header */
#define APHSRCE		(APHDEST   + APADDRESS)
#define APHLEN		(APHSRCE   + APADDRESS)
#define APHNNN		(APHLEN    + 2)

#endif	/* FLY8_DJGPP_PCUDP_H */
