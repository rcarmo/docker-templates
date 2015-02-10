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


#ifndef F_MAPSTR_TEST_C
#define F_MAPSTR_TEST_C


#include "mapstr.c"
#include <stdlib.h>
#include <stdio.h>


static int mapTestsuiteGenerateASCIIStringCalcXY(int x, int y) {
	return ((1 << y) + x - 1);
}


static int mapTestsuiteGenerateASCIIStringRecursive(struct s_map *map, int *nidarray, const int nodeid, const int level, const int x) {
	int x2 = x*2;
	int nextlevel = level + 1;
	int maxlevel = level;
	int retlevel;
	if(level > 10) return 0; // stop if this gets too big
	if(!(nodeid < 0)) {
		nidarray[mapTestsuiteGenerateASCIIStringCalcXY(x,level)] = nodeid;
		retlevel = mapTestsuiteGenerateASCIIStringRecursive(map, nidarray, map->left[nodeid], nextlevel, x2); if(maxlevel < retlevel) maxlevel = retlevel;
		retlevel = mapTestsuiteGenerateASCIIStringRecursive(map, nidarray, map->right[nodeid], nextlevel, x2+1); if(maxlevel < retlevel) maxlevel = retlevel;
		return maxlevel;
	}
	return maxlevel;
}


static int mapTestsuiteGenerateASCIIString(struct s_map *map, char *str, const int strlen) {
	int size = strlen - 5;
	char c; int i; int j; int k; int l; int m; int n; int o; int p; int h; int x;
	int q = 8192;
	int *nidarray = NULL;
	int cpos = 0;
	if(mapGetKeyCount(map) > 0) {
		nidarray = malloc(q * sizeof(int));
		for(i=0; i<q; i++) nidarray[i]=-1;
		if(nidarray == NULL) return 0;
		h = mapTestsuiteGenerateASCIIStringRecursive(map, nidarray, map->rootid, 0, 0);
		j = 0;
		l = 0;		
		while(j < h) {
			k = 1 << (h-j);
			if(l) {
				m = k >> 1;
				o = 1;
				while(o < m) {
					i = 1 << j;
					x = 0;
					q = nidarray[mapTestsuiteGenerateASCIIStringCalcXY(x,j)];
					n = 0;
					p = 0;
					c = '\0';
					for(;;) {
						c = ' ';
						if(((n + o) % k == 0)) { c = '/'; p = !p; }
						if(((n - o) % k == 0) && (n > o)) c = '\\';
						if(p && (!(q < 0))) str[cpos] = c; else str[cpos] = ' ';
						cpos++; if(!(cpos < size)) return 0;
						if((c == '\\') & p) { x++; q = nidarray[mapTestsuiteGenerateASCIIStringCalcXY(x,j)]; }
						if(!(x < i)) break;
						n++;
					}
					str[cpos] = '\n';
					cpos++; if(!(cpos < size)) return 0;
					o++;
				}
			}
			else {
				x = 0;
				i = 1 << j;
				n = k >> 1;
				while(x < i) {
					while(n < k) {
						snprintf(&str[cpos], 3, "  ");
						cpos = cpos + 2; if(!(cpos < size)) return 0;
						n++;
					}
					n = nidarray[mapTestsuiteGenerateASCIIStringCalcXY(x,j)];
					if(n < 0) {
						snprintf(&str[cpos], 3, "  ");
					}
					else {
						snprintf(&str[cpos-2], 5, "%4d", n);
					}
					cpos = cpos + 2; if(!(cpos < size)) return 0;
					n = 1;
					x++;
				}
				str[cpos] = '\n';
				cpos++; if(!(cpos < size)) return 0;
			}
			if(l) j++;
			l = !l;
		}
		free(nidarray);
	}
	str[cpos] = '\0'; cpos++; if(!(cpos < size)) return 0;
	return cpos;
}


static int mapTestsuiteIntegrityCheckRecursive(struct s_map *map, const int nodeid, int *ctr) {
	int lc;
	int rc;
	char *key;
	if(!(nodeid < 0)) {
		key = mapGetKeyByID(map, nodeid);
		lc = map->left[nodeid];
		rc = map->right[nodeid];
		if(!(lc < 0)) if(!((mapTestsuiteIntegrityCheckRecursive(map, lc, ctr)) && (mapCompareKeysExt(map, lc, key) < 0))) return 0;
		if(!(rc < 0)) if(!((mapTestsuiteIntegrityCheckRecursive(map, rc, ctr)) && (mapCompareKeysExt(map, rc, key) > 0))) return 0;
		if(mapGetOldKeyID(map) < 0) return 0;
		*ctr = *ctr + 1;
	}
	return 1;
}


static int mapTestsuiteIntegrityCheck(struct s_map *map) {
	int ctr = 0;
	int ret = mapTestsuiteIntegrityCheckRecursive(map, map->rootid, &ctr);
	if(!ret) return 0;
	if(ctr != mapGetKeyCount(map)) return 0;
	return 1;
}


static int mapTestsuiteRWDAdd(struct s_map *map, unsigned char *keys, unsigned char *values, int *states, int *keyids, int nr) {
	int key_size = mapGetKeySize(map);
	int value_size = mapGetValueSize(map);
	int toobig = (!(mapGetKeyCount(map) < mapGetMapSize(map)));
	int ret = mapAddReturnID(map, &keys[nr*key_size], &values[nr*value_size]);
	if(ret < 0) {
		if(states[nr] || toobig) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		if(states[nr] || toobig) {
			return 0;
		}
		else {
			states[nr] = 1;
			keyids[nr] = ret;
			return 1;
		}
	}
}


static int mapTestsuiteRWDSet(struct s_map *map, unsigned char *keys, unsigned char *values, int *states, int *keyids, int nr) {
	int key_size = mapGetKeySize(map);
	int value_size = mapGetValueSize(map);
	int toobig = (!(mapGetKeyCount(map) < mapGetMapSize(map)));
	char tval[value_size];
	memcpy(tval, &values[nr*value_size], value_size);
	tval[0]++;
	int ret = mapSetReturnID(map, &keys[nr*key_size], tval);
	if(ret < 0) {
		if(states[nr]) {
			return 0;
		}
		else {
			if(toobig) {
				return 1;
			}
			else {
				return 0;
			}
		}
	}
	else {
		if(!states[nr] && toobig) {
			return 0;
		}
		else {
			if(memcmp(mapGetValueByID(map, ret), tval, value_size) == 0) {
				if(states[nr]) {
					if(keyids[nr] != ret) return 0;
				}
				states[nr] = 1;
				keyids[nr] = ret;
				memcpy(&values[nr*value_size], tval, value_size);
				return 1;
			}
			else {
				return 0;
			}
		}
	}
}


static int mapTestsuiteRWDRem(struct s_map *map, unsigned char *keys, int *states, int *keyids, int nr) {
	int key_size = mapGetKeySize(map);
	int ret = mapRemoveReturnID(map, &keys[nr*key_size]);
	if(ret < 0) {
		if(states[nr]) {
			return 0;
		}
		else {
			return 1;
		}
	}
	else {
		if(keyids[nr] == ret) {
			if(states[nr]) {
				states[nr] = 0;
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
}


static int mapTestsuiteRWDGet(struct s_map *map, unsigned char *keys, unsigned char *values, int *states, int *keyids, int nr) {
	int key_size = mapGetKeySize(map);
	int value_size = mapGetValueSize(map);
	int ret = mapGetKeyID(map, &keys[nr*key_size]);
	char *vptr;
	if(ret < 0) {
		if(states[nr]) {
			return 0;
		}
		else {
			return 1;
		}
	}
	else {
		if(states[nr]) {
			if(ret == keyids[nr]) {
				vptr = mapGetValueByID(map, ret);
				if(vptr == NULL) return 0;
				if(memcmp(&values[nr*value_size], vptr, value_size) == 0) {
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
		else {
			return 0;
		}
	}
}


static int mapTestsuiteRWDPrefixCheck(struct s_map *map, unsigned char *keys, int *states, int nr) {
	int key_size = mapGetKeySize(map);
	unsigned char *kptr = &keys[nr*key_size];
	unsigned char *sptr;
	int i, j;
	if(states[nr]) {
		for(i=0; i<=key_size; i++) {
			j = mapGetPrefixID(map, kptr, i);
			if(j < 0) return 0;
			if(memcmp(kptr, mapGetKeyByID(map, j), i) != 0) return 0;
		}
		return 1;
	}
	else {
		sptr = mapGet(map, kptr);
		if(sptr == NULL) {
			sptr = mapGetN(map, kptr, key_size);
			if(sptr == NULL) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			return 1;
		}
	}
}


static void mapTestsuiteRWDInit(struct s_map *map, int *states) {
	int j;
	for(j=0; j<mapGetMapSize(map); j++) states[j] = 0;
	mapInit(map);
}


static int mapTestsuiteRWD(const int iterations, const int map_size, const int key_size, const int value_size) {
	struct s_map *mymaparray;
	struct s_map *mymap;
	unsigned char *keys;
	unsigned char *values;
	int *keyids;
	int *states;
	int i,j;

	// create data structures
	if((keys = malloc(map_size * key_size)) == NULL) return 0;
	if((values = malloc(map_size * value_size)) == NULL) return 0;
	if((states = malloc(map_size * sizeof(int))) == NULL) return 0;
	if((keyids = malloc(map_size * sizeof(int))) == NULL) return 0;
	if((mymaparray = malloc(3 * sizeof(struct s_map))) == NULL) return 0;
	for(i=0; i<map_size; i++) states[i] = 0;
	memset(&mymaparray[2], 0, sizeof(struct s_map));
	memset(&mymaparray[1], 0, sizeof(struct s_map));
	mymap = &mymaparray[0];

	if(!mapCreate(mymap, map_size, key_size, value_size)) return 0;
	/*
	j = mapMemSize(map_size, key_size, value_size);
	mymap = malloc(j);
	if(!(mapMemInit(mymap, j, map_size, key_size, value_size) > 0)) return 0;
	*/

	// generate test data
	mapTestsuiteRWDInit(mymap, states);
	j = 0;
	while(mapGetKeyCount(mymap) < map_size) {
		for(i=0; i<key_size; i++) keys[(j*key_size+i)] = rand();
		for(i=0; i<value_size; i++) values[(j*value_size+i)] = rand();
		i = mapGetKeyCount(mymap);
		mapTestsuiteRWDAdd(mymap,keys,values,states,keyids,j);
		if(i != mapGetKeyCount(mymap)) j++;
	}

	// do the test
	mapTestsuiteRWDInit(mymap, states);
	for(i=0; i<iterations; i++) {
		if((abs(rand()) % 16384) < 1) {
			if(!mapTestsuiteIntegrityCheck(mymap)) return 0;
		}
		if((abs(rand()) % 16384) < 2) {
			for(j=0; j<map_size; j++) if(!mapTestsuiteRWDRem(mymap,keys,states,keyids,j)) return 0;
			if(mapGetKeyCount(mymap) != 0) return 0;
			for(j=0; j<map_size; j++) if(!mapTestsuiteRWDRem(mymap,keys,states,keyids,j)) return 0;
		}
		if((abs(rand()) % 16384) < 2) {
			for(j=0; j<map_size; j++) if(!mapTestsuiteRWDAdd(mymap,keys,values,states,keyids,j)) return 0;
			if(mapGetKeyCount(mymap) != map_size) return 0;
			for(j=0; j<map_size; j++) if(!mapTestsuiteRWDAdd(mymap,keys,values,states,keyids,j)) return 0;
		}
		if((abs(rand()) % 16384) < 2) {
			mapTestsuiteRWDInit(mymap, states);
		}
		if((abs(rand()) % 100) < 90) {
			if(!mapTestsuiteRWDPrefixCheck(mymap,keys,states,(abs(rand()) % map_size))) return 0;
		}
		else {
			if(!mapTestsuiteRWDGet(mymap,keys,values,states,keyids,(abs(rand()) % map_size))) return 0;
		}
		if((abs(rand()) % 100) < 10) {
			if(!mapTestsuiteRWDSet(mymap,keys,values,states,keyids,(abs(rand()) % map_size))) return 0;
		}
		j = (abs(rand()) % map_size);
		if(states[j]) {
			if(!(mapGetKeyCount(mymap) > 0)) return 0;
			if((mapGetKeyCount(mymap) > ((map_size/4)*3)) || (abs(rand() % 100) < 10)) {
				if(!mapTestsuiteRWDRem(mymap,keys,states,keyids,j)) return 0;
			}
		}
		else {
			if(mapGetKeyCount(mymap) < map_size) if((mapGetKeyCount(mymap) < (map_size/4)) || (abs(rand() % 100) < 10)) {
				if(!mapTestsuiteRWDAdd(mymap,keys,values,states,keyids,j)) return 0;
			}
		}
		if(!mapTestsuiteRWDGet(mymap,keys,values,states,keyids,j)) return 0;
	}

	// destroy data structures
	if(!mapDestroy(mymap)) return 0;
	/*
	free(mymap);
	*/
	mymap = NULL;
	if(!(memcmp(&mymaparray[1], &mymaparray[2], sizeof(struct s_map)) == 0)) return 0;
	free(mymaparray);
	free(keyids);
	free(states);
	free(values);
	free(keys);
	
	return 1;
}


static int mapTestsuite() {
	int i, j, k;
	printf("mapTestsuite started\n");
	struct s_map demomap;
	char demostr[1024];
	mapCreate(&demomap, 8, 2, 2);
	for(i=0; i<8; i++) {
		if(mapIsValidID(&demomap, i)) return 0;
	}
	mapStrAdd(&demomap, " "," ");
	mapStrSet(&demomap, "h","w");
	mapStrSet(&demomap, "e","o");
	mapStrSet(&demomap, "l","r");
	mapStrSet(&demomap, "l","l");
	mapStrSet(&demomap, "o","d");
	mapStrRemove(&demomap, " ");
	mapStrGet(&demomap, "l");
	mapTestsuiteGenerateASCIIString(&demomap, demostr, 1024);
	mapDestroy(&demomap);
	printf("mapTestsuite: tree view test\n%s", demostr);
	for(j=1; j<10; j++) for(i=1; i<10; i++) {
		if(i < 14) {
			k = (128 << i);
		}
		else {
			k = (128 << 14);
		}
		printf("mapTestsuite: read, write & delete with %d byte keys and %d byte values\n",i,j);
		if(!mapTestsuiteRWD(1048576,k,i,j)) {
			printf("mapTestsuite failed!\n");
			return 0;
		}
	}
	printf("mapTestsuite ok\n");
	return 1;
}


#endif // F_MAPSTR_TEST_C
