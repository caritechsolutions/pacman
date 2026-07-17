#ifndef __AOUT_H__
#define __AOUT_H__

typedef enum {
	SRC_R0 = (1<<0),
	SRC_R1 = (1<<1),
	SRC_R2 = (1<<2),
	SRC_R3 = (1<<3)
} AoutStreamSrc;

typedef enum {
	DAC_MCLK_256FS   = 0x0,
	DAC_MCLK_384FS   = 0x1,
	DAC_MCLK_512FS   = 0x2,
	DAC_MCLK_768FS   = 0x3,
	DAC_MCLK_1024FS  = 0x4,
	DAC_MCLK_1536FS  = 0x5,
	DAC_MCLK_128FS   = 0x6,
	DAC_MCLK_192FS   = 0x7
} AoutStreamDacMclk;

typedef enum {
	DAC_BCLK_32FS = 0x0,
	DAC_BCLK_48FS = 0x1,
	DAC_BCLK_64FS = 0x2
} AoutStreamDacBclk;

typedef enum {
	DAC_FORMAT_I2S   = 0x0,
	DAC_FORMAT_LEFT  = 0x1,
	DAC_FORMAT_RIGHT = 0x2
} AoutStreamDacFormat;

typedef enum {
	PCM_OUTPUT_16_BITS = 0x0,
	PCM_OUTPUT_18_BITS = 0x1,
	PCM_OUTPUT_20_BITS = 0x2,
	PCM_OUTPUT_24_BITS = 0x3
} AoutStreamPcmOutputBits;

typedef enum {
	PCM_CHANNEL_1 = 0x0,
	PCM_CHANNEL_2 = 0x1,
	PCM_CHANNEL_3 = 0x2,
	PCM_CHANNEL_4 = 0x3,
	PCM_CHANNEL_5 = 0x4,
	PCM_CHANNEL_6 = 0x5,
	PCM_CHANNEL_7 = 0x6,
	PCM_CHANNEL_8 = 0x7
} AoutStreamPcmChNum;

typedef enum {
	PCM_SOURCE_SDRAM   = 0x0,
	PCM_SOURCE_CPU     = 0x1,
	PCM_SOURCE_AUTOGEN = 0x2,
	PCM_SOURCE_M_SEQ   = 0x3
} AoutStreamPcmSource;


#endif
