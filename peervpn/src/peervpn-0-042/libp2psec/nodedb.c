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


#ifndef F_NODEDB_C
#define F_NODEDB_C


#include "map.c"
#include "peeraddr.c"
#include "nodeid.c"
#include "peeraddr.c"
#include "util.c"


// The NodeDB addrdata structure.
struct s_nodedb_addrdata {
	int lastseen;
	int lastseen_t;
	int lastconnect;
	int lastconnect_t;
	int lastconntry;
	int lastconntry_t;
};


// The NodeDB structure.
struct s_nodedb {
	struct s_map *addrdb;
	int num_peeraddrs;
};


// Initialize NodeDB.
static void nodedbInit(struct s_nodedb *db) {
	mapInit(db->addrdb);
	mapEnableReplaceOld(db->addrdb);
}


// Update NodeDB entry.
static void nodedbUpdate(struct s_nodedb *db, struct s_nodeid *nodeid, struct s_peeraddr *addr, const int update_lastseen, const int update_lastconnect, const int update_lastconntry) {
	int tnow = utilGetClock();
	struct s_map *addrset;
	struct s_map *newaddrset;
	struct s_nodedb_addrdata *addrdata;
	struct s_nodedb_addrdata addrdata_new;

	if(db != NULL && nodeid != NULL && addr != NULL) {
		addrset = mapGet(db->addrdb, nodeid->id);
		if(addrset == NULL) {
			newaddrset = mapGetValueByID(db->addrdb, mapAddReturnID(db->addrdb, nodeid->id, NULL));
			if(newaddrset != NULL) {
				if(mapMemInit(newaddrset, mapMemSize(db->num_peeraddrs, peeraddr_SIZE, sizeof(struct s_nodedb_addrdata)), db->num_peeraddrs, peeraddr_SIZE, sizeof(struct s_nodedb_addrdata))) {
					mapEnableReplaceOld(newaddrset);
					addrset = newaddrset;
				}
			}
		}

		if(addrset != NULL) {
			addrdata_new.lastseen = 0;
			addrdata_new.lastconnect = 0;
			addrdata_new.lastconntry = 0;
			addrdata_new.lastseen_t = 0;
			addrdata_new.lastconnect_t = 0;
			addrdata_new.lastconntry_t = 0;
			if((addrdata = mapGet(addrset, addr->addr)) != NULL) {
				addrdata_new = *addrdata;
			}
			if(update_lastseen > 0) {
				addrdata_new.lastseen = 1;
				addrdata_new.lastseen_t = tnow;
			}
			if(update_lastconnect > 0) {
				addrdata_new.lastconnect = 1;
				addrdata_new.lastconnect_t = tnow;
			}
			if(update_lastconntry > 0) {
				addrdata_new.lastconntry = 1;
				addrdata_new.lastconntry_t = tnow;
			}
			mapSet(addrset, addr->addr, &addrdata_new);
		}
	}
}


// Returns a NodeDB ID that matches the specified criteria, with explicit nid/tnow.
static int nodedbGetDBIDByID(struct s_nodedb *db, const int nid, const int tnow, const int max_lastseen, const int max_lastconnect, const int min_lastconntry) {
	int j, j_max, aid, ret;
	struct s_nodedb_addrdata *dbdata;
	struct s_map *addrset;
	addrset = mapGetValueByID(db->addrdb, nid);
	j_max = mapGetKeyCount(addrset);
	for(j=0; j<j_max; j++) {
		aid = mapGetNextKeyID(addrset);
		dbdata = mapGetValueByID(addrset, aid);
		if(dbdata != NULL) {
			if(
				( (max_lastseen < 0) || (
					(dbdata->lastseen > 0) &&
					((tnow - dbdata->lastseen_t) < max_lastseen) &&
				1)) &&
				( (max_lastconnect < 0) || (
					(dbdata->lastconnect > 0) &&
					((tnow - dbdata->lastconnect_t) < max_lastconnect) &&
				1)) &&
				( (min_lastconntry < 0) || (!(dbdata->lastconntry > 0)) || (
					(dbdata->lastseen > 0) &&
					((tnow - dbdata->lastconntry_t) >= min_lastconntry) &&
					((tnow - dbdata->lastconntry_t) >= ((tnow - dbdata->lastseen_t) / 2)) &&
				1)) &&
			1) {
				ret = (nid * db->num_peeraddrs) + aid;
				return ret;
			}
		}
	}
	return -1;
}


// Returns a NodeDB ID that matches the specified criteria.
static int nodedbGetDBID(struct s_nodedb *db, struct s_nodeid *nodeid, const int max_lastseen, const int max_lastconnect, const int min_lastconntry) {
	int i, i_max, nid, tnow, ret;
	tnow = utilGetClock();
	i_max = mapGetKeyCount(db->addrdb);
	ret = -1;
	if(nodeid == NULL) { // find DBID for any NodeID
		i = 0;
		while((i < i_max) && (ret < 0)) {
			nid = mapGetNextKeyID(db->addrdb);
			ret = nodedbGetDBIDByID(db, nid, tnow, max_lastseen, max_lastconnect, min_lastconntry);
			i++;
		}
	}
	else { // find DBID for specified NodeID
		nid = mapGetKeyID(db->addrdb, nodeid->id);
		if(!(nid < 0)) {
			ret = nodedbGetDBIDByID(db, nid, tnow, max_lastseen, max_lastconnect, min_lastconntry);
		}
	}
	return ret;
}


// Returns node ID of specified NodeDB ID.
static struct s_nodeid *nodedbGetNodeID(struct s_nodedb *db, const int db_id) {
	int nid;
	nid = (db_id / db->num_peeraddrs);
	return mapGetKeyByID(db->addrdb, nid);
}


// Returns node address of specified NodeDB ID.
static struct s_peeraddr *nodedbGetNodeAddress(struct s_nodedb *db, const int db_id) {
	struct s_map *addrset;
	int nid, aid;

	nid = (db_id / db->num_peeraddrs);
	aid = (db_id % db->num_peeraddrs);
	addrset = mapGetValueByID(db->addrdb, nid);
	if(addrset != NULL) {
		return mapGetKeyByID(addrset, aid);
	}

	return NULL;
}


// Create NodeDB.
static int nodedbCreate(struct s_nodedb *db, const int size, const int num_peeraddrs) {
	const int addrdb_vsize = mapMemSize(num_peeraddrs, peeraddr_SIZE, sizeof(struct s_nodedb_addrdata));
	const int addrdb_memsize = mapMemSize(size, nodeid_SIZE, addrdb_vsize);
	struct s_map *addrdb_mem;
	addrdb_mem = NULL;
	if(!((addrdb_mem = malloc(addrdb_memsize)) == NULL)) {
		memset(addrdb_mem, 0, addrdb_memsize);
		if(mapMemInit(addrdb_mem, addrdb_memsize, size, nodeid_SIZE, addrdb_vsize)) {
			db->addrdb = addrdb_mem;
			db->num_peeraddrs = num_peeraddrs;
			nodedbInit(db);
			return 1;
		}
		free(addrdb_mem);
	}
	return 0;
}


// Destroy NodeDB.
static void nodedbDestroy(struct s_nodedb *db) {
	free(db->addrdb);
}


// Generate NodeDB status report.
static void nodedbStatus(struct s_nodedb *db, char *report, const int report_len) {
	int i;
	int j;
	int size;
	int rpsize;
	int pos;
	int tnow;
	unsigned char timediff[4];
	struct s_map *addrset;
	struct s_nodedb_addrdata *addrdata;

	tnow = utilGetClock();
	size = mapGetKeyCount(db->addrdb);
	rpsize = (95 * (1 + db->num_peeraddrs)) + 2;

	pos = 0;
	memcpy(&report[pos], "NodeID + Address                                                  LastSeen  LastConn  LastTry ", 94);
	pos = pos + 94;
	report[pos++] = '\n';

	i = 0;
	while(i < size && pos < (report_len - rpsize)) {
		if(mapIsValidID(db->addrdb, i)) {
			utilByteArrayToHexstring(&report[pos], ((nodeid_SIZE * 2) + 2), mapGetKeyByID(db->addrdb, i), nodeid_SIZE);
			pos = pos + (nodeid_SIZE * 2);
			addrset = mapGetValueByID(db->addrdb, i);
			memcpy(&report[pos], "                              ", 30);
			pos = pos + 30;
			j = 0;
			while(j < db->num_peeraddrs) {
				if(mapIsValidID(addrset, j)) {
					addrdata = mapGetValueByID(addrset, j);
					report[pos++] = '\n';
					memcpy(&report[pos], "            ", 12);
					pos = pos + 12;
					report[pos++] = '-';
					report[pos++] = '-';
					report[pos++] = '>';
					report[pos++] = ' ';
					utilByteArrayToHexstring(&report[pos], ((peeraddr_SIZE * 2) + 2), mapGetKeyByID(addrset, j), peeraddr_SIZE);
					pos = pos + (peeraddr_SIZE * 2);
					report[pos++] = ' ';
					report[pos++] = ' ';
					if(addrdata->lastseen) {
						utilWriteInt32(timediff, (tnow - addrdata->lastseen_t));
						utilByteArrayToHexstring(&report[pos], 10, timediff, 4);
					}
					else {
						memcpy(&report[pos], "--------", 8);
					}
					pos = pos + 8;
					report[pos++] = ' ';
					report[pos++] = ' ';
					if(addrdata->lastconnect) {
						utilWriteInt32(timediff, (tnow - addrdata->lastconnect_t));
						utilByteArrayToHexstring(&report[pos], 10, timediff, 4);
					}
					else {
						memcpy(&report[pos], "--------", 8);
					}
					pos = pos + 8;
					report[pos++] = ' ';
					report[pos++] = ' ';
					if(addrdata->lastconntry) {
						utilWriteInt32(timediff, (tnow - addrdata->lastconntry_t));
						utilByteArrayToHexstring(&report[pos], 10, timediff, 4);
					}
					else {
						memcpy(&report[pos], "--------", 8);
					}
					pos = pos + 8;
				}
				j++;
			}
			report[pos++] = '\n';
		}
		i++;
	}
	report[pos++] = '\0';
}


#endif // F_NODEDB_C
