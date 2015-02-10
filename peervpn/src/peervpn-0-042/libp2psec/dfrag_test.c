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


#ifndef F_DFRAG_TEST_C
#define F_DFRAG_TEST_C


#include "dfrag.c"


static void dfragTestsuiteText(unsigned char *target, const int target_len, const int base, const int newline) {
	int n;
	int nd;
	int ndt;
	if(target_len > 0) {
		n = 1;
		while(n < target_len) {
			target[n - 1] = '.';
			if(base > 1) {
				if((n % base) == 0) {
					nd = 0;
					ndt = n;
					while(ndt > 0) {
						ndt = ndt / 10;
						nd++;
					}
					sprintf((char *)&target[n-nd], "%d", n);
				}
				if(newline == 1) {
					if((n % (base * base)) == 1) { target[n - 1] = '\n'; }
				}
				if(newline == 2) {
					if((n % (base * base)) == 1) { target[n - 1] = '\r'; }
					if((n % (base * base)) == 2) { target[n - 1] = '\n'; }
				}
			}
			n = n + 1;
		}
		if((n > 1) && (newline == 1)) {
			target[n - 2] = '\n';
		}
		if((n > 2) && (newline == 2)) {
			target[n - 3] = '\r';
			target[n - 2] = '\n';
		}
		target[n - 1] = '\0';
	}
}


static int dfragTestsuiteRun(struct s_dfrag *dfrag, const int fragsize, const unsigned char *cmpstr, unsigned char *mystr, const int str_len) {
	int ret;
	int len;
	int i, j, k;
	int pos;
	int msgid;
	int order[] = {
		0, 1, 2, 3,
		0, 1, 3, 2,
		0, 2, 1, 3,
		0, 2, 3, 1,
		0, 3, 1, 2,
		0, 3, 2, 1,
		1, 0, 2, 3,
		1, 0, 3, 2,
		1, 2, 0, 3,
		1, 2, 3, 0,
		1, 3, 0, 2,
		1, 3, 2, 0,
		2, 1, 0, 3,
		2, 1, 3, 0,
		2, 0, 1, 3,
		2, 0, 3, 1,
		2, 3, 1, 0,
		2, 3, 0, 1,
		3, 1, 2, 0,
		3, 1, 0, 2,
		3, 2, 1, 0,
		3, 2, 0, 1,
		3, 0, 1, 2,
		3, 0, 2, 1,
	0 };

	for(k = 0; k < 64; k++) {
		for(j = 0; j < 24; j++) {
			memset(mystr, 0, str_len);
			for(i = 0; i < 4; i++) {
				pos = order[(4*j)+i];
				msgid = (pos/2);
				ret = dfragAssemble(dfrag, k, k, msgid, &cmpstr[((k * fragsize * 4) + (pos * fragsize))], fragsize, (pos - (msgid * 2)), 2);
				if(!(ret < 0)) {
					len = dfragLength(dfrag, ret);
					if(len > 0 && len <= (fragsize * 2)) {
						memcpy(&mystr[(msgid * fragsize * 2)], dfragGet(dfrag, ret), len);
					}
					dfragClear(dfrag, ret);
				}
			}

			for(i = 0; i < 2; i++) {
				if(memcmp(&cmpstr[((k * fragsize * 4) + (i * fragsize * 2))], &mystr[(i * fragsize * 2)], (fragsize * 2)) != 0) { return 0; }
			}
		}
	}

	printf("success!\n");
	
	return 1;
}


static int dfragTestsuite() {
	unsigned char *str1;
	unsigned char *str2;
	unsigned char *str3;
	unsigned char *buf;
	const int fragsize = 256;
	const int fragcount = 4;
	const int str_len = 65536;
	int ret = 0;
	struct s_dfrag *dfrag;
	dfrag = malloc(sizeof(struct s_dfrag));
	if(dfrag != NULL) {
		if(dfragCreate(dfrag, fragsize, fragcount)) {
			str1 = malloc(str_len);
			if(str1 != NULL) {
				str2 = malloc(str_len);
				if(str2 != NULL) {
					str3 = malloc(str_len);
					if(str3 != NULL) {
						buf = malloc(str_len);
						if(buf != NULL) {
							dfragTestsuiteText(str1, 8192, 8, 0);
							dfragTestsuiteText(str2, 8192, 9, 0);
							dfragTestsuiteText(str3, 8192, 10, 0);
							ret = dfragTestsuiteRun(dfrag, fragsize, str1, buf, str_len);
							free(buf);
						}
						free(str3);
					}
					free(str2);
				}
				free(str1);
			}
			dfragDestroy(dfrag);
		}
		free(dfrag);
	}

	return ret;
}


#endif // F_DFRAG_TEST_C
