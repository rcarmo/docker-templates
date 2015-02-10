/***************************************************************************
 *   Copyright (C) 2014 by Tobias Volk                                     *
 *   mail@tobiasvolk.de                                                    *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


#ifndef F_PEERADDR_C
#define F_PEERADDR_C


#include "util.c"


// Internal address types.
#define peeraddr_INTERNAL_INDIRECT 1


// PeerAddr size in bytes.
#define peeraddr_SIZE 24


// Constraints.
#if peeraddr_SIZE != 24
#error invalid peeraddr_SIZE
#endif


// The PeerAddr structure.
struct s_peeraddr {
	unsigned char addr[peeraddr_SIZE];
};


// Returns true if PeerAddr is internal.
static int peeraddrIsInternal(const struct s_peeraddr *peeraddr) {
	int i;
	i = utilReadInt32(&peeraddr->addr[0]);
	if(i == 0) {
		return 1;
	}
	else {
		return 0;
	}
}


// Returns type of internal PeerAddr or -1 if it is not internal.
static int peeraddrGetInternalType(const struct s_peeraddr *peeraddr) {
	if(peeraddrIsInternal(peeraddr)) {
		return utilReadInt32(&peeraddr->addr[4]);
	}
	else {
		return -1;
	}
}


// Get indirect PeerAddr attributes. Returns 1 on success or 0 if the PeerAddr is not indirect.
static int peeraddrGetIndirect(const struct s_peeraddr *peeraddr, int *relayid, int *relayct, int *peerid) {
	if(peeraddrGetInternalType(peeraddr) == peeraddr_INTERNAL_INDIRECT) {
		if(relayid != NULL) {
			*relayid = utilReadInt32(&peeraddr->addr[8]);
		}
		if(relayct != NULL) {
			*relayct = utilReadInt32(&peeraddr->addr[12]);
		}
		if(peerid != NULL) {
			*peerid = utilReadInt32(&peeraddr->addr[16]);
		}
		return 1;
	}
	else {
		return 0;
	}
}


// Construct indirect PeerAddr.
static void peeraddrSetIndirect(struct s_peeraddr *peeraddr, const int relayid, const int relayct, const int peerid) {
	utilWriteInt32(&peeraddr->addr[0], 0);
	utilWriteInt32(&peeraddr->addr[4], peeraddr_INTERNAL_INDIRECT);
	utilWriteInt32(&peeraddr->addr[8], relayid);
	utilWriteInt32(&peeraddr->addr[12], relayct);
	utilWriteInt32(&peeraddr->addr[16], peerid);
	utilWriteInt32(&peeraddr->addr[20], 0);
}


#endif // F_PEERADDR_C
