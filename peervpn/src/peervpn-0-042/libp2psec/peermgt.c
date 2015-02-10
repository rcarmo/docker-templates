/***************************************************************************
 *   Copyright (C) 2015 by Tobias Volk                                     *
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


#ifndef F_PEERMGT_C
#define F_PEERMGT_C


#include "nodedb.c"
#include "authmgt.c"
#include "packet.c"
#include "dfrag.c"


// Minimum message size supported (without fragmentation).
#define peermgt_MSGSIZE_MIN 1024


// Maximum message size supported (with or without fragmentation).
#define peermgt_MSGSIZE_MAX 8192


// Ping buffer size.
#define peermgt_PINGBUF_SIZE 64


// Number of fragment buffers.
#define peermgt_FRAGBUF_COUNT 64


// Maximum packet decode recursion depth.
#define peermgt_DECODE_RECURSION_MAX_DEPTH 2


// NodeDB settings.
#define peermgt_NODEDB_NUM_PEERADDRS 8
#define peermgt_RELAYDB_NUM_PEERADDRS 4


// States.
#define peermgt_STATE_INVALID 0
#define peermgt_STATE_AUTHED 1
#define peermgt_STATE_COMPLETE 2


// Timeouts.
#define peermgt_RECV_TIMEOUT 100
#define peermgt_KEEPALIVE_INTERVAL 10
#define peermgt_PEERINFO_INTERVAL 60
#define peermgt_NEWCONNECT_MAX_LASTSEEN 604800
#define peermgt_NEWCONNECT_MIN_LASTCONNTRY 60
#define peermgt_NEWCONNECT_RELAY_MAX_LASTSEEN 300


// Flags.
#define peermgt_FLAG_USERDATA 0x0001
#define peermgt_FLAG_RELAY 0x0002
#define peermgt_FLAG_F03 0x0004
#define peermgt_FLAG_F04 0x0008
#define peermgt_FLAG_F05 0x0010
#define peermgt_FLAG_F06 0x0020
#define peermgt_FLAG_F07 0x0040
#define peermgt_FLAG_F08 0x0080
#define peermgt_FLAG_F09 0x0100
#define peermgt_FLAG_F10 0x0200
#define peermgt_FLAG_F11 0x0400
#define peermgt_FLAG_F12 0x0800
#define peermgt_FLAG_F13 0x1000
#define peermgt_FLAG_F14 0x2000
#define peermgt_FLAG_F15 0x4000
#define peermgt_FLAG_F16 0x8000


// Constraints.
#if auth_MAXMSGSIZE > peermgt_MSGSIZE_MIN
#error auth_MAXMSGSIZE too big
#endif
#if peermgt_PINGBUF_SIZE > peermgt_MSGSIZE_MIN
#error peermgt_PINGBUF_SIZE too big
#endif


// The peer manager data structure.
struct s_peermgt_data {
	int conntime;
	int lastrecv;
	int lastsend;
	int lastpeerinfo;
	int lastpeerinfosendpeerid;
	struct s_peeraddr remoteaddr;
	int remoteflags;
	int remoteid;
	int64_t remoteseq;
	struct s_seq_state seq;
	int state;
};


// The peer manager structure.
struct s_peermgt {
	struct s_netid netid;
	struct s_map map;
	struct s_nodedb nodedb;
	struct s_nodedb relaydb;
	struct s_authmgt authmgt;
	struct s_dfrag dfrag;
	struct s_nodekey *nodekey;
	struct s_peermgt_data *data;
	struct s_crypto *ctx;
	int localflags;
	unsigned char msgbuf[peermgt_MSGSIZE_MAX];
	unsigned char relaymsgbuf[peermgt_MSGSIZE_MAX];
	unsigned char rrmsgbuf[peermgt_MSGSIZE_MAX];
	int msgsize;
	int msgpeerid;
	struct s_msg outmsg;
	int outmsgpeerid;
	int outmsgbroadcast;
	int outmsgbroadcastcount;
	struct s_msg rrmsg;
	int rrmsgpeerid;
	int rrmsgtype;
	int rrmsgusetargetaddr;
	struct s_peeraddr rrmsgtargetaddr;
	int loopback;
	int fragmentation;
	int fragoutpeerid;
	int fragoutcount;
	int fragoutsize;
	int fragoutpos;
	int lastconntry;
	int tinit;
};


// Return number of connected peers.
static int peermgtPeerCount(struct s_peermgt *mgt) {
	int n;
	n = (mapGetKeyCount(&mgt->map) - 1);
	return n;
}


// Check if PeerID is valid.
static int peermgtIsValidID(struct s_peermgt *mgt, const int peerid) {
	if(!(peerid < 0)) {
		if(peerid < mapGetMapSize(&mgt->map)) {
			if(mapIsValidID(&mgt->map, peerid)) {
				return 1;
			}
		}
	}
	return 0;
}


// Check if PeerID is active (ready to send/recv data).
static int peermgtIsActiveID(struct s_peermgt *mgt, const int peerid) {
	if(peermgtIsValidID(mgt, peerid)) {
		if(mgt->data[peerid].state == peermgt_STATE_COMPLETE) {
			return 1;
		}
	}
	return 0;
}


// Check if PeerID is active and matches the specified connection time.
static int peermgtIsActiveIDCT(struct s_peermgt *mgt, const int peerid, const int conntime) {
	if(peermgtIsActiveID(mgt, peerid)) {
		if(mgt->data[peerid].conntime == conntime) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}


// Check if PeerID is active and remote (> 0).
static int peermgtIsActiveRemoteID(struct s_peermgt *mgt, const int peerid) {
	return ((peerid > 0) && (peermgtIsActiveID(mgt, peerid)));
}


// Check if PeerID is active, remote (> 0) and matches the specified connection time.
static int peermgtIsActiveRemoteIDCT(struct s_peermgt *mgt, const int peerid, const int conntime) {
	return ((peerid > 0) && (peermgtIsActiveIDCT(mgt, peerid, conntime)));
}


// Check if indirect PeerAddr is valid.
static int peermgtIsValidIndirectPeerAddr(struct s_peermgt *mgt, const struct s_peeraddr *addr) {
	int relayid;
	int relayct;
	if(peeraddrGetIndirect(addr, &relayid, &relayct, NULL)) {
		if(peermgtIsActiveRemoteIDCT(mgt, relayid, relayct)) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}


// Return the next valid PeerID.
static int peermgtGetNextID(struct s_peermgt *mgt) {
	return mapGetNextKeyID(&mgt->map);
}


// Return the next valid PeerID, starting from specified ID.
static int peermgtGetNextIDN(struct s_peermgt *mgt, const int start) {
	return mapGetNextKeyIDN(&mgt->map, start);
}


// Get PeerID of NodeID. Returns -1 if it is not found.
static int peermgtGetID(struct s_peermgt *mgt, const struct s_nodeid *nodeid) {
	return mapGetKeyID(&mgt->map, nodeid->id);
}


// Returns PeerID if active PeerID + PeerCT or NodeID is specified. Returns -1 if it is not found or both IDs are specified and don't match the same node.
static int peermgtGetActiveID(struct s_peermgt *mgt, const struct s_nodeid *nodeid, const int peerid, const int peerct) {
	int outpeerid = -1;

	if(nodeid != NULL) {
		outpeerid = peermgtGetID(mgt, nodeid);
		if(outpeerid < 0) return -1;
	}
	if(!(peerid < 0)) {
		if(outpeerid < 0) {
			outpeerid = peerid;
		}
		else {
			if(peerid != outpeerid) return -1;
		}
	}
	if(!(outpeerid < 0)) {
		if(peermgtIsActiveIDCT(mgt, outpeerid, peerct)) {
			return outpeerid;
		}
	}

	return -1;
}


// Get NodeID of PeerID. Returns 1 on success.
static int peermgtGetNodeID(struct s_peermgt *mgt, struct s_nodeid *nodeid, const int peerid) {
	unsigned char *ret;
	if(peermgtIsValidID(mgt, peerid)) {
		ret = mapGetKeyByID(&mgt->map, peerid);
		memcpy(nodeid->id, ret, nodeid_SIZE);
		return 1;
	}
	else {
		return 0;
	}
}


// Reset the data for an ID.
static void peermgtResetID(struct s_peermgt *mgt, const int peerid) {
	mgt->data[peerid].state = peermgt_STATE_INVALID;
	memset(mgt->data[peerid].remoteaddr.addr, 0, peeraddr_SIZE);
	cryptoSetKeysRandom(&mgt->ctx[peerid], 1);
}


// Register new peer.
static int peermgtNew(struct s_peermgt *mgt, const struct s_nodeid *nodeid, const struct s_peeraddr *addr) {
	int tnow = utilGetClock();
	int peerid = mapAddReturnID(&mgt->map, nodeid->id, &tnow);
	if(!(peerid < 0)) {
		mgt->data[peerid].state = peermgt_STATE_AUTHED;
		mgt->data[peerid].remoteaddr = *addr;
		mgt->data[peerid].conntime = tnow;
		mgt->data[peerid].lastrecv = tnow;
		mgt->data[peerid].lastsend = tnow;
		mgt->data[peerid].lastpeerinfo = tnow;
		mgt->data[peerid].lastpeerinfosendpeerid = peermgtGetNextID(mgt);
		seqInit(&mgt->data[peerid].seq, cryptoRand64());
		mgt->data[peerid].remoteflags = 0;
		return peerid;
	}
	return -1;
}


// Unregister a peer using its NodeID.
static void peermgtDelete(struct s_peermgt *mgt, const struct s_nodeid *nodeid) {
	int peerid = peermgtGetID(mgt, nodeid);
	if(peerid > 0) { // don't allow special ID 0 to be deleted.
		mapRemove(&mgt->map, nodeid->id);
		peermgtResetID(mgt, peerid);
	}
}


// Unregister a peer using its ID.
static void peermgtDeleteID(struct s_peermgt *mgt, const int peerid) {
	struct s_nodeid nodeid;
	if(peerid > 0 && peermgtGetNodeID(mgt, &nodeid, peerid)) {
		peermgtDelete(mgt, &nodeid);
	}
}


// Connect to a new peer.
static int peermgtConnect(struct s_peermgt *mgt, const struct s_peeraddr *remote_addr) {
	int addr_ok;
	addr_ok = 0;
	if(remote_addr != NULL) {
		if(peeraddrIsInternal(remote_addr)) {
			if(peermgtIsValidIndirectPeerAddr(mgt, remote_addr)) {
				addr_ok = 1;
			}
		}
		else {
			addr_ok = 1;
		}
	}
	if(addr_ok) {
		if(authmgtStart(&mgt->authmgt, remote_addr)) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}


// Enable/Disable loopback messages.
static void peermgtSetLoopback(struct s_peermgt *mgt, const int enable) {
	if(enable) {
		mgt->loopback = 1;
	}
	else {
		mgt->loopback = 0;
	}
}


// Enable/disable fastauth (ignore send delay after auth status change).
static void peermgtSetFastauth(struct s_peermgt *mgt, const int enable) {
	authmgtSetFastauth(&mgt->authmgt, enable);
}


// Enable/disable packet fragmentation.
static void peermgtSetFragmentation(struct s_peermgt *mgt, const int enable) {
	if(enable) {
		mgt->fragmentation = 1;
	}
	else {
		mgt->fragmentation = 0;
	}
}


// Set flags.
static void peermgtSetFlags(struct s_peermgt *mgt, const int flags) {
	mgt->localflags = flags;
}


// Get single flag.
static int peermgtGetFlag(struct s_peermgt *mgt, const int flag) {
	int f;
	f = (mgt->localflags & flag);
	return (f != 0);
}


// Get single remote flag.
static int peermgtGetRemoteFlag(struct s_peermgt *mgt, const int peerid, const int flag) {
	int f;
	f = (mgt->data[peerid].remoteflags & flag);
	return (f != 0);
}


// Generate peerinfo packet.
static void peermgtGenPacketPeerinfo(struct s_packet_data *data, struct s_peermgt *mgt, const int peerid) {
	const int peerinfo_size = (packet_PEERID_SIZE + nodeid_SIZE + peeraddr_SIZE);
	int peerinfo_max = mapGetKeyCount(&mgt->map);
	int peerinfo_count;
	int peerinfo_limit;
	int pos = 4;
	int i = 0;
	int infoid;
	unsigned char infocid[packet_PEERID_SIZE];
	struct s_nodeid infonid;

	// randomize maximum length of peerinfo packet
	if((abs(cryptoRandInt()) % 2) == 1) { peerinfo_limit = 7; } else { peerinfo_limit = 5; }

	// generate peerinfo entries
	peerinfo_count = 0;
	while((i < peerinfo_max) && (peerinfo_count < peerinfo_limit) && (pos + peerinfo_size < data->pl_buf_size)) {
		infoid = peermgtGetNextIDN(mgt, mgt->data[peerid].lastpeerinfosendpeerid);
		mgt->data[peerid].lastpeerinfosendpeerid = infoid;
		if((infoid > 0) && (mgt->data[infoid].state == peermgt_STATE_COMPLETE) && (!peeraddrIsInternal(&mgt->data[infoid].remoteaddr))) {
			utilWriteInt32(infocid, infoid);
			memcpy(&data->pl_buf[pos], infocid, packet_PEERID_SIZE);
			peermgtGetNodeID(mgt, &infonid, infoid);
			memcpy(&data->pl_buf[(pos + packet_PEERID_SIZE)], infonid.id, nodeid_SIZE);
			memcpy(&data->pl_buf[(pos + packet_PEERID_SIZE + nodeid_SIZE)], &mgt->data[infoid].remoteaddr.addr, peeraddr_SIZE);
			pos = pos + peerinfo_size;
			peerinfo_count++;
		}
		i++;
	}

	// write peerinfo_count
	utilWriteInt32(data->pl_buf, peerinfo_count);

	// set packet metadata
	data->pl_length = (4 + (peerinfo_count * peerinfo_size));
	data->pl_type = packet_PLTYPE_PEERINFO;
	data->pl_options = 0;
}


// Send ping to PeerAddr. Return 1 if successful.
static int peermgtSendPingToAddr(struct s_peermgt *mgt, const struct s_nodeid *tonodeid, const int topeerid, const int topeerct, const struct s_peeraddr *peeraddr) {
	int outpeerid;
	unsigned char pingbuf[peermgt_PINGBUF_SIZE];

	outpeerid = peermgtGetActiveID(mgt, tonodeid, topeerid, topeerct);

	if(outpeerid > 0) {
		cryptoRand(pingbuf, 64); // generate ping message
		memcpy(mgt->rrmsg.msg, pingbuf, peermgt_PINGBUF_SIZE);
		mgt->rrmsgpeerid = outpeerid;
		mgt->rrmsgtype = packet_PLTYPE_PING;
		mgt->rrmsg.len = peermgt_PINGBUF_SIZE;
		mgt->rrmsgusetargetaddr = 1;
		mgt->rrmsgtargetaddr = *peeraddr;
		return 1;
	}

	return 0;
}


// Generate next peer manager packet. Returns length if successful.
static int peermgtGetNextPacketGen(struct s_peermgt *mgt, unsigned char *pbuf, const int pbuf_size, const int tnow, struct s_peeraddr *target) {
	int used = mapGetKeyCount(&mgt->map);
	int len;
	int outlen;
	int fragoutlen;
	int peerid;
	int usetargetaddr;
	int i;
	int j;
	int fragcount;
	int fragpos;
	const int plbuf_size = peermgt_MSGSIZE_MIN;
	unsigned char plbuf[plbuf_size];
	struct s_msg authmsg;
	struct s_packet_data data;
	struct s_nodeid *nodeid;
	struct s_peeraddr *peeraddr;

	// send out user data
	outlen = mgt->outmsg.len;
	fragoutlen = mgt->fragoutsize;
	if(outlen > 0 && (!(fragoutlen > 0))) {
		if(mgt->outmsgbroadcast) { // get PeerID for broadcast message
			do {
				peerid = peermgtGetNextID(mgt);
				mgt->outmsgbroadcastcount++;
			}
			while((!(peermgtIsActiveRemoteID(mgt, peerid) && peermgtGetRemoteFlag(mgt, peerid, peermgt_FLAG_USERDATA))) && (mgt->outmsgbroadcastcount < used));
			if(mgt->outmsgbroadcastcount >= used) {
				mgt->outmsgbroadcast = 0;
				mgt->outmsg.len = 0;
			}
		}
		else { // get PeerID for unicast message
			peerid = mgt->outmsgpeerid;
			mgt->outmsg.len = 0;
		}
		if(peermgtIsActiveRemoteID(mgt, peerid)) {  // check if session is active
			if(peermgtGetRemoteFlag(mgt, peerid, peermgt_FLAG_USERDATA)) {
				if((mgt->fragmentation > 0) && (outlen > peermgt_MSGSIZE_MIN)) {
					// start generating fragmented userdata packets
					mgt->fragoutpeerid = peerid;
					mgt->fragoutcount = (((outlen - 1) / peermgt_MSGSIZE_MIN) + 1); // calculate number of fragments
					mgt->fragoutsize = outlen;
					fragoutlen = outlen;
					mgt->fragoutpos = 0;
				}
				else {
					// generate userdata packet
					data.pl_buf = mgt->outmsg.msg;
					data.pl_buf_size = outlen;
					data.peerid = mgt->data[peerid].remoteid;
					data.seq = ++mgt->data[peerid].remoteseq;
					data.pl_length = outlen;
					data.pl_type = packet_PLTYPE_USERDATA;
					data.pl_options = 0;
					len = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[peerid]);
					if(len > 0) {
						mgt->data[peerid].lastsend = tnow;
						*target = mgt->data[peerid].remoteaddr;
						return len;
					}
				}
			}
		}
	}

	// send out fragments
	if(fragoutlen > 0) {
		fragcount = mgt->fragoutcount;
		fragpos = mgt->fragoutpos;
		peerid = mgt->fragoutpeerid;
		if(peermgtIsActiveRemoteID(mgt, peerid)) {  // check if session is active
			// generate fragmented packet
			data.pl_buf = &mgt->outmsg.msg[(fragpos * peermgt_MSGSIZE_MIN)];
			if(fragoutlen > peermgt_MSGSIZE_MIN) {
				// start or middle fragment
				data.pl_buf_size = peermgt_MSGSIZE_MIN;
				data.pl_length = peermgt_MSGSIZE_MIN;
				mgt->fragoutsize = (fragoutlen - peermgt_MSGSIZE_MIN);
			}
			else {
				// end fragment
				data.pl_buf_size = fragoutlen;
				data.pl_length = fragoutlen;
				mgt->fragoutsize = 0;
			}
			data.peerid = mgt->data[peerid].remoteid;
			data.seq = ++mgt->data[peerid].remoteseq;
			data.pl_type = packet_PLTYPE_USERDATA_FRAGMENT;
			data.pl_options = (fragcount << 4) | (fragpos);
			len = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[peerid]);
			mgt->fragoutpos = (fragpos + 1);
			if(len > 0) {
				mgt->data[peerid].lastsend = tnow;
				*target = mgt->data[peerid].remoteaddr;
				return len;
			}
		}
		else {
			// session not active anymore, abort sending fragments
			mgt->fragoutsize = 0;
		}
	}

	// send out request-response packet
	outlen = mgt->rrmsg.len;
	if(outlen > 0) {
		peerid = mgt->rrmsgpeerid;
		usetargetaddr = mgt->rrmsgusetargetaddr;
		mgt->rrmsg.len = 0;
		mgt->rrmsgusetargetaddr = 0;
		if((outlen < peermgt_MSGSIZE_MAX) && (peermgtIsActiveRemoteID(mgt, peerid))) {  // check if session is active
			data.pl_buf = mgt->rrmsg.msg;
			data.pl_buf_size = peermgt_MSGSIZE_MAX;
			data.pl_length = outlen;
			data.pl_type = mgt->rrmsgtype;
			data.pl_options = 0;
			data.peerid = mgt->data[peerid].remoteid;
			data.seq = ++mgt->data[peerid].remoteseq;

			len = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[peerid]);
			if(len > 0) {
				if(usetargetaddr > 0) {
					*target = mgt->rrmsgtargetaddr;
				}
				else {
					mgt->data[peerid].lastsend = tnow;
					*target = mgt->data[peerid].remoteaddr;
				}
				return len;
			}
		}
	}

	// send peerinfo to peers
	for(i=0; i<used; i++) {
		peerid = peermgtGetNextID(mgt);
		if(peerid > 0) {
			if((tnow - mgt->data[peerid].lastrecv) < peermgt_RECV_TIMEOUT) { // check if session has expired
				if(mgt->data[peerid].state == peermgt_STATE_COMPLETE) {  // check if session is active
					if(((tnow - mgt->data[peerid].lastsend) > peermgt_KEEPALIVE_INTERVAL) || ((tnow - mgt->data[peerid].lastpeerinfo) > peermgt_PEERINFO_INTERVAL)) { // check if we should send peerinfo packet
						data.pl_buf = plbuf;
						data.pl_buf_size = plbuf_size;
						data.peerid = mgt->data[peerid].remoteid;
						data.seq = ++mgt->data[peerid].remoteseq;
						peermgtGenPacketPeerinfo(&data, mgt, peerid);
						len = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[peerid]);
						if(len > 0) {
							mgt->data[peerid].lastsend = tnow;
							mgt->data[peerid].lastpeerinfo = tnow;
							*target = mgt->data[peerid].remoteaddr;
							return len;
						}
					}
				}
			}
			else {
				peermgtDeleteID(mgt, peerid);
			}
		}
	}

	// send auth manager message
	if(authmgtGetNextMsg(&mgt->authmgt, &authmsg, target)) {
		data.pl_buf = authmsg.msg;
		data.pl_buf_size = authmsg.len;
		data.peerid = 0;
		data.seq = 0;
		data.pl_length = authmsg.len;
		if(data.pl_length > 0) {
			data.pl_type = packet_PLTYPE_AUTH;
			data.pl_options = 0;
			len = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[0]);
			if(len > 0) {
				mgt->data[0].lastsend = tnow;
				return len;
			}
		}
	}

	// connect new peer
	if((tnow - mgt->lastconntry) > 0) { // limit to one per second
		mgt->lastconntry = tnow;
		i = -1;

		// find a NodeID and PeerAddr pair in NodeDB
		if(authmgtUsedSlotCount(&mgt->authmgt) <= (authmgtSlotCount(&mgt->authmgt) / 2)) {
			i = nodedbGetDBID(&mgt->nodedb, NULL, peermgt_NEWCONNECT_MAX_LASTSEEN, -1, peermgt_NEWCONNECT_MIN_LASTCONNTRY);
			if((i < 0) && (authmgtUsedSlotCount(&mgt->authmgt) <= (authmgtSlotCount(&mgt->authmgt) / 8))) {
				i = nodedbGetDBID(&mgt->nodedb, NULL, peermgt_NEWCONNECT_MAX_LASTSEEN, -1, -1);
				if((i < 0) && (authmgtUsedSlotCount(&mgt->authmgt) <= (authmgtSlotCount(&mgt->authmgt) / 16))) {
					i = nodedbGetDBID(&mgt->nodedb, NULL, -1, -1, -1);
				}
			}
		}

		// start connection attempt if a node is found
		if(!(i < 0)) {
			nodeid = nodedbGetNodeID(&mgt->nodedb, i);
			peerid = peermgtGetID(mgt, nodeid);
			peeraddr = nodedbGetNodeAddress(&mgt->nodedb, i);
			nodedbUpdate(&mgt->nodedb, nodeid, peeraddr, 0, 0, 1);
			if(peerid < 0) { // node is not connected yet
				if(peermgtConnect(mgt, peeraddr)) { // try to connect
					j = nodedbGetDBID(&mgt->relaydb, nodeid, peermgt_NEWCONNECT_RELAY_MAX_LASTSEEN, -1, peermgt_NEWCONNECT_MIN_LASTCONNTRY);
					if(!(j < 0)) {
						peermgtConnect(mgt, nodedbGetNodeAddress(&mgt->relaydb, j)); // try to connect via relay
						nodedbUpdate(&mgt->relaydb, nodeid, nodedbGetNodeAddress(&mgt->relaydb, j), 0, 0, 1);
					}
				}
			}
			else { // node is already connected
				if(peermgtIsActiveRemoteID(mgt, peerid)) {
					if(peeraddrIsInternal(&mgt->data[peerid].remoteaddr)) { // node connection is indirect
						peermgtSendPingToAddr(mgt, NULL, peerid, mgt->data[peerid].conntime, peeraddr); // try to switch peer to a direct connection
					}
				}
			}
		}

	}

	return 0;
}


// Get next peer manager packet. Also encapsulates packets for relaying if necessary. Returns length if successful.
static int peermgtGetNextPacket(struct s_peermgt *mgt, unsigned char *pbuf, const int pbuf_size, struct s_peeraddr *target) {
	int tnow;
	int outlen;
	int relayid;
	int relayct;
	int relaypeerid;
	int depth;
	struct s_packet_data data;
	tnow = utilGetClock();
	while((outlen = (peermgtGetNextPacketGen(mgt, pbuf, pbuf_size, tnow, target))) > 0) {
		depth = 0;
		while(outlen > 0) {
			if(depth < peermgt_DECODE_RECURSION_MAX_DEPTH) { // limit encapsulation depth
				if(!peeraddrIsInternal(target)) {
					// address is external, packet is ready for sending
					return outlen;
				}
				else {
					if(((packet_PEERID_SIZE + outlen) < peermgt_MSGSIZE_MAX) && (peeraddrGetIndirect(target, &relayid, &relayct, &relaypeerid))) {
						// address is indirect, encapsulate packet for relaying
						if(peermgtIsActiveRemoteIDCT(mgt, relayid, relayct)) {
							// generate relay-in packet
							utilWriteInt32(&mgt->relaymsgbuf[0], relaypeerid);
							memcpy(&mgt->relaymsgbuf[packet_PEERID_SIZE], pbuf, outlen);
							data.pl_buf = mgt->relaymsgbuf;
							data.pl_buf_size = packet_PEERID_SIZE + outlen;
							data.peerid = mgt->data[relayid].remoteid;
							data.seq = ++mgt->data[relayid].remoteseq;
							data.pl_length = packet_PEERID_SIZE + outlen;
							data.pl_type = packet_PLTYPE_RELAY_IN;
							data.pl_options = 0;

							// encode relay-in packet
							outlen = packetEncode(pbuf, pbuf_size, &data, &mgt->ctx[relayid]);
							if(outlen > 0) {
								mgt->data[relayid].lastsend = tnow;
								*target = mgt->data[relayid].remoteaddr;
							}
							else {
								outlen = 0;
							}
						}
						else {
							outlen = 0;
						}
					}
					else {
						outlen = 0;
					}
				}
				depth++;
			}
			else {
				outlen = 0;
			}
		}
	}
	return 0;
}


// Decode auth packet
static int peermgtDecodePacketAuth(struct s_peermgt *mgt, const struct s_packet_data *data, const struct s_peeraddr *source_addr) {
	int tnow = utilGetClock();
	struct s_authmgt *authmgt = &mgt->authmgt;
	struct s_nodeid peer_nodeid;
	int peerid;
	int dupid;
	int64_t remoteflags = 0;

	if(authmgtDecodeMsg(authmgt, data->pl_buf, data->pl_length, source_addr)) {
		if(authmgtGetAuthedPeerNodeID(authmgt, &peer_nodeid)) {
			dupid = peermgtGetID(mgt, &peer_nodeid);
			if(dupid < 0) {
				// Create new PeerID.
				peerid = peermgtNew(mgt, &peer_nodeid, source_addr);
			}
			else {
				// Don't replace active existing session.
				peerid = -1;

				// Upgrade indirect connection to a direct one
				if((peeraddrIsInternal(&mgt->data[dupid].remoteaddr)) && (!peeraddrIsInternal(source_addr))) {
					mgt->data[dupid].remoteaddr = *source_addr;
					peermgtSendPingToAddr(mgt, NULL, dupid, mgt->data[dupid].conntime, source_addr); // send a ping using the new peer address
				}
			}
			if(peerid > 0) {
				// NodeID gets accepted here.
				authmgtAcceptAuthedPeer(authmgt, peerid, seqGet(&mgt->data[peerid].seq), mgt->localflags);
			}
			else {
				// Reject authentication attempt because local PeerID could not be generated.
				authmgtRejectAuthedPeer(authmgt);
			}
		}
		if(authmgtGetCompletedPeerNodeID(authmgt, &peer_nodeid)) {
			peerid = peermgtGetID(mgt, &peer_nodeid);
			if((peerid > 0) && (mgt->data[peerid].state >= peermgt_STATE_AUTHED) && (authmgtGetCompletedPeerLocalID(authmgt)) == peerid) {
				// Node data gets completed here.
				authmgtGetCompletedPeerAddress(authmgt, &mgt->data[peerid].remoteid, &mgt->data[peerid].remoteaddr);
				authmgtGetCompletedPeerSessionKeys(authmgt, &mgt->ctx[peerid]);
				authmgtGetCompletedPeerConnectionParams(authmgt, &mgt->data[peerid].remoteseq, &remoteflags);
				mgt->data[peerid].remoteflags = remoteflags;
				mgt->data[peerid].state = peermgt_STATE_COMPLETE;
				mgt->data[peerid].lastrecv = tnow;
			}
			authmgtFinishCompletedPeer(authmgt);
		}
		return 1;
	}
	else {
		return 0;
	}
}


// Decode peerinfo packet
static int peermgtDecodePacketPeerinfo(struct s_peermgt *mgt, const struct s_packet_data *data) {
	const int peerinfo_size = (packet_PEERID_SIZE + nodeid_SIZE + peeraddr_SIZE);
	struct s_nodeid nodeid;
	struct s_peeraddr addr;
	int peerid;
	int peerinfo_count;
	int peerinfo_max;
	int pos;
	int relaypeerid;
	int i;
	int64_t r;
	if(data->pl_length > 4) {
		peerid = data->peerid;
		if(peermgtIsActiveRemoteID(mgt, peerid)) {
			peerinfo_max = ((data->pl_length - 4) / peerinfo_size);
			peerinfo_count = utilReadInt32(data->pl_buf);
			if(peerinfo_count > 0 && peerinfo_count <= peerinfo_max) {
				r = (abs(cryptoRandInt()) % peerinfo_count); // randomly select a peer
				for(i=0; i<peerinfo_count; i++) {
					pos = (4 + (r * peerinfo_size));
					relaypeerid = utilReadInt32(&data->pl_buf[pos]);
					memcpy(nodeid.id, &data->pl_buf[(pos + (packet_PEERID_SIZE))], nodeid_SIZE);
					memcpy(addr.addr, &data->pl_buf[(pos + (packet_PEERID_SIZE + nodeid_SIZE))], peeraddr_SIZE);
					if(!peeraddrIsInternal(&addr)) { // only accept external PeerAddr
						nodedbUpdate(&mgt->nodedb, &nodeid, &addr, 1, 0, 0);
						if(peermgtGetRemoteFlag(mgt, peerid, peermgt_FLAG_RELAY)) { // add relay data
							peeraddrSetIndirect(&addr, peerid, mgt->data[peerid].conntime, relaypeerid);
							nodedbUpdate(&mgt->relaydb, &nodeid, &addr, 1, 0, 0);
						}
					}
					r = ((r + 1) % peerinfo_count);
				}
				return 1;
			}
		}
	}
	return 0;
}


// Decode ping packet
static int peermgtDecodePacketPing(struct s_peermgt *mgt, const struct s_packet_data *data) {
	int len = data->pl_length;
	if(len == peermgt_PINGBUF_SIZE) {
		memcpy(mgt->rrmsg.msg, data->pl_buf, peermgt_PINGBUF_SIZE);
		mgt->rrmsgpeerid = data->peerid;
		mgt->rrmsgtype = packet_PLTYPE_PONG;
		mgt->rrmsg.len = peermgt_PINGBUF_SIZE;
		mgt->rrmsgusetargetaddr = 0;
		return 1;
	}
	return 0;
}


// Decode pong packet
static int peermgtDecodePacketPong(struct s_peermgt *mgt, const struct s_packet_data *data) {
	int len = data->pl_length;
	if(len == peermgt_PINGBUF_SIZE) {
		// content is not checked, any response is acceptable
		return 1;
	}
	return 0;
}


// Decode relay-in packet
static int peermgtDecodePacketRelayIn(struct s_peermgt *mgt, const struct s_packet_data *data) {
	int targetpeerid;
	int len = data->pl_length;

	if((len > 4) && (len < (peermgt_MSGSIZE_MAX - 4))) {
		targetpeerid = utilReadInt32(data->pl_buf);
		if(peermgtIsActiveRemoteID(mgt, targetpeerid)) {
			utilWriteInt32(&mgt->rrmsg.msg[0], data->peerid);
			memcpy(&mgt->rrmsg.msg[4], &data->pl_buf[4], (len - 4));
			mgt->rrmsgpeerid = targetpeerid;
			mgt->rrmsgtype = packet_PLTYPE_RELAY_OUT;
			mgt->rrmsg.len = len;
			mgt->rrmsgusetargetaddr = 0;
			return 1;
		}
	}
	
	return 0;
}


// Decode fragmented packet
static int peermgtDecodeUserdataFragment(struct s_peermgt *mgt, struct s_packet_data *data) {
	int fragcount = (data->pl_options >> 4);
	int fragpos = (data->pl_options & 0x0F);
	int64_t fragseq = (data->seq - (int64_t)fragpos);
	int peerid = data->peerid;
	int id = dfragAssemble(&mgt->dfrag, mgt->data[peerid].conntime, peerid, fragseq, data->pl_buf, data->pl_length, fragpos, fragcount);
	int len;
	if(!(id < 0)) {
		len = dfragLength(&mgt->dfrag, id);
		if(len > 0 && len <= data->pl_buf_size) {
			memcpy(data->pl_buf, dfragGet(&mgt->dfrag, id), len);
			dfragClear(&mgt->dfrag, id);
			data->pl_length = len;
			return 1;
		}
		else {
			dfragClear(&mgt->dfrag, id);
			data->pl_length = 0;
			return 0;
		}
	}
	else {
		return 0;
	}
}


// Decode input packet recursively. Decapsulates relayed packets if necessary.
static int peermgtDecodePacketRecursive(struct s_peermgt *mgt, const unsigned char *packet, const int packet_len, const struct s_peeraddr *source_addr, const int tnow, const int depth) {
	int ret;
	int peerid;
	struct s_packet_data data = { .pl_buf_size = peermgt_MSGSIZE_MAX, .pl_buf = mgt->msgbuf };
	struct s_peeraddr indirect_addr;
	struct s_nodeid peer_nodeid;
	ret = 0;
	if(packet_len > (packet_PEERID_SIZE + packet_HMAC_SIZE) && (depth < peermgt_DECODE_RECURSION_MAX_DEPTH)) {
		peerid = packetGetPeerID(packet);
		if(peermgtIsActiveID(mgt, peerid)) {
			if(peerid > 0) {
				// packet has an active PeerID
				mgt->msgsize = 0;
				if(packetDecode(&data, packet, packet_len, &mgt->ctx[peerid], &mgt->data[peerid].seq) > 0) {
					if((data.pl_length > 0) && (data.pl_length < peermgt_MSGSIZE_MAX)) {
						switch(data.pl_type) {
							case packet_PLTYPE_USERDATA:
								if(peermgtGetFlag(mgt, peermgt_FLAG_USERDATA)) {
									ret = 1;
									mgt->msgsize = data.pl_length;
									mgt->msgpeerid = data.peerid;
								}
								else {
									ret = 0;
								}
								break;
							case packet_PLTYPE_USERDATA_FRAGMENT:
								if(peermgtGetFlag(mgt, peermgt_FLAG_USERDATA)) {
									ret = peermgtDecodeUserdataFragment(mgt, &data);
									if(ret > 0) {
										mgt->msgsize = data.pl_length;
										mgt->msgpeerid = data.peerid;
									}
								}
								else {
									ret = 0;
								}
								break;
							case packet_PLTYPE_PEERINFO:
								ret = peermgtDecodePacketPeerinfo(mgt, &data);
								break;
							case packet_PLTYPE_PING:
								ret = peermgtDecodePacketPing(mgt, &data);
								break;
							case packet_PLTYPE_PONG:
								ret = peermgtDecodePacketPong(mgt, &data);
								break;
							case packet_PLTYPE_RELAY_IN:
								if(peermgtGetFlag(mgt, peermgt_FLAG_RELAY)) {
									ret = peermgtDecodePacketRelayIn(mgt, &data);
								}
								else {
									ret = 0;
								}
								break;
							case packet_PLTYPE_RELAY_OUT:
								if(data.pl_length > packet_PEERID_SIZE) {
									memcpy(mgt->relaymsgbuf, &data.pl_buf[4], (data.pl_length - packet_PEERID_SIZE)); 
									peeraddrSetIndirect(&indirect_addr, peerid, mgt->data[peerid].conntime, utilReadInt32(&data.pl_buf[0])); // generate indirect PeerAddr
									ret = peermgtDecodePacketRecursive(mgt, mgt->relaymsgbuf, (data.pl_length - packet_PEERID_SIZE), &indirect_addr, tnow, (depth + 1)); // decode decapsulated packet
								}
								break;
							default:
								ret = 0;
								break;
						}
						if(ret > 0) {
							if(mgt->data[peerid].lastrecv != tnow) { // Update NodeDB (maximum once per second).
								if(!peeraddrIsInternal(&mgt->data[peerid].remoteaddr)) { // do not pollute NodeDB with internal addresses
									if(peermgtGetNodeID(mgt, &peer_nodeid, peerid)) {
										nodedbUpdate(&mgt->nodedb, &peer_nodeid, &mgt->data[peerid].remoteaddr, 1, 1, 0);
									}
								}
							}
							mgt->data[peerid].lastrecv = tnow;
							mgt->data[peerid].remoteaddr = *source_addr;
							return 1;
						}
					}
				}
			}
			else if(peerid == 0) {
				// packet has an anonymous PeerID
				if(packetDecode(&data, packet, packet_len, &mgt->ctx[0], NULL)) {
					switch(data.pl_type) {
						case packet_PLTYPE_AUTH:
							return peermgtDecodePacketAuth(mgt, &data, source_addr);
						default:
							return 0;
					}
				}
			}
		}
	}
	return 0;
}


// Decode input packet. Returns 1 on success.
static int peermgtDecodePacket(struct s_peermgt *mgt, const unsigned char *packet, const int packet_len, const struct s_peeraddr *source_addr) {
	int tnow;
	tnow = utilGetClock();
	return peermgtDecodePacketRecursive(mgt, packet, packet_len, source_addr, tnow, 0);
}


// Return received user data. Return 1 if successful.
static int peermgtRecvUserdata(struct s_peermgt *mgt, struct s_msg *recvmsg, struct s_nodeid *fromnodeid, int *frompeerid, int *frompeerct) {
	if((mgt->msgsize > 0) && (recvmsg != NULL)) {
		recvmsg->msg = mgt->msgbuf;
		recvmsg->len = mgt->msgsize;
		if(fromnodeid != NULL) peermgtGetNodeID(mgt, fromnodeid, mgt->msgpeerid);
		if(frompeerid != NULL) *frompeerid = mgt->msgpeerid;
		if(frompeerct != NULL) {
			if(peermgtIsActiveID(mgt, mgt->msgpeerid)) {
				*frompeerct = mgt->data[mgt->msgpeerid].conntime;
			}
			else {
				*frompeerct = 0;
			}
		}
		mgt->msgsize = 0;
		return 1;
	}
	else {
		return 0;
	}
}


// Send user data. Return 1 if successful.
static int peermgtSendUserdata(struct s_peermgt *mgt, const struct s_msg *sendmsg, const struct s_nodeid *tonodeid, const int topeerid, const int topeerct) {
	int outpeerid;

	mgt->outmsgbroadcast = 0;
	mgt->outmsg.len = 0;
	if(sendmsg != NULL) {
		if((sendmsg->len > 0) && (sendmsg->len <= peermgt_MSGSIZE_MAX)) {
			outpeerid = peermgtGetActiveID(mgt, tonodeid, topeerid, topeerct);
			if(outpeerid >= 0) {
				if(outpeerid > 0) {
					// message goes out
					mgt->outmsg.msg = sendmsg->msg;
					mgt->outmsg.len = sendmsg->len;
					mgt->outmsgpeerid = outpeerid;
					return 1;
				}
				else {
					// message goes to loopback
					if(mgt->loopback) {
						memcpy(mgt->msgbuf, sendmsg->msg, sendmsg->len);
						mgt->msgsize = sendmsg->len;
						mgt->msgpeerid = outpeerid;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}


// Send user data to all connected peers. Return 1 if successful.
static int peermgtSendBroadcastUserdata(struct s_peermgt *mgt, const struct s_msg *sendmsg) {
	mgt->outmsgbroadcast = 0;
	mgt->outmsg.len = 0;
	if(sendmsg != NULL) {
		if((sendmsg->len > 0) && (sendmsg->len <= peermgt_MSGSIZE_MAX)) {
			mgt->outmsg.msg = sendmsg->msg;
			mgt->outmsg.len = sendmsg->len;
			mgt->outmsgpeerid = -1;
			mgt->outmsgbroadcast = 1;
			mgt->outmsgbroadcastcount = 0;
			return 1;
		}
	}
	return 0;
}


// Set NetID from network name.
static int peermgtSetNetID(struct s_peermgt *mgt, const char *netname, const int netname_len) {
	return netidSet(&mgt->netid, netname, netname_len);
}


// Set shared group password.
static int peermgtSetPassword(struct s_peermgt *mgt, const char *password, const int password_len) {
	return cryptoSetSessionKeysFromPassword(&mgt->ctx[0], (const unsigned char *)password, password_len, crypto_AES256, crypto_SHA256);
}


// Initialize peer manager object.
static int peermgtInit(struct s_peermgt *mgt) {
	const char *defaultpw = "default";
	int tnow;
	int i;
	int s = mapGetMapSize(&mgt->map);
	struct s_peeraddr empty_addr;
	struct s_nodeid *local_nodeid = &mgt->nodekey->nodeid;

	mgt->msgsize = 0;
	mgt->loopback = 0;
	mgt->outmsg.len = 0;
	mgt->outmsgbroadcast = 0;
	mgt->outmsgbroadcastcount = 0;
	mgt->rrmsg.len = 0;
	mgt->rrmsgpeerid = 0;
	mgt->rrmsgusetargetaddr = 0;
	mgt->fragoutpeerid = 0;
	mgt->fragoutcount = 0;
	mgt->fragoutsize = 0;
	mgt->fragoutpos = 0;
	mgt->localflags = 0;

	for(i=0; i<s; i++) {
		mgt->data[i].state = peermgt_STATE_INVALID;
	}

	memset(empty_addr.addr, 0, peeraddr_SIZE);
	mapInit(&mgt->map);
	authmgtReset(&mgt->authmgt);
	nodedbInit(&mgt->nodedb);
	nodedbInit(&mgt->relaydb);

	if(peermgtNew(mgt, local_nodeid, &empty_addr) == 0) { // ID 0 should always represent local NodeID
		if(peermgtGetID(mgt, local_nodeid) == 0) {
			if(peermgtSetNetID(mgt, defaultpw, 7) && peermgtSetPassword(mgt, defaultpw, 7)) {
				mgt->data[0].state = peermgt_STATE_COMPLETE;
				tnow = utilGetClock();
				mgt->tinit = tnow;
				mgt->lastconntry = tnow;
				return 1;
			}
		}
	}

	return 0;
}


// Return peer manager uptime in seconds.
static int peermgtUptime(struct s_peermgt *mgt) {
	int uptime = utilGetClock() - mgt->tinit;
	return uptime;
}


// Generate peer manager status report.
static void peermgtStatus(struct s_peermgt *mgt, char *report, const int report_len) {
	int tnow = utilGetClock();
	int pos = 0;
	int size = mapGetMapSize(&mgt->map);
	int maxpos = (((size + 2) * (160)) + 1);
	unsigned char infoid[packet_PEERID_SIZE];
	unsigned char infostate[1];
	unsigned char infoflags[2];
	unsigned char inforq[1];
	unsigned char timediff[4];
	struct s_nodeid nodeid;
	int i = 0;

	if(maxpos > report_len) { maxpos = report_len; }
	
	memcpy(&report[pos], "PeerID    NodeID                                                            Address                                       Status  LastPkt   SessAge   Flag  RQ", 158);
	pos = pos + 158;
	report[pos++] = '\n';

	while(i < size && pos < maxpos) {
		if(peermgtGetNodeID(mgt, &nodeid, i)) {
			utilWriteInt32(infoid, i);
			utilByteArrayToHexstring(&report[pos], ((packet_PEERID_SIZE * 2) + 2), infoid, packet_PEERID_SIZE);
			pos = pos + (packet_PEERID_SIZE * 2);
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilByteArrayToHexstring(&report[pos], ((nodeid_SIZE * 2) + 2), nodeid.id, nodeid_SIZE);
			pos = pos + (nodeid_SIZE * 2);
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilByteArrayToHexstring(&report[pos], ((peeraddr_SIZE * 2) + 2), mgt->data[i].remoteaddr.addr, peeraddr_SIZE);
			pos = pos + (peeraddr_SIZE * 2);
			report[pos++] = ' ';
			report[pos++] = ' ';
			infostate[0] = mgt->data[i].state;
			utilByteArrayToHexstring(&report[pos], 4, infostate, 1);
			pos = pos + 2;
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt32(timediff, (tnow - mgt->data[i].lastrecv));
			utilByteArrayToHexstring(&report[pos], 10, timediff, 4);
			pos = pos + 8;
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt32(timediff, (tnow - mgt->data[i].conntime));
			utilByteArrayToHexstring(&report[pos], 10, timediff, 4);
			pos = pos + 8;
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt16(infoflags, mgt->data[i].remoteflags);
			utilByteArrayToHexstring(&report[pos], 6, infoflags, 2);
			pos = pos + 4;
			report[pos++] = ' ';
			report[pos++] = ' ';
			inforq[0] = seqRQ(&mgt->data[i].seq);
			utilByteArrayToHexstring(&report[pos], 4, inforq, 1);
			pos = pos + 2;
			report[pos++] = '\n';
		}
		i++;
	}
	report[pos++] = '\0';
}


// Create peer manager object.
static int peermgtCreate(struct s_peermgt *mgt, const int peer_slots, const int auth_slots, struct s_nodekey *local_nodekey, struct s_dh_state *dhstate) {
	const char *defaultid = "default";
	struct s_peermgt_data *data_mem;
	struct s_crypto *ctx_mem;

	if((peer_slots > 0) && (auth_slots > 0) && (peermgtSetNetID(mgt, defaultid, 7))) {
		data_mem = malloc(sizeof(struct s_peermgt_data) * (peer_slots + 1));
		if(data_mem != NULL) {
			ctx_mem = malloc(sizeof(struct s_crypto) * (peer_slots + 1));
			if(ctx_mem != NULL) {
				if(cryptoCreate(ctx_mem, (peer_slots + 1))) {
					if(dfragCreate(&mgt->dfrag, peermgt_MSGSIZE_MIN, peermgt_FRAGBUF_COUNT)) {
						if(authmgtCreate(&mgt->authmgt, &mgt->netid, auth_slots, local_nodekey, dhstate)) {
							if(nodedbCreate(&mgt->relaydb, (peer_slots + 1), peermgt_RELAYDB_NUM_PEERADDRS)) {
								if(nodedbCreate(&mgt->nodedb, ((peer_slots * 8) + 1), peermgt_NODEDB_NUM_PEERADDRS)) {
									if(mapCreate(&mgt->map, (peer_slots + 1), nodeid_SIZE, 1)) {
										mgt->nodekey = local_nodekey;
										mgt->data = data_mem;
										mgt->ctx = ctx_mem;
										mgt->rrmsg.msg = mgt->rrmsgbuf;
										if(peermgtInit(mgt)) {
											return 1;
										}
										mgt->nodekey = NULL;
										mgt->data = NULL;
										mgt->ctx = NULL;
										mapDestroy(&mgt->map);
									}
									nodedbDestroy(&mgt->nodedb);
								}
								nodedbDestroy(&mgt->relaydb);
							}
							authmgtDestroy(&mgt->authmgt);
						}
						dfragDestroy(&mgt->dfrag);
					}
					cryptoDestroy(ctx_mem, (peer_slots + 1));
				}
				free(ctx_mem);
			}
			free(data_mem);
		}
	}
	return 0;
}


// Destroy peer manager object.
static void peermgtDestroy(struct s_peermgt *mgt) {
	int size = mapGetMapSize(&mgt->map);
	mapDestroy(&mgt->map);
	nodedbDestroy(&mgt->nodedb);
	nodedbDestroy(&mgt->relaydb);
	authmgtDestroy(&mgt->authmgt);
	dfragDestroy(&mgt->dfrag);
	cryptoDestroy(mgt->ctx, size);
	free(mgt->ctx);
	free(mgt->data);
}


#endif // F_PEERMGT_C
