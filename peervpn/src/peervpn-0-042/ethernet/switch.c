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


#ifndef F_SWITCH_C
#define F_SWITCH_C


#include "../libp2psec/map.c"
#include "../libp2psec/util.c"


// Constants.
#define switch_FRAME_TYPE_INVALID 0
#define switch_FRAME_TYPE_BROADCAST 1
#define switch_FRAME_TYPE_UNICAST 2
#define switch_FRAME_MINSIZE 14
#define switch_MACADDR_SIZE 6
#define switch_MACMAP_SIZE 8192
#define switch_TIMEOUT 86400


// Constraints.
#if switch_FRAME_MINSIZE < switch_MACADDR_SIZE + switch_MACADDR_SIZE
#error switch_FRAME_MINSIZE too small
#endif
#if switch_MACADDR_SIZE != 6
#error switch_MACADDR_SIZE is not 6
#endif


// Switchstate structures.
struct s_switch_mactable_entry {
	int portid;
	int portts;
	int ents;
};
struct s_switch_state {
	struct s_map mactable;
};


// Get type of outgoing frame. If it is an unicast frame, also returns PortID and PortTS.
static int switchFrameOut(struct s_switch_state *switchstate, const unsigned char *frame, const int frame_len, int *portid, int *portts) {
	struct s_switch_mactable_entry *mapentry;
	const unsigned char *macaddr;
	int pos;
	if(frame_len > switch_FRAME_MINSIZE) {
		macaddr = &frame[0];
		pos = mapGetKeyID(&switchstate->mactable, macaddr);
		if(!(pos < 0)) {
			mapentry = (struct s_switch_mactable_entry *)mapGetValueByID(&switchstate->mactable, pos);
			if((utilGetClock() - mapentry->ents) < switch_TIMEOUT) {
				// valid entry found
				*portid = mapentry->portid;
				*portts = mapentry->portts;
				return switch_FRAME_TYPE_UNICAST;
			}
			else {
				// outdated entry found
				return switch_FRAME_TYPE_BROADCAST;
			}
		}
		else {
			// no entry found
			return switch_FRAME_TYPE_BROADCAST;
		}
	}
	else {
		// invalid frame
		return switch_FRAME_TYPE_INVALID;
	}
}


// Learn PortID+PortTS of incoming frame.
static void switchFrameIn(struct s_switch_state *switchstate, const unsigned char *frame, const int frame_len, const int portid, const int portts) {
	struct s_switch_mactable_entry mapentry;
	const unsigned char *macaddr;
	if(frame_len > switch_FRAME_MINSIZE) {
		macaddr = &frame[6];
		if((macaddr[0] & 0x01) == 0) { // only insert unicast address into mactable
			mapentry.portid = portid;
			mapentry.portts = portts;
			mapentry.ents = utilGetClock();
			mapSet(&switchstate->mactable, macaddr, &mapentry);
		}
	}
}


// Generate MAC table status report.
static void switchStatus(struct s_switch_state *switchstate, char *report, const int report_len) {
	int tnow = utilGetClock();
	struct s_map *map = &switchstate->mactable;
	struct s_switch_mactable_entry *mapentry;
	int pos = 0;
	int size = mapGetMapSize(map);
	int maxpos = (((size + 2) * (49)) + 1);
	unsigned char infomacaddr[switch_MACADDR_SIZE];
	unsigned char infoportid[4];
	unsigned char infoportts[4];
	unsigned char infoents[4];
	int i = 0;
	int j = 0;
	
	if(maxpos > report_len) { maxpos = report_len; }
	
	memcpy(&report[pos], "MAC                PortID    PortTS    LastFrm ", 47);
	pos = pos + 47;
	report[pos++] = '\n';

	while(i < size && pos < maxpos) {
		if(mapIsValidID(map, i)) {
			mapentry = (struct s_switch_mactable_entry *)mapGetValueByID(&switchstate->mactable, i);
			memcpy(infomacaddr, mapGetKeyByID(map, i), switch_MACADDR_SIZE);
			j = 0;
			while(j < switch_MACADDR_SIZE) {
				utilByteArrayToHexstring(&report[pos + (j * 3)], (4 + 2), &infomacaddr[j], 1);
				report[pos + (j * 3) + 2] = ':';
				j++;
			}
			pos = pos + ((switch_MACADDR_SIZE * 3) - 1);
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt32(infoportid, mapentry->portid);
			utilByteArrayToHexstring(&report[pos], ((4 * 2) + 2), infoportid, 4);
			pos = pos + (4 * 2);
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt32(infoportts, mapentry->portts);
			utilByteArrayToHexstring(&report[pos], ((4 * 2) + 2), infoportts, 4);
			pos = pos + (4 * 2);
			report[pos++] = ' ';
			report[pos++] = ' ';
			utilWriteInt32(infoents, (tnow - mapentry->ents));
			utilByteArrayToHexstring(&report[pos], ((4 * 2) + 2), infoents, 4);
			pos = pos + (4 * 2);
			report[pos++] = '\n';
		}
		i++;
	}
	report[pos++] = '\0';
}


// Create switchstate structure.
static int switchCreate(struct s_switch_state *switchstate) {
	if(mapCreate(&switchstate->mactable, switch_MACMAP_SIZE, switch_MACADDR_SIZE, sizeof(struct s_switch_mactable_entry))) {
		mapEnableReplaceOld(&switchstate->mactable);
		mapInit(&switchstate->mactable);
		return 1;
	}
	return 0;
}


// Destroy switchstate structure.
static void switchDestroy(struct s_switch_state *switchstate) {
	mapDestroy(&switchstate->mactable);
}


#endif // F_SWITCH_C
