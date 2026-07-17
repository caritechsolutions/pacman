#if(defined(ECOS_OS) || defined(LINUX_OS))
#else
#include "gxmedia_module.h"
#include "gxmedia_common.h"

struct std_hw {
	int std;
	int hw;
};

static struct std_hw acodec_std_hw[] = {
	{ AUDIO_CODEC_MPEG1,     CODEC_MPEG12A, },
	{ AUDIO_CODEC_MPEG2,     CODEC_MPEG12A, },
	{ AUDIO_CODEC_AAC_LATM,  CODEC_MPEG4_AAC, },
	{ AUDIO_CODEC_AAC_ADTS,  CODEC_MPEG4_AAC, },
	{ AUDIO_CODEC_VORBIS,       CODEC_VORBIS, },
	{ AUDIO_CODEC_RA_AAC,    CODEC_RA_AAC, },
	{ AUDIO_CODEC_RA_RA8LBR, CODEC_RA_RA8LBR, },
	{ AUDIO_CODEC_AC3,       CODEC_AC3, },
	{ AUDIO_CODEC_AVS,       CODEC_AVSA, },
	{ AUDIO_CODEC_PCM,       CODEC_PCM, },
	{ AUDIO_CODEC_EAC3,      CODEC_EAC3, },
	{ AUDIO_CODEC_DTS,       CODEC_DTS, },
	{ AUDIO_CODEC_DRA1,      CODEC_DRA, },
	{ AUDIO_CODEC_DTS_HD,    CODEC_DTS, },
	{-1, -1},
};

static struct std_hw vcodec_std_hw[] = {
	{ VIDEO_CODEC_MPEG12,    CODEC_MPEG2V, },
	{ VIDEO_CODEC_MPEG4,     CODEC_MPEG4V, },
	{ VIDEO_CODEC_H263,      CODEC_H263, },
	{ VIDEO_CODEC_H264,      CODEC_H264, },
	{ VIDEO_CODEC_REAL,      CODEC_RV, },
	{ VIDEO_CODEC_AVS,       CODEC_AVSV, },
	{ VIDEO_CODEC_H265,      CODEC_HEVC, },
	{ VIDEO_CODEC_DIV3,      CODEC_DIV3, },
	{-1, -1},
};

int acodec_std2hw(int type)
{
	int i = 0;
	while (acodec_std_hw[i].std != -1) {
		if (acodec_std_hw[i].std == type)
			return acodec_std_hw[i].hw;
		i++;
	}

	return -1;
}

int acodec_hw2std(int type)
{
	int i = 0;
	while (acodec_std_hw[i].hw != -1) {
		if (acodec_std_hw[i].hw == type)
			return acodec_std_hw[i].std;
		i++;
	}

	return -1;
}

int vcodec_std2hw(int type)
{
	int i = 0;
	while (vcodec_std_hw[i].std != -1) {
		if (vcodec_std_hw[i].std == type)
			return vcodec_std_hw[i].hw;
		i++;
	}

	return -1;
}

int vcodec_hw2std(int type)
{
	int i = 0;
	while (vcodec_std_hw[i].hw != -1) {
		if (vcodec_std_hw[i].hw == type)
			return vcodec_std_hw[i].std;
		i++;
	}

	return -1;
}

#endif
