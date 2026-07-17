#include "xtr_mpeg4.h"

#define FF_PROFILE_UNKNOWN   -99
#define FF_LEVEL_UNKNOWN     -99
#define VOS_STARTCODE        0x1B0
#define USER_DATA_STARTCODE  0x1B2
#define GOP_STARTCODE        0x1B3
#define VISUAL_OBJ_STARTCODE 0x1B5
#define VOP_STARTCODE        0x1B6

#define SIMPLE_VO_TYPE       1
#define CORE_VO_TYPE         3
#define MAIN_VO_TYPE         4
#define NBIT_VO_TYPE         5
#define ARTS_VO_TYPE         10
#define ACE_VO_TYPE          12
#define ADV_SIMPLE_VO_TYPE   17

#define FF_BUG_MS            8192 ///< Work around various bugs in Microsoft's broken decoders.
#define RECT_SHAPE           0
#define FF_ASPECT_EXTENDED   15


static const AVRational ff_h263_pixel_aspect[16]={
	{0, 1},
	{1, 1},
	{12, 11},
	{10, 11},
	{16, 11},
	{40, 33},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
};

/* *
 *  * Return the 4 bit value that specifies the given aspect ratio.
 *   * This may be one of the standard aspect ratios or it specifies
 *    * that the aspect will be stored explicitly later.
 *     */
static int _ff_h263_aspect_to_info(AVRational aspect){
	int i;

	if(aspect.num==0) aspect= (AVRational){1,1};

	for(i=1; i<6; i++){
		if(av_cmp_q(ff_h263_pixel_aspect[i], aspect) == 0){
			return i;
		}
	}

	return FF_ASPECT_EXTENDED;
}

static int mpeg4_check_is_ok(uint8_t *buf, int size)
{
	if (!buf || size <= 4)
		return -1;

	/*  simple check  */
	if (buf[0] == 0x0 && buf[1] == 0x0 &&
		buf[2] == 0x1 && buf[3] == 0xb6)
		return -1;

	return 0;
}

static int prepare_mpeg4_params(AVFormatContext* avfc, AVCodecContext* codec,
		GxDemuxPacket* dp, GxStreamVideoHeader* sh)
{
	int ret = 0;

	if(!avfc || !codec || !dp || !sh)
		return -1;
	//for the default
	sh->mpeg4.rtp_mode = 0;
	sh->mpeg4.data_partitioning = 0;
	sh->mpeg4.mpeg_quant = 0;
	sh->mpeg4.low_delay = 0; //default by s->flags
	sh->mpeg4.profile = FF_PROFILE_UNKNOWN;
	sh->mpeg4.level = FF_LEVEL_UNKNOWN;
	sh->mpeg4.workaround_bugs = 0;
	sh->mpeg4.progressive_sequence = 1; //default 1 (can be parsed in decode_vol_header)
	sh->mpeg4.quarter_sample = 0; //default 0 (can be parsed in decode_vol_header)
	sh->mpeg4.max_b_frames = 0; //decoding: unused

	sh->mpeg4.width = codec->width;
	sh->mpeg4.height = codec->height;
	sh->mpeg4.ratio_num = codec->sample_aspect_ratio.num;
	sh->mpeg4.ratio_den = codec->sample_aspect_ratio.den;
	sh->mpeg4.timebase_num = codec->time_base.num;
	if (codec->time_base.den && 4*codec->time_base.den < 1<<15) {
		codec->time_base.den = 1<<15;
	}
	sh->mpeg4.timebase_den = codec->time_base.den;

	return ret;
}

/*
 *  add mpeg4 stuffing bits (01...1)
 */
static void ff_mpeg4_stuffing(PutBitContext *pbc)
{
	int length;
	put_bits(pbc, 1, 0);
	length = (-put_bits_count(pbc)) & 7;
	if (length)
		put_bits(pbc, length, (1 << length) - 1);
}

static void mpeg4_encode_visual_object_header(GxStreamVideoHeader* sh, PutBitContext* pb)
{
	int profile_and_level_indication;
	int vo_ver_id;

	if (sh->mpeg4.profile != FF_PROFILE_UNKNOWN) {
		profile_and_level_indication = sh->mpeg4.profile << 4;
	} else if (sh->mpeg4.max_b_frames || sh->mpeg4.quarter_sample) {
		profile_and_level_indication = 0xF0;  // adv simple
	} else {
		profile_and_level_indication = 0x00;  // simple
	}

	if (sh->mpeg4.level != FF_LEVEL_UNKNOWN)
		profile_and_level_indication |= sh->mpeg4.level;
	else
		profile_and_level_indication |= 1;   // level 1

	if (profile_and_level_indication >> 4 == 0xF)
		vo_ver_id = 5;
	else
		vo_ver_id = 1;

	// FIXME levels

	put_bits(pb, 16, 0);
	put_bits(pb, 16, VOS_STARTCODE);

	put_bits(pb, 8, profile_and_level_indication);

	put_bits(pb, 16, 0);
	put_bits(pb, 16, VISUAL_OBJ_STARTCODE);

	put_bits(pb, 1, 1);
	put_bits(pb, 4, vo_ver_id);
	put_bits(pb, 3, 1);     // priority

	put_bits(pb, 4, 1);     // visual obj type== video obj

	put_bits(pb, 1, 0);     // video signal type == no clue // FIXME

	ff_mpeg4_stuffing(pb);
}

static void mpeg4_encode_vol_header(GxStreamVideoHeader* sh,
		int vo_number,
		int vol_number,
		PutBitContext* pb)
{
	int vo_ver_id;
	AVRational sample_aspect_ratio;
	int aspect_ratio_info;

	if (sh->mpeg4.max_b_frames || sh->mpeg4.quarter_sample) {
		vo_ver_id  = 5;
		sh->mpeg4.vo_type = ADV_SIMPLE_VO_TYPE;
	} else {
		vo_ver_id  = 1;
		sh->mpeg4.vo_type = SIMPLE_VO_TYPE;
	}
	sample_aspect_ratio.num = sh->mpeg4.ratio_num;
	sample_aspect_ratio.den = sh->mpeg4.ratio_den;

	put_bits(pb, 16, 0);
	put_bits(pb, 16, 0x100 + vo_number);        /* video obj */
	put_bits(pb, 16, 0);
	put_bits(pb, 16, 0x120 + vol_number);       /* video obj layer */

	put_bits(pb, 1, 0);             /* random access vol */
	put_bits(pb, 8, sh->mpeg4.vo_type);    /* video obj type indication */
	if (sh->mpeg4.workaround_bugs & FF_BUG_MS) {
		put_bits(pb, 1, 0);         /* is obj layer id= no */
	} else {
		put_bits(pb, 1, 1);         /* is obj layer id= yes */
		put_bits(pb, 4, vo_ver_id); /* is obj layer ver id */
		put_bits(pb, 3, 1);         /* is obj layer priority */
	}

	aspect_ratio_info = _ff_h263_aspect_to_info(sample_aspect_ratio);

	put_bits(pb, 4, aspect_ratio_info); /* aspect ratio info */
	if (aspect_ratio_info == FF_ASPECT_EXTENDED) {
		av_reduce(&sh->mpeg4.ratio_num, &sh->mpeg4.ratio_den,
				sh->mpeg4.ratio_num,  sh->mpeg4.ratio_den, 255);
		put_bits(pb, 8, sh->mpeg4.ratio_num);
		put_bits(pb, 8, sh->mpeg4.ratio_den);
	}

	if (sh->mpeg4.workaround_bugs & FF_BUG_MS) {
		put_bits(pb, 1, 0);         /* vol control parameters= no @@@ */
	} else {
		put_bits(pb, 1, 1);         /* vol control parameters= yes */
		put_bits(pb, 2, 1);         /* chroma format YUV 420/YV12 */
		put_bits(pb, 1, sh->mpeg4.low_delay);
		put_bits(pb, 1, 0);         /* vbv parameters= no */
	}

	put_bits(pb, 2, RECT_SHAPE);    /* vol shape= rectangle */
	put_bits(pb, 1, 1);             /* marker bit */

	put_bits(pb, 16, sh->mpeg4.timebase_den);
#if 0
	if (s->time_increment_bits < 1)
		s->time_increment_bits = 1;
#endif
	put_bits(pb, 1, 1);             /* marker bit */
	put_bits(pb, 1, 0);             /* fixed vop rate=no */
	put_bits(pb, 1, 1);             /* marker bit */
	put_bits(pb, 13, sh->mpeg4.width);     /* vol width */
	put_bits(pb, 1, 1);             /* marker bit */
	put_bits(pb, 13, sh->mpeg4.height);    /* vol height */
	put_bits(pb, 1, 1);             /* marker bit */
	put_bits(pb, 1, sh->mpeg4.progressive_sequence ? 0 : 1);
	put_bits(pb, 1, 1);             /* obmc disable */
	if (vo_ver_id == 1)
		put_bits(pb, 1, 0);       /* sprite enable */
	else
		put_bits(pb, 2, 0);       /* sprite enable */

	put_bits(pb, 1, 0);             /* not 8 bit == false */
	put_bits(pb, 1, sh->mpeg4.mpeg_quant); /* quant type= (0=h263 style)*/

#if 0
	if (s->mpeg_quant) {
		ff_write_quant_matrix(pb, s->avctx->intra_matrix);
		ff_write_quant_matrix(pb, s->avctx->inter_matrix);
	}
#endif

	if (vo_ver_id != 1)
		put_bits(pb, 1, sh->mpeg4.quarter_sample);
	put_bits(pb, 1, 1);             /* complexity estimation disable */
	put_bits(pb, 1, sh->mpeg4.rtp_mode ? 0 : 1); /* resync marker disable */
	put_bits(pb, 1, sh->mpeg4.data_partitioning ? 1 : 0);
	if (sh->mpeg4.data_partitioning)
		put_bits(pb, 1, 0);         /* no rvlc */

	if (vo_ver_id != 1) {
		put_bits(pb, 1, 0);         /* newpred */
		put_bits(pb, 1, 0);         /* reduced res vop */
	}
	put_bits(pb, 1, 0);             /* scalability */

	ff_mpeg4_stuffing(pb);

#if 0
	/* user data */
	if (!(s->flags & CODEC_FLAG_BITEXACT)) {
		put_bits(pb, 16, 0);
		put_bits(pb, 16, 0x1B2);    /* user_data */
		avpriv_put_string(pb, LIBAVCODEC_IDENT, 0);
	}
#endif
}

int xtr_mpeg4_packet(GxStreamVideoHeader* sh,
		uint8_t* buffer,
		int src_len,
		GxDemuxPacket *dp_dst)
{
	int ret = 0;
	int header_len;
	PutBitContext pb;

	memset(dp_dst->buffer, 0xff, dp_dst->len);
	init_put_bits(&pb, dp_dst->buffer, dp_dst->len);

	mpeg4_encode_visual_object_header(sh, &pb);
	mpeg4_encode_vol_header(sh, 0, 0, &pb);
	flush_put_bits(&pb);
	header_len = put_bits_count(&pb) / 8;

	if (header_len + src_len > dp_dst->len){
		gxlogd("mpeg4 buffer is small\n");
		return -1;
	}
	memcpy(dp_dst->buffer + header_len, buffer, src_len);
	dp_dst->write_size = header_len + src_len;
	dp_dst->len = header_len + src_len;

	return ret;
}

void xtr_mpeg4_check(GxDemuxer* demuxer)
{
	GxDemuxPacket* dp = NULL;
	GxStreamVideoHeader* sh = NULL;
	GxDemuxStream* ds = demuxer->video;
	DemuxLavfPriv* priv = demuxer->priv;
	AVFormatContext* avfc = priv->avfc;
	AVCodecContext* codec = NULL;

	if (!priv || !priv->avfc || !ds || !ds->sh)
		return;

	sh = ds->sh;
	codec = avfc->streams[priv->video_index]->codec;

	if (sh->format != VIDEO_CODEC_MPEG4 ||
		(codec->extradata && codec->extradata_size > 0))
		return;

	while (ds->packs == 0 && !demux_lavf_fill_buffer(demuxer, ds));
	if (ds->packs > 0) {
		dp = ds->first;
		if (mpeg4_check_is_ok(dp->buffer, dp->len) < 0) {
			if(prepare_mpeg4_params(avfc, codec, dp, sh)) {
				gxlogd("######### LAVF mpeg4 params prepare error  #########\n");
				return;
			}
			GxDemuxPacket *dp_dst = GxDemuxPacket_Create(ds->demuxer, NULL, dp->len + MPEG4_EXTRA_LEN);
			if (!dp_dst){
				gxlogd("######### LAVF OOM mpeg4 #########\n");
				return;
			}
			xtr_mpeg4_packet(sh, dp->buffer, dp->len, dp_dst);

			dp_dst->write_size = dp_dst->len;
			dp_dst->pts        = dp->pts;
			dp_dst->endpts     = dp->endpts;
			dp_dst->pos        = dp->pos;
			dp_dst->flags      = dp->flags;
			dp_dst->next       = dp->next;
			ds->bytes = dp_dst->len;
			ds->first = ds->last = dp_dst;

			sh->mpeg4_needmodify = 1;
			GxDemuxPacket_Destroy(dp);
		}
	}

	return;
}
