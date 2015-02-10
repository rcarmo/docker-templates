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


#ifndef F_P2PSEC_C
#define F_P2PSEC_C


#include "p2psec.h"
#include "peermgt.c"


P2PSEC_CTX {
	struct s_peermgt mgt;
	struct s_nodekey nk;
	struct s_dh_state dh;
	int started;
	int key_loaded;
	int dh_loaded;
	int peer_count;
	int auth_count;
	int loopback_enable;
	int fastauth_enable;
	int fragmentation_enable;
	int flags;
	char password[1024];
	int password_len;
	char netname[1024];
	int netname_len;
};


int p2psecStart(P2PSEC_CTX *p2psec) {
	if((!p2psec->started) && (p2psec->key_loaded) && (p2psec->dh_loaded)) {
		if(peermgtCreate(&p2psec->mgt, p2psec->peer_count, p2psec->auth_count, &p2psec->nk, &p2psec->dh)) {
			peermgtSetLoopback(&p2psec->mgt, p2psec->loopback_enable);
			peermgtSetFastauth(&p2psec->mgt, p2psec->fastauth_enable);
			peermgtSetFragmentation(&p2psec->mgt, p2psec->fragmentation_enable);
			peermgtSetNetID(&p2psec->mgt, p2psec->netname, p2psec->netname_len);
			peermgtSetPassword(&p2psec->mgt, p2psec->password, p2psec->password_len);
			peermgtSetFlags(&p2psec->mgt, p2psec->flags);
			p2psec->started = 1;
			return 1;
		}
	}
	return 0;
}


void p2psecStop(P2PSEC_CTX *p2psec) {
	if(p2psec->started) {
		peermgtDestroy(&p2psec->mgt);
		p2psec->started = 0;
	}
}


int p2psecGetAddrSize() {
	return peeraddr_SIZE;
}


int p2psecGetNodeIDSize() {
	return nodeid_SIZE;
}


int p2psecLoadPrivkey(P2PSEC_CTX *p2psec, unsigned char *pemdata, const int pemdata_len) {
	if(p2psec->key_loaded) nodekeyDestroy(&p2psec->nk);
	if(nodekeyCreate(&p2psec->nk)) {
		if(nodekeyLoadPrivatePEM(&p2psec->nk, pemdata, pemdata_len)) {
			p2psec->key_loaded = 1;
			return 1;
		}
		nodekeyDestroy(&p2psec->nk);
	}
	p2psec->key_loaded = 0;
	return 0;
}


int p2psecGeneratePrivkey(P2PSEC_CTX *p2psec, const int bits) {
	if(p2psec->key_loaded) nodekeyDestroy(&p2psec->nk);
	if(bits >= 1024 && bits <= 3072) {
		if(nodekeyCreate(&p2psec->nk)) {
			if(nodekeyGenerate(&p2psec->nk, bits)) {
				p2psec->key_loaded = 1;
				return 1;
			}
			nodekeyDestroy(&p2psec->nk);
		}
	}
	p2psec->key_loaded = 0;
	return 0;
}


int p2psecLoadDH(P2PSEC_CTX *p2psec) {
	if(p2psec->dh_loaded) return 1;
	if(dhCreate(&p2psec->dh)) {
		p2psec->dh_loaded = 1;
		return 1;
	}
	return 0;
}


void p2psecSetMaxConnectedPeers(P2PSEC_CTX *p2psec, const int peer_count) {
	if(peer_count > 0) p2psec->peer_count = peer_count;
	
}


void p2psecSetAuthSlotCount(P2PSEC_CTX *p2psec, const int auth_slot_count) {
	if(auth_slot_count > 0) p2psec->auth_count = auth_slot_count;
}


void p2psecSetNetname(P2PSEC_CTX *p2psec, const char *netname, const int netname_len) {
	int len;
	if(netname_len < 1024) {
		len = netname_len;
	}
	else {
		len = 1023;
	}
	memset(p2psec->netname, 0, 1024);
	if(len > 0) {
		utilStringFilter(p2psec->netname, netname, len);
		p2psec->netname_len = len;
	}
	else {
		memcpy(p2psec->netname, "default", 7);
		p2psec->netname_len = 7;
	}
	if(p2psec->started) peermgtSetNetID(&p2psec->mgt, p2psec->netname, p2psec->netname_len);
}


void p2psecSetPassword(P2PSEC_CTX *p2psec, const char *password, const int password_len) {
	int len;
	if(password_len < 1024) {
		len = password_len;
	}
	else {
		len = 1023;
	}
	memset(p2psec->password, 0, 1024);
	if(len > 0) {
		memcpy(p2psec->password, password, len);
		p2psec->password_len = len;
	}
	else {
		memcpy(p2psec->password, "default", 7);
		p2psec->password_len = 7;
	}
	if(p2psec->started) peermgtSetPassword(&p2psec->mgt, p2psec->password, p2psec->password_len);
}


void p2psecEnableLoopback(P2PSEC_CTX *p2psec) {
	p2psec->loopback_enable = 1;
	if(p2psec->started) peermgtSetLoopback(&p2psec->mgt, 1);
}


void p2psecDisableLoopback(P2PSEC_CTX *p2psec) {
	p2psec->loopback_enable = 0;
	if(p2psec->started) peermgtSetLoopback(&p2psec->mgt, 0);
}


void p2psecEnableFastauth(P2PSEC_CTX *p2psec) {
	p2psec->fastauth_enable = 1;
	if(p2psec->started) peermgtSetFastauth(&p2psec->mgt, 1);
}


void p2psecDisableFastauth(P2PSEC_CTX *p2psec) {
	p2psec->fastauth_enable = 0;
	if(p2psec->started) peermgtSetFastauth(&p2psec->mgt, 0);
}


void p2psecEnableFragmentation(P2PSEC_CTX *p2psec) {
	p2psec->fragmentation_enable = 1;
	if(p2psec->started) peermgtSetFragmentation(&p2psec->mgt, 1);
}


void p2psecDisableFragmentation(P2PSEC_CTX *p2psec) {
	p2psec->fragmentation_enable = 0;
	if(p2psec->started) peermgtSetFragmentation(&p2psec->mgt, 0);
}


void p2psecSetFlag(P2PSEC_CTX *p2psec, const int flag, const int enable) {
	int f;
	if(enable) {
		f = (p2psec->flags | flag);
	}
	else {
		f = (p2psec->flags & (~flag));
	}
	p2psec->flags = f;
	if(p2psec->started) peermgtSetFlags(&p2psec->mgt, f);
}


void p2psecEnableUserdata(P2PSEC_CTX *p2psec) {
	p2psecSetFlag(p2psec, peermgt_FLAG_USERDATA, 1);
}


void p2psecDisableUserdata(P2PSEC_CTX *p2psec) {
	p2psecSetFlag(p2psec, peermgt_FLAG_USERDATA, 0);
}


void p2psecEnableRelay(P2PSEC_CTX *p2psec) {
	p2psecSetFlag(p2psec, peermgt_FLAG_RELAY, 1);
}


void p2psecDisableRelay(P2PSEC_CTX *p2psec) {
	p2psecSetFlag(p2psec, peermgt_FLAG_RELAY, 0);
}


int p2psecLoadDefaults(P2PSEC_CTX *p2psec) {
	if(!p2psecLoadDH(p2psec)) return 0;
	p2psecSetFlag(p2psec, (~(0)), 0);
	p2psecSetMaxConnectedPeers(p2psec, 256);
	p2psecSetAuthSlotCount(p2psec, 32);
	p2psecDisableLoopback(p2psec);
	p2psecEnableFastauth(p2psec);
	p2psecDisableFragmentation(p2psec);
	p2psecEnableUserdata(p2psec);
	p2psecDisableRelay(p2psec);
	p2psecSetNetname(p2psec, NULL, 0);
	p2psecSetPassword(p2psec, NULL, 0);
	return 1;
}


P2PSEC_CTX *p2psecCreate() {
	P2PSEC_CTX *p2psec;
	p2psec = malloc(sizeof(P2PSEC_CTX));
	if(p2psec != NULL) {
		p2psec->started = 0;
		p2psec->key_loaded = 0;
		p2psec->dh_loaded = 0;
		p2psec->flags = 0;
		p2psecLoadDefaults(p2psec);
		return p2psec;
	}
	return NULL;
}


void p2psecDestroy(P2PSEC_CTX *p2psec) {
	p2psecStop(p2psec);
	if(p2psec->key_loaded) nodekeyDestroy(&p2psec->nk);
	if(p2psec->dh_loaded) dhDestroy(&p2psec->dh);
	p2psec->started = 0;
	p2psec->key_loaded = 0;
	p2psec->dh_loaded = 0;
	memset(p2psec->password, 0, 1024);
	p2psec->password_len = 0;
	memset(p2psec->netname, 0, 1024);
	p2psec->netname_len = 0;
	free(p2psec);
}


void p2psecStatus(P2PSEC_CTX *p2psec, char *status_report, const int status_report_len) {
	peermgtStatus(&p2psec->mgt, status_report, status_report_len);
}


void p2psecNodeDBStatus(P2PSEC_CTX *p2psec, char *status_report, const int status_report_len) {
	nodedbStatus(&p2psec->mgt.nodedb, status_report, status_report_len);
}


int p2psecConnect(P2PSEC_CTX *p2psec, const unsigned char *destination_addr) {
	struct s_peeraddr addr;
	memcpy(addr.addr, destination_addr, peeraddr_SIZE);
	return peermgtConnect(&p2psec->mgt, &addr);
}


int p2psecInputPacket(P2PSEC_CTX *p2psec, const unsigned char *packet_input, const int packet_input_len, const unsigned char *packet_source_addr) {
	struct s_peeraddr addr;
	memcpy(addr.addr, packet_source_addr, peeraddr_SIZE);
	return peermgtDecodePacket(&p2psec->mgt, packet_input, packet_input_len, &addr);
}


unsigned char *p2psecRecvMSG(P2PSEC_CTX *p2psec, unsigned char *source_nodeid, int *message_len) {
	struct s_msg msg;
	struct s_nodeid nodeid;
	if(peermgtRecvUserdata(&p2psec->mgt, &msg, &nodeid, NULL, NULL)) {
		*message_len = msg.len;
		if(source_nodeid != NULL) memcpy(source_nodeid, nodeid.id, nodeid_SIZE);
		return msg.msg;
	}
	else {
		return NULL;
	}
}


unsigned char *p2psecRecvMSGFromPeerID(P2PSEC_CTX *p2psec, int *source_peerid, int *source_peerct, int *message_len) {
	struct s_msg msg;
	if(peermgtRecvUserdata(&p2psec->mgt, &msg, NULL, source_peerid, source_peerct)) {
		*message_len = msg.len;
		return msg.msg;
	}
	else {
		return NULL;
	}
}


int p2psecSendMSG(P2PSEC_CTX *p2psec, const unsigned char *destination_nodeid, unsigned char *message, int message_len) {
	struct s_msg msg = { .msg = message, .len = message_len };
	struct s_nodeid nodeid;
	memcpy(nodeid.id, destination_nodeid, nodeid_SIZE);
	return peermgtSendUserdata(&p2psec->mgt, &msg, &nodeid, -1, -1);
}


int p2psecSendBroadcastMSG(P2PSEC_CTX *p2psec, unsigned char *message, int message_len) {
	struct s_msg msg = { .msg = message, .len = message_len };
	return peermgtSendBroadcastUserdata(&p2psec->mgt, &msg);
}


int p2psecSendMSGToPeerID(P2PSEC_CTX *p2psec, const int destination_peerid, const int destination_peerct, unsigned char *message, int message_len) {
	struct s_msg msg = { .msg = message, .len = message_len };
	return peermgtSendUserdata(&p2psec->mgt, &msg, NULL, destination_peerid, destination_peerct);
}


int p2psecOutputPacket(P2PSEC_CTX *p2psec, unsigned char *packet_output, const int packet_output_len, unsigned char *packet_destination_addr) {
	struct s_peeraddr addr;
	int len = peermgtGetNextPacket(&p2psec->mgt, packet_output, packet_output_len, &addr);
	if(len > 0) {
		memcpy(packet_destination_addr, addr.addr, peeraddr_SIZE);
		return len;
	}
	else {
		return 0;
	}
}


int p2psecPeerCount(P2PSEC_CTX *p2psec) {
	int n = peermgtPeerCount(&p2psec->mgt);
	return n;
}


int p2psecUptime(P2PSEC_CTX *p2psec) {
	int uptime = peermgtUptime(&p2psec->mgt);
	return uptime;
}


#endif // F_P2PSEC_C
