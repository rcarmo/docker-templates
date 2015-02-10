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


// generates warning message
static void logWarning(char *msg) {
	if(msg != NULL) printf("[%d] warning: %s\n", p2psecUptime(g_p2psec), msg);
}


// Connect initpeers.
static void connectInitpeers() {
	int i,j,k,l;
	char *hostname = NULL;
	char *port = NULL;
	struct s_io_addrinfo new_peeraddrs;
	i=0;j=0;k=0;l=0;
	for(;;) {
		j = g_initpeers[i];
		if((j > 0) && (i+j+1 < INITPEER_STORAGE)) {
			if(k) {
				port = &g_initpeers[i+1];
				printf("[%d] resolving %s:%s...\n",p2psecUptime(g_p2psec),hostname,port);
				if(ioResolveName(&new_peeraddrs, hostname, port)) {
					for(l=0; l<new_peeraddrs.count; l++) {
						if(p2psecConnect(g_p2psec,new_peeraddrs.item[l].addr)) {
							printf("             done.\n");
						}
						else {
							printf("             failed: connection could not be created.\n");
						}
					}
				}
				else {
					printf("             failed: name could not be resolved.\n");
				}
				k=0;
			}
			else {
				hostname = &g_initpeers[i+1];
				k=1;
			}
			i=i+j+1;
		}
		else {
			break;
		}
	}
}


// the mainloop
static void mainLoop() {
	int fd;
	int tnow;
	unsigned char sockdata_buf[4096];
	int sockdata_len;
	int sockdata_lastlen;
	unsigned char tapmsg_buf[1024];
	int tapmsg_len;
	unsigned char *msg;
	unsigned char *msg_buf;
	int msg_len;
	int msg_offset;
	int msg_ok;
	int lastinit = 0;
	int laststatus = 0;
	int lastconnectcount = -1;
	int connectcount = 0;
	int do_broadcast = 0;
	int ndp_peerid = 0;
	int ndp_peerct = 0;
	int frametype;
	int source_peerid;
	int source_peerct;
	struct s_io_addr new_peeraddr;

	msg_len = 0;
	sockdata_len = 0;
	sockdata_lastlen = 0;
	tapmsg_len = 0;

	while(g_mainloop) {
		tnow = utilGetClock();
		
		// read all fds
		ioReadAll(&iostate);

		// check udp sockets
		while(!((fd = (ioGetGroup(&iostate, IOGRP_SOCKET))) < 0)) {
			if(p2psecInputPacket(g_p2psec, ioGetData(&iostate, fd), ioGetDataLen(&iostate, fd), ioGetAddr(&iostate, fd)->addr)) {
				// output frames to tap device
				msg = p2psecRecvMSGFromPeerID(g_p2psec, &source_peerid, &source_peerct, &msg_len);
				if(msg != NULL && msg_len > 12 && g_enableeth > 0) {
					switchFrameIn(&g_switchstate, msg, msg_len, source_peerid, source_peerct);
					ndp6PacketIn(&g_ndpstate, msg, msg_len, source_peerid, source_peerct);
					if(!(ioWriteGroup(&iostate, IOGRP_TAP, msg, msg_len, NULL) > 0)) {
						logWarning("could not write to tap device!");
					}
				}
				
				// output packets
				while((sockdata_len = (p2psecOutputPacket(g_p2psec, sockdata_buf, 4096, new_peeraddr.addr))) > 0) {
					sockdata_lastlen = sockdata_len;
					if(!(ioWriteGroup(&iostate, IOGRP_SOCKET, sockdata_buf, sockdata_len, &new_peeraddr) > 0)) {
						logWarning("could not send packet!");
					}
				}
			}
			ioGetClear(&iostate, fd);
		}

		// check for ethernet frames on tap device
		if(g_enableeth > 0) {
			while(!((fd = (ioGetGroup(&iostate, IOGRP_TAP))) < 0)) {
				msg_buf = ioGetData(&iostate, fd);
				msg_len = ioGetDataLen(&iostate, fd);
				msg_ok = 1;

				// check frame
				if((sockdata_lastlen > 0) && (msg_len > sockdata_lastlen)) {
					msg_offset = msg_len - sockdata_lastlen;
					if(memcmp(&msg_buf[msg_offset], sockdata_buf, sockdata_lastlen) == 0) {
						// drop packets which have been sent out via PeerVPN's socket before to avoid loops
						logWarning("recursive packet filtered!");
						msg_ok = 0;
					}
				}

				// process frame
				if(msg_ok) {
					if((g_enablevirtserv) && ((tapmsg_len = (virtservFrame(&g_virtserv, tapmsg_buf, 1024, msg_buf, msg_len))) > 0)) {
						// virtual service frame
						if(!(ioWriteGroup(&iostate, IOGRP_TAP, tapmsg_buf, tapmsg_len, NULL) > 0)) {
							logWarning("could not write to tap device!");
						}
					}
					else {
						// regular frame
						frametype = switchFrameOut(&g_switchstate, msg_buf, msg_len, &source_peerid, &source_peerct);
						switch(frametype) {
							case switch_FRAME_TYPE_UNICAST:
								if(peermgtIsActiveIDCT(&g_p2psec->mgt, source_peerid, source_peerct)) {
									do_broadcast = 0;
									p2psecSendMSGToPeerID(g_p2psec, source_peerid, source_peerct, msg_buf, msg_len);
								}
								else {
									do_broadcast = 1;
								}
								break;
							case(switch_FRAME_TYPE_BROADCAST):
								do_broadcast = 1;
								break;
							default:
								do_broadcast = 0;
								break;
						}
						if(do_broadcast) {
							if(g_enablendpcache) {
								// ndp cache enabled, check whether we can avoid the broadcast and answer from the cache instead
								tapmsg_len = ndp6GenAdv(&g_ndpstate, msg_buf, msg_len, tapmsg_buf, 128, &ndp_peerid, &ndp_peerct);
								if(tapmsg_len > 0) {
									if(peermgtIsActiveIDCT(&g_p2psec->mgt, ndp_peerid, ndp_peerct)) {
										// answer from cache
										if(!(ioWriteGroup(&iostate, IOGRP_TAP, tapmsg_buf, tapmsg_len, NULL) > 0)) {
											logWarning("could not write to tap device!");
										}
									}
									else {
										// cache entry is outdated, send broadcast
										p2psecSendBroadcastMSG(g_p2psec, msg_buf, msg_len);
									}
								}
								else {
									// no cache entry or message not a neighbour solicitation, send broadcast
									p2psecSendBroadcastMSG(g_p2psec, msg_buf, msg_len);
								}
							}
							else {
								// ndp cache disabled, send broadcast
								p2psecSendBroadcastMSG(g_p2psec, msg_buf, msg_len);
							}
						}
					
						// output packets
						while((sockdata_len = (p2psecOutputPacket(g_p2psec, sockdata_buf, 4096, new_peeraddr.addr))) > 0) {
							sockdata_lastlen = sockdata_len;
							if(!(ioWriteGroup(&iostate, IOGRP_SOCKET, sockdata_buf, sockdata_len, &new_peeraddr) > 0)) {
								logWarning("could not send packet!");
							}
						}
					}
				}

				// clear frame
				ioGetClear(&iostate, fd);
			}
		}

		// output packets
		while((sockdata_len = (p2psecOutputPacket(g_p2psec, sockdata_buf, 4096, new_peeraddr.addr))) > 0) {
			sockdata_lastlen = sockdata_len;
			if(!(ioWriteGroup(&iostate, IOGRP_SOCKET, sockdata_buf, sockdata_len, &new_peeraddr) > 0)) {
				logWarning("could not send packet!");
			}
		}

		// show status
		if((tnow - laststatus) > 10) {
			laststatus = tnow;
			connectcount = p2psecPeerCount(g_p2psec);
			if(lastconnectcount != connectcount) {
				printf("[%d] %d peers connected.\n", p2psecUptime(g_p2psec), connectcount);
				lastconnectcount = connectcount;
			}
		}
		/*
		if((tnow - laststatus) > 0) {
			laststatus = tnow;
			connectcount = p2psecPeerCount(g_p2psec);
			printf("\ruptime: %d, peers connected: %d", p2psecUptime(g_p2psec), connectcount);
		}
		*/

		// connect initpeers
		if(((tnow - lastinit) > 30) && (!(mapGetKeyCount(&g_p2psec->mgt.map) > 1))) {
			lastinit = tnow;
			connectInitpeers();
		}
		
		// check console
		if(g_enableconsole > 0) {
			if(!((fd = (ioGetGroup(&iostate, IOGRP_CONSOLE))) < 0)) {
				decodeConsole((char *)ioGetData(&iostate, fd), ioGetDataLen(&iostate, fd));
				ioGetClear(&iostate, fd);
			}
		}
	}
}

