#include "gxplayer_url.h"
#include "gxplayer_decoder.h"
#include "stheader.h"
#include "gx_config.h"
#include "gx_avutil.h"

struct url_std {
	int url;
	int std;
};
static struct url_std acodec_url_std [] = {
	{ GX_URL_VAL_ACODEC_MPG1,         AUDIO_CODEC_MPEG1 },
	{ GX_URL_VAL_ACODEC_MPG2,         AUDIO_CODEC_MPEG2 },
	{ GX_URL_VAL_ACODEC_LATM,         AUDIO_CODEC_AAC_LATM },
	{ GX_URL_VAL_ACODEC_AC3,          AUDIO_CODEC_AC3 },
	{ GX_URL_VAL_ACODEC_ATDS,         AUDIO_CODEC_AAC_ADTS },
	{ GX_URL_VAL_ACODEC_EAC3,         AUDIO_CODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_LPCM,         AUDIO_CODEC_PCM },
	{ GX_URL_VAL_ACODEC_DTS,          AUDIO_CODEC_DTS },
	{ GX_URL_VAL_ACODEC_DOLBY_TRUEHD, AUDIO_CODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_AC3_PLUS,     AUDIO_CODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_DTS_HD,       AUDIO_CODEC_DTS_HD },
	{ GX_URL_VAL_ACODEC_DTS_MA,       AUDIO_CODEC_DTS },
	{ GX_URL_VAL_ACODEC_AC3_PLUS_SEC, AUDIO_CODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_DTS_HD_SEC,   AUDIO_CODEC_DTS_HD },
	{ GX_URL_VAL_ACODEC_DRA1,         AUDIO_CODEC_DRA1 },
	{ -1, -1 },
};
static struct url_std vcodec_url_std [] = {
	{ GX_URL_VAL_VCODEC_MPG2,         VIDEO_CODEC_MPEG12 },
	{ GX_URL_VAL_VCODEC_AVS,          VIDEO_CODEC_AVS },
	{ GX_URL_VAL_VCODEC_H264,         VIDEO_CODEC_H264 },
	{ GX_URL_VAL_VCODEC_H265,         VIDEO_CODEC_H265 },
	{ GX_URL_VAL_VCODEC_MPG4,         VIDEO_CODEC_MPEG4 },
	{ -1, -1 },
};

int acodec_url2std(int type)
{
	int i = 0;
	while (acodec_url_std[i].url != -1) {
		if (acodec_url_std[i].url == type)
			return acodec_url_std[i].std;
		i++;
	}

	return -1;
}

int acodec_std2url(int type)
{
	int i = 0;
	while (acodec_url_std[i].std != -1) {
		if (acodec_url_std[i].std == type)
			return acodec_url_std[i].url;
		i++;
	}

	return -1;
}

int vcodec_url2std(int type)
{
	int i = 0;
	while (vcodec_url_std[i].url != -1) {
		if (vcodec_url_std[i].url == type)
			return vcodec_url_std[i].std;
		i++;
	}

	return -1;
}

struct fourcc_std {
	int fourcc;
	int std;
	char *codec_name;
};
static struct fourcc_std acodec_fourcc_std[] = {
	{ PLAYER_ACODEC_MPG1,    AUDIO_CODEC_MPEG2 , "MPEG2" },
	{ PLAYER_ACODEC_MPG2,    AUDIO_CODEC_MPEG2 , "MPEG2" },
	{ PLAYER_ACODEC_MPG3,    AUDIO_CODEC_MPEG2 , "MPEG2" },
	{ PLAYER_ACODEC_LATM,    AUDIO_CODEC_AAC_LATM , "AAC_LATM" },
	{ PLAYER_ACODEC_AAAC,    AUDIO_CODEC_AAC_ADTS , "AAC_ADTS" },
	{ PLAYER_ACODEC_MP4A,    AUDIO_CODEC_AAC_ADTS , "AAC_ADTS" },
	{ PLAYER_ACODEC_AC31,    AUDIO_CODEC_AC3 ,     "AC3" },
	{ PLAYER_ACODEC_AC32,    AUDIO_CODEC_AC3 ,     "AC3" },
	{ PLAYER_ACODEC_DVDPCM,  AUDIO_CODEC_PCM ,     "PCM" },
	{ PLAYER_ACODEC_DTS,     AUDIO_CODEC_DTS ,     "DTS" },
	{ PLAYER_ACODEC_DRA,     AUDIO_CODEC_DRA1 ,    "DRA" },
	{ PLAYER_ACODEC_EAC3,    AUDIO_CODEC_EAC3 ,    "EAC3" },
	{ PLAYER_ACODEC_VORBIS,  AUDIO_CODEC_VORBIS ,  "VORBIS" },
	{ PLAYER_ACODEC_OPUS,    AUDIO_CODEC_OPUS ,    "OPUS" },
	{ PLAYER_ACODEC_RA8B,    AUDIO_CODEC_RA_RA8LBR , "RA-RA8LBR" },
	{ PLAYER_ACODEC_RA8,     AUDIO_CODEC_RA_RA8LBR , "RA-RA8LBR" },
	{ PLAYER_ACODEC_RAAC1,   AUDIO_CODEC_RA_AAC , "RA-AAC" },
	{ PLAYER_ACODEC_RAAC,    AUDIO_CODEC_RA_AAC , "RA-AAC" },
	{ PLAYER_ACODEC_DB71,    AUDIO_CODEC_EAC3 ,   "EAC3" },
	{ PLAYER_ACODEC_DB51,    AUDIO_CODEC_EAC3 ,   "EAC3" },
	{ PLAYER_ACODEC_DBCV,    AUDIO_CODEC_EAC3 ,   "EAC3" },
	{ PLAYER_ACODEC_FLAC,    AUDIO_CODEC_FLAC ,   "FLAC" },
	{ PLAYER_ACODEC_APE,     AUDIO_CODEC_APE ,    "APE" },
	{ -1, -1 , NULL},
};
static struct fourcc_std vcodec_fourcc_std[] = {
	{ PLAYER_VCODEC_H263,    VIDEO_CODEC_H263 ,  "H263" },
	{ PLAYER_VCODEC_H264,    VIDEO_CODEC_H264 ,  "H264" },
	{ PLAYER_VCODEC_HEVC,    VIDEO_CODEC_H265 ,  "HEVC" },
	{ PLAYER_VCODEC_XVID,    VIDEO_CODEC_MPEG4 , "MPEG4" },
	{ PLAYER_VCODEC_XVIDD,   VIDEO_CODEC_MPEG4 , "MPEG4" },
	{ PLAYER_VCODEC_MP4V,    VIDEO_CODEC_MPEG4 , "MPEG4" },
	{ PLAYER_VCODEC_FMP4,    VIDEO_CODEC_MPEG4 , "MPEG4" },
#ifdef CONFIG_AVCODEC_DIVX
	{ PLAYER_VCODEC_DIVXD,   VIDEO_CODEC_MPEG4 , "MPEG4" },
	{ PLAYER_VCODEC_DIVX,    VIDEO_CODEC_MPEG4 , "MPEG4" },
	{ PLAYER_VCODEC_DX50,    VIDEO_CODEC_MPEG4 , "MPEG4" },
#endif
	{ PLAYER_VCODEC_DIV3,    VIDEO_CODEC_DIV3   , "div3" },
	{ PLAYER_VCODEC_MPG1,    VIDEO_CODEC_MPEG12 , "MPEG1/2 Video" },
	{ PLAYER_VCODEC_MPG2,    VIDEO_CODEC_MPEG12 , "MPEG1/2 Video" },
	{ PLAYER_VCODEC_MPEG,    VIDEO_CODEC_MPEG12 , "MPEG1/2 Video" },
	{ PLAYER_VCODEC_MPG3,    VIDEO_CODEC_MPEG12 , "MPEG1/2 Video" },
	{ PLAYER_VCODEC_MPG11,   VIDEO_CODEC_MPEG12 , "MPEG1/2 Video" },
	{ PLAYER_VCODEC_WVC1,    VIDEO_CODEC_VC1 ,    "VC1" },
	{ PLAYER_VCODEC_WMV3,    VIDEO_CODEC_VC1 ,    "VC1" },
	{ PLAYER_VCODEC_AVS,     VIDEO_CODEC_AVS ,    "AVS" },
	{ PLAYER_VCODEC_RV40,	 VIDEO_CODEC_REAL ,   "REAL" },
	{ PLAYER_VCODEC_RV1,     VIDEO_CODEC_REAL ,   "REAL" },
	{ PLAYER_VCODEC_MJPEG,   VIDEO_CODEC_MJPEG ,  "MJPEG" },
	{ -1, -1 , NULL},
};

int acodec_fourcc2std(int type)
{
	int i = 0;
	while (acodec_fourcc_std[i].fourcc != -1) {
		if (acodec_fourcc_std[i].fourcc == type)
			return acodec_fourcc_std[i].std;
		i++;
	}

	return -1;
}

char *acodec_get_fourcc2_name(int type)
{
	int i = 0;
	while (acodec_fourcc_std[i].fourcc != -1) {
		if (acodec_fourcc_std[i].fourcc == type)
			return acodec_fourcc_std[i].codec_name;
		i++;
	}
	return "unknown_codec";
}

int vcodec_fourcc2std(int type)
{
	int i = 0;
	while (vcodec_fourcc_std[i].fourcc != -1) {
		if (vcodec_fourcc_std[i].fourcc == type)
			return vcodec_fourcc_std[i].std;
		i++;
	}

	return -1;
}

char *vcodec_get_fourcc2_name(int type)
{
	int i = 0;
	while (vcodec_fourcc_std[i].fourcc != -1) {
		if (vcodec_fourcc_std[i].fourcc == type)
			return vcodec_fourcc_std[i].codec_name;
		i++;
	}
	return "unknown_codec";
}

struct std_hw {
	int std;
	int hw;
};
static struct std_hw acodec_std_hw[] = {
	{ AUDIO_CODEC_MPEG1,     CODEC_MPEG12A, },
	{ AUDIO_CODEC_MPEG2,     CODEC_MPEG12A, },
	{ AUDIO_CODEC_AAC_LATM,  CODEC_MPEG4_AAC, },
	{ AUDIO_CODEC_AAC_ADTS,  CODEC_MPEG4_AAC, },
	{ AUDIO_CODEC_VORBIS,    CODEC_VORBIS, },
	{ AUDIO_CODEC_RA_AAC,    CODEC_RA_AAC, },
	{ AUDIO_CODEC_RA_RA8LBR, CODEC_RA_RA8LBR, },
	{ AUDIO_CODEC_AC3,       CODEC_AC3, },
	{ AUDIO_CODEC_AVS,       CODEC_AVSA, },
	{ AUDIO_CODEC_PCM,       CODEC_PCM, },
	{ AUDIO_CODEC_EAC3,      CODEC_EAC3, },
	{ AUDIO_CODEC_DTS,       CODEC_DTS, },
	{ AUDIO_CODEC_DRA1,      CODEC_DRA, },
	{ AUDIO_CODEC_DTS_HD,    CODEC_DTS, },
	{ AUDIO_CODEC_FLAC,      CODEC_FLAC, },
	{ AUDIO_CODEC_OPUS,      CODEC_OPUS, },
	{ AUDIO_CODEC_SBC,       CODEC_SBC, },
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

struct std_codecid {
	int std;
	int codecid;
};

struct std_codecid acodec_std_codecid[] = {
	{AUDIO_CODEC_MPEG1,    CODEC_ID_MP1},
	{AUDIO_CODEC_MPEG2,    CODEC_ID_MP2},
	{AUDIO_CODEC_AAC_LATM, CODEC_ID_AAC_LATM},
	{AUDIO_CODEC_AAC_ADTS, CODEC_ID_AAC},
	{AUDIO_CODEC_AC3,      CODEC_ID_AC3},
	{AUDIO_CODEC_AVS,      CODEC_ID_AVS},
	{AUDIO_CODEC_EAC3,     CODEC_ID_EAC3},
	{AUDIO_CODEC_DTS,      CODEC_ID_DTS},
	{AUDIO_CODEC_DRA1,     CODEC_ID_DRA},
	{-1,-1},
};

struct std_codecid vcodec_std_codecid[] = {
	{VIDEO_CODEC_MPEG12, CODEC_ID_MPEG2VIDEO},
	{VIDEO_CODEC_MPEG4,  CODEC_ID_MPEG4},
	{VIDEO_CODEC_H263,   CODEC_ID_H263},
	{VIDEO_CODEC_H264,   CODEC_ID_H264},
	{VIDEO_CODEC_AVS,    CODEC_ID_AVS},
	{VIDEO_CODEC_H265,   CODEC_ID_HEVC},
	{VIDEO_CODEC_VC1,    0X31435657},
	{VIDEO_CODEC_MJPEG,  0x47504a4d},
	{-1,-1},
};

int acodec_std2codecid(int type)
{
	int i = 0;
	while (acodec_std_codecid[i].std != -1) {
		if (acodec_std_codecid[i].std == type)
			return acodec_std_codecid[i].codecid;
		i++;
	}

	return -1;
}

int vcodec_std2codecid(int type)
{
	int i = 0;
	while (vcodec_std_codecid[i].std != -1) {
		if (vcodec_std_codecid[i].std == type)
			return vcodec_std_codecid[i].codecid;
		i++;
	}

	return -1;
}

struct url_fourcc {
	int url;
	int fourcc;
};

static struct url_fourcc acodec_url_fourcc [] = {
	{ GX_URL_VAL_ACODEC_MPG1,         PLAYER_ACODEC_MPG1 },
	{ GX_URL_VAL_ACODEC_MPG2,         PLAYER_ACODEC_MPG2 },
	{ GX_URL_VAL_ACODEC_LATM,         PLAYER_ACODEC_LATM },
	{ GX_URL_VAL_ACODEC_AC3,          PLAYER_ACODEC_AC31 },
	{ GX_URL_VAL_ACODEC_ATDS,         PLAYER_ACODEC_MP4A },
	{ GX_URL_VAL_ACODEC_EAC3,         PLAYER_ACODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_LPCM,         PLAYER_ACODEC_DVDPCM },
	{ GX_URL_VAL_ACODEC_DTS,          PLAYER_ACODEC_DTS },
	{ GX_URL_VAL_ACODEC_DOLBY_TRUEHD, PLAYER_ACODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_AC3_PLUS,     PLAYER_ACODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_DTS_HD,       PLAYER_ACODEC_DTS },
	{ GX_URL_VAL_ACODEC_DTS_MA,       PLAYER_ACODEC_DTS },
	{ GX_URL_VAL_ACODEC_AC3_PLUS_SEC, PLAYER_ACODEC_EAC3 },
	{ GX_URL_VAL_ACODEC_DTS_HD_SEC,   PLAYER_ACODEC_DTS },
	{ GX_URL_VAL_ACODEC_DRA1,         PLAYER_ACODEC_DRA },
	{ -1, -1 },
};

static struct url_fourcc vcodec_url_fourcc [] = {
	{ GX_URL_VAL_VCODEC_MPG2,         PLAYER_VCODEC_MPG2 },
	{ GX_URL_VAL_VCODEC_AVS,          PLAYER_VCODEC_AVS },
	{ GX_URL_VAL_VCODEC_H264,         PLAYER_VCODEC_H264 },
	{ GX_URL_VAL_VCODEC_H265,         PLAYER_VCODEC_HEVC },
	{ GX_URL_VAL_VCODEC_MPG4,         PLAYER_VCODEC_MP4V },
	{ -1, -1 },
};

int acodec_url2fourcc(int url)
{
	int i = 0;
	while (acodec_url_fourcc[i].url != -1) {
		if (acodec_url_fourcc[i].url == url)
			return acodec_url_fourcc[i].fourcc;
		i++;
	}

	return -1;
}

int vcodec_url2fourcc(int url)
{
	int i = 0;
	while (vcodec_url_fourcc[i].url != -1) {
		if (vcodec_url_fourcc[i].url == url)
			return vcodec_url_fourcc[i].fourcc;
		i++;
	}

	return -1;
}

struct std_str {
	int std;
	const char *str;
};

static struct std_str a_std2str[] = {
	{AUDIO_CODEC_MPEG1    , "mp1"   },
	{AUDIO_CODEC_MPEG2    , "mp2"   },
	{AUDIO_CODEC_AAC_LATM , "aac"   },
	{AUDIO_CODEC_AAC_ADTS , "aac"   },
	{AUDIO_CODEC_VORBIS   , "vorbis"},
	{AUDIO_CODEC_AC3      , "ac3"   },
	{AUDIO_CODEC_AVS      , "avs"   },
	{AUDIO_CODEC_PCM      , "pcm"   },
	{AUDIO_CODEC_EAC3     , "eac3"  },
	{AUDIO_CODEC_DTS      , "dts"   },
	{AUDIO_CODEC_DRA1     , "dra"   },
	{AUDIO_CODEC_DTS_HD   , "dts"   },
	{AUDIO_CODEC_REAL     , "real"  },
	{AUDIO_CODEC_RA_AAC   , "ra8"   },
	{AUDIO_CODEC_RA_RA8LBR, "ra8"   },
	{AUDIO_CODEC_FLAC     , "flac"  },
	{AUDIO_CODEC_OPUS     , "opus"  },
	{AUDIO_CODEC_AMR_NB   , "amrnb" },
	{AUDIO_CODEC_AMR_WB   , "amrwb" },
	{AUDIO_CODEC_APE      , "ape"   },
};

static struct std_str v_std2str[] = {
	{VIDEO_CODEC_MPEG12, "mpeg12"},
	{VIDEO_CODEC_MPEG4 , "mpeg4" },
	{VIDEO_CODEC_H263  , "h263"  },
	{VIDEO_CODEC_H264  , "h264"  },
	{VIDEO_CODEC_REAL  , "real"  },
	{VIDEO_CODEC_AVS   , "avs"   },
	{VIDEO_CODEC_H265  , "h265"  },
	{VIDEO_CODEC_VC1   , "vc1"   },
	{VIDEO_CODEC_MJPEG , "mjpeg" },
	{VIDEO_CODEC_DIV3  , "div3"  }
};

const char *acodec_std2str(int std)
{
	int i = 0;

	for (i = 0; i < sizeof(a_std2str)/sizeof(struct std_str); i++) {
		if (a_std2str[i].std == std)
			return a_std2str[i].str;
	}

	return "unkown";
}

const char *vcodec_std2str(int std)
{
	int i = 0;

	for (i = 0; i < sizeof(v_std2str)/sizeof(struct std_str); i++) {
		if (v_std2str[i].std == std)
			return v_std2str[i].str;
	}

	return "unkown";
}

int acodec_check_ad_enable(int type0, int type1)
{
	unsigned int enable_flag  = 0;
	AudioCodecType codectype0 = type0;
	AudioCodecType codectype1 = type1;

	if ((codectype0 == AUDIO_CODEC_AC3 || codectype0 == AUDIO_CODEC_EAC3) &&
			(codectype1 == AUDIO_CODEC_AC3 || codectype1 == AUDIO_CODEC_EAC3)) {
		PlayerVaildAD support;
		AudioAc3Mode ac3mode = AUDIO_AC3_DECODE_MODE;

		GxPlayer_SystemGet(PSYS_AOUT_AC3MODE, &ac3mode);
		support.codec = acodec_std2hw(codectype0);
		support.mode  = 0;
		support.support = 0;
		if ((ac3mode & AUDIO_AC3_BYPASS_MODE) == AUDIO_AC3_BYPASS_MODE)
			support.mode |= BYPASS_MODE;
		if ((ac3mode & AUDIO_AC3_DECODE_MODE) == AUDIO_AC3_DECODE_MODE)
			support.mode |= DECODE_MODE;
		GxPlayer_SystemGet(PSYS_AUDIO_DESCRIPTOR_VAILD, &support);
		enable_flag = support.support;
	} else if ((codectype0 == AUDIO_CODEC_AAC_LATM || codectype0 == AUDIO_CODEC_AAC_ADTS) &&
		(codectype1 == AUDIO_CODEC_AAC_LATM || codectype1 == AUDIO_CODEC_AAC_ADTS)) {
		PlayerVaildAD support;
		AudioAACMode aacmode = AUDIO_AAC_DECODE_MODE;

		GxPlayer_SystemGet(PSYS_AOUT_AACMODE, &aacmode);
		support.codec = acodec_std2hw(codectype0);
		support.mode  = DECODE_MODE;
		support.support = 0;
		if ((aacmode & AUDIO_AAC_CONVERT_MODE) == AUDIO_AAC_CONVERT_MODE)
			support.mode |= CONVERT_MODE;
		if ((aacmode & AUDIO_AAC_BYPASS_MODE) == AUDIO_AAC_BYPASS_MODE)
			support.mode |= BYPASS_MODE;
		GxPlayer_SystemGet(PSYS_AUDIO_DESCRIPTOR_VAILD, &support);
		enable_flag = support.support;
	} else if (codectype0 == codectype1) {
		PlayerVaildAD support;

		support.codec = acodec_std2hw(codectype0);
		support.mode  = DECODE_MODE;
		support.support = 0;
		GxPlayer_SystemGet(PSYS_AUDIO_DESCRIPTOR_VAILD, &support);
		enable_flag = support.support;
	}

	return enable_flag;
}


