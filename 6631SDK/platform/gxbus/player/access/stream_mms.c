/*
 * MMST implementation taken from the xine-mms plugin made by
 * Major MMS (http://geocities.com/majormms/).
 * Ported to MPlayer by Abhijeet Phatak <abhijeetphatak@yahoo.com>.
 *
 * Information about the MMS protocol can be found at http://get.to/sdp
 *
 * copyright (C) 2002 Abhijeet Phatak <abhijeetphatak@yahoo.com>
 * copyright (C) 2002 the xine project
 * copyright (C) 2000-2001 major mms
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>

#include "config.h"
#include "gx_common.h"
#include "gx_url.h"
#include "stream_mms.h"
#include "gx_network.h"
#include "gx_demux.h"
#include "tcp.h"
#include "http.h"
#include "asf.h"
#include "asfguid.h"
#include "../avutil/intreadwrite.h"
#include "stream_m3u8.h"

#define BUF_SIZE 102400
#define HDR_BUF_SIZE 8192
#define MMS_MAX_STREAMS 20

typedef struct
{
	uint8_t buf[BUF_SIZE];
	int     num_bytes;

} command_t;

static int seq_num;
static int num_stream_ids;
static int stream_ids[MMS_MAX_STREAMS];

static int max_idx(int s_count, int *s_rates, int bound) {
	int i, best = -1, rate = -1;
	for (i = 0; i < s_count; i++) {
		if (s_rates[i] > rate && s_rates[i] <= bound) {
			rate = s_rates[i];
			best = i;
		}
	}
	return best;
}


static void put_32 (command_t *cmd, uint32_t value)
{
	cmd->buf[cmd->num_bytes  ] = value % 256;
	value = value >> 8;
	cmd->buf[cmd->num_bytes+1] = value % 256 ;
	value = value >> 8;
	cmd->buf[cmd->num_bytes+2] = value % 256 ;
	value = value >> 8;
	cmd->buf[cmd->num_bytes+3] = value % 256 ;

	cmd->num_bytes += 4;
}

static uint32_t get_32 (char *cmd, int offset)
{
	uint32_t ret;

	ret = cmd[offset] ;
	ret |= cmd[offset+1]<<8 ;
	ret |= cmd[offset+2]<<16 ;
	ret |= cmd[offset+3]<<24 ;

	return ret;
}

static void send_command (int s, int command, uint32_t switches,
		uint32_t extra, int length,
		char *data)
{
	command_t  cmd;
	int        len8;

	len8 = (length + 7) / 8;
	cmd.num_bytes = 0;
	put_32 (&cmd, 0x00000001); /* start sequence */
	put_32 (&cmd, 0xB00BFACE); /* #-)) */
	put_32 (&cmd, len8*8 + 32);
	put_32 (&cmd, 0x20534d4d); /* protocol type "MMS " */
	put_32 (&cmd, len8 + 4);
	put_32 (&cmd, seq_num);
	seq_num++;
	put_32 (&cmd, 0x0);        /* unknown */
	put_32 (&cmd, 0x0);
	put_32 (&cmd, len8+2);
	put_32 (&cmd, 0x00030000 | command); /* dir | command */
	put_32 (&cmd, switches);
	put_32 (&cmd, extra);

	memcpy (&cmd.buf[48], data, length);
	if (length & 7)
		memset(&cmd.buf[48 + length], 0, 8 - (length & 7));

	if (send (s, cmd.buf, len8*8+48, 0) != (len8*8+48)) {
		gxlogf("%s [%d]: mmst send len error!\n", __FUNCTION__, __LINE__);
	}
}

#ifdef CONFIG_ICONV
static iconv_t url_conv;
#endif

static void string_utf16(char *dest, char *src, int len) {
	int i;
#ifdef CONFIG_ICONV
	size_t len1, len2;
	char *ip, *op;

	if (url_conv != (iconv_t)(-1))
	{
		memset(dest, 0, 1000);
		len1 = len; len2 = 1000;
		ip = src; op = dest;

		iconv(url_conv, &ip, &len1, &op, &len2);
	}
	else
	{
#endif
		if (len > 499) len = 499;
		for (i=0; i<len; i++) {
			dest[i*2] = src[i];
			dest[i*2+1] = 0;
		}
		/* trailing zeroes */
		dest[i*2] = 0;
		dest[i*2+1] = 0;
#ifdef CONFIG_ICONV
	}
#endif
}

static void get_answer (int s)
{
	char mms_answer_data[BUF_SIZE]={0};
	int   command = 0x1b;

	while (command == 0x1b) {
		int len;
		len = recv (s, mms_answer_data, BUF_SIZE, 0) ;
		if (!len) {
			gxlogf("%s [%d]: mmst recv len error!\n", __FUNCTION__, __LINE__);
			return;
		}

		command = get_32 (mms_answer_data, 36) & 0xFFFF;

		if (command == 0x1b)
			send_command (s, 0x1b, 0, 0, 0, mms_answer_data);
	}
}

static int get_data (int s, unsigned char *buf, size_t count)
{
	ssize_t  len;
	size_t total = 0;

	while (total < count) {

		len = recv (s, (char*)&buf[total], count-total, 0);

		if (len < 0) {
			gxlogf("read error:\n");
			return 0;
		}

		total += len;
	}

	return 1;

}

static int get_header (int s, uint8_t *header, GxStreamingCtrl *streaming_ctrl)
{
	unsigned char  pre_header[8];
	int            header_len;

	header_len = 0;

	while (1) {
		if (!get_data (s, pre_header, 8)) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return 0;
		}
		if (pre_header[4] == 0x02) {

			int packet_len;

			packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;


			if (packet_len < 0 || packet_len > HDR_BUF_SIZE - header_len) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return 0;
			}

			if (!get_data (s, &header[header_len], packet_len)) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return 0;
			}

			header_len += packet_len;

			if ( (header[header_len-1] == 1) && (header[header_len-2]==1)) {


				if( streaming_bufferize( streaming_ctrl, (char*)header, header_len )<0 ) {
					return -1;
				}


				return header_len;

			}

		} else {

			int32_t packet_len;
			int command;
			char mms_header_data[BUF_SIZE]={0};

			if (!get_data (s, (unsigned char*)&packet_len, 4)) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return 0;
			}
			packet_len = get_32 ((char*)&packet_len, 0) + 4;
			if (packet_len < 0 || packet_len > BUF_SIZE) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return 0;
			}
			if (!get_data (s, (unsigned char*)mms_header_data, packet_len)) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return 0;
			}
			command = get_32 (mms_header_data, 24) & 0xFFFF;
			if (command == 0x1b)
				send_command (s, 0x1b, 0, 0, 0, mms_header_data);
		}
	}
}

static int interp_header (uint8_t *header, int header_len)
{
	int i;
	int packet_length=-1;

	/*
	 * parse header
	 */

	i = 30;
	while (i<header_len) {

		uint64_t  guid_1, guid_2, length;

		guid_2 = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8)
			| ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
			| ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
			| ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);
		i += 8;

		guid_1 = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8)
			| ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
			| ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
			| ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);
		i += 8;


		length = (uint64_t)header[i] | ((uint64_t)header[i+1]<<8)
			| ((uint64_t)header[i+2]<<16) | ((uint64_t)header[i+3]<<24)
			| ((uint64_t)header[i+4]<<32) | ((uint64_t)header[i+5]<<40)
			| ((uint64_t)header[i+6]<<48) | ((uint64_t)header[i+7]<<56);

		i += 8;

		if ( (guid_1 == 0x6cce6200aa00d9a6ULL) && (guid_2 == 0x11cf668e75b22630ULL) ) {
			gxlogf("%s [%d]: header object\n", __FUNCTION__, __LINE__);
		} else if ((guid_1 == 0x6cce6200aa00d9a6ULL) && (guid_2 == 0x11cf668e75b22636ULL)) {
			gxlogf("%s [%d]: data object\n", __FUNCTION__, __LINE__);
		} else if ((guid_1 == 0x6553200cc000e48eULL) && (guid_2 == 0x11cfa9478cabdca1ULL)) {

			packet_length = get_32((char*)header, i+92-24);
			gxlogf("%s [%d]: packet(%d)(%d)\n", __FUNCTION__, __LINE__, packet_length, get_32((char*)header, i+96-24));


		} else if ((guid_1 == 0x6553200cc000e68eULL) && (guid_2 == 0x11cfa9b7b7dc0791ULL)) {

			int stream_id = header[i+48] | header[i+49] << 8;

			gxlogf("%s [%d]: stream_id(%d)\n", __FUNCTION__, __LINE__, stream_id);

			if (num_stream_ids < MMS_MAX_STREAMS) {
				stream_ids[num_stream_ids] = stream_id;
				num_stream_ids++;
			} else {
				gxlogf("%s [%d]: many stream_id\n", __FUNCTION__, __LINE__);
			}

		} else {
#if 0
			int b = i;
			gxlogf ("unknown object (guid: %016llx, %016llx, len: %lld)\n", guid_1, guid_2, length);
			for (; b < length; b++)
			{
				if (isascii(header[b]) || isalpha(header[b]))
					gxlogf("%c ", header[b]);
				else
					gxlogf("%x ", header[b]);
			}
			gxlogf("\n");
#else
			gxlogf("%s [%d]: unknown object\n", __FUNCTION__, __LINE__);
#endif
		}


		i += length-24;

	}

	return packet_length;

}


static int get_media_packet (int s, int padding, GxStreamingCtrl *stream_ctrl) {
	unsigned char  pre_header[8];
	char mms_packet_data[BUF_SIZE]={0};

	if (!get_data (s, pre_header, 8)) {
		gxlogf("%s [%d]: pre header error\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if (pre_header[4] == 0x04) {

		int packet_len;

		packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;


		if (packet_len < 0 || packet_len > BUF_SIZE) {
			gxlogf("%s [%d]: invalid packet len\n", __FUNCTION__, __LINE__);
			return 0;
		}

		memset(mms_packet_data, 0, BUF_SIZE);
		if (!get_data (s, (unsigned char*)mms_packet_data, packet_len)) {
			gxlogf("%s [%d]: media data read failed\n", __FUNCTION__, __LINE__);
			return 0;
		}

		streaming_bufferize(stream_ctrl, mms_packet_data, padding);

	} else {

		int32_t packet_len;
		int command;

		if (!get_data (s, (unsigned char*)&packet_len, 4)) {
			gxlogf("%s [%d]: media data read failed\n", __FUNCTION__, __LINE__);
			return 0;
		}

		packet_len = get_32 ((char*)&packet_len, 0) + 4;

		if (packet_len < 0 || packet_len > BUF_SIZE) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return 0;
		}

		memset(mms_packet_data, 0, BUF_SIZE);
		if (!get_data (s, (unsigned char*)mms_packet_data, packet_len)) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return 0;
		}

		if ( (pre_header[7] != 0xb0) || (pre_header[6] != 0x0b)
				|| (pre_header[5] != 0xfa) || (pre_header[4] != 0xce) ) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return -1;
		}

		command = get_32 (mms_packet_data, 24) & 0xFFFF;

		if (command == 0x1b)
			send_command (s, 0x1b, 0, 0, 0, mms_packet_data);
		else if (command == 0x1e) {
			gxlogf("%s [%d]: patented technology joke\n", __FUNCTION__, __LINE__);
			return 0;
		}
		else if (command == 0x21 ) {
			// Looks like it's new in WMS9
			// Unknown command, but ignoring it seems to work.
			return 0;
		}
		else if (command != 0x05) {
			gxlogf("%s [%d]: unknown command\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	return 1;
}


static int packet_length1;

static int asf_mmst_streaming_read( int fd, uint8_t *buffer, size_t size, GxStreamingCtrl *stream_ctrl )
{
	int len;

	while( stream_ctrl->buffer_size==0 ) {
		// buffer is empty - fill it!
		int ret = get_media_packet( fd, packet_length1, stream_ctrl);
		if( ret<0 ) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return -1;
		} else if (ret==0) //EOF?
			return ret;
	}

	len = stream_ctrl->buffer_size-stream_ctrl->buffer_pos;
	if(len>size) len=size;
	memcpy( buffer, (stream_ctrl->buffer)+(stream_ctrl->buffer_pos), len );
	stream_ctrl->buffer_pos += len;
	if( stream_ctrl->buffer_pos>=stream_ctrl->buffer_size ) {
		av_free( stream_ctrl->buffer );
		stream_ctrl->buffer = NULL;
		stream_ctrl->buffer_size = 0;
		stream_ctrl->buffer_pos = 0;
	}
	return len;

}

static int asf_mmst_streaming_seek( int fd, off_t pos, GxStreamingCtrl *streaming_ctrl )
{
	return -1;
	// Shut up gcc warning
	fd++;
	pos++;
	streaming_ctrl=NULL;
}

int asf_mmst_streaming_start(GxStream *stream)
{
	char                 str[1024];
	int                  asf_header_len;
	int                  len, i, packet_length;
	char                *path, *unescpath;
	GxURL *url1 = stream->streaming_ctrl->url;
	int s = stream->fd;
	char mms_data[BUF_SIZE] = {0};
	uint8_t asf_header[HDR_BUF_SIZE] = {0};

	if( s>0 ) {
		closesocket( stream->fd );
		stream->fd = -1;
	}

	/* parse url */
	path = strchr(url1->file,'/') + 1;

	/* mmst filename are not url_escaped by MS MediaPlayer and are expected as
	 * "plain text" by the server, so need to decode it here
	 */
	unescpath=av_malloc(strlen(path)+1);
	if (!unescpath) {
		gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	url_unescape_string(unescpath,path);
	path=unescpath;


	if( url1->port==0 ) {
		url1->port=1755;
	}
	s = connect_2_server( url1->hostname, url1->port, 1, stream->interrupt_cbk);
	if( s<0 ) {
		av_free(path);
		return s;
	}
	fcntl(s, F_SETFL, fcntl(s, F_GETFL)&~O_NONBLOCK);

	seq_num=0;

	/*
	 * Send the initial connect info including player version no. Client GUID (random) and the host address being connected to.
	 * This command is sent at the very start of protocol initiation. It sends local information to the serve
	 * cmd 1 0x01
	 * */

	/* prepare for the url encoding conversion */
#ifdef CONFIG_ICONV
	url_conv = iconv_open("UTF-16LE", "UTF-8");
#endif

	memset(mms_data, 0 , BUF_SIZE);
	snprintf (str, 1023, "\034\003NSPlayer/7.0.0.1956; {33715801-BAB3-9D85-24E9-03B90328270A}; Host: %s", url1->hostname);
	string_utf16 (mms_data, str, strlen(str));
	// send_command(s, commandno ....)
	send_command (s, 1, 0, 0x0004000b, strlen(str)*2+2, mms_data);

	len = recv (s, mms_data, BUF_SIZE, 0) ;

	/*This sends details of the local machine IP address to a Funnel system at the server.
	 * Also, the TCP or UDP transport selection is sent.
	 *
	 * here 192.168.0.1 is local ip address TCP/UDP states the tronsport we r using
	 * and 1037 is the  local TCP or UDP socket number
	 * cmd 2 0x02
	 *  */

	string_utf16 (&mms_data[8], "\002\000\\\\192.168.0.1\\TCP\\1037", 24);
	memset (mms_data, 0, 8);
	send_command (s, 2, 0, 0, 24*2+10, mms_data);

	len = recv (s, mms_data, BUF_SIZE, 0) ;

	/* This command sends file path (at server) and file name request to the server.
	 * 0x5 */

	string_utf16 (&mms_data[8], path, strlen(path));
	memset (mms_data, 0, 8);
	send_command (s, 5, 0, 0, strlen(path)*2+10, mms_data);
	av_free(path);

	get_answer (s);

	/* The ASF header chunk request. Includes ?session' variable for pre header value.
	 * After this command is sent,
	 * the server replies with 0x11 command and then the header chunk with header data follows.
	 * 0x15 */

	memset (mms_data, 0, 40);
	mms_data[32] = 2;

	send_command (s, 0x15, 1, 0, 40, mms_data);

	num_stream_ids = 0;
	/* get_headers(s, asf_header);  */

	asf_header_len = get_header (s, asf_header, stream->streaming_ctrl);
	if (asf_header_len==0) { //error reading header
		closesocket(s);
		return -1;
	}
	packet_length = interp_header (asf_header, asf_header_len);


	/*
	 * This command is the media stream MBR selector. Switches are always 6 bytes in length.
	 * After all switch elements, data ends with bytes [00 00] 00 20 ac 40 [02].
	 * Where:
	 * [00 00] shows 0x61 0x00 (on the first 33 sent) or 0xff 0xff for ASF files, and with no ending data for WMV files.
	 * It is not yet understood what all this means.
	 * And the last [02] byte is probably the header ?session' value.
	 *
	 *  0x33 */

	memset (mms_data, 0, 40);

	//  if (audio_id > 0) {
	//    data[2] = 0xFF;
	//    data[3] = 0xFF;
	//    data[4] = audio_id;
	//    send_command(s, 0x33, num_stream_ids, 0xFFFF | audio_id << 16, 8, data);
	//  } else {
	for (i=1; i<num_stream_ids; i++) {
		mms_data [ (i-1) * 6 + 2 ] = 0xFF;
		mms_data [ (i-1) * 6 + 3 ] = 0xFF;
		mms_data [ (i-1) * 6 + 4 ] = stream_ids[i];
		mms_data [ (i-1) * 6 + 5 ] = 0x00;
	}

	send_command (s, 0x33, num_stream_ids, 0xFFFF | stream_ids[0] << 16, (num_stream_ids-1)*6+2 , mms_data);
	//  }

	get_answer (s);

	/* Start sending file from packet xx.
	 * This command is also used for resume downloads or requesting a lost packet.
	 * Also used for seeking by sending a play point value which seeks to the media time point.
	 * Includes ?session' value in pre header and the maximum media stream time.
	 * 0x07 */

	memset (mms_data, 0, 40);

	for (i=8; i<16; i++)
		mms_data[i] = 0xFF;

	mms_data[20] = 0x04;

	send_command (s, 0x07, 1, 0xFFFF | stream_ids[0] << 16, 24, mms_data);

	stream->fd = s;
	stream->streaming_ctrl->streaming_read = asf_mmst_streaming_read;
	stream->streaming_ctrl->streaming_seek = asf_mmst_streaming_seek;
	stream->streaming_ctrl->buffering = 1;
	stream->streaming_ctrl->status = streaming_playing_e;

	packet_length1 = packet_length;

	return 0;
}

static int asf_http_streaming_start(GxStream *stream, int *demuxer_type);

static int asf_read_wrapper(int fd, void *buffer, int len, GxStreamingCtrl *stream_ctrl) {
	uint8_t *buf = buffer;
	while (len > 0) {
		int got = nop_streaming_read(fd, buf, len, stream_ctrl);
		if (got <= 0) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return got;
		}
		buf += got;
		len -= got;
	}
	return 1;
}

// We can try several protocol for asf streaming
// * first the UDP protcol, if there is a firewall, UDP
//   packets will not come back, so the mmsu will fail.
// * Then we can try TCP, but if there is a proxy for
//   internet connection, the TCP connection will not get
//   through
// * Then we can try HTTP.
//
// Note: Using 	WMP sequence  MMSU then MMST and then HTTP.

static int asf_streaming_start( GxStream *stream, int *demuxer_type) {
	char *proto = stream->streaming_ctrl->url->protocol;
	int fd = -1;
	int port = stream->streaming_ctrl->url->port;

	// Is protocol mms or mmsu?
	if (!strcasecmp(proto, "mmsu") || !strcasecmp(proto, "mms"))
	{
		//fd = asf_mmsu_streaming_start( stream );
		if( fd>-1 ) return fd; //mmsu support is not implemented yet - using this code
		if( fd==-2 ) return -1;
	}

	//Is protocol mms or mmst?
	if (!strcasecmp(proto, "mmst") || !strcasecmp(proto, "mms"))
	{
		fd = asf_mmst_streaming_start( stream );
		stream->streaming_ctrl->url->port = port;
		if( fd>-1 ) return fd;
		if( fd==-2 ) return -1;
	}

	//Is protocol http, http_proxy, or mms?
	if (!strcasecmp(proto, "mms") || !strcasecmp(proto, "mmsh") ||
			!strcasecmp(proto, "mmshttp"))
	{
		fd = asf_http_streaming_start( stream, demuxer_type );
		stream->streaming_ctrl->url->port = port;
		if( fd>-1 ) return fd;
		if( fd==-2 ) return -1;
	}

	//everything failed
	return -1;
}

static int asf_streaming(ASF_stream_chunck_t *stream_chunck, int *drop_packet ) {
	/*
	   gxlogf("ASF stream chunck size=%d\n", stream_chunck->size);
	   gxlogf("length: %d\n", length );
	   gxlogf("0x%02X\n", stream_chunck->type );
	   */
	if( drop_packet!=NULL ) *drop_packet = 0;

	if( stream_chunck->size<8 ) {
		gxlogf("%s [%d] chunk_size (%d) error\n",__FUNCTION__, __LINE__,stream_chunck->size);
		return -1;
	}
	if( stream_chunck->size!=stream_chunck->size_confirm ) {
		gxlogf("%s [%d] chunk_size (%d) error\n",__FUNCTION__, __LINE__,stream_chunck->size);
		return -1;
	}
	/*
	   gxlogf("  type: 0x%02X\n", stream_chunck->type );
	   gxlogf("  size: %d (0x%02X)\n", stream_chunck->size, stream_chunck->size );
	   gxlogf("  sequence_number: 0x%04X\n", stream_chunck->sequence_number );
	   gxlogf("  unknown: 0x%02X\n", stream_chunck->unknown );
	   gxlogf("  size_confirm: 0x%02X\n", stream_chunck->size_confirm );
	   */
	switch(stream_chunck->type) {
		case ASF_STREAMING_CLEAR:	// $C	Clear ASF configuration
			gxlogf("=====> Clearing ASF stream configuration!\n");
			if( drop_packet!=NULL ) *drop_packet = 1;
			return stream_chunck->size;
			break;
		case ASF_STREAMING_DATA:	// $D	Data follows
			//			gxlogf("=====> Data follows\n");
			break;
		case ASF_STREAMING_END_TRANS:	// $E	Transfer complete
			gxlogf("=====> Transfer complete\n");
			if( drop_packet!=NULL ) *drop_packet = 1;
			return stream_chunck->size;
			break;
		case ASF_STREAMING_HEADER:	// $H	ASF header chunk follows
			gxlogf("=====> ASF header chunk follows\n");
			break;
		default:
			gxlogf("=====> Unknown stream type 0x%x\n", stream_chunck->type );
	}
	return stream_chunck->size+4;
}

static int asf_streaming_parse_header(int fd, GxStreamingCtrl* streaming_ctrl) {
	ASF_stream_chunck_t chunk;
	asf_http_streaming_ctrl_t* asf_ctrl = streaming_ctrl->data;
	char* buffer=NULL, *chunk_buffer=NULL;
	int i,r,size,pos = 0;
	int start;
	int buffer_size = 0;
	int chunk_size2read = 0;
	int bw = streaming_ctrl->bandwidth;
	int *v_rates = NULL, *a_rates = NULL;
	int v_rate = 0, a_rate = 0, a_idx = -1, v_idx = -1;

	if(asf_ctrl == NULL) return -1;

	// The ASF header can be in several network chunks. For example if the content description
	// is big, the ASF header will be split in 2 network chunk.
	// So we need to retrieve all the chunk before starting to parse the header.
	do {
		if (asf_read_wrapper(fd, &chunk, sizeof(ASF_stream_chunck_t), streaming_ctrl) <= 0)
			return -1;
		// Endian handling of the stream chunk
		le2me_ASF_stream_chunck_t(&chunk);
		size = asf_streaming( &chunk, &r) - sizeof(ASF_stream_chunck_t);
		if(r) gxlogf("%s [%d]: Warn Drop Header\n", __FUNCTION__, __LINE__);
		if(size < 0){
			gxlogf("%s [%d]: Error Parse ChunkHeader\n", __FUNCTION__, __LINE__);
			return -1;
		}
		if (chunk.type != ASF_STREAMING_HEADER) {
			gxlogf("%s [%d]: No Header At First Chunk\n", __FUNCTION__, __LINE__);
			return -1;
		}

		// audit: do not overflow buffer_size
		if (size > SIZE_MAX - buffer_size) return -1;
		buffer = av_malloc(size+buffer_size);
		if(buffer == NULL) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			return -1;
		}
		if( chunk_buffer!=NULL ) {
			memcpy( buffer, chunk_buffer, buffer_size );
			av_free( chunk_buffer );
		}
		chunk_buffer = buffer;
		buffer += buffer_size;
		buffer_size += size;

		if (asf_read_wrapper(fd, buffer, size, streaming_ctrl) <= 0)
			return -1;

		if( chunk_size2read==0 ) {
			ASF_header_t *asfh = (ASF_header_t *)buffer;
			if(size < (int)sizeof(ASF_header_t)) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return -1;
			} else gxlogf("Got chunk\n");
			chunk_size2read = AV_RL64(&asfh->objh.size);
			gxlogf("Size 2 read=%d\n", chunk_size2read);
		}
	} while( buffer_size<chunk_size2read);
	buffer = chunk_buffer;
	size = buffer_size;

	start = sizeof(ASF_header_t);

	pos = find_asf_guid(buffer, asf_file_header_guid, start, size);
	if (pos >= 0) {
		ASF_file_header_t *fileh = (ASF_file_header_t *) &buffer[pos];
		pos += sizeof(ASF_file_header_t);
		if (pos > size) goto len_err_out;
		/*
		   if(fileh.packetsize != fileh.packetsize2) {
		   gxlogf("Error packetsize check don't match\n");
		   return -1;
		   }
		   */
		asf_ctrl->packet_size = AV_RL32(&fileh->max_packet_size);
		// before playing.
		// preroll: time in ms to bufferize before playing
		streaming_ctrl->prebuffer_size = (unsigned int)(((double)fileh->preroll/1000.0)*((double)fileh->max_bitrate/8.0));
	}

	pos = start;
	while ((pos = find_asf_guid(buffer, asf_stream_header_guid, pos, size)) >= 0)
	{
		ASF_stream_header_t *streamh = (ASF_stream_header_t *)&buffer[pos];
		pos += sizeof(ASF_stream_header_t);
		if (pos > size) goto len_err_out;
		switch(ASF_LOAD_GUID_PREFIX(streamh->type)) {
			case 0xF8699E40 : // audio stream
				if(asf_ctrl->audio_streams == NULL){
					asf_ctrl->audio_streams = av_malloc(sizeof(int));
					asf_ctrl->n_audio = 1;
				} else {
					asf_ctrl->n_audio++;
					asf_ctrl->audio_streams = av_realloc(asf_ctrl->audio_streams,
							asf_ctrl->n_audio*sizeof(int));
				}
				asf_ctrl->audio_streams[asf_ctrl->n_audio-1] = AV_RL16(&streamh->stream_no);
				break;
			case 0xBC19EFC0 : // video stream
				if(asf_ctrl->video_streams == NULL){
					asf_ctrl->video_streams = av_malloc(sizeof(int));
					asf_ctrl->n_video = 1;
				} else {
					asf_ctrl->n_video++;
					asf_ctrl->video_streams = av_realloc(asf_ctrl->video_streams,
							asf_ctrl->n_video*sizeof(int));
				}
				asf_ctrl->video_streams[asf_ctrl->n_video-1] = AV_RL16(&streamh->stream_no);
				break;
		}
	}

	// always allocate to avoid lots of ifs later
	v_rates = av_calloc(asf_ctrl->n_video, sizeof(int));
	a_rates = av_calloc(asf_ctrl->n_audio, sizeof(int));

	pos = find_asf_guid(buffer, asf_stream_group_guid, start, size);
	if (pos >= 0) {
		// stream bitrate properties object
		int stream_count;
		char *ptr = &buffer[pos];
		char *end = &buffer[size];

		gxlogf("Stream bitrate properties object\n");
		if (ptr + 2 > end) goto len_err_out;
		stream_count = AV_RL16(ptr);
		ptr += 2;
		gxlogf(" stream count=[0x%x][%u]\n", stream_count, stream_count );
		for( i=0 ; i<stream_count ; i++ ) {
			uint32_t rate;
			int id;
			int j;
			if (ptr + 6 > end) goto len_err_out;
			id = AV_RL16(ptr);
			ptr += 2;
			rate = AV_RL32(ptr);
			ptr += 4;
			gxlogf("  stream id=[0x%x][%u]\n", id, id);
			gxlogf("  max bitrate=[0x%x][%u]\n", rate, rate);
			for (j = 0; j < asf_ctrl->n_video; j++) {
				if (id == asf_ctrl->video_streams[j]) {
					gxlogf("  is video stream\n");
					v_rates[j] = rate;
					break;
				}
			}
			for (j = 0; j < asf_ctrl->n_audio; j++) {
				if (id == asf_ctrl->audio_streams[j]) {
					gxlogf("  is audio stream\n");
					a_rates[j] = rate;
					break;
				}
			}
		}
	}
	av_free(buffer);

	// automatic stream selection based on bandwidth
	if (bw == 0) bw = INT_MAX;
	gxlogf("Max bandwidth set to %d\n", bw);

	if (asf_ctrl->n_audio) {
		// find lowest-bitrate audio stream
		a_rate = a_rates[0];
		a_idx = 0;
		for (i = 0; i < asf_ctrl->n_audio; i++) {
			if (a_rates[i] < a_rate) {
				a_rate = a_rates[i];
				a_idx = i;
			}
		}
		if (max_idx(asf_ctrl->n_video, v_rates, bw - a_rate) < 0) {
			// both audio and video are not possible, try video only next
			a_idx = -1;
			a_rate = 0;
		}
	}
	// find best video stream
	v_idx = max_idx(asf_ctrl->n_video, v_rates, bw - a_rate);
	if (v_idx >= 0)
		v_rate = v_rates[v_idx];

	// find best audio stream
	a_idx = max_idx(asf_ctrl->n_audio, a_rates, bw - v_rate);

	av_free(v_rates);
	av_free(a_rates);

	if (a_idx < 0 && v_idx < 0) {
		gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	//  if (audio_id > 0)
	// a audio stream was forced
	//    asf_ctrl->audio_id = audio_id;
	if (a_idx >= 0)
		asf_ctrl->audio_id = asf_ctrl->audio_streams[a_idx];
	//  else if (asf_ctrl->n_audio) {
	//    mp_msg(MSGT_NETWORK, MSGL_WARN, MSGTR_MPDEMUX_ASF_Bandwidth2SmallDeselectedAudio);
	//    audio_id = -2;
	//  }

	//  if (video_id > 0)
	// a video stream was forced
	//    asf_ctrl->video_id = video_id;
	if (v_idx >= 0)
		asf_ctrl->video_id = asf_ctrl->video_streams[v_idx];
	//  else if (asf_ctrl->n_video) {
	//    mp_msg(MSGT_NETWORK, MSGL_WARN, MSGTR_MPDEMUX_ASF_Bandwidth2SmallDeselectedVideo);
	//    video_id = -2;
	//  }

	return 1;

len_err_out:
	gxlogf("%s [%d]: Invalid Len In Header\n", __FUNCTION__, __LINE__);
	av_free(buffer);
	av_free(v_rates);
	av_free(a_rates);
	return -1;
}

static int asf_http_streaming_read( int fd, uint8_t *buffer, size_t size, GxStreamingCtrl *streaming_ctrl ) {
	static ASF_stream_chunck_t chunk;
	int read,chunk_size = 0;
	static int rest = 0, drop_chunk = 0, waiting = 0;
	asf_http_streaming_ctrl_t *asf_http_ctrl = (asf_http_streaming_ctrl_t*)streaming_ctrl->data;

	while(1) {
		if (rest == 0 && waiting == 0) {
			if (asf_read_wrapper(fd, &chunk, sizeof(ASF_stream_chunck_t), streaming_ctrl) <= 0)
				return -1;

			// Endian handling of the stream chunk
			le2me_ASF_stream_chunck_t(&chunk);
			chunk_size = asf_streaming( &chunk, &drop_chunk );
			if(chunk_size < 0) {
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				return -1;
			}
			chunk_size -= sizeof(ASF_stream_chunck_t);

			if(chunk.type != ASF_STREAMING_HEADER && (!drop_chunk)) {
				if (asf_http_ctrl->packet_size < chunk_size) {
					gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
					return -1;
				}
				waiting = asf_http_ctrl->packet_size;
			} else {
				waiting = chunk_size;
			}

		} else if (rest){
			chunk_size = rest;
			rest = 0;
		}

		read = 0;
		if ( waiting >= chunk_size) {
			if (chunk_size > size){
				rest = chunk_size - size;
				chunk_size = size;
			}
			if (asf_read_wrapper(fd, buffer, chunk_size, streaming_ctrl) <= 0)
				return -1;
			read = chunk_size;
			waiting -= read;
			if (drop_chunk) continue;
		}
		if (rest == 0 && waiting > 0 && size-read > 0) {
			int s = FFMIN(waiting,size-read);
			memset(buffer+read,0,s);
			waiting -= s;
			read += s;
		}
		break;
	}

	return read;
}

static int asf_http_streaming_seek( int fd, off_t pos, GxStreamingCtrl *streaming_ctrl ) {
	return -1;
	// to shut up gcc warning
	fd++;
	pos++;
	streaming_ctrl=NULL;
}

static int asf_header_check( GxHttpHeader *http_hdr ) {
	ASF_obj_header_t *objh;
	if( http_hdr==NULL ) return -1;
	if( http_hdr->body==NULL || http_hdr->body_size<sizeof(ASF_obj_header_t) ) return -1;

	objh = (ASF_obj_header_t*)http_hdr->body;
	if( ASF_LOAD_GUID_PREFIX(objh->guid)==0x75B22630 ) return 0;
	return -1;
}

static int asf_http_streaming_type(char *content_type, char *features, GxHttpHeader *http_hdr ) {
	if( content_type==NULL ) return ASF_Unknown_e;
	if( 	!strcasecmp(content_type, "application/octet-stream") ||
			!strcasecmp(content_type, "application/vnd.ms.wms-hdr.asfv1") ||        // New in Corona, first request
			!strcasecmp(content_type, "application/x-mms-framed") ||                // New in Corana, second request
			!strcasecmp(content_type, "video/x-ms-wmv") ||
			!strcasecmp(content_type, "video/x-ms-asf")) {

		if( strstr(features, "broadcast") ) {
			gxlogf("=====> ASF Live stream\n");
			return ASF_Live_e;
		} else {
			gxlogf("=====> ASF Prerecorded\n");
			return ASF_Prerecorded_e;
		}
	} else {
		// Ok in a perfect world, web servers should be well configured
		// so we could used mime type to know the stream type,
		// but guess what? All of them are not well configured.
		// So we have to check for an asf header :(, but it works :p
		if( http_hdr->body_size>sizeof(ASF_obj_header_t) ) {
			if( asf_header_check( http_hdr )==0 ) {
				gxlogf("=====> ASF Plain text\n");
				return ASF_PlainText_e;
			} else if( (!strcasecmp(content_type, "text/html")) ) {
				gxlogf("=====> HTML, MPlayer is not a browser...yet!\n");
				return ASF_Unknown_e;
			} else {
				gxlogf("=====> ASF Redirector\n");
				return ASF_Redirector_e;
			}
		} else {
			if(	(!strcasecmp(content_type, "audio/x-ms-wax")) ||
					(!strcasecmp(content_type, "audio/x-ms-wma")) ||
					(!strcasecmp(content_type, "video/x-ms-asf")) ||
					(!strcasecmp(content_type, "video/x-ms-afs")) ||
					(!strcasecmp(content_type, "video/x-ms-wmv")) ||
					(!strcasecmp(content_type, "video/x-ms-wma")) ) {
				gxlogf("%s [%d]:Redirector\n", __FUNCTION__, __LINE__);
				return ASF_Redirector_e;
			} else if( !strcasecmp(content_type, "text/plain") ) {
				gxlogf("=====> ASF Plain text\n");
				return ASF_PlainText_e;
			} else {
				gxlogf("=====> ASF unknown content-type: %s\n", content_type );
				return ASF_Unknown_e;
			}
		}
	}
	return ASF_Unknown_e;
}

static GxHttpHeader *asf_http_request(GxStreamingCtrl *streaming_ctrl) {
	GxHttpHeader *http_hdr;
	GxURL *url = NULL;
	GxURL *server_url = NULL;
	asf_http_streaming_ctrl_t *asf_http_ctrl;
	char str[250] ={0};
	char *ptr;
	int i, enable;

	int offset_hi=0, offset_lo=0, length=0;
	int asf_nb_stream=0, stream_id;

	// Sanity check
	if( streaming_ctrl==NULL ) return NULL;
	url = streaming_ctrl->url;
	asf_http_ctrl = (asf_http_streaming_ctrl_t*)streaming_ctrl->data;
	if( url==NULL || asf_http_ctrl==NULL ) return NULL;

	// Common header for all requests.
	http_hdr = http_new_header();
	http_set_field( http_hdr, "Accept: */*" );
	http_set_field( http_hdr, "User-Agent: NSPlayer/4.1.0.3856" );
	http_add_basic_authentication( http_hdr, url->username, url->password );

	// Check if we are using a proxy
	if( !strcasecmp( url->protocol, "http_proxy" ) ) {
		server_url = url_new( (url->file)+1 );
		if( server_url==NULL ) {
			gxlogf("%s [%d]: Invalid ProxyURL\n", __FUNCTION__, __LINE__);
			http_free( http_hdr );
			return NULL;
		}
		http_set_uri( http_hdr, server_url->url );
		snprintf( str, sizeof(str), "Host: %.220s:%d", server_url->hostname, server_url->port );
		url_free( server_url );
	} else {
		http_set_uri( http_hdr, url->file );
		snprintf( str, sizeof(str), "Host: %.220s:%d", url->hostname, url->port );
	}

	http_set_field( http_hdr, str );
	http_set_field( http_hdr, "Pragma: xClientGUID={c77e7400-738a-11d2-9add-0020af0a3278}" );
	snprintf(str, sizeof(str),
			"Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=%u:%u,request-context=%d,max-duration=%u",
			offset_hi, offset_lo, asf_http_ctrl->request, length );
	http_set_field( http_hdr, str );

	switch( asf_http_ctrl->streaming_type ) {
		case ASF_Live_e:
		case ASF_Prerecorded_e:
			http_set_field( http_hdr, "Pragma: xPlayStrm=1" );
			ptr = str;
			ptr += snprintf( ptr, , sizeof(str), "Pragma: stream-switch-entry=");
			if(asf_http_ctrl->n_audio > 0) {
				for( i=0; i<asf_http_ctrl->n_audio ; i++ ) {
					stream_id = asf_http_ctrl->audio_streams[i];
					if(stream_id == asf_http_ctrl->audio_id) {
						enable = 0;
					} else {
						enable = 2;
						continue;
					}
					asf_nb_stream++;
					ptr += snprintf(ptr, , sizeof(str), "ffff:%x:%d ", stream_id, enable);
				}
			}
			if(asf_http_ctrl->n_video > 0) {
				for( i=0; i<asf_http_ctrl->n_video ; i++ ) {
					stream_id = asf_http_ctrl->video_streams[i];
					if(stream_id == asf_http_ctrl->video_id) {
						enable = 0;
					} else {
						enable = 2;
						continue;
					}
					asf_nb_stream++;
					ptr += snprintf(ptr, sizeof(str), "ffff:%x:%d ", stream_id, enable);
				}
			}
			http_set_field( http_hdr, str );
			snprintf( str, sizeof(str), "Pragma: stream-switch-count=%d", asf_nb_stream );
			http_set_field( http_hdr, str );
			break;
		case ASF_Redirector_e:
			break;
		case ASF_Unknown_e:
			// First request goes here.
			break;
		default:
			gxlogf("%s [%d]: unknown asf stream type\n", __FUNCTION__, __LINE__);
	}

	http_set_field( http_hdr, "Connection: Close" );
	http_build_request( http_hdr );

	return http_hdr;
}

static int asf_http_parse_response(asf_http_streaming_ctrl_t *asf_http_ctrl, GxHttpHeader *http_hdr ) {
	char *content_type, *pragma;
	char features[64] = "\0";
	size_t len;
	if( http_response_parse(http_hdr)<0 ) {
		gxlogf("%s [%d]: failed parse http response\n", __FUNCTION__, __LINE__);
		return -1;
	}
	switch( http_hdr->status_code ) {
		case 200:
			break;
		case 401: // Authentication required
			return ASF_Authenticate_e;
		default:
			gxlogf("%s [%d]: error code(%d)\n", __FUNCTION__, __LINE__, http_hdr->status_code);
			return -1;
	}

	content_type = http_get_field( http_hdr, "Content-Type");
	//gxlogf("Content-Type: [%s]\n", content_type);

	pragma = http_get_field( http_hdr, "Pragma");
	while( pragma!=NULL ) {
		char *comma_ptr=NULL;
		char *end;
		//gxlogf("Pragma: [%s]\n", pragma );
		// The pragma line can get severals attributes
		// separeted with a comma ','.
		do {
			if( !strncasecmp( pragma, "features=", 9) ) {
				pragma += 9;
				end = strstr( pragma, "," );
				if( end==NULL ) {
					len = strlen(pragma);
				} else {
					len = (unsigned int)(end-pragma);
				}
				if(len > sizeof(features) - 1) {
					gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
					len = sizeof(features) - 1;
				}
				strncpy( features, pragma, len );
				features[len]='\0';
				break;
			}
			comma_ptr = strstr( pragma, "," );
			if( comma_ptr!=NULL ) {
				pragma = comma_ptr+1;
				if( pragma[0]==' ' ) pragma++;
			}
		} while( comma_ptr!=NULL );
		pragma = http_get_next_field( http_hdr );
	}
	asf_http_ctrl->streaming_type = asf_http_streaming_type( content_type, features, http_hdr );
	return 0;
}

static int asf_http_streaming_start( GxStream *stream, int *demuxer_type ) {
	GxHttpHeader *http_hdr=NULL;
	GxURL *url = stream->streaming_ctrl->url;
	asf_http_streaming_ctrl_t *asf_http_ctrl;
	char buffer[2048];
	int i, ret;
	int fd = stream->fd;
	int done;
	int auth_retry = 0;

	asf_http_ctrl = av_malloc(sizeof(asf_http_streaming_ctrl_t));
	if( asf_http_ctrl==NULL ) {
		gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	asf_http_ctrl->streaming_type = ASF_Unknown_e;
	asf_http_ctrl->request = 1;
	asf_http_ctrl->audio_streams = asf_http_ctrl->video_streams = NULL;
	asf_http_ctrl->n_audio = asf_http_ctrl->n_video = 0;
	stream->streaming_ctrl->data = (void*)asf_http_ctrl;

	do {
		done = 1;
		if( fd>0 ) closesocket( fd );

		if( !strcasecmp( url->protocol, "http_proxy" ) ) {
			if( url->port==0 ) url->port = 8080;
		} else {
			if( url->port==0 ) url->port = 80;
		}
		fd = connect_2_server( url->hostname, url->port, 1, stream->interrupt_cbk);
		if( fd<0 ) return fd;

		http_hdr = asf_http_request( stream->streaming_ctrl );
		gxlogf("Request [%s]\n", http_hdr->buffer );
		for(i=0; i < (int)http_hdr->buffer_size ; ) {
			int r = send( fd, http_hdr->buffer+i, http_hdr->buffer_size-i, 0);
			if(r <0) {
				gxlogf("%s [%d]: error (%s)\n", __FUNCTION__, __LINE__, strerror(errno));
				goto err_out;
			}
			i += r;
		}
		http_free( http_hdr );
		http_hdr = http_new_header();
		do {
			i = recv( fd, buffer, 2048, 0);
			//gxlogf("read: %d\n", i );
			if( i<=0 ) {
				gxloge ("read");
				goto err_out;
			}
			http_response_append( http_hdr, buffer, i );
		} while( !http_is_header_entire( http_hdr ) );

		http_hdr->buffer[http_hdr->buffer_size]='\0';
		gxlogf("Response [%s]\n", http_hdr->buffer );
		ret = asf_http_parse_response(asf_http_ctrl, http_hdr);
		if( ret<0 ) {
			gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
			goto err_out;
		}
		switch( asf_http_ctrl->streaming_type ) {
			case ASF_Live_e:
			case ASF_Prerecorded_e:
			case ASF_PlainText_e:
				if( http_hdr->body_size>0 ) {
					if( streaming_bufferize( stream->streaming_ctrl, http_hdr->body, http_hdr->body_size )<0 ) {
						goto err_out;
					}
				}
				if( asf_http_ctrl->request==1 ) {
					if( asf_http_ctrl->streaming_type!=ASF_PlainText_e ) {
						// First request, we only got the ASF header.
						ret = asf_streaming_parse_header(fd,stream->streaming_ctrl);
						if(ret < 0) goto err_out;
						if(asf_http_ctrl->n_audio == 0 && asf_http_ctrl->n_video == 0) {
							gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
							goto err_out;
						}
						asf_http_ctrl->request++;
						done = 0;
					} else {
						done = 1;
					}
				}
				break;
			case ASF_Redirector_e:
				if( http_hdr->body_size>0 ) {
					if( streaming_bufferize( stream->streaming_ctrl, http_hdr->body, http_hdr->body_size )<0 ) {
						goto err_out;
					}
				}
				*demuxer_type = GX_DEMUXER_TYPE_PLAYLIST;
				done = 1;
				break;
			case ASF_Authenticate_e:
				if( http_authenticate( http_hdr, url, &auth_retry)<0 ) return -1;
				asf_http_ctrl->streaming_type = ASF_Unknown_e;
				done = 0;
				break;
			case ASF_Unknown_e:
			default:
				gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
				goto err_out;
		}
		// Check if we got a redirect.
	} while(!done);

	stream->fd = fd;
	if( asf_http_ctrl->streaming_type==ASF_PlainText_e || asf_http_ctrl->streaming_type==ASF_Redirector_e ) {
		stream->streaming_ctrl->streaming_read = nop_streaming_read;
		stream->streaming_ctrl->streaming_seek = nop_streaming_seek;
	} else {
		stream->streaming_ctrl->streaming_read = asf_http_streaming_read;
		stream->streaming_ctrl->streaming_seek = asf_http_streaming_seek;
		stream->streaming_ctrl->buffering = 1;
	}
	stream->streaming_ctrl->status = streaming_playing_e;

	http_free( http_hdr );
	return 0;

err_out:
	if (fd > 0)
		closesocket(fd);
	stream->fd = -1;
	http_free(http_hdr);
	return -1;
}

static int mms_open(GxStream *stream,int mode) {
	GxURL *url;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &stream->interrupt_cbk);
	stream->streaming_ctrl = streaming_ctrl_new();
	if( stream->streaming_ctrl==NULL ) {
		return GX_PLAYER_ERROR;
	}
	//	stream->streaming_ctrl->bandwidth = network_bandwidth;
	url = url_new(stream->url);
	stream->streaming_ctrl->url = check4proxies(url);
	url_free(url);

	gxlogf("%s [%d]: url %s\n", __FUNCTION__, __LINE__, stream->url);

	if(asf_streaming_start(stream, &stream->demuxer_type) < 0) {
		gxlogf("%s [%d]: error\n", __FUNCTION__, __LINE__);
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}

	stream->demuxer_type = GX_DEMUXER_TYPE_ASF;
	stream->file_format = GX_STREAMTYPE_STREAM;
	return GX_PLAYER_OK;
}

static int mms_read(GxStream*  stream, uint8_t * buffer, size_t max_len)
{
	int len = 0;

	if (stream != NULL && stream->streaming_ctrl != NULL) {
		//calc really read data len

		len = stream->streaming_ctrl->streaming_read(stream->fd, buffer, max_len,
				stream->streaming_ctrl);
	}

	return len;
}

static int mms_seek(GxStream*  stream, off_t newpos)
{
	return GX_PLAYER_ERROR;
}

static int mms_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if(!streaming_ctrl){
		return GX_PLAYER_ERROR;
	}

	if((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)){
		streaming_ctrl->buffer_pos -= size;
	}else
		streaming_bufferize(streaming_ctrl, buffer, size);
	return GX_PLAYER_OK;
}

static int mms_control(GxStream *s, int cmd, void *arg)
{
	switch(cmd){
		case GX_STREAM_CTRL_ROLL_BACK:
			{
				GxStream_RollBack* rol = arg;
				if(!rol)
					return GX_PLAYER_ERROR;
				mms_rollback(s, rol->buffer, rol->size);
				return GX_PLAYER_OK;
			}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void mms_close(GxStream *stream) {
	if(stream->streaming_ctrl){
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
	}
	if(stream->fd >= 0){
		closesocket(stream->fd);
		stream->fd = -1;
	}
	stream->interrupt_cbk = NULL;
}

GxStreamClass gx_stream_mms = {
	._inherit = {
		._inherit = {
			.name   = "StreamMMS",
			.parent = &gx_streambase,
			.size   = sizeof(GxStream),
		},
	},
	.protocols = {"mms", "mmsu", "mmst", "mmsh", "mmshttp", NULL},
	DEF_AUTHOR("Stream","MMS","No description","L.F","No comment"),

	.flags   = 0,
	.open    = mms_open,
	.read    = mms_read,
	.write   = NULL,
	.seek    = mms_seek,
	.control = mms_control,
	.close   = mms_close,
};
