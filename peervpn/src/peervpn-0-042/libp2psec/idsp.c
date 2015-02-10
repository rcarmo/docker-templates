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


#ifndef F_IDSP_C
#define F_IDSP_C


#include <stdlib.h>


#define idsp_ALIGN_BOUNDARY 16


struct s_idsp {
	int *idfwd;
	int *idlist;
	int count;
	int used;
	int iter;
};


static void idspReset(struct s_idsp *idsp) {
	int i;
	for(i=0; i<idsp->count; i++) {
		idsp->idfwd[i] = -1;
		idsp->idlist[i] = i;
	}
	idsp->used = 0;
	idsp->iter = 0;
}


static int idspMemSize(const int size) {
	const int align_boundary = idsp_ALIGN_BOUNDARY;
	int memsize;
	memsize = 0;
	memsize = memsize + ((((sizeof(struct s_idsp)) + (align_boundary - 1)) / align_boundary) * align_boundary); // struct
	memsize = memsize + ((((sizeof(int) * size) + (align_boundary - 1)) / align_boundary) * align_boundary); // idfwd_mem
	memsize = memsize + ((((sizeof(int) * size) + (align_boundary - 1)) / align_boundary) * align_boundary); // idlist_mem
	return memsize;
}


static int idspMemInit(struct s_idsp *idsp, const int mem_size, const int size) {
	const int align_boundary = idsp_ALIGN_BOUNDARY;
	const int idfwd_mem_offset = ((((sizeof(struct s_idsp)) + (align_boundary - 1)) / align_boundary) * align_boundary);
	const int idlist_mem_offset = idfwd_mem_offset + ((((sizeof(int) * size) + (align_boundary - 1)) / align_boundary) * align_boundary);
	const int min_mem_size = idlist_mem_offset + ((((sizeof(int) * size) + (align_boundary - 1)) / align_boundary) * align_boundary);
	unsigned char *idsp_mem;
	idsp_mem = (unsigned char *)idsp;
	if(min_mem_size == idspMemSize(size)) {
		if(size > 0 && mem_size >= min_mem_size) {
			idsp->idfwd = (int *)(&idsp_mem[idfwd_mem_offset]);
			idsp->idlist = (int *)(&idsp_mem[idlist_mem_offset]);
			idsp->count = size;
			idspReset(idsp);
			return 1;
		}
	}
	return 0;
}


static int idspCreate(struct s_idsp *idsp, const int size) {
	int *idfwd_mem;
	int *idlist_mem;
	idfwd_mem = NULL;
	idlist_mem = NULL;
	if(size > 0) {
		idfwd_mem = malloc(sizeof(int) * size);
		if(idfwd_mem != NULL) {
			idlist_mem = malloc(sizeof(int) * size);
			if(idlist_mem != NULL) {
				idsp->idfwd = idfwd_mem;
				idsp->idlist = idlist_mem;
				idsp->count = size;
				idspReset(idsp);
				return 1;
			}
			free(idfwd_mem);
		}
	}
	return 0;
}


static int idspNextN(struct s_idsp *idsp, const int start) {
	int nextid;
	int iter;
	int used;
	int pos;
	used = idsp->used;
	if(used > 0) {
		if(!(start < 0) && (start < idsp->count)) {
			pos = start;
		}
		else {
			pos = 0;
		}
		iter = idsp->idfwd[pos];
		if(iter < 0) {
			iter = 0;
		}
		nextid = idsp->idlist[((iter + 1) % used)];
		return nextid;
	}
	return -1;
}


static int idspNext(struct s_idsp *idsp) {
	int iter;
	int used;
	iter = idsp->iter;
	used = idsp->used;
	if(used > 0) {
		if(!(iter < used)) { iter = 0; }
		idsp->iter = (iter + 1);
		return idsp->idlist[iter];
	}
	return -1;
}



/*
static int idspNext(struct s_idsp *idsp) {
	int iter;
	int used;
	iter = idsp->iter;
	used = idsp->used;
	if(used > 0) {
		if(!(iter < used)) { iter = 0; }
		idsp->iter = (iter + 1);
		return idsp->idlist[iter];
	}
	else {
		return -1;
	}
}
*/


static int idspNew(struct s_idsp *idsp) {
	int new_id;
	int new_pos;
	if(idsp->used < idsp->count) {
		new_pos = idsp->used++;
		new_id = idsp->idlist[new_pos];
		idsp->idfwd[new_id] = new_pos;
		return new_id;
	}
	else {
		return -1;
	}
}


static int idspGetPos(struct s_idsp *idsp, const int id) {
	if((id >= 0) && (id < idsp->count)) {
		return idsp->idfwd[id];
	}
	else {
		return -1;
	}
}


static void idspDelete(struct s_idsp *idsp, const int id) {
	int pos;
	int swp_id;
	int swp_pos;
	pos = idspGetPos(idsp, id);
	if(!(pos < 0)) {
		idsp->idfwd[id] = -1;
		swp_pos = --idsp->used;
		if(swp_pos != pos) {
			swp_id = idsp->idlist[swp_pos];
			idsp->idlist[swp_pos] = id;
			idsp->idlist[pos] = swp_id;
			idsp->idfwd[swp_id] = pos;
		}
	}
}


static int idspIsValid(struct s_idsp *idsp, const int id) {
	return (!(idspGetPos(idsp, id) < 0));
}


static int idspUsedCount(struct s_idsp *idsp) {
	return idsp->used;
}


static int idspSize(struct s_idsp *idsp) {
	return idsp->count;
}


static void idspDestroy(struct s_idsp *idsp) {
	idsp->used = 0;
	idsp->count = 0;
	free(idsp->idlist);
	free(idsp->idfwd);
}


#endif // F_IDSP_C
