#ifndef __AOUT_REG_H__
#define __AOUT_REG_H__

typedef struct {
	unsigned int AUDIO_PLAY_I2S_DACINFO;       //0x00
	unsigned int AUDIO_PLAY_I2S_INDATA;
	unsigned int AUDIO_PLAY_I2S_OUTDATA;
	unsigned int AUDIO_PLAY_I2S_CHANNELsel;
	unsigned int AUDIO_PLAY_I2S_CPUSetPCM;     //0x10
	unsigned int AUDIO_PLAY_I2S_FIFOCTRL;
	unsigned int AUDIO_PLAY_I2S_DATAINV;
	unsigned int AUDIO_PLAY_I2S_INTEN;
	unsigned int AUDIO_PLAY_I2S_INT;           //0x20
	unsigned int AUDIO_PLAY_I2S_TIME_GATE;
	unsigned int AUDIO_PLAY_I2S_playingINDATA;
} AoutI2SRegs;

typedef struct {
	AoutI2SRegs *i2sRegs_0;  //baseAddr: Audio_play_I2S Reg
	AoutI2SRegs *i2sRegs_1;  //baseAddr: Audio_play_I2S Reg + 0x40
} AoutRegs;

static unsigned int mask[33] = {
	0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
	0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

#define AOUT_REG_SET_VAL(reg, value) do {                                \
	(*(volatile unsigned int *)reg) = value;                             \
} while(0)

#define AOUT_REG_GET_VAL(reg)                                            \
	(*(volatile unsigned int *)reg)

#define AOUT_REG_SET_FIELD(reg, val, c, offset) do {                     \
	unsigned int Reg = *(volatile unsigned int *)reg;                    \
	Reg &= ~((mask[c])<<(offset));                                       \
	Reg |= ((val)&(mask[c]))<<(offset);                                  \
	(*(volatile unsigned int *)reg) = Reg;                               \
} while(0)


static void aout_dac_set_mclk(AoutRegs *optReg, AoutStreamDacMclk value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 3, 4);
}

static void aout_dac_set_bclk(AoutRegs *optReg, AoutStreamDacBclk value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 2, 2);
}

static void aout_dac_set_format(AoutRegs *optReg, AoutStreamDacFormat value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 2, 0);
}

static void aout_src_set_pcm_output_bits(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmOutputBits value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), value, 2, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), value, 2, 1);
	}
}

static void aout_src_set_pcm_channelnum(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmChNum value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 4, 0);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 4, 0);
	}
}

static void aout_src_set_pcm_source(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmSource value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), value, 2, 6);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), value, 2, 6);
	}
}

static void aout_src_set_pcm_value(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CPUSetPCM), value);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CPUSetPCM), value);
	}
}

static void aout_src_enable_playback(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 3);
	}
}

static void aout_src_enable_work(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 4);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 4);
	}
}

static void aout_src_enable_reset(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 8);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 8);
	}
}

static void aout_src_enable_silent_end(AoutRegs *optReg, AoutStreamSrc src, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 0);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 0);
	}
}
#endif
