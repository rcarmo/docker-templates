/***************************************************************************
 *   Copyright (C) 2012 by Tobias Volk                                     *
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


#ifndef F_SEQ_C
#define F_SEQ_C


#include <stdint.h>


// Size of sequence number in bytes.
#define seq_SIZE 8


// Window size.
#define seq_WINDOWSIZE 16384


// The sequence number state structure.
struct s_seq_state {
	int64_t start;
	uint64_t mask;
};


// Get sequence number state.
static int64_t seqGet(struct s_seq_state *state) {
	return state->start;
}


// Initialize sequence number state.
static void seqInit(struct s_seq_state *state, const int64_t seq) {
	state->start = seq;
	state->mask = 0;
}


// Verify sequence number. Returns 1 if accepted, else 0.
static int seqVerify(struct s_seq_state *state, const int64_t seq) {
	const uint_least64_t one = 1;
	int64_t start = state->start;
	int64_t seqdiff = (seq - start);
	uint64_t mask = state->mask;
	uint64_t vmask;
	if((seqdiff > 0) && (seqdiff < seq_WINDOWSIZE)) {
		// move the window
		if(seqdiff > 64) {
			seqdiff = (seqdiff - 64);
			start = (start + seqdiff);
			if(seqdiff > 64) {
				mask = 0;
			}
			else {
				mask = (mask << seqdiff);
			}
			seqdiff = 64;
		}

		// check for duplicates
		vmask = (one << (64 - seqdiff));
		if((vmask & mask) == 0) {
			// sequence number is accepted
			mask = (mask | vmask);
			state->start = start;
			state->mask = mask;
			return 1;
		}
		else {
			// duplicate sequence number is rejected
			return 0;
		}
	}
	else {
		// out of window sequence number is rejected
		return 0;
	}
}


// Returns the amount of received sequence numbers out of the last 64.
static int seqRQ(struct s_seq_state *state) {
	const int bitc[256] = {
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
	};
	const unsigned char *bytes = (unsigned char *)&state->mask;
	int c = 0;
	int i;
	for(i=0; i<(sizeof(uint64_t)); i++) {
		c = c + bitc[bytes[i]];
	}
	return c;
}


#endif // F_SEQ_C
