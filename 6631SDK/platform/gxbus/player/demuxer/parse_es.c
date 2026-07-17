#include "parse_es.h"

int GxParseES_Init(GxParseES*  es, GxDemuxStream * ds)
{
	if (es == NULL)
		return -1;

	es->videobuffer = NULL;
	es->videobuf_len = 0;
	es->next_nal = -1;
	es->videobuf_code_len = 0;
	es->ds = ds;

	return 0;
}

int GxParseES_Reset(GxParseES*  es)
{
	if (es == NULL)
		return -1;

	es->videobuf_len = 0;
	es->next_nal = -1;
	es->videobuf_code_len = 0;

	return 0;
}

int GxParseES_SyncVideoPacket(GxParseES*  es)
{
	if (es == NULL)
		return -1;

	if (!es->videobuf_code_len) {
		int skipped = 0;
		if (!GxDemuxStream_Pattern3(es->ds, NULL, MAX_SYNCLEN, &skipped, 0x100)) {
			goto eof_out;
		}
		es->next_nal = GX_DEMUX_GETC(es->ds);
		if (es->next_nal < 0)
			goto eof_out;
		es->videobuf_code_len = 4;
	}
	return 0x100 | es->next_nal;

eof_out:
	es->next_nal = -1;
	es->videobuf_code_len = 0;

	return 0;
}

int GxParseES_ReadVideoPacket(GxParseES*  es)
{
	int packet_start;
	int res, read;

	if (es == NULL)
		return -1;

	if (VIDEOBUFFER_SIZE - es->videobuf_len < 5)
		return 0;
	// COPY STARTCODE:
	packet_start = es->videobuf_len;
	es->videobuffer[es->videobuf_len + 0] = 0;
	es->videobuffer[es->videobuf_len + 1] = 0;
	es->videobuffer[es->videobuf_len + 2] = 1;
	es->videobuffer[es->videobuf_len + 3] = es->next_nal;
	es->videobuf_len += 4;

	// READ PACKET:
	res =
	    GxDemuxStream_Pattern3(es->ds, &es->videobuffer[es->videobuf_len], VIDEOBUFFER_SIZE - es->videobuf_len,
				   &read, 0x100);
	es->videobuf_len += read;
	if (!res)
		goto eof_out;

	es->videobuf_len -= 3;

	// Save next packet code:
	es->next_nal = GX_DEMUX_GETC(es->ds);
	if (es->next_nal < 0)
		goto eof_out;
	es->videobuf_code_len = 4;

	return es->videobuf_len - packet_start;

      eof_out:
	es->next_nal = -1;
	es->videobuf_code_len = 0;

	return es->videobuf_len - packet_start;
}

int GxParseES_SkipVideoPacket(GxParseES*  es)
{
	es->videobuf_code_len = 0;	// force resync

	return GxParseES_SyncVideoPacket(es);
}
