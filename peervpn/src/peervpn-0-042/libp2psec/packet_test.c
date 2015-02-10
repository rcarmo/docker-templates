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


#ifndef F_PACKET_TEST_C
#define F_PACKET_TEST_C


#include "packet.c"
#include <stdlib.h>
#include <stdio.h>

#define packetTestsuite_PLBUF_SIZE 16
#define packetTestsuite_PKBUF_SIZE 1536

#if EVP_MAX_KEY_LENGTH < 16
#error invalid EVP_MAX_KEY_LENGTH
#endif


static int packetTestsuiteMsg(const int random_msg) {
	unsigned char plbuf[packetTestsuite_PLBUF_SIZE];
	unsigned char plbufdec[packetTestsuite_PLBUF_SIZE];
	struct s_packet_data testdata = { .pl_buf_size = packetTestsuite_PLBUF_SIZE, .pl_buf = plbuf };
	struct s_packet_data testdatadec = { .pl_buf_size = packetTestsuite_PLBUF_SIZE, .pl_buf = plbufdec };
	unsigned char pkbuf[packetTestsuite_PKBUF_SIZE];
	struct s_crypto ctx[2];
	unsigned char secret[64];
	unsigned char nonce[16];
	struct s_seq_state seqstate;
	char str[4096];
	int len;
	
	memset(secret, 23, 64);
	memset(nonce, 5, 16);
	
	cryptoCreate(ctx, 2);
	
	if(!cryptoSetKeys(&ctx[0], 1, secret, 64, nonce, 16)) return 0;
	if(!cryptoSetKeys(&ctx[1], 1, secret, 64, nonce, 16)) return 0;

	seqInit(&seqstate, 0);
	
	memset(plbuf, 0, packetTestsuite_PLBUF_SIZE);
	if(random_msg) RAND_pseudo_bytes(plbuf, packetTestsuite_PLBUF_SIZE);
	else strcpy((char *)plbuf, "moo");
	len = packetTestsuite_PLBUF_SIZE;
	testdata.pl_length = len;
	testdata.pl_type = 0;
	testdata.pl_options = 0;
	testdata.peerid = plbuf[0];
	testdata.seq = 1;
	utilByteArrayToHexstring(str, 4096, plbuf, len);
	printf("%s (len=%d, peerid=%d) -> ", str, len, testdata.peerid);
	len = packetEncode(pkbuf, packetTestsuite_PKBUF_SIZE, &testdata, &ctx[0]);
	if(!(len > 0)) return 0;
	utilByteArrayToHexstring(str, 4096, pkbuf, len);
	printf("%s (%d) -> ", str, len);
	if(!(packetDecode(&testdatadec, pkbuf, len, &ctx[1], &seqstate))) return 0;
	if(!(testdatadec.pl_length > 0)) return 0;
	if(!(testdatadec.peerid == plbuf[0])) return 0;
	if(!memcmp(testdatadec.pl_buf, testdata.pl_buf, packetTestsuite_PLBUF_SIZE) == 0) return 0;
	utilByteArrayToHexstring(str, 4096, testdatadec.pl_buf, testdatadec.pl_length);
	printf("%s (len=%d, peerid=%d)\n", str, testdatadec.pl_length, testdatadec.peerid);
	
	cryptoDestroy(ctx, 2);

	return 1;
}


static int packetTestsuite() {
	int i;
	for(i=0; i<100; i++) if(!packetTestsuiteMsg(1)) return 0;
	for(i=0; i<100; i++) if(!packetTestsuiteMsg(0)) return 0;
	return 1;
}


#endif // F_PACKET_TEST_C
