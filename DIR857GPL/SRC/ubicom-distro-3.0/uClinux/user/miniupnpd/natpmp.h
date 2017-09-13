/* $Id: natpmp.h,v 1.1 2009/04/21 11:14:48 jimmy_huang Exp $ */
/* MiniUPnP project
 * author : Thomas Bernard
 * website : http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 */
#ifndef __NATPMP_H__
#define __NATPMP_H__

#define NATPMP_PORT (5351)

int OpenAndConfNATPMPSocket(void);

void ProcessIncomingNATPMPPacket(int s);

int ScanNATPMPforExpiration(void);

int CleanExpiredNATPMP(void);

#endif

