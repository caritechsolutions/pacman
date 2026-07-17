#include "gx_decoder.h"
#include "iframe_probe.h"

static int mpeg12_has_iframe(unsigned char *buf, int size)
{
	unsigned int c = 0;
	unsigned int es_data = 0xFFFFFFFF;
	int head_valid = 0;
	int find_i = 0;

	while (c < size) {
		es_data = (es_data<<8) + (buf[c] & 0xff);
		c++;
		if ((es_data >> 8) == 0x000001) {
			switch (es_data & 0xff) {
			case 0xB3: //SEQUENCE_HEADER_CODE
				head_valid = 1;
				break;
			case 0x00: //PIC_START_CODE
				{
					unsigned int es_value = 0;
					unsigned int picture_coding_type;

					if (c + 2 >= size)
						return 0;

					es_value = (es_value << 8) + (buf[c] & 0xff);
					c++;
					es_value = (es_value << 8) + (buf[c] & 0xff);
					c++;
					picture_coding_type = (es_value >> 3) & 0x7;
					if (picture_coding_type == 1)
						find_i = 1;
					else
						find_i = 0;
				}
				break;
			default:
				break;
			}
		}

		if (find_i)
			return 1;
	}

	return 0;
}

static int mpeg4_has_iframe(unsigned char *buf, int size)
{
	unsigned int c = 0;
	unsigned int es_data = 0xFFFFFFFF;
	int find_i = 0;

	while (c < size) {
		es_data = (es_data<<8) + (buf[c] & 0xff);
		c++;
		if ((es_data >> 8) == 0x000001) {
			switch (es_data & 0xff) {
			case 0xB6: //VOP_START_CODE
				{
					unsigned int vop_coding_type;

					if ((c + 1) >= size)
						return 0;

					vop_coding_type = (buf[c] >> 6) & 0x3;
					if(vop_coding_type == 0) //I_TYPE = 0 in MPEG4
						find_i = 1;
					else
						find_i = 0;
				}
				break;
			default:
				break;
			}
		}

		if (find_i)
			return 1;
	}

	return 0;
}

static unsigned char getBit(unsigned char *buf, unsigned int size, unsigned int *c, int first)
{
	static unsigned char bin = 0;
	static unsigned char mask = 0;
	unsigned char val;

	if (first) {
		mask = 0;
		bin  = 0;
	}

	if (mask == 0) {
		bin = buf[*c];
		*c += 1;;
		mask = 0xff;
	}

	mask >>= 1;
	val = (bin & (~mask)) ? 1: 0;
	bin &= mask;

	return val;
}

static unsigned int getBits(unsigned char *buf, unsigned int size, unsigned int *c, unsigned int num)
{
	unsigned int val = 1, i = 0;

	for (i = 0; i < num; i++) {
		val = (val<<1) + getBit(buf, size, c, 0);
		if (*c >= size)
			return 0;
	}

	return val;
}

static unsigned int getBins(unsigned char *buf, unsigned int size, unsigned int *c, unsigned int num)
{
	unsigned int val = 0, i = 0;

	for (i = 0; i < num; i++) {
		val = (val<<1) + getBit(buf, size, c, 0);
		if (*c >= size)
			return 0;
	}

	return val;
}

unsigned int ue_v(unsigned char *buf, unsigned int size, unsigned int *c, char first)
{
	unsigned int k = 0;
	unsigned int val = 0;

	while (*c < size) {
		if (getBit(buf, size, c, first) == 0) {
			first = 0;
			k++;
		} else {
			first = 0;
			break;
		}
	}

	val = getBits(buf, size, c, k);

	return (val -1);
}

static int h264_has_iframe(unsigned char *buf, int size, char has_sps)
{
	unsigned int c = 0;
	unsigned int es_data = 0xFFFFFFFF;
	int find_i = 0;
	int sps_valid = 0;
	int pps_valid = 0;
	int idr_flag  = 0;

	while (c < size) {
		es_data = (es_data << 8) + (buf[c] & 0xff);
		c++;
		if ((es_data >> 8) == 0x000001) {
			switch (es_data & 0x1f) {
			case 7:  //NAL_SPS
				sps_valid = 1;
				break;
			case 8:
				pps_valid = 1;
				break;
			case 5:  //NAL_SLICE_IDR
				idr_flag = 1;
			case 19: //NAL_SLICE_SEI
			case 1:  //NAL_SLICE_NIDR
			case 2:  //NAL_SLICE_A
				{
					unsigned int first_mb_in_slice;
					unsigned int slice_type;

					first_mb_in_slice = ue_v(buf, size, &c, 1);
					slice_type = ue_v(buf, size, &c, 0);
					if ((slice_type == 2) ||      //I_TYPE
							(slice_type == 4) || //SI_TYPE
							(slice_type == 7) || //I_TYPE_1
							(slice_type == 9)) {  //SI_TYPE_1
						find_i = 1;
					} else
						find_i = 0;
				}
				break;
			default:
				break;
			}
		}

		if (find_i && (sps_valid || has_sps)) {
			gxlogd("Find I Frame SPS\n");
			return 1;
		}

		if (has_sps && pps_valid) {
			gxlogd("Find I Frame PPS\n");
			return 1;
		}
	}

	return 0;
}

static int h265_has_iframe(unsigned char *buf, int size, char has_sps)
{
	unsigned int  c = 0;
	unsigned int  es_data = 0xFFFFFFFF;
	unsigned char vps_valid = 0;
	unsigned char sps_valid = 0;
	unsigned char pps_valid = 0;
	unsigned char find_i = 0;

	while (c < size) {
		es_data = (es_data << 8) + (buf[c] & 0xff);
		c++;
		if ((es_data & 0xffffff) == 0x000001) {
			unsigned int forbidden_zero_bit;
			unsigned int nal_unit_type;
			unsigned int nuh_layer_id;
			unsigned int nuh_temporal_id_plus1;

			forbidden_zero_bit    = getBit (buf, size, &c, 1);
			nal_unit_type         = getBins(buf, size, &c, 6);
			nuh_layer_id          = getBins(buf, size, &c, 6);
			nuh_temporal_id_plus1 = getBins(buf, size, &c, 3);

			switch (nal_unit_type) {
				case 32:
					vps_valid = 1;
					break;
				case 33:
					sps_valid = 1;
					break;
				case 34:
					pps_valid = 1;
					break;
				case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
					find_i = 1;
					break;
				case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
				case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
				{
					unsigned int slice_type;
					unsigned int slice_pic_parameter_set_id;

					if (getBit(buf, size, &c, 0) == 0) {
						slice_type = 3;
						break;
					}

					slice_pic_parameter_set_id = ue_v(buf, size, &c, 0);
					slice_type = ue_v(buf, size, &c, 0);
					if (slice_type == 2) //I_TYPE
						find_i = 1;
					break;
				}
				default:
					break;
			}
		}

		if (find_i && (sps_valid || has_sps)) {
			gxlogd("Find I Frame SPS\n");
			return 1;
		}

		if (has_sps && pps_valid) {
			gxlogd("Find I Frame PPS\n");
			return 1;
		}
	}

	return 0;
}

static int avs_has_iframe(unsigned char *buf, int size)
{
	unsigned int c = 0;
	unsigned int es_data = 0xFFFFFFFF;
	int head_valid = 0;
	int find_i = 0;

	while (c < size) {
		es_data = (es_data<<8) + (buf[c] & 0xff);
		c++;
		if ((es_data >> 8) == 0x000001) {
			switch(es_data & 0xff) {
			case 0xB0:      //VIDEO_SEQUENCE_START_CODE
				head_valid = 1;
				break;
			case 0xB3:      //I_PICTURE_START_CODE
				find_i = 1;
				break;
			default:
				break;
			}
		}

		if (find_i)
			return 1;
	}

	return 0;
}

int avpacket_probe_has_iframe(GxDemuxer* demuxer, unsigned char *buf, int size)
{
	int ret = 0;
	GxStreamVideoHeader* sh_video = NULL;

	if (!HAVE_VIDEO(demuxer))
		return 0;

	if ((buf == NULL) || (size <= 0))
		return -1;

	sh_video = demuxer->video->sh;
	switch (sh_video->format) {
	case VIDEO_CODEC_MPEG12:
		ret = mpeg12_has_iframe(buf, size);
		break;
	case VIDEO_CODEC_MPEG4:
		ret = mpeg4_has_iframe(buf, size);
		break;
	case VIDEO_CODEC_H264:
		{
			GxStreamHeadPriv* hdr = &(sh_video->priv);
			ret = h264_has_iframe(buf, size, (hdr && hdr->header.len > 0)?1:0);
		}
		break;
	case VIDEO_CODEC_H265:
		{
			GxStreamHeadPriv* hdr = &(sh_video->priv);
			ret = h265_has_iframe(buf, size, (hdr && hdr->header.len > 0)?1:0);
		}
		break;
	case VIDEO_CODEC_AVS:
		ret = avs_has_iframe(buf, size);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int avpacket_parse_header(GxDemuxer* demuxer, unsigned char *buf, int size)
{
	unsigned int c = 0;
	unsigned int es_data = 0xFFFFFFFF;
	int sps_valid = 0;
	int pps_valid = 0;
	int header_start = 0;
	int header_len   = 0;

	GxStreamHeadPriv* hdr = NULL;
	GxStreamVideoHeader* sh_video = NULL;

	if (!HAVE_VIDEO(demuxer))
		return 0;

	sh_video = demuxer->video->sh;
	hdr = &(sh_video->priv);

	if ((buf == NULL) || (size <= 0))
		return -1;

	if (sh_video->format == VIDEO_CODEC_H264) {
		while (c < size) {
			es_data = (es_data << 8) + (buf[c] & 0xff);
			c++;
			if ((es_data >> 8) == 0x000001) {
				switch (es_data & 0x1f) {
				case 7:
					sps_valid = 1;
					header_start = c - 4;
					break;
				case 8:
					pps_valid = 1;
					if (sps_valid == 0)
						header_start = c - 4;
					break;
				default:
					if (sps_valid)
						header_len = (c - 4 - header_start);
					break;
				}
			}
			if (header_len > 0)
				break;
		}
	} else if (sh_video->format == VIDEO_CODEC_H265) {
		int vps_valid = 0;
		while (c < size) {
			es_data = (es_data << 8) + (buf[c] & 0xff);
			c++;
			if ((es_data & 0xffffff) == 0x000001) {
				unsigned int forbidden_zero_bit;
				unsigned int nal_unit_type;
				unsigned int nuh_layer_id;
				unsigned int nuh_temporal_id_plus1;

				forbidden_zero_bit    = getBit (buf, size, &c, 1);
				nal_unit_type         = getBins(buf, size, &c, 6);
				nuh_layer_id          = getBins(buf, size, &c, 6);
				nuh_temporal_id_plus1 = getBins(buf, size, &c, 3);

				switch (nal_unit_type) {
				case 32:
					vps_valid = 1;
					header_start = c -5;
					break;
				case 33:
					sps_valid = 1;
					if (vps_valid == 0)
						header_start = c - 5;
					break;
				case 34:
					break;
				default:
					if (sps_valid)
						header_len = (c - 5 - header_start);
					break;
				}
			}
			if (header_len > 0)
				break;
		}
	} else if (sh_video->format == VIDEO_CODEC_MPEG12) {
		while (c < size) {
			es_data = (es_data << 8) + (buf[c] & 0xff);
			c++;
			if ((es_data>>8) == 0x000001) {
				if ((es_data&0xff) == 0xB3)
					header_start = c - 4;
				if ((es_data&0xff) == 0x00) {
					header_len = c - 4 - header_start;
					break;
				}
			}
		}
	}

	if (header_len > 0 && hdr) {
		if(hdr->header.data)
			av_free(hdr->header.data);
		hdr->header.data = av_malloc(header_len);
		hdr->header.len  = header_len;
		hdr->header.flag = 0;
		if(hdr->header.data) {
			memcpy(hdr->header.data, buf + header_start, header_len);
		}
		return 1;
	}

	return 0;
}

