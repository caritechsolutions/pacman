/* Imported from the dvbstream-0.2 project
 *
 *  Modified for use with MPlayer, for details see the changelog at
 *  http://svn.mplayerhq.hu/mplayer/trunk/
 *  $Id: rtp.c 19325 2006-08-04 19:36:41Z ben $
 */

/* MPEG-2 TS RTP stack*/
// RTP reorder routines
// Also handling of repeated UDP packets (a bug of ExtremeNetworks switches firmware)
// rtpreord procedures
// write rtp packets in cache
// get rtp packets reordered

#include "gx_stream.h"
#include "gx_common.h"
#include "rtp.h"
#include "gx_poll.h"
#include "gx_system.h"
#include <sys/socket.h>

struct rtpbits {
	unsigned int v:2;	/* version: 2*/
	unsigned int p:1;	/* is there padding appended: 0*/
	unsigned int x:1;	/* number of extension headers: 0*/
	unsigned int cc:4;	/* number of CSRC identifiers: 0*/
	unsigned int m:1;	/* marker: 0*/
	unsigned int pt:7;	/* payload type: 33 for MPEG2 TS - RFC 1890*/
	unsigned int sequence:16;	/* sequence number: random*/
};

struct rtpheader {		/* in network byte order*/
	struct rtpbits b;
	int timestamp;		/* start: random*/
	int ssrc;		/* random*/
};

struct rtppktdata{
	int   first;
	int   number;
	unsigned char  data[MAX_RTP_PACKET_COUNT][MAX_RTP_PACKET_SIZE];
	unsigned short len[MAX_RTP_PACKET_COUNT];
};

struct rtpbuffer {
	unsigned char  data[MAX_RTP_PACKET_COUNT][MAX_RTP_PACKET_SIZE];
	unsigned short seq[MAX_RTP_PACKET_COUNT];
	unsigned short len[MAX_RTP_PACKET_COUNT];
	unsigned short first;
	unsigned char  buffer[STREAM_RTP_MAX_PKT_SIZE];
	unsigned int   buffer_size;
	unsigned int   buffer_pos;
	unsigned int   error_count;
};
static struct rtpbuffer  *prtpbuf = NULL;
static struct rtppktdata *prtpdata = NULL;
static int is_first = 1;
static int last_seq = -1;
static int32_t  debug_last_seq   = -1;
static int32_t  debug_packet_num = 0;

// RTP Reordering functions
// Algorithm works as follows:
// If next packet is in sequence just copy it to buffer
// Otherwise copy it in cache according to its sequence number
// Cache is a circular array where "prtpbuf->first" points to next sequence slot
// and keeps track of expected sequence

// Initialize rtp cache
static void rtp_cache_reset(unsigned short seq)
{
	int i;

	is_first = 1;
	prtpbuf->first = 0;
	prtpbuf->seq[0] = ++seq;

	for (i = 0; i < MAX_RTP_PACKET_COUNT; i++) {
		prtpbuf->len[i] = 0;
	}
}

static int rtp_recv(int fd, struct rtppktdata* data, int over)
{
	int ret = 0;
	int ev = POLLIN;
	struct pollfd p = { .fd = fd, .events = ev, .revents = 0 };
	ret = poll(&p, 1, 200);
	if(ret > 0){
#ifdef LINUX_OS
		unsigned int linux_os_version;

		GxPlayer_SystemGet(PSYS_SYSTEM_VERSION, &linux_os_version);
		if (linux_os_version >= KERNEL_VERSION(2,6,33)) {
			int i=0;
			struct mmsghdr msgs[MAX_RTP_PACKET_COUNT];
			struct iovec iovecs[MAX_RTP_PACKET_COUNT];
			struct timespec timeout;
			timeout.tv_sec = 1;
			timeout.tv_nsec = 0;

			memset(msgs, 0, sizeof(msgs));
			for (i = 0; i < MAX_RTP_PACKET_COUNT; i++) {
				data->len[i] = 0;
				iovecs[i].iov_base = data->data[i];
				iovecs[i].iov_len  = MAX_RTP_PACKET_SIZE;
				msgs[i].msg_hdr.msg_iov = &iovecs[i];
				msgs[i].msg_hdr.msg_iovlen = 1;
			}

			data->number = recvmmsg(fd, msgs, MAX_RTP_PACKET_COUNT, 0x10000, &timeout);
			if(data->number < 0)
			{
				gxlogd("error get no data\n");
				return 0;
			}
			for(i = 0; i < data->number; i++)
			{
				data->len[i] = msgs[i].msg_len;
			}
			if(data->number != (MAX_RTP_PACKET_COUNT))
				GxCore_ThreadDelay(10);
		} else {
			data->len[0] = recv(fd, &data->data[0], MAX_RTP_PACKET_SIZE, 0);
			data->number = 1;
		}
#else
		data->len[0] = recv(fd, &data->data[0], MAX_RTP_PACKET_SIZE, 0);
		data->number = 1;
#endif
		data->first = 0;
		prtpbuf->error_count = 0;
	}else if(ret == 0){
		prtpbuf->error_count++;
		if(over || prtpbuf->error_count > 50){
			gxlogf("error get  data timeout\n");
			ret = -1;
		}
	}
	return ret;
}

static int getrtp2(int fd, struct rtpheader* rh, char **data, int *lengthData, int over)
{
	if(prtpdata->number <= 0){
		if(rtp_recv(fd, prtpdata, over) < 0)
			return -1;
	}

	if(prtpdata->number > 0){
		unsigned int intP;
		char* charP = (char *)&intP;
		int headerSize;
		int lengthPacket = prtpdata->len[prtpdata->first];
		unsigned char* buf = prtpdata->data[prtpdata->first];
		if (lengthPacket < 12){
			*lengthData = 0;
			return 0;
		}
		rh->b.v = (unsigned int)((buf[0] >> 6) & 0x03);
		rh->b.p = (unsigned int)((buf[0] >> 5) & 0x01);
		rh->b.x = (unsigned int)((buf[0] >> 4) & 0x01);
		rh->b.cc = (unsigned int)((buf[0] >> 0) & 0x0f);
		rh->b.m = (unsigned int)((buf[1] >> 7) & 0x01);
		rh->b.pt = (unsigned int)((buf[1] >> 0) & 0x7f);
		intP = 0;
		memcpy(charP + 2, &buf[2], 2);
		rh->b.sequence = ntohl(intP);
		intP = 0;
		memcpy(charP, &buf[4], 4);
		rh->timestamp = ntohl(intP);

		headerSize = 12 + 4*  rh->b.cc;	/* in bytes */

		*lengthData = lengthPacket - headerSize;
		*data = (char* )buf + headerSize;
		prtpdata->first++;
		prtpdata->number--;
	}
	else{
		*data = NULL;
		*lengthData = 0;
	}
	//  gxlogf(MSGT_NETWORK,MSGL_DBG2,"Reading rtp: v=%x p=%x x=%x cc=%x m=%x pt=%x seq=%x ts=%x lgth=%d\n",rh->b.v,rh->b.p,rh->b.x,rh->b.cc,rh->b.m,rh->b.pt,rh->b.sequence,rh->timestamp,lengthPacket);

	return (0);
}

static void check_rtp_packet(int fd, unsigned short seq)
{
	int debug_rtp_packt = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_RTP_PACKET, &debug_rtp_packt);
	if (debug_rtp_packt) {
		uint16_t tmp_seq = seq & 0x3ff;

		if (debug_last_seq == -1)
			debug_last_seq = tmp_seq;
		if (tmp_seq < debug_last_seq) {
			gxlogi("---> 0x%x: %d %d %d, %.2f\n",
					fd, debug_last_seq, tmp_seq, debug_packet_num, (debug_packet_num *100.0)/(1<<10));
			debug_packet_num = 0;
		}
		debug_last_seq = tmp_seq;
		debug_packet_num++;
	}
}

// Write in a cache the rtp packet in right rtp sequence order
static int rtp_cache(int fd, char* buffer, int length, int time_over)
{
	struct rtpheader rh;
	int newseq;
	char* data = NULL;
	unsigned short seq;

	if(getrtp2(fd, &rh, &data, &length, time_over) < 0)
		return -1;

	if (!length)
		return 0;

	check_rtp_packet(fd, rh.b.sequence);

	seq = rh.b.sequence;
	newseq = seq - prtpbuf->seq[prtpbuf->first];

	if (last_seq == -1) {
		last_seq = seq;
	} else {
		if (((seq + MAX_RTP_PACKET_COUNT - last_seq)%MAX_RTP_PACKET_COUNT) != 1)
			gxlogd("rtp waring: last_seq %d new_seq %d\n", last_seq, seq);
		last_seq = seq;
	}

	if ((newseq == 0) || is_first) {
		is_first = 0;

		//gxlogf(MSGT_NETWORK, MSGL_DBG4, "RTP (seq[%d]=%d seq=%d, newseq=%d)\n", prtpbuf->first, prtpbuf->seq[prtpbuf->first], seq, newseq);
		prtpbuf->first = (1 + prtpbuf->first) % MAX_RTP_PACKET_COUNT;
		prtpbuf->seq[prtpbuf->first] = ++seq;
		goto feed;
	}

	if (newseq > MAX_RTP_PACKET_COUNT) {
		gxlogf("Overrun(seq[%d]=%d seq=%d, newseq=%d)\n", prtpbuf->first, prtpbuf->seq[prtpbuf->first], seq, newseq);
		rtp_cache_reset(seq);
		goto feed;
	}

	if (newseq < 0) {
		int i;

		// Is it a stray packet re-sent to network?
		for (i = 0; i < MAX_RTP_PACKET_COUNT; i++) {
			if (prtpbuf->seq[i] == seq) {
				gxlogf("Stray packet (seq[%d]=%d seq=%d, newseq=%d found at %d)\n", prtpbuf->first,
				       prtpbuf->seq[prtpbuf->first], seq, newseq, i);
				return 0;	// Yes, it is!
			}
		}
		// Some heuristic to decide when to drop packet or to restart everything
		if (newseq > -(3*  MAX_RTP_PACKET_COUNT)) {
			gxlogf("Too Old packet (seq[%d]=%d seq=%d, newseq=%d)\n", prtpbuf->first,
			       prtpbuf->seq[prtpbuf->first], seq, newseq);
			return 0;	// Yes, it is!
		}

		gxlogf("Underrun(seq[%d]=%d seq=%d, newseq=%d)\n", prtpbuf->first, prtpbuf->seq[prtpbuf->first], seq, newseq);

		rtp_cache_reset(seq);
		goto feed;
	}

	gxlogf("Out of Seq (seq[%d]=%d seq=%d, newseq=%d)\n", prtpbuf->first, prtpbuf->seq[prtpbuf->first], seq, newseq);
	newseq = (newseq + prtpbuf->first) % MAX_RTP_PACKET_COUNT;
	memcpy(prtpbuf->data[newseq], data, length);
	prtpbuf->len[newseq] = length;
	prtpbuf->seq[newseq] = seq;

	return 0;

      feed:
	memcpy(buffer, data, length);
	return length;
}

// Get next packet in cache
// Look in cache to get first packet in sequence
static int rtp_get_next(int fd, char* buffer, int length, int time_over)
{
	int i;
	unsigned short nextseq;

	// If we have empty buffer we loop to fill it
	for (i = 0; i < MAX_RTP_PACKET_COUNT - 3; i++) {
		if (prtpbuf->len[prtpbuf->first] != 0)
			break;

		length = rtp_cache(fd, buffer, length, time_over);

		// returns on first packet in sequence
		if (length > 0) {
			//gxlogf(MSGT_NETWORK, MSGL_DBG4, "Getting rtp [%d] %hu\n", i, prtpbuf->first);
			return length;
		} else if (length < 0)
			return -1;

		// Only if length == 0 loop continues!
	}

	i = prtpbuf->first;
	while (prtpbuf->len[i] == 0) {
		gxlogf("Lost packet %hu\n", prtpbuf->seq[i]);
		i = (1 + i) % MAX_RTP_PACKET_COUNT;
		if (prtpbuf->first == i)
			break;
	}
	prtpbuf->first = i;

	// Copy next non empty packet from cache
	gxlogf("Getting rtp from cache [%d] %hu\n", prtpbuf->first, prtpbuf->seq[prtpbuf->first]);
	memcpy(buffer, prtpbuf->data[prtpbuf->first], prtpbuf->len[prtpbuf->first]);
	length = prtpbuf->len[prtpbuf->first];	// can be zero?

	// Reset fisrt slot and go next in cache
	prtpbuf->len[prtpbuf->first] = 0;
	nextseq = prtpbuf->seq[prtpbuf->first];
	prtpbuf->first = (1 + prtpbuf->first) % MAX_RTP_PACKET_COUNT;
	prtpbuf->seq[prtpbuf->first] = nextseq + 1;

	return length;
}

// Read next rtp packet using cache
int read_rtp_from_server(int fd, char* buffer, int length, int time_over)
{
	// Following test is ASSERT (i.e. uneuseful if code is correct)
	if (buffer == NULL || length <= 0) {
		gxlogf("RTP buffer invalid; no data return from network\n");
		return 0;
	}

read_again:
	if (prtpbuf->buffer_size > 0) {
		length = length>prtpbuf->buffer_size?prtpbuf->buffer_size:length;
		memcpy(buffer, prtpbuf->buffer+prtpbuf->buffer_pos, length);
		prtpbuf->buffer_pos += length;
		prtpbuf->buffer_size -= length;
		return length;
	}

	if (length < STREAM_RTP_MAX_PKT_SIZE) {
		int ret = rtp_get_next(fd, (char*)prtpbuf->buffer, STREAM_RTP_MAX_PKT_SIZE, time_over);
		if (ret <= 0) {
			prtpbuf->buffer_pos = 0;
			prtpbuf->buffer_size = 0;
			return ret;
		}
		prtpbuf->buffer_pos = 0;
		prtpbuf->buffer_size = ret;
		goto read_again;
	}
	// loop just to skip empty packets
	length = rtp_get_next(fd, buffer, length, time_over);

	return length;
}

void init_rtp_cache(void)
{
	is_first = 1;
	last_seq = -1;
	debug_last_seq = -1;
	debug_packet_num = 0;

	if (prtpbuf) {
		memset(prtpbuf,  0, sizeof(struct rtpbuffer));
	}
	else {
		prtpbuf = av_mallocz(sizeof(struct rtpbuffer));
	}
	if (prtpdata) {
		memset(prtpdata,  0, sizeof(struct rtppktdata));
	}
	else {
		prtpdata = av_mallocz(sizeof(struct rtppktdata));
	}
}

void uninit_rtp_cache(void)
{
	if (prtpbuf) {
		av_free(prtpbuf);
		prtpbuf = NULL;
	}
	if (prtpdata) {
		av_free(prtpdata);
		prtpdata = NULL;
	}
}
