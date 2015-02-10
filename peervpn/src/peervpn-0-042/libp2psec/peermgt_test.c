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


#ifndef F_PEERMGT_TEST_C
#define F_PEERMGT_TEST_C


#include "peermgt.c"
#include "dfrag_test.c"
#include "authmgt_test.c"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#define peermgtTestsuite_NODECOUNT authmgtTestsuite_NODECOUNT
#define peermgtTestsuite_PEERSLOTS (peermgtTestsuite_NODECOUNT * 4)
#define peermgtTestsuite_AUTHSLOTS (peermgtTestsuite_NODECOUNT / 4)

#if peermgtTestsuite_NODECOUNT < 4
#error not enough nodes!
#endif


struct s_peermgt_test {
	struct s_authmgt_test authtest;
	struct s_peermgt peermgts[peermgtTestsuite_NODECOUNT];
};


static int peermgtTestsuiteGetID(const struct s_peeraddr *addr) {
	int id;
	if(addr->addr[0] == 42 && addr->addr[1] == 23 && addr->addr[2] == 66 && addr->addr[3] == 13) {
		id = utilReadInt32(&addr->addr[4]);
		if((!(id < 0)) && (id < peermgtTestsuite_NODECOUNT)) {
			return id;
		}
	}
	return -1;
}


static int peermgtTestsuiteGetAddr(struct s_peeraddr *addr, const int id) {
	if((!(id < 0)) && (id < peermgtTestsuite_NODECOUNT)) {
		memset(addr->addr, 0, peeraddr_SIZE);
		addr->addr[0] = 42;
		addr->addr[1] = 23;
		addr->addr[2] = 66;
		addr->addr[3] = 13;
		utilWriteInt32(&addr->addr[4], id);
		return 1;
	}
	return 0;
}


static int peermgtTestsuiteRun(struct s_peermgt_test *teststate) {
	unsigned char pbuf[4096];
	char text[4096];
	int len;
	int r;
	int i;
	int j;
	int l;
	struct s_peeraddr addr = {{0}};
	struct s_peeraddr sourceaddr = {{0}};
	
	/*
	const char *msg1_text = "hello world!";
	struct s_msg msg1 = { .msg = (unsigned char *)msg1_text, .len = strlen(msg1_text) };
	
	const char *msg2_text = "foobar!";
	struct s_msg msg2 = { .msg = (unsigned char *)msg2_text, .len = strlen(msg2_text) };
	
	const char *msg3_text = "hi all!";
	struct s_msg msg3 = { .msg = (unsigned char *)msg3_text, .len = strlen(msg3_text) };

	char *longmsg_text[3584];
	dfragTestsuiteText((unsigned char *)longmsg_text, 3584, 10, 1);
	struct s_msg longmsg1 = { .msg = (unsigned char *)longmsg_text, .len = 1024 };
	struct s_msg longmsg2 = { .msg = (unsigned char *)longmsg_text, .len = 2048 };
	struct s_msg longmsg3 = { .msg = (unsigned char *)longmsg_text, .len = 3072 };
	*/
	
	struct s_msg msgr;
	
	for(r=0; r<10000; r++) {
		// start some events
		switch(r) {
			case 3:
				// all nodes connect to node 0
				peermgtTestsuiteGetAddr(&addr, 0);
				for(i=0; i<peermgtTestsuite_NODECOUNT; i++) if(i != 0) {
					peermgtConnect(&teststate->peermgts[i], &addr);
				}
				break;
			/*
			case 17:
				// send message from node 0 to peerid 1
				peermgtSendUserdata(&teststate->peermgts[0], &msg1, NULL, 1);
				break;
			case 23:
				// send message from node 0 to peerid 1
				peermgtSendUserdata(&teststate->peermgts[0], &longmsg1, NULL, 1);
				break;
			case 27:
				// send message from node 0 to peerid 1
				peermgtSendUserdata(&teststate->peermgts[0], &longmsg2, NULL, 1);
				break;
			case 31:
				// send message from node 0 to peerid 1
				peermgtSendUserdata(&teststate->peermgts[0], &longmsg3, NULL, 1);
				break;
			case 42:
				// send message from node 0 to peerid 5
				peermgtSendUserdata(&teststate->peermgts[0], &msg2, NULL, 5);
				break;
			case 66:
				// send message from node 3 to peerid 8
				peermgtSendUserdata(&teststate->peermgts[3], &msg2, NULL, 8);
				break;
			case 69:
				// send message from node 3 to peerid 8
				peermgtSendUserdata(&teststate->peermgts[3], &longmsg3, NULL, 8);
				break;
			case 80:
				// send broadcast message
				peermgtSendBroadcastUserdata(&teststate->peermgts[0], &msg3);
				break;
			*/
			case 100:
				// reset node 3
				peermgtInit(&teststate->peermgts[3]);
				peermgtSetFastauth(&teststate->peermgts[3], 1);
				peermgtSetLoopback(&teststate->peermgts[3], 0);
				peermgtSetFragmentation(&teststate->peermgts[3], 1);
				peermgtSetNetID(&teststate->peermgts[3], "testnet", 7);
				break;
			case 103:
				// reconnect node 3
				peermgtTestsuiteGetAddr(&addr, 0);
				peermgtConnect(&teststate->peermgts[3], &addr);
			default:
				break;
		}
		
		// route the packets
		for(i=0; i<peermgtTestsuite_NODECOUNT; i++) {
			peermgtTestsuiteGetAddr(&sourceaddr, i);
			while((len = (peermgtGetNextPacket(&teststate->peermgts[i], pbuf, 4096, &addr))) > 0) {
				j = peermgtTestsuiteGetID(&addr);
				if(!(j < 0)) {
					if(peermgtDecodePacket(&teststate->peermgts[j], pbuf, len, &sourceaddr)) {
						if(peermgtRecvUserdata(&teststate->peermgts[j], &msgr, NULL, &l, NULL)) {
							memset(text, 0, 4096);
							if(msgr.len < 4095) memcpy(text, msgr.msg, msgr.len);
							printf("node %d received message \"%s\" from peerid %d\n", j, text, l);
						}
						if(peermgtRecvUserdata(&teststate->peermgts[j], &msgr, NULL, NULL, NULL)) return 0;
					}
				}
				else {
					return 0;
				}
			}
		}

		// print round info
		printf("*** round %05d *** [", r);
		for(i=0; i<peermgtTestsuite_NODECOUNT; i++) {
			printf("%02d ", mapGetKeyCount(&teststate->peermgts[i].map));
		}
		printf("]\n");
		if(r % 10 == 0) {
			for(i=0; i<peermgtTestsuite_NODECOUNT; i++) {
				printf("node %d:\n", i);
				peermgtStatus(&teststate->peermgts[i], text, 4096);
				printf("%s", text);
			}
		}
		printf("\n");
		
		// sleep
		sleep(1);
	}
	
	return 1;
}


static int peermgtTestsuitePrepare(struct s_peermgt_test *teststate) {
	int count;
	int ret = 0;
	if(authmgtTestsuiteCreateNodes(&teststate->authtest)) {
		count = 0;
		while(count < peermgtTestsuite_NODECOUNT) {
			if(!peermgtCreate(&teststate->peermgts[count], peermgtTestsuite_PEERSLOTS, peermgtTestsuite_AUTHSLOTS, &teststate->authtest.nk[count], &teststate->authtest.dhstate[count])) break;
			peermgtSetFastauth(&teststate->peermgts[count], 1);
			peermgtSetLoopback(&teststate->peermgts[count], 0);
			peermgtSetFragmentation(&teststate->peermgts[count], 1);
			peermgtSetNetID(&teststate->peermgts[count], "testnet", 7);
			count++;
		}
		if(!(count < peermgtTestsuite_NODECOUNT)) {
			ret = peermgtTestsuiteRun(teststate);
		}
		while(count > 0) {
			count --;
			peermgtDestroy(&teststate->peermgts[count]);
		}
		authmgtTestsuiteDestroyNodes(&teststate->authtest);
	}
	return ret;
}


static int peermgtTestsuite() {
	int ret;
	struct s_peermgt_test *teststate;
	teststate = malloc(sizeof(struct s_peermgt_test));
	if(teststate != NULL) {
		ret = peermgtTestsuitePrepare(teststate);
		free(teststate);
		return ret;
	}
	else {
		return 0;
	}
}


#endif // F_PEERMGT_TEST_C
