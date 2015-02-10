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


#ifndef F_CTR_C
#define F_CTR_C


#include "util.c"


// Number of seconds for averaging.
#define ctr_SECS 16


// The sequence number state structure.
struct s_ctr_state {
	int total;
	int cur;
	int last[ctr_SECS + 16];
	int lasttime;
	int lastpos;
};


// Clear counters.
static void ctrClear(struct s_ctr_state *ctr) {
	int i;
	ctr->total = 0;
	ctr->cur = 0;
	for(i=0; i<ctr_SECS; i++) {
		ctr->last[i] = 0;
	}
}


// Initialize sequence number state.
static void ctrInit(struct s_ctr_state *ctr) {
	ctrClear(ctr);
	ctr->lasttime = utilGetClock();
	ctr->lastpos = 0;
}


// Increment counter.
static void ctrIncr(struct s_ctr_state *ctr, const int inc) {
	int diff;
	int lasttime = ctr->lasttime;
	int tnow = utilGetClock();
	
	diff = (tnow - lasttime);
	if(diff > ctr_SECS) {
		ctrClear(ctr);
	}
	else {
		while(diff > 1) {
			ctr->last[ctr->lastpos] = 0;
			ctr->lastpos = ((ctr->lastpos + 1) % ctr_SECS);
			diff--;
		}
		if(diff > 0) {
			ctr->last[ctr->lastpos] = ctr->cur;
			ctr->lastpos = ((ctr->lastpos + 1) % ctr_SECS);
			ctr->cur = 0;
			diff--;
		}
	}
	
	ctr->cur = (ctr->cur + inc);
	ctr->total = (ctr->total + inc);
	ctr->lasttime = tnow;
}


static int ctrAvg(struct s_ctr_state *ctr) {
	int i = 0;
	int sum = 0;
	while(i < ctr_SECS) {
		sum = sum + ctr->last[i];
		i++;
	}
	return(sum / ctr_SECS);
}


#endif // F_CTR_C
