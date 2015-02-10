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


#ifndef F_NETID_C
#define F_NETID_C


#include "crypto.c"


// NetID size in bytes.
#define netid_SIZE 32


// The NetID structure.
struct s_netid {
	unsigned char id[netid_SIZE];
};


static int netidSet(struct s_netid *netid, const char *netname, const int netname_len) {
	memset(netid->id, 0, netid_SIZE);
	return cryptoCalculateSHA256(netid->id, netid_SIZE, (unsigned char *)netname, netname_len);
}


#endif // F_NETID_C
