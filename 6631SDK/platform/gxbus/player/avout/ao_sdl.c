
#include <SDL/SDL.h>

#include "ao.h"
#include "aformat.h"

#define USE_SDL_INTERNAL_MIXER

#define SAMPLESIZE 2048

#define CHUNK_SIZE 4096
#define NUM_CHUNKS 8

#define BUFFSIZE ((NUM_CHUNKS + 1)*  CHUNK_SIZE)

typedef struct ao_control_vol_s {
	float left;
	float right;
} ao_control_vol_t;

static unsigned char* sdl_buffer;

// may only be modified by SDL's playback thread or while it is stopped
static volatile int read_pos;
// may only be modified by mplayer's thread
static volatile int write_pos;
#ifdef USE_SDL_INTERNAL_MIXER
static unsigned char volume = SDL_MIX_MAXVOLUME;
#endif

static int buf_used(void)
{
	int used = write_pos - read_pos;
	if (used < 0)
		used += BUFFSIZE;
	return used;
}

static int buf_free(void)
{
	int av_free = read_pos - write_pos - CHUNK_SIZE;
	if (av_free < 0)
		av_free += BUFFSIZE;
	return av_free;
}

static int read_buffer(unsigned char* data, int len)
{
	int first_len = BUFFSIZE - read_pos;
	int buffered = buf_used();
	if (len > buffered)
		len = buffered;
	if (first_len > len)
		first_len = len;
	// till end of buffer
#ifdef USE_SDL_INTERNAL_MIXER
	SDL_MixAudio(data, &sdl_buffer[read_pos], first_len, volume);
#else
	memcpy(data, &sdl_buffer[read_pos], first_len);
#endif
	if (len > first_len) {	// we have to wrap around
		// remaining part from beginning of buffer
#ifdef USE_SDL_INTERNAL_MIXER
		SDL_MixAudio(&data[first_len], sdl_buffer, len - first_len, volume);
#else
		memcpy(&data[first_len], sdl_buffer, len - first_len);
#endif
	}
	read_pos = (read_pos + len) % BUFFSIZE;
	return len;
}

static int write_buffer(unsigned char* data, int len)
{
	int first_len = BUFFSIZE - write_pos;
	int av_free = buf_free();
	if (len > av_free)
		len = av_free;
	if (first_len > len)
		first_len = len;
	// till end of buffer
	memcpy(&sdl_buffer[write_pos], data, first_len);
	if (len > first_len) {	// we have to wrap around
		// remaining part from beginning of buffer
		memcpy(sdl_buffer, &data[first_len], len - first_len);
	}
	write_pos = (write_pos + len) % BUFFSIZE;
	return len;
}

// SDL Callback function
void outputaudio(void* unused, Uint8 * stream, int len)
{

	read_buffer(stream, len);
}

void aout_sdl_reset(GxAudioOut*  ao)
{
	SDL_PauseAudio(1);
	/* Reset ring-buffer state*/
	read_pos = 0;
	write_pos = 0;
	SDL_PauseAudio(0);
}

int aout_sdl_open(GxAudioOut*  ao, void *arg)
{

	/* SDL Audio Specifications*/
	SDL_AudioSpec aspec, obtained;

	/* Allocate ring-buffer memory*/
	sdl_buffer = (unsigned char* )av_malloc(BUFFSIZE);

	ao->bps = ao->channels*  ao->samplerate;
	if (ao->format != AF_FORMAT_U8 && ao->format != AF_FORMAT_S8)
		ao->bps*= 2;

	/* The desired audio format (see SDL_AudioSpec)*/
	switch (ao->format) {
	case AF_FORMAT_U8:
		aspec.format = AUDIO_U8;
		break;
	case AF_FORMAT_S16_LE:
		aspec.format = AUDIO_S16LSB;
		break;
	case AF_FORMAT_S16_BE:
		aspec.format = AUDIO_S16MSB;
		break;
	case AF_FORMAT_S8:
		aspec.format = AUDIO_S8;
		break;
	case AF_FORMAT_U16_LE:
		aspec.format = AUDIO_U16LSB;
		break;
	case AF_FORMAT_U16_BE:
		aspec.format = AUDIO_U16MSB;
		break;
	default:
		aspec.format = AUDIO_S16LSB;
		ao->format = AF_FORMAT_S16_LE;

	}

	/* The desired audio frequency in samples-per-second.*/
	aspec.freq = ao->samplerate;

	/* Number of channels (mono/stereo)*/
	aspec.channels = ao->channels;

	/* The desired size of the audio buffer in samples. This number should be a power of two, and may be adjusted by the audio driver to a value more suitable for the hardware. Good values seem to range between 512 and 8192 inclusive, depending on the application and CPU speed. Smaller values yield faster response time, but can lead to underflow if the application is doing heavy processing and cannot fill the audio buffer in time. A stereo sample consists of both right and left channels in LR ordering. Note that the number of samples is directly related to time by the following formula: ms = (samples*1000)/freq*/
	aspec.samples = SAMPLESIZE;

	/* This should be set to a function that will be called when the audio device is ready for more data. It is passed a pointer to the audio buffer, and the length in bytes of the audio buffer. This function usually runs in a separate thread, and so you should protect data structures that it accesses by calling SDL_LockAudio and SDL_UnlockAudio in your code. The callback prototype is:
	   void callback(void* userdata, Uint8 *stream, int len); userdata is the pointer stored in userdata field of the SDL_AudioSpec. stream is a pointer to the audio buffer you want to fill with information and len is the length of the audio buffer in bytes. */
	aspec.callback = outputaudio;

	/* This pointer is passed as the first parameter to the callback function.*/
	aspec.userdata = NULL;

	/* initialize the SDL Audio system*/
	if (SDL_Init(SDL_INIT_AUDIO /*|SDL_INIT_NOPARACHUTE*/ )) {
		return -1;
	}

	/* Open the audio device and start playing sound!*/
	if (SDL_OpenAudio(&aspec, &obtained) < 0) {
		return -1;
	}

	/* did we got what we wanted ?*/
	ao->channels = obtained.channels;
	ao->samplerate = obtained.freq;

	switch (obtained.format) {
	case AUDIO_U8:
		ao->format = AF_FORMAT_U8;
		break;
	case AUDIO_S16LSB:
		ao->format = AF_FORMAT_S16_LE;
		break;
	case AUDIO_S16MSB:
		ao->format = AF_FORMAT_S16_BE;
		break;
	case AUDIO_S8:
		ao->format = AF_FORMAT_S8;
		break;
	case AUDIO_U16LSB:
		ao->format = AF_FORMAT_U16_LE;
		break;
	case AUDIO_U16MSB:
		ao->format = AF_FORMAT_U16_BE;
		break;
	default:
		return -1;
	}

	ao->buffersize = obtained.size;
	ao->outburst = CHUNK_SIZE;

	aout_sdl_reset(ao);
	/* unsilence audio, if callback is ready*/
	SDL_PauseAudio(0);

	return 0;
}

void aout_sdl_close(GxAudioOut*  ao, int immed)
{
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int aout_sdl_control(GxAudioOut*  ao, int cmd, va_list arg)
{
#ifdef USE_SDL_INTERNAL_MIXER
	switch (cmd) {
	case AOCONTROL_GET_VOLUME:
		{
			ao_control_vol_t* vol = (ao_control_vol_t *) arg;
			vol->left = vol->right = volume*  100 / SDL_MIX_MAXVOLUME;
			return CONTROL_OK;
		}
	case AOCONTROL_SET_VOLUME:
		{
			int diff;
			ao_control_vol_t* vol = (ao_control_vol_t *) arg;
			diff = (vol->left + vol->right) / 2;
			volume = diff*  SDL_MIX_MAXVOLUME / 100;
			return CONTROL_OK;
		}
	}
#endif
	return CONTROL_UNKNOWN;
}

int aout_sdl_get_space(GxAudioOut*  ao)
{
	return buf_free();
}

int aout_sdl_play(GxAudioOut*  ao, void *data, int len, int flags)
{

	if (!(flags & AOPLAY_FINAL_CHUNK))
		len = (len / ao->outburst)*  ao->outburst;
#if 0
	int ret;

	/* Audio locking prohibits call of outputaudio*/
	SDL_LockAudio();
	// copy audio stream into ring-buffer 
	ret = write_buffer(data, len);
	SDL_UnlockAudio();

	return ret;
#else
	return write_buffer(data, len);
#endif
}

float aout_sdl_get_delay(GxAudioOut*  ao)
{
	int buffered = BUFFSIZE - CHUNK_SIZE - buf_free();	// could be less
	return (float)(buffered + ao->buffersize) / (float)ao->bps;
}

void aout_sdl_pause(GxAudioOut*  ao)
{

	SDL_PauseAudio(1);

}

void aout_sdl_resume(GxAudioOut*  ao)
{
	SDL_PauseAudio(0);
}

GxAudioOutClass gx_aout_sdl = {
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

	.name      = GX_AO_DEV_NAME_SDL,
	.open      = aout_sdl_open,
	.close     = aout_sdl_close,
	.control   = aout_sdl_control,
	.reset     = aout_sdl_reset,
	.get_space = aout_sdl_get_space,
	.play      = aout_sdl_play,
	.get_delay = aout_sdl_get_delay,
	.pause     = aout_sdl_pause,
	.resume    = aout_sdl_resume,
};
