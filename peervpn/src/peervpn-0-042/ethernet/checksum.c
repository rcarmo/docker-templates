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


#ifndef F_CHECKSUM_C
#define F_CHECKSUM_C


// The checksum structure.
struct s_checksum {
	uint32_t checksum;
};


// Zeroes the checksum.
static void checksumZero(struct s_checksum *cs) {
	cs->checksum = 0;
}


// Adds 16 bit to the checksum.
static void checksumAdd(struct s_checksum *cs, const uint16_t x) {
	cs->checksum += x;
}


// Get checksum
static uint16_t checksumGet(struct s_checksum *cs) {
	uint16_t ret;
	cs->checksum = ((cs->checksum & 0xFFFF) + (cs->checksum >> 16));
	cs->checksum = ((cs->checksum & 0xFFFF) + (cs->checksum >> 16));
	ret = ~(cs->checksum);
	return ret;
}


#endif // F_CHECKSUM_C
