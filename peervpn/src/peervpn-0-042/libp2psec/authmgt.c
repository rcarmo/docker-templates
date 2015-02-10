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


#ifndef F_AUTHMGT_C
#define F_AUTHMGT_C


#include "auth.c"
#include "peeraddr.c"
#include "idsp.c"


// Timeouts.
#define authmgt_RECV_TIMEOUT 30
#define authmgt_RESEND_TIMEOUT 3


// The auth manager structure.
struct s_authmgt {
	struct s_idsp idsp;
	struct s_auth_state *authstate;
	struct s_peeraddr *peeraddr;
	int *lastrecv;
	int *lastsend;
	int fastauth;
	int current_authed_id;
	int current_completed_id;
};


// Return number of auth slots.
static int authmgtSlotCount(struct s_authmgt *mgt) {
	return idspSize(&mgt->idsp);
}


// Return number of used auth slots.
static int authmgtUsedSlotCount(struct s_authmgt *mgt) {
	return idspUsedCount(&mgt->idsp);	
}


// Create new auth session. Returns ID of session if successful.
static int authmgtNew(struct s_authmgt *mgt, const struct s_peeraddr *peeraddr) {
	int authstateid = idspNew(&mgt->idsp);
	int tnow = utilGetClock();
	if(!(authstateid < 0)) {
		if(mgt->fastauth) {
			mgt->lastsend[authstateid] = (tnow - authmgt_RESEND_TIMEOUT - 3);
		}
		else {
			mgt->lastsend[authstateid] = tnow;
		}
		mgt->lastrecv[authstateid] = tnow;
		mgt->peeraddr[authstateid] = *peeraddr;
		return authstateid;
	}
	else {
		return -1;
	}
}


// Delete auth session.
static void authmgtDelete(struct s_authmgt *mgt, const int authstateid) {
	if(mgt->current_authed_id == authstateid) mgt->current_authed_id = -1;
	if(mgt->current_completed_id == authstateid) mgt->current_completed_id = -1;
	authReset(&mgt->authstate[authstateid]);
	idspDelete(&mgt->idsp, authstateid);
}


// Start new auth session. Returns 1 on success.
static int authmgtStart(struct s_authmgt *mgt, const struct s_peeraddr *peeraddr) {
	int authstateid = authmgtNew(mgt, peeraddr);
	if(!(authstateid < 0)) {
		authStart(&mgt->authstate[authstateid]);
		return 1;
	}
	else {
		return 0;
	}
}


// Check if auth manager has an authed peer.
static int authmgtHasAuthedPeer(struct s_authmgt *mgt) {
	if(!(mgt->current_authed_id < 0)) {
		return 1;
	}
	else {
		return 0;
	}
}


// Get the NodeID of the current authed peer.
static int authmgtGetAuthedPeerNodeID(struct s_authmgt *mgt, struct s_nodeid *nodeid) {
	if(authmgtHasAuthedPeer(mgt)) {
		return authGetRemoteNodeID(&mgt->authstate[mgt->current_authed_id], nodeid);
	}
	else {
		return 0;
	}
}


// Accept the current authed peer.
static void authmgtAcceptAuthedPeer(struct s_authmgt *mgt, const int local_peerid, const int64_t seq, const int64_t flags) {
	if(authmgtHasAuthedPeer(mgt)) {
		authSetLocalData(&mgt->authstate[mgt->current_authed_id], local_peerid, seq, flags);
		mgt->current_authed_id = -1;
	}
}


// Reject the current authed peer.
static void authmgtRejectAuthedPeer(struct s_authmgt *mgt) {
	if(authmgtHasAuthedPeer(mgt)) authmgtDelete(mgt, mgt->current_authed_id);
}


// Check if auth manager has a completed peer.
static int authmgtHasCompletedPeer(struct s_authmgt *mgt) {
	if(!(mgt->current_completed_id < 0)) {
		return 1;
	}
	else {
		return 0;
	}
}


// Get the local PeerID of the current completed peer.
static int authmgtGetCompletedPeerLocalID(struct s_authmgt *mgt) {
	if(authmgtHasCompletedPeer(mgt)) {
		return mgt->authstate[mgt->current_completed_id].local_peerid;
	}
	else {
		return 0;
	}
}


// Get the NodeID of the current completed peer.
static int authmgtGetCompletedPeerNodeID(struct s_authmgt *mgt, struct s_nodeid *nodeid) {
	if(authmgtHasCompletedPeer(mgt)) {
		return authGetRemoteNodeID(&mgt->authstate[mgt->current_completed_id], nodeid);
	}
	else {
		return 0;
	}
}


// Get the remote PeerID and PeerAddr of the current completed peer.
static int authmgtGetCompletedPeerAddress(struct s_authmgt *mgt, int *remote_peerid, struct s_peeraddr *remote_peeraddr) {
	if(authmgtHasCompletedPeer(mgt)) {
		if(!authGetRemotePeerID(&mgt->authstate[mgt->current_completed_id], remote_peerid)) return 0;
		*remote_peeraddr = mgt->peeraddr[mgt->current_completed_id];
		return 1;
	}
	else {
		return 0;
	}
}


// Get the shared session keys of the current completed peer.
static int authmgtGetCompletedPeerSessionKeys(struct s_authmgt *mgt, struct s_crypto *ctx) {
	if(authmgtHasCompletedPeer(mgt)) {
		return authGetSessionKeys(&mgt->authstate[mgt->current_completed_id], ctx);
	}
	else {
		return 0;
	}
}


// Get the connection parameters of the current completed peer.
static int authmgtGetCompletedPeerConnectionParams(struct s_authmgt *mgt, int64_t *remoteseq, int64_t *remoteflags) {
	if(authmgtHasCompletedPeer(mgt)) {
		return authGetConnectionParams(&mgt->authstate[mgt->current_completed_id], remoteseq, remoteflags);
	}
	else {
		return 0;
	}
}

// Finish the current completed peer.
static void authmgtFinishCompletedPeer(struct s_authmgt *mgt) {
	mgt->current_completed_id = -1;
}


// Get next auth manager message.
static int authmgtGetNextMsg(struct s_authmgt *mgt, struct s_msg *out_msg, struct s_peeraddr *target) {
	int used = idspUsedCount(&mgt->idsp);
	int tnow = utilGetClock();
	int authstateid;
	int i;
	for(i=0; i<used; i++) {
		authstateid = idspNext(&mgt->idsp);
		if((tnow - mgt->lastrecv[authstateid]) < authmgt_RECV_TIMEOUT) { // check if auth session has expired
			if((tnow - mgt->lastsend[authstateid]) > authmgt_RESEND_TIMEOUT) { // only send one auth message per specified time interval and session
				if(authGetNextMsg(&mgt->authstate[authstateid], out_msg)) {
					mgt->lastsend[authstateid] = tnow;
					*target = mgt->peeraddr[authstateid];
					return 1;
				}
			}
		}
		else {
			authmgtDelete(mgt, authstateid);
		}
	}
	return 0;
}


// Find auth session with specified PeerAddr.
static int authmgtFindAddr(struct s_authmgt *mgt, const struct s_peeraddr *addr) {
	int count = idspUsedCount(&mgt->idsp);
	int i;
	int j;
	for(i=0; i<count; i++) {
		j = idspNext(&mgt->idsp);
		if(memcmp(mgt->peeraddr[j].addr, addr->addr, peeraddr_SIZE) == 0) return j;
	}
	return -1;
}


// Find unused auth session.
static int authmgtFindUnused(struct s_authmgt *mgt) {
	int count = idspUsedCount(&mgt->idsp);
	int i;
	int j;
	struct s_auth_state *authstate;
	for(i=0; i<count; i++) {
		j = idspNext(&mgt->idsp);
		authstate = &mgt->authstate[j];
		if((!authIsPreauth(authstate)) || (authIsPeerCompleted(authstate))) return j;
	}
	return -1;
}


// Decode auth message. Returns 1 if message is accepted.
static int authmgtDecodeMsg(struct s_authmgt *mgt, const unsigned char *msg, const int msg_len, const struct s_peeraddr *peeraddr) {
	int authid;
	int authstateid;
	int tnow = utilGetClock();
	int newsession;
	int dupid;
	if(msg_len > 4) {
		authid = utilReadInt32(msg);
		if(authid > 0) {
			// message belongs to existing auth session
			authstateid = (authid - 1);
			if(authstateid < idspSize(&mgt->idsp)) {
				if(authDecodeMsg(&mgt->authstate[authstateid], msg, msg_len)) {
					mgt->lastrecv[authstateid] = tnow;
					mgt->peeraddr[authstateid] = *peeraddr;
					if(mgt->fastauth) {
						mgt->lastsend[authstateid] = (tnow - authmgt_RESEND_TIMEOUT - 3);
					}
					if((authIsAuthed(&mgt->authstate[authstateid])) && (!authIsCompleted(&mgt->authstate[authstateid]))) mgt->current_authed_id = authstateid;
					if((authIsCompleted(&mgt->authstate[authstateid])) && (!authIsPeerCompleted(&mgt->authstate[authstateid]))) mgt->current_completed_id = authstateid;
					return 1;
				}
			}
		}
		else if(authid == 0) {
			// message requests new auth session
			dupid = authmgtFindAddr(mgt, peeraddr);
			if(dupid < 0) {
				newsession = 1;
			}
			else {
				// auth session with same PeerAddr found.
				if(authIsPreauth(&mgt->authstate[dupid])) {
					newsession = 0;
				}
				else {
					authmgtDelete(mgt, dupid);
					newsession = 1; // replace old auth session
				}
			}
			if(newsession) {
				authstateid = authmgtNew(mgt, peeraddr);
				if(authstateid < 0) {
					// all auth slots are full, search for unused sessions that can be replaced
					dupid = authmgtFindUnused(mgt);
					if(!(dupid < 0)) {
						authmgtDelete(mgt, dupid);
						authstateid = authmgtNew(mgt, peeraddr);
					}
				}
				if(!(authstateid < 0)) {
					if(authDecodeMsg(&mgt->authstate[authstateid], msg, msg_len)) {
						mgt->lastrecv[authstateid] = tnow;
						mgt->peeraddr[authstateid] = *peeraddr;
						if(mgt->fastauth) {
							mgt->lastsend[authstateid] = (tnow - authmgt_RESEND_TIMEOUT - 3);
						}
						return 1;
					}
					else {
						authmgtDelete(mgt, authstateid);
					}
				}
			}
		}
	}
	return 0;
}


// Enable/disable fastauth (ignore send timeout after auth status change)
static void authmgtSetFastauth(struct s_authmgt *mgt, const int enable) {
	if(enable) {
		mgt->fastauth = 1;
	}
	else {
		mgt->fastauth = 0;
	}
}


// Reset auth manager object.
static void authmgtReset(struct s_authmgt *mgt) {
	int i;
	int count = idspSize(&mgt->idsp);
	for(i=0; i<count; i++) {
		authReset(&mgt->authstate[i]);
	}
	idspReset(&mgt->idsp);
	mgt->fastauth = 0;
	mgt->current_authed_id = -1;
	mgt->current_completed_id = -1;
}


// Create auth manager object.
static int authmgtCreate(struct s_authmgt *mgt, struct s_netid *netid, const int auth_slots, struct s_nodekey *local_nodekey, struct s_dh_state *dhstate) {
	int ac;
	struct s_auth_state *authstate_mem;
	struct s_peeraddr *peeraddr_mem;
	int *lastsend_mem;
	int *lastrecv_mem;
	if(auth_slots > 0) {
		lastsend_mem = malloc(sizeof(int) * auth_slots);
		if(lastsend_mem != NULL) {
			lastrecv_mem = malloc(sizeof(int) * auth_slots);
			if(lastrecv_mem != NULL) {
				authstate_mem = malloc(sizeof(struct s_auth_state) * auth_slots);
				if(authstate_mem != NULL) {
					peeraddr_mem = malloc(sizeof(struct s_peeraddr) * auth_slots);
					if(peeraddr_mem != NULL) {
						ac = 0;
						while(ac < auth_slots) {
							if(!authCreate(&authstate_mem[ac], netid, local_nodekey, dhstate, (ac + 1))) break;
							ac++;
						}
						if(!(ac < auth_slots)) {
							if(idspCreate(&mgt->idsp, auth_slots)) {
								mgt->lastsend = lastsend_mem;
								mgt->lastrecv = lastrecv_mem;
								mgt->authstate = authstate_mem;
								mgt->peeraddr = peeraddr_mem;
								authmgtReset(mgt);
								return 1;
							}
						}
						while(ac > 0) {
							ac--;
							authDestroy(&authstate_mem[ac]);
						}
						free(peeraddr_mem);
					}
					free(authstate_mem);
				}
				free(lastrecv_mem);
			}
			free(lastsend_mem);
		}
	}
	return 0;
}


// Destroy auth manager object.
static void authmgtDestroy(struct s_authmgt *mgt) {
	int i;
	int count = idspSize(&mgt->idsp);
	idspDestroy(&mgt->idsp);
	for(i=0; i<count; i++) authDestroy(&mgt->authstate[i]);
	free(mgt->peeraddr);
	free(mgt->authstate);
	free(mgt->lastrecv);
	free(mgt->lastsend);
}


#endif // F_AUTHMGT_C
