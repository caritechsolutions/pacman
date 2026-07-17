
#include "aformat.h"
#include "ao.h"

#if USE_AOUT_OSS

#ifndef LINUX_OS
#include "soundcard.h"
#endif

#define HAVE_AUDIO_SELECT
#define PATH_DEV_DSP "/dev/dsp"
#define PATH_DEV_MIXER "/dev/mixer"

#define MAX_OUTBURST 65536

typedef struct ao_control_vol_s {
	float left;
	float right;
} ao_control_vol_t;


typedef struct {
  int samplerate;
  int channels;
  int format;
  int bps;
  int outburst;
  int buffersize;
  int pts;
}AoOssPriv;

int aout_oss_close(GxAudioOut*  ao, int immed);


char* mixer_device = NULL;
char* mixer_channel = NULL;

static int format2oss(int format)
{
	switch (format) {
	case AF_FORMAT_U8:
		return AFMT_U8;
	case AF_FORMAT_S8:
		return AFMT_S8;
	case AF_FORMAT_U16_LE:
		return AFMT_U16_LE;
	case AF_FORMAT_U16_BE:
		return AFMT_U16_BE;
	case AF_FORMAT_S16_LE:
		return AFMT_S16_LE;
	case AF_FORMAT_S16_BE:
		return AFMT_S16_BE;
#ifdef AFMT_U24_LE
	case AF_FORMAT_U24_LE:
		return AFMT_U24_LE;
#endif
#ifdef AFMT_U24_BE
	case AF_FORMAT_U24_BE:
		return AFMT_U24_BE;
#endif
#ifdef AFMT_S24_LE
	case AF_FORMAT_S24_LE:
		return AFMT_S24_LE;
#endif
#ifdef AFMT_S24_BE
	case AF_FORMAT_S24_BE:
		return AFMT_S24_BE;
#endif
#ifdef AFMT_U32_LE
	case AF_FORMAT_U32_LE:
		return AFMT_U32_LE;
#endif
#ifdef AFMT_U32_BE
	case AF_FORMAT_U32_BE:
		return AFMT_U32_BE;
#endif
#ifdef AFMT_S32_LE
	case AF_FORMAT_S32_LE:
		return AFMT_S32_LE;
#endif
#ifdef AFMT_S32_BE
	case AF_FORMAT_S32_BE:
		return AFMT_S32_BE;
#endif
#ifdef AFMT_FLOAT
	case AF_FORMAT_FLOAT_NE:
		return AFMT_FLOAT;
#endif
		// SPECIALS
	case AF_FORMAT_MU_LAW:
		return AFMT_MU_LAW;
	case AF_FORMAT_A_LAW:
		return AFMT_A_LAW;
	case AF_FORMAT_IMA_ADPCM:
		return AFMT_IMA_ADPCM;
#ifdef AFMT_MPEG
	case AF_FORMAT_MPEG2:
		return AFMT_MPEG;
#endif
#ifdef AFMT_AC3
	case AF_FORMAT_AC3:
		return AFMT_AC3;
#endif
	}

	return -1;
}

static int oss2format(int format)
{
	switch (format) {
	case AFMT_U8:
		return AF_FORMAT_U8;
	case AFMT_S8:
		return AF_FORMAT_S8;
	case AFMT_U16_LE:
		return AF_FORMAT_U16_LE;
	case AFMT_U16_BE:
		return AF_FORMAT_U16_BE;
	case AFMT_S16_LE:
		return AF_FORMAT_S16_LE;
	case AFMT_S16_BE:
		return AF_FORMAT_S16_BE;
#ifdef AFMT_U24_LE
	case AFMT_U24_LE:
		return AF_FORMAT_U24_LE;
#endif
#ifdef AFMT_U24_BE
	case AFMT_U24_BE:
		return AF_FORMAT_U24_BE;
#endif
#ifdef AFMT_S24_LE
	case AFMT_S24_LE:
		return AF_FORMAT_S24_LE;
#endif
#ifdef AFMT_S24_BE
	case AFMT_S24_BE:
		return AF_FORMAT_S24_BE;
#endif
#ifdef AFMT_U32_LE
	case AFMT_U32_LE:
		return AF_FORMAT_U32_LE;
#endif
#ifdef AFMT_U32_BE
	case AFMT_U32_BE:
		return AF_FORMAT_U32_BE;
#endif
#ifdef AFMT_S32_LE
	case AFMT_S32_LE:
		return AF_FORMAT_S32_LE;
#endif
#ifdef AFMT_S32_BE
	case AFMT_S32_BE:
		return AF_FORMAT_S32_BE;
#endif
#ifdef AFMT_FLOAT
	case AFMT_FLOAT:
		return AF_FORMAT_FLOAT_NE;
#endif
		// SPECIALS
	case AFMT_MU_LAW:
		return AF_FORMAT_MU_LAW;
	case AFMT_A_LAW:
		return AF_FORMAT_A_LAW;
	case AFMT_IMA_ADPCM:
		return AF_FORMAT_IMA_ADPCM;
#ifdef AFMT_MPEG
	case AFMT_MPEG:
		return AF_FORMAT_MPEG2;
#endif
#ifdef AFMT_AC3
	case AFMT_AC3:
		return AF_FORMAT_AC3;
#endif
	}

	return -1;
}

static char* dsp = PATH_DEV_DSP;
static audio_buf_info zz;
static int audio_fd = -1;
static int prepause_space;

static const char* oss_mixer_device = PATH_DEV_MIXER;
static int oss_mixer_channel = SOUND_MIXER_PCM;

static int audio_delay_method = 2;

void aout_oss_reset(GxAudioOut*  ao)
{
	int oss_format;
	aout_oss_close(ao, 1);
	audio_fd = open(dsp, O_WRONLY);
	if (audio_fd < 0) {
		return;
	}
#if defined(FD_CLOEXEC) && defined(F_SETFD)
	fcntl(audio_fd, F_SETFD, FD_CLOEXEC);
#endif

	oss_format = format2oss(ao->header->format);
	GxCore_Ioctl(audio_fd, SNDCTL_DSP_SETFMT, &oss_format);
	if (ao->header->format != AF_FORMAT_AC3) {
		if (ao->header->channels > 2)
			GxCore_Ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &ao->header->channels);
		else {
			int c = ao->header->channels - 1;
			GxCore_Ioctl(audio_fd, SNDCTL_DSP_STEREO, &c);
		}
		GxCore_Ioctl(audio_fd, SNDCTL_DSP_SPEED, &ao->header->samplerate);
	}
}

int aout_oss_open(GxAudioOut*  ao)
{
	char* mixer_channels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
	int oss_format;
	int format = ao->header->format;
	int rate = ao->header->samplerate;
	int channels = ao->header->channels;
	char* mdev = mixer_device, *mchan = mixer_channel;
	AoOssPriv* oss_priv;
	
	oss_priv =  av_malloc(sizeof(AoOssPriv));

	if(!oss_priv)
		return GX_PLAYER_ERROR;
	
	ao->priv = (void*)oss_priv;

	oss_priv->format = ao->header->format;
	oss_priv->channels= ao->header->channels;
	oss_priv->samplerate= ao->header->samplerate;
	

	if (mdev)
		oss_mixer_device = mdev;
	else
		oss_mixer_device = PATH_DEV_MIXER;

	if(mdev)
		oss_mixer_device=mdev;
	else
		oss_mixer_device=PATH_DEV_MIXER;

	if(mchan){
		int fd, devs, i;
		if ((fd = open(oss_mixer_device, O_RDONLY)) != -1){
			GxCore_Ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
			close(fd);

			for (i=0; i<SOUND_MIXER_NRDEVICES; i++){
				if(!strcasecmp(mixer_channels[i], mchan)){
					if(!(devs & (1 << i))){
						i = SOUND_MIXER_NRDEVICES+1;
						break;
					}
					oss_mixer_channel = i;
					break;
				}
			}

		}
	} 
	else
	    oss_mixer_channel = SOUND_MIXER_PCM;


#ifdef LINUX_OS
	audio_fd=open(dsp, O_WRONLY | O_NONBLOCK);
#else
	audio_fd=open(dsp, O_WRONLY);
#endif
	if(audio_fd<0){
		return GX_PLAYER_ERROR;
  }

#ifdef LINUX_OS
	if(fcntl(audio_fd, F_SETFL, 0) < 0) {
		return GX_PLAYER_ERROR;
	}
#endif

#if defined(FD_CLOEXEC) && defined(F_SETFD)
	fcntl(audio_fd, F_SETFD, FD_CLOEXEC);
#endif

	if(format == AF_FORMAT_AC3) {
		oss_priv->samplerate=rate;
		GxCore_Ioctl (audio_fd, SNDCTL_DSP_SPEED, &oss_priv->samplerate);
	}

ac3_retry:
	oss_priv->format=format;
	oss_format=format2oss(format);
	if (oss_format == -1) {
#ifdef HAVE_BIGENDIAN
		oss_format=AFMT_S16_BE;
#else
		oss_format=AFMT_S16_LE;
#endif
		format=AF_FORMAT_S16_NE;
  }
	if( GxCore_Ioctl(audio_fd, SNDCTL_DSP_SETFMT, &oss_format)<0 ||
		oss_format != format2oss(format)) {
		format=AF_FORMAT_S16_NE;
		goto ac3_retry;
	}

	oss_priv->format = oss2format(oss_format);
	if (oss_priv->format == -1) 
		return 0;

	oss_priv->channels = channels;
	if(format != AF_FORMAT_AC3) {
	    if (oss_priv->channels > 2) {
			if(GxCore_Ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &oss_priv->channels) == -1 || oss_priv->channels != channels ) {
				return GX_PLAYER_ERROR;
			}
		}
		else {
			int c = oss_priv->channels-1;
			if (GxCore_Ioctl (audio_fd, SNDCTL_DSP_STEREO, &c) == -1) {
				return GX_PLAYER_ERROR;
			}
			oss_priv->channels=c+1;
	    }
		// set rate
		oss_priv->samplerate=rate;
		GxCore_Ioctl (audio_fd, SNDCTL_DSP_SPEED, &oss_priv->samplerate);
	}

	if(GxCore_Ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &zz)==-1){
		int r=0;
		if(GxCore_Ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &r)!=-1) {
			oss_priv->outburst=r;
		}
	} 
	else {
		if(oss_priv->buffersize==-1) 
			oss_priv->buffersize=zz.bytes;
		oss_priv->outburst=zz.fragsize;
	}

	if(oss_priv->buffersize==-1){
		// Measuring buffer size:
		void* data;
		oss_priv->buffersize=0;
#ifdef HAVE_AUDIO_SELECT
		data=av_malloc(oss_priv->outburst); memset(data,0,oss_priv->outburst);
		while(oss_priv->buffersize<0x40000){
			fd_set rfds;
			struct timeval tv;
			FD_ZERO(&rfds); FD_SET(audio_fd,&rfds);
			tv.tv_sec=0; tv.tv_usec = 0;
			if(!select(audio_fd+1, NULL, &rfds, NULL, &tv)) break;
			write(audio_fd,data,oss_priv->outburst);
			oss_priv->buffersize+=oss_priv->outburst;
		}
		av_free(data);
		if(oss_priv->buffersize==0){
			return GX_PLAYER_ERROR;
		}
#endif
	}

	oss_priv->bps=oss_priv->channels;
	switch (oss_priv->format & AF_FORMAT_BITS_MASK) {
		case AF_FORMAT_8BIT:
			break;
		case AF_FORMAT_16BIT:
			oss_priv->bps*=2;
			break;
		case AF_FORMAT_24BIT:
			oss_priv->bps*=3;
			break;
		case AF_FORMAT_32BIT:
			oss_priv->bps*=4;
			break;
	}

	oss_priv->outburst -= oss_priv->outburst % oss_priv->bps;	
	oss_priv->bps*= oss_priv->samplerate;
	oss_priv->outburst = 10240;
	
	return 0;
}

int aout_oss_close(GxAudioOut*  ao, int immed)
{
	if (audio_fd == -1)
		return GX_PLAYER_ERROR;
	
#ifdef SNDCTL_DSP_SYNC
	if (!immed)
		GxCore_Ioctl(audio_fd, SNDCTL_DSP_SYNC, NULL);
#endif

#ifdef SNDCTL_DSP_RESET
	if (immed)
		GxCore_Ioctl(audio_fd, SNDCTL_DSP_RESET, NULL);
#endif

	close(audio_fd);
	audio_fd = -1;
	
	return GX_PLAYER_OK;
}


int aout_oss_get_space(GxAudioOut*  ao)
{
	AoOssPriv* oss_priv;
	oss_priv = (AoOssPriv* )ao->priv;
	
	int playsize = oss_priv->outburst;

#ifdef SNDCTL_DSP_GETOSPACE
	if (GxCore_Ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &zz) != -1) {
		playsize = zz.fragments*  zz.fragsize;
		if (playsize > MAX_OUTBURST)
			playsize = (MAX_OUTBURST / zz.fragsize)*  zz.fragsize;
		return playsize;
	}
#endif

#ifdef HAVE_AUDIO_SELECT
	{
		fd_set rfds;
		struct timeval tv;
		FD_ZERO(&rfds);
		FD_SET(audio_fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		if (!select(audio_fd + 1, NULL, &rfds, NULL, &tv))
			return 0;	 
	}
#endif

	return oss_priv->outburst;
}

int aout_oss_play(GxAudioOut*  ao, void *data, int len, int flags)
{
	AoOssPriv* oss_priv;
	oss_priv = (AoOssPriv* )ao->priv;

	if (len == 0)
		return len;
	if (len > oss_priv->outburst || !(flags & 0)) {
		len /= oss_priv->outburst;
		len*= oss_priv->outburst;
	}
	len = write(audio_fd, data, len);
	return len;
}

float aout_oss_get_delay(GxAudioOut*  ao)
{
	AoOssPriv* oss_priv;
	oss_priv = (AoOssPriv* )ao->priv;

	/* Calculate how many bytes/second is sent out*/
	if (audio_delay_method == 2) {
#ifdef SNDCTL_DSP_GETODELAY
		int r = 0;
		if (GxCore_Ioctl(audio_fd, SNDCTL_DSP_GETODELAY, &r) != -1)
			return ((float)r) / (float)oss_priv->bps;
#endif
		audio_delay_method = 1;	// fallback if not supported
	}
	if (audio_delay_method == 1) {
		// SNDCTL_DSP_GETOSPACE
		if (GxCore_Ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &zz) != -1)
			return ((float)(oss_priv->buffersize - zz.bytes)) / (float)oss_priv->bps;
		audio_delay_method = 0;	// fallback if not supported
	}
	return ((float)oss_priv->buffersize) / (float)oss_priv->bps;
}

void aout_oss_pause(GxAudioOut*  ao)
{
	prepause_space = aout_oss_get_space(ao);
	aout_oss_close(ao, 1);
}

void aout_oss_resume(GxAudioOut*  ao)
{
	int fillcnt;
	aout_oss_reset(ao);
	fillcnt = aout_oss_get_space(ao) - prepause_space;
	if (fillcnt > 0) {
		void* silence = av_calloc(fillcnt, 1);
		aout_oss_play(ao, silence, fillcnt, 0);
		av_free(silence);
	}
}

int aout_oss_control(GxAudioOut*  ao, int cmd, void * arg)
{

	switch (cmd) {
		case GX_AO_GET_SPACE:
			{
				int space = aout_oss_get_space(ao);
				*(int*)arg = space;
				break;
			}

		default :
			break;
	}
	return GX_PLAYER_OK;
}

GxAudioOutClass gx_aout_oss = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Aout Base",
			.parent  = &gx_AudioOutBase,
			.size    = sizeof(GxAudioOut),
			.version = {1, 0},
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	.author = {
		.info    = "AudioOut base",
		.name    = "",
		.desc    = "",
		.author  = "L.F.",
		.comment = "based on the code from mplayer (probably Arpi)"},

	.name    = "OSS",
	.open    = aout_oss_open,
	.close   = aout_oss_close,
	.control = aout_oss_control,
	.play    = aout_oss_play,
};

#endif

