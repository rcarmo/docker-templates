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


#ifndef F_PACKET_C
#define F_PACKET_C


#include "crypto.c"
#include "seq.c"
#include "util.c"


// size of packet header fields in bytes
#define packet_PEERID_SIZE 4 // peer ID
#define packet_HMAC_SIZE 32 // hmac that includes sequence number, node ID, pl* fields (pllen, pltype, plopt) and payload
#define packet_IV_SIZE 16 // IV
#define packet_SEQ_SIZE seq_SIZE // packet sequence number
#define packet_PLLEN_SIZE 2 // payload length
#define packet_PLTYPE_SIZE 1 // payload type
#define packet_PLOPT_SIZE 1 // payload options
#define packet_CRHDR_SIZE (packet_SEQ_SIZE + packet_PLLEN_SIZE + packet_PLTYPE_SIZE + packet_PLOPT_SIZE)


// position of packet header fields
#define packet_CRHDR_SEQ_START (0)
#define packet_CRHDR_PLLEN_START (packet_CRHDR_SEQ_START + packet_SEQ_SIZE)
#define packet_CRHDR_PLTYPE_START (packet_CRHDR_PLLEN_START + packet_PLLEN_SIZE)
#define packet_CRHDR_PLOPT_START (packet_CRHDR_PLTYPE_START + packet_PLTYPE_SIZE)


// payload types
#define packet_PLTYPE_USERDATA 0
#define packet_PLTYPE_USERDATA_FRAGMENT 1
#define packet_PLTYPE_AUTH 2
#define packet_PLTYPE_PEERINFO 3
#define packet_PLTYPE_PING 4
#define packet_PLTYPE_PONG 5
#define packet_PLTYPE_RELAY_IN 6
#define packet_PLTYPE_RELAY_OUT 7


// constraints
#if packet_PEERID_SIZE != 4
#error invalid packet_PEERID_SIZE
#endif
#if packet_SEQ_SIZE != 8
#error invalid packet_SEQ_SIZE
#endif
#if packet_PLLEN_SIZE != 2
#error invalid packet_PLLEN_SIZE
#endif
#if packet_CRHDR_SIZE < (3 * packet_PEERID_SIZE)
#error invalid packet_CRHDR_SIZE
#endif


// packet data structure
struct s_packet_data {
	int peerid;
	int64_t seq;
	int pl_length;
	int pl_type;
	int pl_options;
	unsigned char *pl_buf;
	int pl_buf_size;
};


// return the peer ID
static int packetGetPeerID(const unsigned char *pbuf) {
	int32_t *scr_peerid = ((int32_t *)pbuf);
	int32_t ne_peerid = (scr_peerid[0] ^ (scr_peerid[1] ^ scr_peerid[2]));
	return utilReadInt32((unsigned char *)&ne_peerid);
}


// encode packet
static int packetEncode(unsigned char *pbuf, const int pbuf_size, const struct s_packet_data *data, struct s_crypto *ctx) {
	unsigned char dec_buf[packet_CRHDR_SIZE + data->pl_buf_size];
	int32_t *scr_peerid = ((int32_t *)pbuf);
	int32_t ne_peerid;
	int len;
	
	// check if enough space is available for the operation
	if(data->pl_length > data->pl_buf_size) { return 0; }
	
	// prepare buffer
	utilWriteInt64(&dec_buf[packet_CRHDR_SEQ_START], data->seq);
	utilWriteInt16(&dec_buf[packet_CRHDR_PLLEN_START], data->pl_length);
	dec_buf[packet_CRHDR_PLTYPE_START] = data->pl_type;
	dec_buf[packet_CRHDR_PLOPT_START] = data->pl_options;
	memcpy(&dec_buf[packet_CRHDR_SIZE], data->pl_buf, data->pl_length);
	
	// encrypt buffer
	len = cryptoEnc(ctx, &pbuf[packet_PEERID_SIZE], (pbuf_size - packet_PEERID_SIZE), dec_buf, (packet_CRHDR_SIZE + data->pl_length), packet_HMAC_SIZE, packet_IV_SIZE);
	if(len < (packet_HMAC_SIZE + packet_IV_SIZE + packet_CRHDR_SIZE)) { return 0; }
	
	// write the scrambled peer ID
	utilWriteInt32((unsigned char *)&ne_peerid, data->peerid);
	scr_peerid[0] = (ne_peerid ^ (scr_peerid[1] ^ scr_peerid[2]));

	// return length of encoded packet
	return (packet_PEERID_SIZE + len);
}


// decode packet
static int packetDecode(struct s_packet_data *data, const unsigned char *pbuf, const int pbuf_size, struct s_crypto *ctx, struct s_seq_state *seqstate) {
	unsigned char dec_buf[pbuf_size];
	int len;

	// decrypt packet
	if(pbuf_size < (packet_PEERID_SIZE + packet_HMAC_SIZE + packet_IV_SIZE)) { return 0; }
	len = cryptoDec(ctx, dec_buf, pbuf_size, &pbuf[packet_PEERID_SIZE], (pbuf_size - packet_PEERID_SIZE), packet_HMAC_SIZE, packet_IV_SIZE);
	if(len < packet_CRHDR_SIZE) { return 0; };

	// get packet data
	data->peerid = packetGetPeerID(pbuf);
	data->seq = utilReadInt64(&dec_buf[packet_CRHDR_SEQ_START]);
	if(seqstate != NULL) if(!seqVerify(seqstate, data->seq)) { return 0; }
	data->pl_options = dec_buf[packet_CRHDR_PLOPT_START];
	data->pl_type = dec_buf[packet_CRHDR_PLTYPE_START];
	data->pl_length = utilReadInt16(&dec_buf[packet_CRHDR_PLLEN_START]);
	if(!(data->pl_length > 0)) {
		data->pl_length = 0;
		return 0;
	}
	if(len < (packet_CRHDR_SIZE + data->pl_length)) { return 0; }
	if(data->pl_length > data->pl_buf_size) { return 0; }
	memcpy(data->pl_buf, &dec_buf[packet_CRHDR_SIZE], data->pl_length);

	// return length of decoded payload
	return (data->pl_length);
}


#endif // F_PACKET_C
