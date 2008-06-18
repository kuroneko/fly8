/* --------------------------------- f8dis.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This file under construction! IT IS NOT IN USE YET!!!
 *
 * Structures used by the DIS comms stuff.
 * The section numbers refer to IST-CR-93-46:
 *	Enumeration and Bit-encoded values for use with IEEE-1278.1-1994.
*/

#ifndef FLY8_DIS_H
#define FLY8_DIS_H

/* 3.1 Protocol Version
*/

#define DISVER_DIS_PDU_1_0		1
#define DISVER_IEEE_1278_1993		2
#define DISVER_DIS_PDU_2_0		3
#define DISVER_IEEE_1278_1_1994		4


/* 3.2 PDU Type
*/

#define DISPDU_OTHER			0
#define DISPDU_ENTITY_STATE		1
#define DISPDU_FIRE			2
#define DISPDU_DETONATION		3
#define DISPDU_COLLISION		4
#define DISPDU_SERVICE_REQUSET		5
#define DISPDU_RESUPPLY_OFFER		6
#define DISPDU_RESUPPLY_RECEIVED	7
#define DISPDU_RESUPPLY_CANCEL		8
#define DISPDU_REPAIR_COMPLETE		9
#define DISPDU_REPAIR_RESPONSE		10

#define DISPDU_CREATE_ENTITY		11
#define DISPDU_REMOVE_ENTITY		12
#define DISPDU_START_RESUME		13
#define DISPDU_STOP_FREEZE		14
#define DISPDU_ACKNOWLEDGE		15
#define DISPDU_ACTION_REQUEST		16
#define DISPDU_ACTION_RESPONSE		17
#define DISPDU_DATA_QUERY		18
#define DISPDU_SET_DATA			19
#define DISPDU_DATA			20
#define DISPDU_EVENT_REPORT		21
#define DISPDU_MESSAGE			22
#define DISPDU_ELECTROMAGNETIC_EMISSION	23
#define DISPDU_DESIGNATOR		24
#define DISPDU_TRANSMITTER		25
#define DISPDU_SIGNAL			26
#define DISPDU_REVEIVER			27


/* 3.3 Protocol Family
*/

#define DISFAM_OTHER					0
#define DISFAM_ENTITY_INFORMATION_INTERACTION		1
#define DISFAM_WARFARE					2
#define DISFAM_LOGISTICS				3
#define DISFAM_RADIO_COMMUNICATIONS			4
#define DISFAM_SIMULATION_MANAGEMENT			5
#define DISFAM_DISTRIBUTED_EMISSION_REGENERATION	6


/* 4.1 Force ID
*/

#define DISFID_OTHER			0
#define DISFID_FRIENDLY			1
#define DISFID_OPPOSING			2
#define DISFID_NEUTRAL			3


/* 4.2.1 Entity Kind
*/

#define DISEKND_OTHER			0
#define DISEKND_PLATFORM		1
#define DISEKND_MUNITION		2
#define DISEKND_LIFE_FORM		3
#define DISEKND_ENVIRONMENTAL		4
#define DISEKND_CULTURAL_FEATURE	5
#define DISEKND_SUPPLY			6
#define DISEKND_RADIO			7


/* 4.2.1.1.1 Platform Domain
 * This is used as the encoding for many other Domain fields.
*/

#define DISDOM_OTHER			0
#define DISDOM_LAND			1
#define DISDOM_AIR			2
#define DISDOM_SURFACE			3
#define DISDOM_SUBSURFACE		4
#define DISDOM_SPACE			5


/* 4.2.2 Country
*/

#define DISCRY_OTHER			0
#define DISCRY_AUSTRALIA		13	/* The lucky country :-) */


/* 4.4 Dead-Reckoning Algorithm
*/

#define DISDRA_OTHER			0
#define DISDRA_STATIC			1
#define DISDRA_DRM_FPW			2
#define DISDRA_DRM_RPW			3
#define DISDRA_DRM_RVW			4
#define DISDRA_DRM_FVW			5
#define DISDRA_DRM_FPB			6
#define DISDRA_DRM_RPB			7
#define DISDRA_DRM_RVB			8
#define DISDRA_DRM_FVB			9


/* 4.5 Entity Marking
*/

#define DISMCS_UNUSED			0
#define DISMCS_ASCII			1


/* 4.6 Entity Capabilities Record
*/

#define DISECAP_AMMUNITION_SUPPLY	0
#define DISECAP_FUEL_SUPPLY		1
#define DISECAP_RECOVERY		2
#define DISECAP_REPAIR			3


/* 4.7.2 Articulated Parts
*/

#define DISART_POSITION			1
#define DISART_POSITION_RATE		2
#define DISART_EXTENSION		3
#define DISART_EXTENSION_RATE		4
#define DISART_X			5
#define DISART_X_RATE			6
#define DISART_Y			7
#define DISART_Y_RATE			8
#define DISART_Z			9
#define DISART_Z_RATE			10
#define DISART_AZIMUTH			11
#define DISART_AZIMUTH_RATE		12
#define DISART_ELEVATION		13
#define DISART_ELEVATION_RATE		14
#define DISART_ROTATION			15
#define DISART_ROTATION_RATE		16


/* 6.1 Service Type
*/

#define DISSVC_OTHER			0
#define DISSVC_RESUPPLY			1
#define DISSVC_REPAIR			2


/* 6.2.1 General Repair Code
*/

#define DISRPC_NO_REPAIRS		0
#define DISRPC_ALL_REPAIRS		1


/* 6.3 Repair Response PDU
*/

#define DISRPR_OTHER			0
#define DISRPR_ENDED			1
#define DISRPR_INVALID			2
#define DISRPR_INTERRUPTED		3
#define DISRPR_CANCELLED_BY_SUPPLIER	4


#endif
