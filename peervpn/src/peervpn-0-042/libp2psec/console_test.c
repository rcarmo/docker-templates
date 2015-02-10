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


#ifndef F_CONSOLE_TEST_C
#define F_CONSOLE_TEST_C


#include "console.c"
#include "p2psec.c"
#include "ctr.c"
#include "peermgt_test.c"
#include "authmgt_test.c"
#include "mapstr_test.c"
#include "packet_test.c"
#include <stdio.h>
#include <unistd.h>


void consoleTestsuiteExit(struct s_console_args *args) {
	*(char *)args->arg[0] = 0;
}


void consoleTestsuiteEcho(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	int i;
	for(i=1; i<args->count; i++) {
		if(args->arg[i] != NULL) {
			consoleMsg(console, args->arg[i]);
			consoleMsg(console, " ");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteHexstr(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char hexcode[32];
	if(args->len[1] > 0) {
		consoleMsg(console, "string \"");
		consoleMsg(console, args->arg[1]);
		consoleMsg(console, "\" has hexstring \"");
		utilByteArrayToHexstring(hexcode, 32, args->arg[1], args->len[1]);
		consoleMsg(console, hexcode);
		consoleMsg(console, "\"");
	}
	else {
		consoleMsg(console, "error: no argument");
	}
	consoleNL(console);
}


void consoleTestsuiteHexint(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	int i;
	unsigned char c16[2];
	unsigned char c32[4];
	unsigned char c64[8];
	int64_t i64;
	int32_t i32;
	int16_t i16;
	char hexcode[32];
	if(args->len[1] > 0) {
		sscanf(args->arg[1], "%d", &i);
		utilWriteInt16(c16, i);
		utilWriteInt32(c32, i);
		utilWriteInt64(c64, i);
		i16 = utilReadInt16(c16);
		i32 = utilReadInt32(c32);
		i64 = utilReadInt64(c64);
		consoleMsg(console, "int=");
		snprintf(hexcode, 32, "%d", i);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", nat16=");
		utilByteArrayToHexstring(hexcode, 32, (unsigned char *)&i16, 2);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", nat32=");
		utilByteArrayToHexstring(hexcode, 32, (unsigned char *)&i32, 4);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", nat64=");
		utilByteArrayToHexstring(hexcode, 32, (unsigned char *)&i64, 8);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", big16=");
		utilByteArrayToHexstring(hexcode, 32, c16, 2);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", big32=");
		utilByteArrayToHexstring(hexcode, 32, c32, 4);
		consoleMsg(console, hexcode);
		consoleMsg(console, ", big64=");
		utilByteArrayToHexstring(hexcode, 32, c64, 8);
		consoleMsg(console, hexcode);
	}
	else {
		consoleMsg(console, "error: no argument");
	}
	consoleNL(console);
}


int consoleTestsuiteLoadNodeidHelper(struct s_nodekey *peerid, unsigned char *keybuf, const int keysize, const int mode) {
	if(mode == 1) {
		return nodekeyLoadPEM(peerid, keybuf, keysize);
	}
	else if(mode == 2) {
		return nodekeyLoadPrivatePEM(peerid, keybuf, keysize);
	}
	else {
		return 0;
	}
}


void consoleTestsuiteLoadNodeid(struct s_console *console, char *filename_priv, int mode) {
	struct s_nodeid peerid;
	struct s_nodekey peeridf;
	struct s_nodekey peeridf2;
	FILE *f_priv;
	unsigned char *buf_priv;
	int size_priv;
	const int hexsize = 3072;
	char hexcode[hexsize];
	unsigned char dercode[hexsize];
	int ksize;
	nodekeyCreate(&peeridf);
	nodekeyCreate(&peeridf2);
	if(filename_priv == NULL) {
		consoleMsg(console, "error: no filename for key specified");
	}
	else {	
			f_priv = fopen(filename_priv, "r");
			if(f_priv != NULL) {
				buf_priv = malloc(16384);
				size_priv = fread(buf_priv, 1, 16384, f_priv);
				if(size_priv > 0) {
					if(consoleTestsuiteLoadNodeidHelper(&peeridf, buf_priv, size_priv, mode)) {	
						consoleMsg(console, "peerid loaded!"); consoleNL(console);
						ksize = nodekeyGetDER(dercode, hexsize, &peeridf);
						if(!(ksize > 0)) { consoleMsg(console, "failed."); consoleNL(console); }
						snprintf(hexcode, hexsize, "%d", ksize);
						consoleMsg(console, "size=");
						consoleMsg(console, hexcode);
						consoleNL(console);
						utilByteArrayToHexstring(hexcode, hexsize, dercode, ksize);
						consoleMsg(console, "key=");
						consoleMsg(console, hexcode);
						consoleNL(console);
						peerid = peeridf.nodeid;
						utilByteArrayToHexstring(hexcode, hexsize, peerid.id, nodeid_SIZE);
						consoleMsg(console, "sha256=");
						consoleMsg(console, hexcode);
						consoleNL(console);
						if(!nodekeyLoadDER(&peeridf2, dercode, ksize)) { consoleMsg(console, "failed."); consoleNL(console); }
						utilByteArrayToHexstring(hexcode, hexsize, peeridf2.nodeid.id, nodeid_SIZE);
						consoleMsg(console, "sha256=");
						consoleMsg(console, hexcode);
					}
					else {
						consoleMsg(console, "error: key is invalid");
					}
				}
				else {
					consoleMsg(console, "error: key file is empty");
				}
				free(buf_priv);
				fclose(f_priv);
			}
			else {
				consoleMsg(console, "error: key file could not be opened");
			}
	}
	nodekeyDestroy(&peeridf);
	nodekeyDestroy(&peeridf2);
	consoleNL(console);
}


void consoleTestsuiteLoadPublicNodeid(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char *filename_priv = args->arg[1];
	consoleTestsuiteLoadNodeid(console, filename_priv, 1); 
}


void consoleTestsuiteLoadPrivateNodeid(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char *filename_priv = args->arg[1];
	consoleTestsuiteLoadNodeid(console, filename_priv, 2); 
}


void consoleTestsuiteLoad(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char *filename = args->arg[1];
	FILE *f;
	int bufsize = console->buffer_size;
	char buf[bufsize];
	int len;
	int promptstatus = consoleGetPromptStatus(console);
	consoleSetPromptStatus(console, 0);
	if(filename == NULL) {
		consoleMsg(console, "error: no filename specified");
	}
	else {
		f = fopen(filename, "r");
		if(f == NULL) {
			consoleMsg(console, "error: file could not be opened");
		}
		else {
			while((len = (fread(buf, 1, bufsize, f))) > 0) {
				consoleWrite(console, buf, len);
			}
			consoleNL(console);
			fclose(f);
		}
	}
	consoleSetPromptStatus(console, promptstatus);
	consoleNL(console);
}


void consoleTestsuiteCopyCmd(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char *target = args->arg[1];
	char *alias = args->arg[2];
	struct s_console_command *cmd;
	if(alias == NULL || target == NULL) {
		consoleMsg(console, "error: invalid number of arguments");
	}
	else {
		cmd = consoleGetCommand(console, target);
		if(cmd == NULL) {
			consoleMsg(console, "error: target \"");
			consoleMsg(console, target);
			consoleMsg(console, "\" does not exist");
		}
		else {
			if(consoleRegisterCommand(console, alias, cmd->function, cmd->fixed_args)) {
				consoleMsg(console, "command has been copied");
			}
			else {
				consoleMsg(console, "error: command could not be copied");
			}
		}
	}
	consoleNL(console);
}


void consoleTestsuiteUnregister(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	char *name = args->arg[1];
	if(name == NULL) {
		consoleMsg(console, "error: no function specified");
	}
	else {
		if(consoleUnregisterCommand(console, name)) {
			consoleMsg(console, "command \"");
			consoleMsg(console, name);
			consoleMsg(console, "\" has been removed");
		}
		else {
			consoleMsg(console, "command \"");
			consoleMsg(console, name);
			consoleMsg(console, "\" not found");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteMapTestsuite(struct s_console_args *args) {
	mapTestsuite();
}


void consoleTestsuitePacketTestsuite(struct s_console_args *args) {
	packetTestsuite();
}


void consoleTestsuiteTextgen(struct s_console_args *args) {
	char vstr[3584];
	struct s_console *console = args->arg[0];
	char *value = args->arg[1];
	char *base = args->arg[2];
	int n;
	int b;
	if(value != NULL) {
		if(base != NULL) {
			sscanf(value, "%d", &n);
			if(!(n < 0)) {
				if((n + 1) < 3584) {
					sscanf(base, "%d", &b);
					if(!(b < 0)) {
						dfragTestsuiteText((unsigned char *)vstr, (n + 1), b, 0);
						consoleMsg(console, vstr);
					}
					else {
						consoleMsg(console, "error: base must be a positive integer");
					}
				}
				else {
					consoleMsg(console, "error: number too big");
				}
			}
			else {
				consoleMsg(console, "error: number must be a positive integer");
			}
		}
		else {
			consoleMsg(console, "error: no base specified");
		}
	}
	else {
		consoleMsg(console, "error: no value specified");
	}
	consoleNL(console);
}


void consoleTestsuiteSeqInit(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_seq_state *state = args->arg[1];
	char *value = args->arg[2];
	char vstr[128];
	long long n;
	if(value == NULL) {
		consoleMsg(console, "error: no value specified");
	}
	else {
		sscanf(value, "%lld", &n);
		sprintf(vstr, "%lld", n);
		seqInit(state, n);
		consoleMsg(console, "sequence number initialized to ");
		consoleMsg(console, vstr);
	}
	consoleNL(console);
}


void consoleTestsuiteSeqVerify(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_seq_state *state = args->arg[1];
	char *value = args->arg[2];
	char vstr[128];
	long long n;
	if(value == NULL) {
		consoleMsg(console, "error: no value specified");
	}
	else {
		sscanf(value, "%lld", &n);
		sprintf(vstr, "%lld", n);
		consoleMsg(console, "sequence number ");
		consoleMsg(console, vstr);
		if(seqVerify(state, n)) {
			consoleMsg(console, " accepted");
		}
		else {
			consoleMsg(console, " rejected");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteTreeview(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	const int treeview_size = 8192;
	char treeview[treeview_size];
	int len = mapTestsuiteGenerateASCIIString(map, treeview, treeview_size);
	consoleNL(console);
	consoleMsgN(console, treeview, len);
	consoleNL(console);
}


void consoleTestsuiteGetpf(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	char *key = args->arg[2];
	if(key == NULL) {
		consoleMsg(console, "error: no key specified");
	}
	else {
		char *str = mapStrGetN(map, key);
		consoleMsg(console, "\"");
		consoleMsg(console, key);
		consoleMsg(console, "\" = ");
		if(str == NULL) {
			consoleMsg(console, "(empty)");
		}
		else {
			consoleMsg(console, "\"");
			consoleMsg(console, str);
			consoleMsg(console, "\"");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteGet(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	char *key = args->arg[2];
	if(key == NULL) {
		consoleMsg(console, "error: no key specified");
	}
	else {
		char *str = mapStrGet(map, key);
		consoleMsg(console, "\"");
		consoleMsg(console, key);
		consoleMsg(console, "\" = ");
		if(str == NULL) {
			consoleMsg(console, "(empty)");
		}
		else {
			consoleMsg(console, "\"");
			consoleMsg(console, str);
			consoleMsg(console, "\"");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteGetold(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	
	char str[64];
	int i = mapGetOldKeyID(map);
	snprintf(str, 32, "%d", i);
	consoleMsg(console, str);
	consoleNL(console);
}


void consoleTestsuiteSet(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	char *key = args->arg[2];
	char *value = args->arg[3];
	if(key == NULL) {
		consoleMsg(console, "error: no key specified");
	}
	else if(value == NULL) {
		consoleMsg(console, "error: no value specified");
	}
	else {
		if(mapStrSet(map, key, value)) {
			consoleMsg(console, "\"");
			consoleMsg(console, key);
			consoleMsg(console, "\" <= \"");
			consoleMsg(console, value);
			consoleMsg(console, "\"");
		}
		else {
			consoleMsg(console, "error: operation failed");
		}
	}
	consoleNL(console);
}


void consoleTestsuiteUnset(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	char *key = args->arg[2];
	if(key == NULL) {
		consoleMsg(console, "error: no key specified");
	}
	else {
		mapStrRemove(map, key);
		consoleMsg(console, "\"");
		consoleMsg(console, key);
		consoleMsg(console, "\" <= (empty)");
	}
	consoleNL(console);
}


void consoleTestsuiteIdspNext(struct s_console_args *args) {
	char s[1024];
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	struct s_idsp *idsp = &map->idsp;
	int n;
	n = idspNext(idsp);
	snprintf(s, 1024, "%d", n);
	consoleMsg(console, s);
	consoleNL(console);
}


void consoleTestsuiteIdspNextN(struct s_console_args *args) {
	char s[1024];
	struct s_console *console = args->arg[0];
	struct s_map *map = args->arg[1];
	char *value = args->arg[2];
	struct s_idsp *idsp = &map->idsp;
	int n,m;
	sscanf(value, "%d", &m);
	n = idspNextN(idsp, m);
	snprintf(s, 1024, "%d", n);
	consoleMsg(console, s);
	consoleNL(console);
}


void consoleTestsuiteKeygen(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_dh_state dhstate;
	
	struct s_nodeid peerid;
	struct s_nodekey peeridf;
	struct s_nodekey peeridf2;
	
	const int hexsize = 3072;
	char hexcode[hexsize];
	unsigned char dercode[hexsize];
	int ksize;
	
	if(nodekeyCreate(&peeridf)) {
		if(nodekeyCreate(&peeridf2)) {
			if(nodekeyGenerate(&peeridf,2048)) {
				consoleMsg(console, "nodekey generated.");
				consoleNL(console);
				ksize = nodekeyGetDER(dercode, hexsize, &peeridf);
				if(!(ksize > 0)) { consoleMsg(console, "failed."); consoleNL(console); }
				snprintf(hexcode, hexsize, "%d", ksize);
				consoleMsg(console, "size=");
				consoleMsg(console, hexcode);
				consoleNL(console);
				utilByteArrayToHexstring(hexcode, hexsize, dercode, ksize);
				consoleMsg(console, "key=");
				consoleMsg(console, hexcode);
				consoleNL(console);
				peerid = peeridf.nodeid;
				utilByteArrayToHexstring(hexcode, hexsize, peerid.id, nodeid_SIZE);
				consoleMsg(console, "sha256=");
				consoleMsg(console, hexcode);
				consoleNL(console);
				if(!nodekeyLoadDER(&peeridf2, dercode, ksize)) { consoleMsg(console, "failed."); consoleNL(console); }
				peerid = peeridf2.nodeid;
				utilByteArrayToHexstring(hexcode, hexsize, peerid.id, nodeid_SIZE);
				consoleMsg(console, "sha256=");
				consoleMsg(console, hexcode);
				consoleNL(console);
			}
			else {
				consoleMsg(console, "peeridGenerate failed.");
			}
			nodekeyDestroy(&peeridf2);
		}
		else {
			consoleMsg(console, "peeridInit failed.");
		}
		nodekeyDestroy(&peeridf);
	}
	else {
		consoleMsg(console, "peeridInit failed.");
	}
	
	if(dhCreate(&dhstate)) {
		dhGenKey(&dhstate);
		consoleMsg(console, "dh pubkey generated.");
		consoleNL(console);
		ksize = dhGetPubkey(dercode, hexsize, &dhstate);
		if(!(ksize > 0)) { consoleMsg(console, "failed."); consoleNL(console); }
		snprintf(hexcode, hexsize, "%d", ksize);
		consoleMsg(console, "size=");
		consoleMsg(console, hexcode);
		consoleNL(console);
		utilByteArrayToHexstring(hexcode, hexsize, dercode, ksize);
		consoleMsg(console, "key=");
		consoleMsg(console, hexcode);
		consoleNL(console);
		dhDestroy(&dhstate);
	}
	else {
		consoleMsg(console, "dhInit failed.");
	}
	
	consoleNL(console);
}


void consoleTestsuiteMassKeygen(struct s_console_args *args) {
	struct s_console *console = args->arg[0];	
	char *loopcount_str = args->arg[1];
	struct s_nodekey nodekey_a;
	struct s_nodekey nodekey_b;
	
	const int hexsize = 1024;
	char hexcode[hexsize];
	unsigned char dercode[rsa_MAXSIZE];
	int ksize;
	int loopcount;
	int i;
	
	if(loopcount_str != NULL) {
		sscanf(loopcount_str, "%d", &loopcount);
		if(nodekeyCreate(&nodekey_a)) {
			if(nodekeyCreate(&nodekey_b)) {
				for(i=0; i<loopcount; i++) {
					if(nodekeyGenerate(&nodekey_a,1024)) {
						ksize = nodekeyGetDER(dercode, rsa_MAXSIZE, &nodekey_a);
						if(!(ksize > 0)) { 
							consoleMsg(console, "nodekeyGetDER failed.");
							break;
						}
						if(!nodekeyLoadDER(&nodekey_b, dercode, ksize)) { 
							consoleMsg(console, "nodekeyLoadDER failed.");
							break;
						}				
						if(memcmp(nodekey_a.nodeid.id, nodekey_b.nodeid.id, nodeid_SIZE) != 0) {
							consoleMsg(console, "failed.");
							break;
						}
						utilByteArrayToHexstring(hexcode, hexsize, nodekey_a.nodeid.id, nodeid_SIZE);
						consoleMsg(console, "sha256=");
						consoleMsg(console, hexcode);
						consoleNL(console);
					}
					else {
						consoleMsg(console, "nodekeyGenerate failed.");
						break;
					}
				}
				nodekeyDestroy(&nodekey_b);
			}
			else {
				consoleMsg(console, "nodekeyCreate failed.");
			}
			nodekeyDestroy(&nodekey_a);
		}
		else {
			consoleMsg(console, "nodekeyCreate failed.");
		}
	}
	else {
		consoleMsg(console, "error: please specify number of keys to generate!");
	}
	consoleNL(console);
}


void consoleTestsuiteAuthtestZ(struct s_console *console, struct s_auth_state *user_a) {
	struct s_nodeid nid_local = {{0}};
	struct s_nodeid nid_remote = {{0}};
	int pid;
	
	char text[2048];
	
	if(authIsCompleted(user_a)) {
		consoleMsg(console, "authentication successful!"); consoleNL(console);
		consoleMsg(console, "  authid_local  = ");
		pid = utilReadInt32(user_a->local_authid);
		snprintf(text, 2048, "%d", pid);
		consoleMsg(console, text); consoleNL(console);
		consoleMsg(console, "  authid_remote = ");
		pid = utilReadInt32(user_a->remote_authid);
		snprintf(text, 2048, "%d", pid);
		consoleMsg(console, text); consoleNL(console);
		consoleMsg(console, "  peerid_local  = ");
		pid = user_a->local_peerid;
		snprintf(text, 2048, "%d", pid);
		consoleMsg(console, text); consoleNL(console);
		consoleMsg(console, "  peerid_remote = ");
		authGetRemotePeerID(user_a, &pid);
		snprintf(text, 2048, "%d", pid);
		consoleMsg(console, text); consoleNL(console);
		consoleMsg(console, "  nodeid_local  = ");
		nid_local = user_a->local_nodekey->nodeid;
		utilByteArrayToHexstring(text, 2048, nid_local.id, nodeid_SIZE);
		consoleMsg(console, text); consoleNL(console);
		consoleMsg(console, "  nodeid_remote = ");
		authGetRemoteNodeID(user_a, &nid_remote);
		utilByteArrayToHexstring(text, 2048, nid_remote.id, nodeid_SIZE);
		consoleMsg(console, text); consoleNL(console);
	}
	else {
		consoleMsg(console, "authentication failed!");
		consoleNL(console);
	}
}


void consoleTestsuiteAuthtestY(struct s_console *console, struct s_auth_state *user_a, struct s_auth_state *user_b, const int dir) {
	char state_a[4];
	char state_b[4];
	char state_msg[4];
	int state_msgi;
	if(dir < 0) {
		authDecodeMsg(user_a, user_b->nextmsg, user_b->nextmsg_size);
		state_msgi = utilReadInt16(&user_b->nextmsg[4]); snprintf(state_msg, 4, "%d", state_msgi);
		consoleMsg(console, "  a <- ("); consoleMsg(console, state_msg); consoleMsg(console,") <- b:    ");
	}
	else if(dir > 0) {
		authDecodeMsg(user_b, user_a->nextmsg, user_a->nextmsg_size);
		state_msgi = utilReadInt16(&user_a->nextmsg[4]); snprintf(state_msg, 4, "%d", state_msgi);
		consoleMsg(console, "  a -> ("); consoleMsg(console, state_msg); consoleMsg(console,") -> b:    ");
	}
	else {
		consoleMsg(console, "  no data:        ");
	}
	snprintf(state_a, 4, "%d", user_a->state); consoleMsg(console, "a="); consoleMsg(console, state_a); consoleMsg(console,", ");
	snprintf(state_b, 4, "%d", user_b->state); consoleMsg(console, "b="); consoleMsg(console, state_b); consoleNL(console);
}


void consoleTestsuiteAuthtestX(struct s_console *console, struct s_auth_state *user_a, struct s_auth_state *user_b) {
	consoleMsg(console, "sending messages:"); consoleNL(console);
	authStart(user_a);
	authReset(user_b);	
	consoleTestsuiteAuthtestY(console, user_a, user_b,  0);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleMsg(console, "  insert fault"); consoleNL(console);
	user_b->nextmsg[23]++;
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	user_b->nextmsg[23]--;
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleMsg(console, "  send data of user a"); consoleNL(console);
	authSetLocalData(user_a, 7, 2305, 999);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleMsg(console, "  send data of user b"); consoleNL(console);
	authSetLocalData(user_b, 13, 1337, 777);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
	consoleTestsuiteAuthtestY(console, user_a, user_b, -1);
	consoleTestsuiteAuthtestY(console, user_a, user_b,  1);
}


void consoleTestsuiteAuthtest(struct s_console_args *args) {
	struct s_console *console = args->arg[0];

	struct s_dh_state dh_user_a;
	struct s_dh_state dh_user_b;
	struct s_nodekey nk_user_a;
	struct s_nodekey nk_user_b;
	struct s_auth_state user_a;
	struct s_auth_state user_b;
	struct s_netid netid_a;
	struct s_netid netid_b;
	
	const char *netid_str_a = "test";
	const char *netid_str_b = "test";

	if(netidSet(&netid_a, netid_str_a, strlen(netid_str_a)) && netidSet(&netid_b, netid_str_b, strlen(netid_str_b))) {
		if(dhCreate(&dh_user_a)) {
			if(dhCreate(&dh_user_b)) {
				if(nodekeyCreate(&nk_user_a)) {
					if(nodekeyCreate(&nk_user_b)) {
						if(nodekeyGenerate(&nk_user_a, 1024)) {
							if(nodekeyGenerate(&nk_user_b, 1024)) {
								if(authCreate(&user_a, &netid_a, &nk_user_a, &dh_user_a, 5)) {
									if(authCreate(&user_b, &netid_b, &nk_user_b, &dh_user_b, 23)) {
										consoleMsg(console, "auth state structures loaded."); consoleNL(console);
										consoleTestsuiteAuthtestX(console, &user_a, &user_b);
										consoleMsg(console, "user a: "); consoleTestsuiteAuthtestZ(console, &user_a);
										consoleMsg(console, "user b: "); consoleTestsuiteAuthtestZ(console, &user_b);
										authDestroy(&user_b);
									}
									else {
										consoleMsg(console, "authCreate failed");
									}
									authDestroy(&user_a);
								}
								else {
									consoleMsg(console, "authCreate failed");
								}
							}
							else {
								consoleMsg(console, "nodekeyGenerate failed");
							}
						}
						else {
							consoleMsg(console, "nodekeyGenerate failed");
						}
						nodekeyDestroy(&nk_user_b);
					}
					else {
						consoleMsg(console, "nodekeyCreate failed");
					}
					nodekeyDestroy(&nk_user_a);
				}
				else {
					consoleMsg(console, "nodekeyCreate failed");
				}
				dhDestroy(&dh_user_b);
			}
			else {
				consoleMsg(console, "dhCreate failed");
			}
			dhDestroy(&dh_user_a);
		}
		else {
			consoleMsg(console, "dhCreate failed");
		}
	}
	else {
		consoleMsg(console, "netidSet failed");
	}
	consoleNL(console);
}


void consoleTestsuiteAuthTestsuite(struct s_console_args *args) {
	authmgtTestsuite();
}


void consoleTestsuitePeerTestsuite(struct s_console_args *args) {
	peermgtTestsuite();
}


void consoleTestsuiteDfragTestsuite(struct s_console_args *args) {
	dfragTestsuite();
}


void consoleTestsuiteEndian(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	if(utilIsLittleEndian()) {
		consoleMsg(console, "little endian.");
	}
	else {
		consoleMsg(console, "big endian.");
	}
	consoleNL(console);
}


void consoleTestsuiteCtrInc(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_ctr_state *ctr = args->arg[1];
	ctrIncr(ctr, 1);
	consoleNL(console);
}


void consoleTestsuiteCtrShow(struct s_console_args *args) {
	struct s_console *console = args->arg[0];
	struct s_ctr_state *ctr = args->arg[1];
	char text[256];
	ctrIncr(ctr, 0);
	snprintf(text, 256, "[total: %d] [avg: %d] [cur: %d] [last: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d] [pos: %d]\n", ctr->total, ctrAvg(ctr), ctr->cur, ctr->last[0], ctr->last[1], ctr->last[2], ctr->last[3], ctr->last[4], ctr->last[5], ctr->last[6], ctr->last[7], ctr->last[8], ctr->last[9], ctr->last[10], ctr->last[11], ctr->last[12], ctr->last[13], ctr->last[14], ctr->last[15], ctr->lastpos);
	consoleMsg(console, text);
	consoleNL(console);
}


int consoleTestsuite() {
	const int cl_bufsize = 4096;
	const int rw_bufsize = 16;
	const int teststr_size = 64;
	struct s_console console;
	struct s_map testmap;
	struct s_seq_state seqstate;
	struct s_ctr_state testctr;
	char buf[rw_bufsize];
	int run = 1;
	int len;

	ctrInit(&testctr);

	mapCreate(&testmap, 32, teststr_size, teststr_size);
	mapEnableReplaceOld(&testmap);

	consoleCreate(&console, 32, 512, cl_bufsize);
	consoleSetPrompt(&console, "test-console $ ");
	consoleSetPromptStatus(&console, 1);
	consoleWrite(&console, "\r\n", 2);

	consoleRegisterCommand(&console, "exit", &consoleTestsuiteExit, consoleArgs1(&run));
	consoleRegisterCommand(&console, "echo", &consoleTestsuiteEcho, consoleArgs9(&console, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
	consoleRegisterCommand(&console, "hexstr", &consoleTestsuiteHexstr, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "hexint", &consoleTestsuiteHexint, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "loadpublicnodeid", &consoleTestsuiteLoadPublicNodeid, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "loadprivatenodeid", &consoleTestsuiteLoadPrivateNodeid, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "load", &consoleTestsuiteLoad, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "copycmd", &consoleTestsuiteCopyCmd, consoleArgs3(&console, NULL, NULL));
	consoleRegisterCommand(&console, "unregister", &consoleTestsuiteUnregister, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "maptestsuite", &consoleTestsuiteMapTestsuite, consoleArgs0());
	consoleRegisterCommand(&console, "packettestsuite", &consoleTestsuitePacketTestsuite, consoleArgs0());
	consoleRegisterCommand(&console, "seqinit", &consoleTestsuiteSeqInit, consoleArgs3(&console, &seqstate, NULL));
	consoleRegisterCommand(&console, "seqverify", &consoleTestsuiteSeqVerify, consoleArgs3(&console, &seqstate, NULL));
	consoleRegisterCommand(&console, "treeview", &consoleTestsuiteTreeview, consoleArgs2(&console, &testmap));
	consoleRegisterCommand(&console, "get", &consoleTestsuiteGet, consoleArgs3(&console, &testmap, NULL));
	consoleRegisterCommand(&console, "getpf", &consoleTestsuiteGetpf, consoleArgs3(&console, &testmap, NULL));
	consoleRegisterCommand(&console, "getold", &consoleTestsuiteGetold, consoleArgs2(&console, &testmap));
	consoleRegisterCommand(&console, "set", &consoleTestsuiteSet, consoleArgs4(&console, &testmap, NULL, NULL));
	consoleRegisterCommand(&console, "unset", &consoleTestsuiteUnset, consoleArgs3(&console, &testmap, NULL));
	consoleRegisterCommand(&console, "idspnext", &consoleTestsuiteIdspNext, consoleArgs2(&console, &testmap));
	consoleRegisterCommand(&console, "idspnextn", &consoleTestsuiteIdspNextN, consoleArgs3(&console, &testmap, NULL));
	consoleRegisterCommand(&console, "keygen", &consoleTestsuiteKeygen, consoleArgs1(&console));
	consoleRegisterCommand(&console, "masskeygen", &consoleTestsuiteMassKeygen, consoleArgs2(&console, NULL));
	consoleRegisterCommand(&console, "authtest", &consoleTestsuiteAuthtest, consoleArgs1(&console));
	consoleRegisterCommand(&console, "authtestsuite", &consoleTestsuiteAuthTestsuite, consoleArgs0());
	consoleRegisterCommand(&console, "peermgttest", &consoleTestsuitePeerTestsuite, consoleArgs0());
	consoleRegisterCommand(&console, "dfragtest", &consoleTestsuiteDfragTestsuite, consoleArgs0());
	consoleRegisterCommand(&console, "textgen", &consoleTestsuiteTextgen, consoleArgs3(&console, NULL, NULL));
	consoleRegisterCommand(&console, "endian", &consoleTestsuiteEndian, consoleArgs1(&console));
	consoleRegisterCommand(&console, "ctrinc", &consoleTestsuiteCtrInc, consoleArgs2(&console, &testctr));
	consoleRegisterCommand(&console, "ctrshow", &consoleTestsuiteCtrShow, consoleArgs2(&console, &testctr));
	
	while(run) {
		while((len = consoleRead(&console, buf, rw_bufsize)) > 0) {
			if(!(fwrite(buf, 1, len, stdout) > 0)) return 0;
		}
		if((len = fread(buf, 1, 1, stdin)) > 0) {
			consoleWrite(&console, buf, len);
		}
	}
	
	consoleDestroy(&console);
	
	mapDestroy(&testmap);
	
	return 1;
}


#endif // F_CONSOLE_TEST_C
