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



// print table of active peers
static void printActivePeerTable() {
	char str[32768];
	p2psecStatus(g_p2psec, str, 32768);
	printf("%s\n", str);
}


// print NodeDB
static void printNodeDB() {
	char str[32768];
	p2psecNodeDBStatus(g_p2psec, str, 32768);
	printf("%s\n", str);
}


// print RelayDB
static void printRelayDB() {
	char str[32768];
	nodedbStatus(&g_p2psec->mgt.relaydb, str, 32768);
	printf("%s\n", str);
}


// print table of mac addrs
static void printMacTable() {
	char str[32768];
	switchStatus(&g_switchstate, str, 32768);
	printf("%s\n", str);
}


// print table of ndp cache
static void printNDPTable() {
	char str[32768];
	ndp6Status(&g_ndpstate, str, 32768);
	printf("%s\n", str);
}


// parse command
static void decodeConsole(char *cmd, int cmdlen) {
	char text[4096];
	char pa[1024];
	char pb[1024];
	char pc[1024];
	int i, j;
	struct s_io_addrinfo new_peeraddrs;
	struct s_peeraddr new_peeraddr;	
	
	pa[0] = '\0';
	pb[0] = '\0';
	pc[0] = '\0';
	sscanf(cmd,"%s %s %s",pa,pb,pc);
	if(pa[0] == 'A' || pa[0] == 'a') {
		// ACTIVEPEERTABLE
		printActivePeerTable();
	}
	if(pa[0] == 'D' || pa[0] == 'd') {
		// DB
		printNodeDB();
	}
	if(pa[0] == 'F' || pa[0] == 'f') {
		// FDB
		printRelayDB();
	}
	if(pa[0] == 'I' || pa[0] == 'i') {
		// INSERTPEER <address>
		if(ioResolveName(&new_peeraddrs, pb, pc)) {
			for(i=0; i<new_peeraddrs.count; i++) {
				if(p2psecConnect(g_p2psec, new_peeraddrs.item[i].addr)) {
					utilByteArrayToHexstring(text, 4096, new_peeraddrs.item[i].addr, peeraddr_SIZE);
					printf("connecting to %s...\n", text);
				}
			}
		}
		else {
			printf("could not get peer address.\n");
		}
	}
	if(pa[0] == 'M' || pa[0] == 'm') {
		// MACTABLE
		printMacTable();
	}
	if(pa[0] == 'N' || pa[0] == 'n') {
		// NDPTABLE
		printNDPTable();
	}
	if(pa[0] == 'P' || pa[0] == 'p') {
		// PEERTABLE
		printActivePeerTable();
	}
	if(pa[0] == 'R' || pa[0] == 'r') {
		// RELAYEDINSERTPEER <relayid> <relaypeerid>
		sscanf(pb,"%d",&i);
		sscanf(pc,"%d",&j);
		if(peermgtIsActiveRemoteID(&g_p2psec->mgt, i)) {
			peeraddrSetIndirect(&new_peeraddr, i, g_p2psec->mgt.data[i].conntime, j);
			if(p2psecConnect(g_p2psec, new_peeraddr.addr)) {
				utilByteArrayToHexstring(text, 4096, new_peeraddr.addr, peeraddr_SIZE);
				printf("connecting to %s...\n", text);
			}
		}
		else {
			printf("could not get peer address.\n");
		}
	}
	if(pa[0] == 'Q' || pa[0] == 'q') {
		// QUIT
		g_mainloop = 0;
	}
}
