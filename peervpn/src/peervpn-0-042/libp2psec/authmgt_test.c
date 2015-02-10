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


#ifndef F_AUTHMGT_TEST_C
#define F_AUTHMGT_TEST_C


#include "authmgt.c"
#include <stdlib.h>
#include <stdio.h>


#define authmgtTestsuite_NODECOUNT 16
#define authmgtTestsuite_PUBKEYSIZE 2048


struct s_authmgt_test {
	struct s_nodekey nk[authmgtTestsuite_NODECOUNT];
	struct s_dh_state dhstate[authmgtTestsuite_NODECOUNT];
	struct s_authmgt mgt[authmgtTestsuite_NODECOUNT];
	struct s_nodeid evilnode;
	struct s_crypto cryptoctx;
	struct s_netid netid;
};


static int authmgtTestsuiteRun(struct s_authmgt_test *teststate) {
	int i;
	int j;
	int k;
	int r;
	int starttime;
	int elapsedtime;
	int counter;
	struct s_peeraddr target = {{0}};
	struct s_msg msg;
	struct s_nodeid nodeid;
	int peerid;
	target.addr[0] = 42;
	target.addr[1] = 42;
	target.addr[2] = 42;
	target.addr[3] = 42;
	
	printf("initalizing authmgts...\n");
	for(i=0; i<authmgtTestsuite_NODECOUNT; i++) {
		authmgtReset(&teststate->mgt[i]);
		authmgtSetFastauth(&teststate->mgt[i], 1);
	}
	
	printf("starting authentication...\n");
	for(i=0; i<authmgtTestsuite_NODECOUNT; i++) {
		for(j=0; j<authmgtTestsuite_NODECOUNT; j++) {
			if(i != j) {
				utilWriteInt32(&target.addr[4], j);
				if(!authmgtStart(&teststate->mgt[i], &target)) return 0;
			}
		}
	}
	
	printf("sending auth messages...\n");
	counter = 0;
	starttime = utilGetClock();
	for(r=0; r<(authmgtTestsuite_NODECOUNT * 6); r++) {
		for(i=0; i<authmgtTestsuite_NODECOUNT; i++) {
			if(authmgtGetNextMsg(&teststate->mgt[i], &msg, &target)) {
				j = utilReadInt32(&target.addr[4]);
				if(!(j >= 0 && j < authmgtTestsuite_NODECOUNT)) return 0;
				utilWriteInt32(&target.addr[4], i);
				if(authmgtDecodeMsg(&teststate->mgt[j], msg.msg, msg.len, &target)) {
					if(authmgtGetAuthedPeerNodeID(&teststate->mgt[j], &nodeid)) {
						if(memcmp(nodeid.id, teststate->evilnode.id, nodeid_SIZE) == 0) {
							authmgtRejectAuthedPeer(&teststate->mgt[j]);
						}
						else {
							authmgtAcceptAuthedPeer(&teststate->mgt[j], 23, 1337, 0);
						}
					}
					if(authmgtGetCompletedPeerNodeID(&teststate->mgt[j], &nodeid)) {
						if(!authmgtGetCompletedPeerAddress(&teststate->mgt[j], &peerid, &target)) return 0;
						if(!authmgtGetCompletedPeerSessionKeys(&teststate->mgt[j], &teststate->cryptoctx)) return 0;
						k = utilReadInt32(&target.addr[4]);
						if(!(k >= 0 && k < authmgtTestsuite_NODECOUNT)) return 0;
						authmgtFinishCompletedPeer(&teststate->mgt[j]);
						counter++;
					}
				}
			}
		}
	}
	elapsedtime = (utilGetClock() - starttime);
	printf("   %d authentications completed after %d seconds\n", counter, elapsedtime);
	k = ((authmgtTestsuite_NODECOUNT - 1) * (authmgtTestsuite_NODECOUNT - 2));
	if(counter != k) {
		printf("   warning: %d authentications were expected!\n", k);
	}

	return 1;
}


static int authmgtTestsuiteCreateNodes(struct s_authmgt_test *teststate) {
	int nkc = 0;
	int nkkc = 0;
	int dhc = 0;
	int mgtc = 0;
	if(cryptoCreate(&teststate->cryptoctx, 1)) {
		while(nkc < authmgtTestsuite_NODECOUNT) {
			if(!nodekeyCreate(&teststate->nk[nkc])) break;
			nkc++;
		}
		if(!(nkc < authmgtTestsuite_NODECOUNT)) {
			while(nkkc < authmgtTestsuite_NODECOUNT) {
				if(!nodekeyGenerate(&teststate->nk[nkkc], authmgtTestsuite_PUBKEYSIZE)) break;
				nkkc++;
			}
			if(!(nkkc < authmgtTestsuite_NODECOUNT)) {
				while(dhc < authmgtTestsuite_NODECOUNT) {
					if(!dhCreate(&teststate->dhstate[dhc])) break;
					dhc++;
				}
				if(!(dhc < authmgtTestsuite_NODECOUNT)) {
					while(mgtc < authmgtTestsuite_NODECOUNT) {
						if(!authmgtCreate(&teststate->mgt[mgtc], &teststate->netid, (authmgtTestsuite_NODECOUNT * 2), &teststate->nk[mgtc], &teststate->dhstate[mgtc])) break;
						mgtc++;
					}
					if(!(mgtc < authmgtTestsuite_NODECOUNT)) {
						teststate->evilnode = teststate->nk[0].nodeid;
						return 1;
					}
					while(mgtc > 0) {
						mgtc--;
						authmgtDestroy(&teststate->mgt[mgtc]);
					}
				}
				while(dhc > 0) {
					dhc--;
					dhDestroy(&teststate->dhstate[dhc]);
				}
			}
			while(nkkc > 0) {
				nkkc--;
			}
		}
		while(nkc > 0) {
			nkc--;
			nodekeyDestroy(&teststate->nk[nkc]);
		}
		cryptoDestroy(&teststate->cryptoctx, 1);
	}
	return 0;
}


static void authmgtTestsuiteDestroyNodes(struct s_authmgt_test *teststate) {
	int i;
	for(i=0; i<authmgtTestsuite_NODECOUNT; i++) {
		authmgtDestroy(&teststate->mgt[i]);
		dhDestroy(&teststate->dhstate[i]);
		nodekeyDestroy(&teststate->nk[i]);
	}
	cryptoDestroy(&teststate->cryptoctx, 1);
}


static int authmgtTestsuitePrepare(struct s_authmgt_test *teststate) {
	int ret;
	int i;
	if(netidSet(&teststate->netid, "test", 4)) {
		if(authmgtTestsuiteCreateNodes(teststate)) {
			ret = 1;
			i = 0;
			while((ret > 0) && (i < 10)) {
				ret = authmgtTestsuiteRun(teststate);
				i++;
			}
			authmgtTestsuiteDestroyNodes(teststate);
			return ret;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}


static int authmgtTestsuite() {
	int ret;
	struct s_authmgt_test *teststate;
	teststate = malloc(sizeof(struct s_authmgt_test));
	if(teststate != NULL) {
		ret = authmgtTestsuitePrepare(teststate);
		free(teststate);
		return ret;
	}
	else {
		return 0;
	}
}


#endif // F_AUTHMGT_TEST_C
