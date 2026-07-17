/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   $Id: xtr_avc.cpp 3436 2007-01-07 20:13:06Z mosu $

   extracts tracks from Matroska files into other files

   Written by Matt Rice <topquark@sluggy.net>.
*/

#include "gx_common.h"
#include "demux_mkv.h"

static const uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
const int aac_sampling_freq[16] = {96000, 88200, 64000, 48000, 44100, 32000,
                                   24000, 22050, 16000, 12000, 11025,  8000,
                                   0, 0, 0, 0}; // filling


int parse_aac_data(unsigned char* data, int size, struct aac_data *aac)
{
	int profile = 1, channels = 0, sample_rate = 0, output_sample_rate = 0, sbr = 0;

	if (size < 2)
		return -1;

	profile = data[0] >> 3;
	if (0 == profile)
		return -1;
	--profile;
	sample_rate = aac_sampling_freq[((data[0] & 0x07) << 1) | (data[1] >> 7)];
	channels = (data[1] & 0x7f) >> 3;
	if ((5 == profile) && (5 <= size)) {
		output_sample_rate = aac_sampling_freq[(data[4] & 0x7f) >> 3];
		sbr = 1;
	} else if (sample_rate <= 24000) {
		output_sample_rate = 2*  sample_rate;
		sbr = 1;
	} else
		sbr = 0;


	aac->profile            = profile;
	aac->channels           = channels;
	aac->sample_rate        = sample_rate;
	aac->output_sample_rate = output_sample_rate;
	aac->sbr                = sbr;

	return 0;
}

static void write_nal(GxDemuxPacket*  dp, const uint8_t * data, int *ppos, int data_size, int nal_size_size)
{
	int i;
	int nal_size = 0;

	int pos =* ppos;

	if(nal_size_size)
	{
renal:
		for (i = 0; i < nal_size_size; ++i)
			nal_size = (nal_size << 8) | data[pos++];

		if ((pos + nal_size) > data_size){
			gxlogd("*");
			pos-=nal_size_size;
			nal_size_size--;
			nal_size = 0;
			goto renal;
			//GxDemuxPacket_Resize(dp,dp->len + pos + nal_size - data_size);
			nal_size = data_size - pos;
			//return ;
		}
		GxDemuxPacket_Write(dp, start_code, 4);
	}
	else
		nal_size = data_size-pos;

	GxDemuxPacket_Write(dp, data + pos, nal_size);

	pos += nal_size;

	*ppos = pos;
}

void handle_firstframe(DemuxMkvPriv*  mkv, MkvTrack * track, GxDemuxPacket * dp)
{
	int priv_size = track->private_size;
	uint8_t* buf = (uint8_t *) track->private_data;

	int i, pos = 6, numsps, numpps;

	if (buf == NULL){
		gxlogd("Track %d with the CodecID '%s' is missing the \"codec "
		       "private\" element and cannot be extracted.\n", track->tnum, track->codec_id);
		return ;
	}

	if (priv_size < 6)
		gxlogd("Track %d CodecPrivate is too small.\n", track->tnum);

	mkv->nal_size_size = 1 + (buf[4] & 3);

	numsps = buf[5] & 0x1f;
	gxlogf("priva_size=%d, nal_size_size=%d, numsps=%d\n", priv_size, mkv->nal_size_size, numsps);

	for (i = 0; (i < numsps) && (priv_size > pos); i++)
		write_nal(dp, buf, &pos, priv_size, 2);

	if (priv_size <= pos)
		return;

	numpps = buf[pos++];

	for (i = 0; (i < numpps) && (priv_size > pos); i++)
		write_nal(dp, buf, &pos, priv_size, 2);
}

void handle_frame(DemuxMkvPriv*  mkv, GxDemuxPacket * dp, uint8_t * frame, size_t frame_size)
{
	int pos = 0;

	while (frame_size > pos)
		write_nal(dp, frame, &pos, frame_size, mkv->nal_size_size);
}
