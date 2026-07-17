#include "./dw_hdmi.h"
#include "sys/types.h"
#include "gx_api.h"
#if defined CONFIG_ARCH_ARM_SIRIUS || defined CONFIG_ARCH_ARM_CANOPUS || defined CONFIG_ARCH_ARM_VEGA
#define HDMI_WR_BASE_ADDR (0x8ae00000)
#else
#define HDMI_WR_BASE_ADDR (0xa4f00000)
#endif
static const u16 AUD_BASE_ADDR = 0x3100;
static const u16 AUDIO_DMA   = 0x0500;
static const u8 AHB_DMA_MASK      = 0x14;
static const u8 AHB_DMA_BUFFMASK  = 0x19;

const u16 ID_BASE_ADDR = 0x0000;
const u16 IH_BASE_ADDR = 0x0100;
static const u8 IH_MUTE = 0xFF;

static u16 dtd_GetPixelRepetitionInput(const dtd_t *dtd)
{
	return dtd->mPixelRepetitionInput;
}
static u16 dtd_GetPixelClock(const dtd_t *dtd)
{
	return dtd->mPixelClock;
}
u16 dtd_GetHImageSize(const dtd_t *dtd)
{
	return dtd->mHImageSize;
}
u16 dtd_GetVImageSize(const dtd_t *dtd)
{
	return dtd->mVImageSize;
}
u8 dtd_GetCode(const dtd_t *dtd)
{
	return dtd->mCode;
}


void videoParams_Reset(videoParams_t *params)
{
	params->mHdmi = 0;
	params->mEncodingOut = RGB;
	params->mEncodingIn = YCC444;
	params->mColorResolution = 8;
	params->mPixelRepetitionFactor = 0;
	params->mRgbQuantizationRange = 0;
	params->mPixelPackingDefaultPhase = 0;
	params->mColorimetry = 0;
	params->mScanInfo = 0;
	params->mActiveFormatAspectRatio = 8;
	params->mNonUniformScaling = 0;
	params->mExtColorimetry = ~0;
	params->mItContent = 0;
	params->mEndTopBar = ~0;
	params->mStartBottomBar = ~0;
	params->mEndLeftBar = ~0;
	params->mStartRightBar = ~0;
	params->mCscFilter = 0;
	params->mHdmiVideoFormat = 0;
	params->m3dStructure = 0;
	params->m3dExtData = 0;
	params->mHdmiVic = 0;
}

u8 videoParams_GetActiveFormatAspectRatio(videoParams_t *params)
{
	return params->mActiveFormatAspectRatio;
}

u8 videoParams_GetColorimetry(videoParams_t *params)
{
	return params->mColorimetry;
}

u8 videoParams_GetColorResolution(videoParams_t *params)
{
	return params->mColorResolution;
}

u8 videoParams_GetCscFilter(videoParams_t *params)
{
	return params->mCscFilter;
}

dtd_t* videoParams_GetDtd(videoParams_t *params)
{
	return &(params->mDtd);
}

encoding_t videoParams_GetEncodingIn(videoParams_t *params)
{
	return params->mEncodingIn;
}

encoding_t videoParams_GetEncodingOut(videoParams_t *params)
{
	return params->mEncodingOut;
}

u16 videoParams_GetEndLeftBar(videoParams_t *params)
{
	return params->mEndLeftBar;
}

u16 videoParams_GetEndTopBar(videoParams_t *params)
{
	return params->mEndTopBar;
}

u8 videoParams_GetExtColorimetry(videoParams_t *params)
{
	return params->mExtColorimetry;
}

u8 videoParams_GetHdmi(videoParams_t *params)
{
	return params->mHdmi;
}

u8 videoParams_GetItContent(videoParams_t *params)
{
	return params->mItContent;
}

u8 videoParams_GetNonUniformScaling(videoParams_t *params)
{
	return params->mNonUniformScaling;
}

u8 videoParams_GetPixelPackingDefaultPhase(videoParams_t *params)
{
	return params->mPixelPackingDefaultPhase;
}

u8 videoParams_GetPixelRepetitionFactor(videoParams_t *params)
{
	return params->mPixelRepetitionFactor;
}

u8 videoParams_GetRgbQuantizationRange(videoParams_t *params)
{
	return params->mRgbQuantizationRange;
}

u8 videoParams_GetScanInfo(videoParams_t *params)
{
	return params->mScanInfo;
}

u16 videoParams_GetStartBottomBar(videoParams_t *params)
{
	return params->mStartBottomBar;
}

u16 videoParams_GetStartRightBar(videoParams_t *params)
{
	return params->mStartRightBar;
}

u16 videoParams_GetCscScale(videoParams_t *params)
{
	return params->mCscScale;
}

void videoParams_SetActiveFormatAspectRatio(videoParams_t *params, u8 value)
{
	params->mActiveFormatAspectRatio = value;
}

void videoParams_SetColorimetry(videoParams_t *params, u8 value)
{
	params->mColorimetry = value;
}

void videoParams_SetColorResolution(videoParams_t *params, u8 value)
{
	params->mColorResolution = value;
}

void videoParams_SetCscFilter(videoParams_t *params, u8 value)
{
	params->mCscFilter = value;
}

void videoParams_SetDtd(videoParams_t *params, dtd_t *dtd)
{
	params->mDtd = *dtd;
}

void videoParams_SetEncodingIn(videoParams_t *params, encoding_t value)
{
	params->mEncodingIn = value;
}

void videoParams_SetEncodingOut(videoParams_t *params, encoding_t value)
{
	params->mEncodingOut = value;
}

void videoParams_SetEndLeftBar(videoParams_t *params, u16 value)
{
	params->mEndLeftBar = value;
}

void videoParams_SetEndTopBar(videoParams_t *params, u16 value)
{
	params->mEndTopBar = value;
}

void videoParams_SetExtColorimetry(videoParams_t *params, u8 value)
{
	params->mExtColorimetry = value;
}

void videoParams_SetHdmi(videoParams_t *params, u8 value)
{
	params->mHdmi = value;
}

void videoParams_SetItContent(videoParams_t *params, u8 value)
{
	params->mItContent = value;
}

void videoParams_SetNonUniformScaling(videoParams_t *params, u8 value)
{
	params->mNonUniformScaling = value;
}

void videoParams_SetPixelPackingDefaultPhase(videoParams_t *params, u8 value)
{
	params->mPixelPackingDefaultPhase = value;
}

void videoParams_SetPixelRepetitionFactor(videoParams_t *params, u8 value)
{
	params->mPixelRepetitionFactor = value;
}

void videoParams_SetRgbQuantizationRange(videoParams_t *params, u8 value)
{
	params->mRgbQuantizationRange = value;
}

void videoParams_SetScanInfo(videoParams_t *params, u8 value)
{
	params->mScanInfo = value;
}

void videoParams_SetStartBottomBar(videoParams_t *params, u16 value)
{
	params->mStartBottomBar = value;
}

void videoParams_SetStartRightBar(videoParams_t *params, u16 value)
{
	params->mStartRightBar = value;
}

u16 * videoParams_GetCscA(videoParams_t *params)
{
	videoParams_UpdateCscCoefficients(params);
	return params->mCscA;
}

void videoParams_SetCscA(videoParams_t *params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscA) / sizeof(params->mCscA[0]); i++)
	{
		params->mCscA[i] = value[i];
	}
}

u16 * videoParams_GetCscB(videoParams_t *params)
{
	videoParams_UpdateCscCoefficients(params);
	return params->mCscB;
}

void videoParams_SetCscB(videoParams_t *params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscB) / sizeof(params->mCscB[0]); i++)
	{
		params->mCscB[i] = value[i];
	}
}

u16 * videoParams_GetCscC(videoParams_t *params)
{
	videoParams_UpdateCscCoefficients(params);
	return params->mCscC;
}

void videoParams_SetCscC(videoParams_t *params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mCscC) / sizeof(params->mCscC[0]); i++)
	{
		params->mCscC[i] = value[i];
	}
}

void videoParams_SetCscScale(videoParams_t *params, u16 value)
{
	params->mCscScale = value;
}

/* [0.01 MHz] */
u16 videoParams_GetPixelClock(videoParams_t *params)
{
	if (videoParams_GetHdmiVideoFormat(params) == 2)
	{
		if (videoParams_Get3dStructure(params) == 0)
		{
			return 2 * dtd_GetPixelClock(&(params->mDtd));
		}
	}
	return dtd_GetPixelClock(&(params->mDtd));
}

/* [0.01 MHz] */
u16 videoParams_GetTmdsClock(videoParams_t *params)
{
	return (u16) ((u32) (videoParams_GetPixelClock(params)
			* (u32) (videoParams_GetRatioClock(params))) / 100);
}

/* 0.01 */
unsigned videoParams_GetRatioClock(videoParams_t *params)
{
	unsigned ratio = 100;

	return ratio * (params->mPixelRepetitionFactor + 1);
}

int videoParams_IsColorSpaceConversion(videoParams_t *params)
{
	return params->mEncodingIn != params->mEncodingOut;
}

int videoParams_IsColorSpaceDecimation(videoParams_t *params)
{
	return params->mEncodingOut == YCC422 && (params->mEncodingIn == RGB
			|| params->mEncodingIn == YCC444);
}

int videoParams_IsColorSpaceInterpolation(videoParams_t *params)
{
	return params->mEncodingIn == YCC422 && (params->mEncodingOut == RGB
			|| params->mEncodingOut == YCC444);
}

int videoParams_IsPixelRepetition(videoParams_t *params)
{
	return params->mPixelRepetitionFactor > 0 || dtd_GetPixelRepetitionInput(
			&(params->mDtd)) > 0;
}
u8 videoParams_GetHdmiVideoFormat(videoParams_t *params)
{
	return params->mHdmiVideoFormat;
}

void videoParams_SetHdmiVideoFormat(videoParams_t *params, u8 value)
{
	params->mHdmiVideoFormat = value;
}

u8 videoParams_Get3dStructure(videoParams_t *params)
{
	return params->m3dStructure;
}

void videoParams_Set3dStructure(videoParams_t *params, u8 value)
{
	params->m3dStructure = value;
}

u8 videoParams_Get3dExtData(videoParams_t *params)
{
	return params->m3dExtData;
}

void videoParams_Set3dExtData(videoParams_t *params, u8 value)
{
	params->m3dExtData = value;
}

u8 videoParams_GetHdmiVic(videoParams_t *params)
{
	return params->mHdmiVic;
}

void videoParams_SetHdmiVic(videoParams_t *params, u8 value)
{
	params->mHdmiVic = value;
}

#if 1 //rgb
void videoParams_UpdateCscCoefficients(videoParams_t *params)
{
	u16 i = 0;
	if (!videoParams_IsColorSpaceConversion(params))
	{
		for (i = 0; i < 4; i++)
		{
			params->mCscA[i] = 0;
			params->mCscB[i] = 0;
			params->mCscC[i] = 0;
		}
		params->mCscA[0] = 0x2400;
		params->mCscB[1] = 0x2000;
		params->mCscC[2] = 0x2000;
		params->mCscScale = 1;
	}
	else if (videoParams_IsColorSpaceConversion(params) && params->mEncodingOut
			== RGB)
	{
			params->mCscA[0] = 0x2400;
			params->mCscA[1] = 0x7106;
			params->mCscA[2] = 0x7a02;
			params->mCscA[3] = 0x0081;

			params->mCscB[0] = 0x2400;
			params->mCscB[1] = 0x3264;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e47;

			params->mCscC[0] = 0x2400;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x3b61;
			params->mCscC[3] = 0x7dff;

			params->mCscScale = 1;
	}
	/* else use user coefficients */
}
#else
void videoParams_UpdateCscCoefficients(videoParams_t *params)
{
	u16 i = 0;
	if (!videoParams_IsColorSpaceConversion(params))
	{
		for (i = 0; i < 4; i++)
		{
			params->mCscA[i] = 0;
			params->mCscB[i] = 0;
			params->mCscC[i] = 0;
		}
		params->mCscA[0] = 0x2000;
		params->mCscB[1] = 0x2000;
		params->mCscC[2] = 0x2000;
		params->mCscScale = 1;
	}
	else if (videoParams_IsColorSpaceConversion(params) && params->mEncodingOut
			== RGB)
	{
		if (params->mColorimetry == ITU601)
		{
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x6926;
			params->mCscA[2] = 0x74fd;
			params->mCscA[3] = 0x010e;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x2cdd;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e9a;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x38b4;
			params->mCscC[3] = 0x7e3b;

			params->mCscScale = 1;
		}
		else if (params->mColorimetry == ITU709)
		{
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x7106;
			params->mCscA[2] = 0x7a02;
			params->mCscA[3] = 0x00a7;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x3264;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e6d;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x3b61;
			params->mCscC[3] = 0x7e25;

			params->mCscScale = 1;
		}
	}
	else if (videoParams_IsColorSpaceConversion(params) && params->mEncodingIn
			== RGB)
	{
		if (params->mColorimetry == ITU601)
		{
			params->mCscA[0] = 0x2591;
			params->mCscA[1] = 0x1322;
			params->mCscA[2] = 0x074b;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x6535;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7acc;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6acd;
			params->mCscC[1] = 0x7534;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		}
		else if (params->mColorimetry == ITU709)
		{
			params->mCscA[0] = 0x2dc5;
			params->mCscA[1] = 0x0d9b;
			params->mCscA[2] = 0x049e;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x62f0;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7d11;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6756;
			params->mCscC[1] = 0x78ab;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		}
	}
	/* else use user coefficients */
}
#endif

static int dtd_Fill(dtd_t *dtd, u8 code)
{
	dtd->mCode = code;
	dtd->mHBorder = 0;
	dtd->mVBorder = 0;
	dtd->mPixelRepetitionInput = 0;
	dtd->mHImageSize = 16;
	dtd->mVImageSize = 9;

	switch (code)
	{
	case 2: /* 720x480p @ 59.94/60Hz 4:3 */
		dtd->mHImageSize = 4;
		dtd->mVImageSize = 3;
		dtd->mHActive = 720;
		dtd->mVActive = 480;
		dtd->mHBlanking = 138;
		dtd->mVBlanking = 45;
		dtd->mHSyncOffset = 16;
		dtd->mVSyncOffset = 9;
		dtd->mHSyncPulseWidth = 62;
		dtd->mVSyncPulseWidth = 6;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 2700;
		break;
	case 4: /* 1280x720p @ 59.94/60Hz 16:9 */
		dtd->mHActive = 1280;
		dtd->mVActive = 720;
		dtd->mHBlanking = 370;
		dtd->mVBlanking = 30;
		dtd->mHSyncOffset = 110;
		dtd->mVSyncOffset = 5;
		dtd->mHSyncPulseWidth = 40;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 7425;
		break;
	case 5: /* 1920x1080i @ 59.94/60Hz 16:9 */
		dtd->mHActive = 1920;
		dtd->mVActive = 540;
		dtd->mHBlanking = 280;
		dtd->mVBlanking = 22;
		dtd->mHSyncOffset = 88;
		dtd->mVSyncOffset = 2;
		dtd->mHSyncPulseWidth = 44;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 1;
		dtd->mPixelClock = 7425;
		break;
	case 6: /* 720(1440)x480i @ 59.94/60Hz 4:3 */
		dtd->mHImageSize = 4;
		dtd->mVImageSize = 3;
		dtd->mHActive = 1440;
		dtd->mVActive = 240;
		dtd->mHBlanking = 276;
		dtd->mVBlanking = 22;
		dtd->mHSyncOffset = 38;
		dtd->mVSyncOffset = 4;
		dtd->mHSyncPulseWidth = 124;
		dtd->mVSyncPulseWidth = 3;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
		dtd->mInterlaced = 1;
		dtd->mPixelClock = 2700;
		dtd->mPixelRepetitionInput = 1;
		break;
	case 16: /* 1920x1080p @ 59.94/60Hz 16:9 */
		dtd->mHActive = 1920;
		dtd->mVActive = 1080;
		dtd->mHBlanking = 280;
		dtd->mVBlanking = 45;
		dtd->mHSyncOffset = 88;
		dtd->mVSyncOffset = 4;
		dtd->mHSyncPulseWidth = 44;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 14850;
		break;
	case 17: /* 720x576p @ 50Hz 4:3 */
		dtd->mHImageSize = 4;
		dtd->mVImageSize = 3;
		dtd->mHActive = 720;
		dtd->mVActive = 576;
		dtd->mHBlanking = 144;
		dtd->mVBlanking = 49;
		dtd->mHSyncOffset = 12;
		dtd->mVSyncOffset = 5;
		dtd->mHSyncPulseWidth = 64;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 2700;
		break;
	case 19: /* 1280x720p @ 50Hz 16:9 */
		dtd->mHActive = 1280;
		dtd->mVActive = 720;
		dtd->mHBlanking = 700;
		dtd->mVBlanking = 30;
		dtd->mHSyncOffset = 440;
		dtd->mVSyncOffset = 5;
		dtd->mHSyncPulseWidth = 40;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 7425;
		break;
	case 20: /* 1920x1080i @ 50Hz 16:9 */
		dtd->mHActive = 1920;
		dtd->mVActive = 540;
		dtd->mHBlanking = 720;
		dtd->mVBlanking = 22;
		dtd->mHSyncOffset = 528;
		dtd->mVSyncOffset = 2;
		dtd->mHSyncPulseWidth = 44;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 1;
		dtd->mPixelClock = 7425;
		break;
	case 21: /* 720(1440)x576i @ 50Hz 4:3 */
		dtd->mHImageSize = 4;
		dtd->mVImageSize = 3;
		dtd->mHActive = 1440;
		dtd->mVActive = 288;
		dtd->mHBlanking = 288;
		dtd->mVBlanking = 24;
		dtd->mHSyncOffset = 24;
		//dtd->mVSyncOffset = 2;
		dtd->mVSyncOffset = 1;
		dtd->mHSyncPulseWidth = 126;
		dtd->mVSyncPulseWidth = 3;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 0;
		dtd->mInterlaced = 1;
		dtd->mPixelClock = 2700;
		dtd->mPixelRepetitionInput = 1;
		break;
	case 31: /* 1920x1080p @ 50Hz 16:9 */
		dtd->mHActive = 1920;
		dtd->mVActive = 1080;
		dtd->mHBlanking = 720;
		dtd->mVBlanking = 45;
		dtd->mHSyncOffset = 528;
		dtd->mVSyncOffset = 4;
		dtd->mHSyncPulseWidth = 44;
		dtd->mVSyncPulseWidth = 5;
		dtd->mHSyncPolarity = dtd->mVSyncPolarity = 1;
		dtd->mInterlaced = 0;
		dtd->mPixelClock = 14850;
		break;
	default:
		dtd->mCode = -1;
		return FALSE;
	}
	return TRUE;
}

static u8 *access_mBaseAddr = 0;
static u8 access_Read(u16 addr)
{
	u32 x= (u32)(addr<< 2) + HDMI_WR_BASE_ADDR;
	return access_mBaseAddr[x];
}

static void access_Write(u8 data, u16 addr)
{
	access_mBaseAddr[(u32)(addr<<2) + HDMI_WR_BASE_ADDR] = data;
}

#define BIT(n)		(1 << n)
static void access_CoreWrite(u8 data, u16 addr, u8 shift, u8 width)
{
	u8 temp = 0;
	u8 mask = 0;
	if (width <= 0) {
		return;
	}
	mask = BIT(width) - 1;
	if (data > mask) {
		return;
	}
	temp = access_Read(addr);
	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	access_Write(temp, addr);
}

static u8 access_CoreReadByte(u16 addr)
{
	u8 data = 0;
	data = access_Read(addr);
	return data;
}

static void access_CoreWriteByte(u8 data, u16 addr)
{
	access_Write(data, addr);
}

void halInterrupt_Mute(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + IH_MUTE));
}

int control_InterruptMute(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	halInterrupt_Mute(baseAddr + IH_BASE_ADDR, value);
	return TRUE;
}
/* register offsets */
#define FC_DBGFORCE  0x00
#define FC_DBGTMDS0  0x19
#define FC_DBGTMDS1  0x1A
#define FC_DBGTMDS2  0x1B
static void halFrameComposerDebug_ForceAudio(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, baseAddr + FC_DBGFORCE, 4, 1);
}

static void halFrameComposerDebug_ForceVideo(u16 baseAddr, u8 bit)
{
	/* avoid glitches */
	if (bit != 0)
	{
		access_CoreWriteByte(bit ? 0x00 : 0x00, baseAddr + FC_DBGTMDS2); /* R */
		access_CoreWriteByte(bit ? 0x00 : 0x00, baseAddr + FC_DBGTMDS1); /* G */
		access_CoreWriteByte(bit ? 0xFF : 0x00, baseAddr + FC_DBGTMDS0); /* B */
		access_CoreWrite(bit, baseAddr + FC_DBGFORCE, 0, 1);
	}
	else
	{
		access_CoreWrite(bit, baseAddr + FC_DBGFORCE, 0, 1);
		access_CoreWriteByte(bit ? 0x00 : 0x00, baseAddr + FC_DBGTMDS2); /* R */
		access_CoreWriteByte(bit ? 0x00 : 0x00, baseAddr + FC_DBGTMDS1); /* G */
		access_CoreWriteByte(bit ? 0xFF : 0x00, baseAddr + FC_DBGTMDS0); /* B */
	}
}

#define TX_BASE_ADDR  0x0200
#define VP_BASE_ADDR  0x0800
#define FC_BASE_ADDR  0x1000
#define MC_BASE_ADDR  0x4000
#define CSC_BASE_ADDR  0x4100
static int video_ForceOutput(u16 baseAddr, u8 force)
{
	halFrameComposerDebug_ForceAudio(baseAddr + FC_BASE_ADDR + 0x0200,
			0);
	halFrameComposerDebug_ForceVideo(baseAddr + FC_BASE_ADDR + 0x0200,
			force);
	return TRUE;
}

#define FC_AUDSCONF 0x63
#define USER_BIT (0)
static const u8 FC_AUDSSTAT = 0x64;
static const u8 FC_AUDSV = 0x65;
static const u8 FC_AUDSU = 0x66;
static const u8 FC_AUDSCHNLS0 = 0x67;
static const u8 FC_AUDSCHNLS1 = 0x68;
static const u8 FC_AUDSCHNLS2 = 0x69;
static const u8 FC_AUDSCHNLS3 = 0x6A;
static const u8 FC_AUDSCHNLS4 = 0x6B;
static const u8 FC_AUDSCHNLS5 = 0x6C;
static const u8 FC_AUDSCHNLS6 = 0x6D;
static const u8 FC_AUDSCHNLS7 = 0x6E;
static const u8 FC_AUDSCHNLS8 = 0x6F;
static const u16 AUDIO_I2S   = 0x0000;
static const u16 AUDIO_CLOCK = 0x0100;
static const u16 AUDIO_SPDIF = 0x0200;
static const u16 AUDIO_HBR   = 0x0300;
static const u16 AUDIO_GPA   = 0x0400;
static const u8 AUD_CONF0 = 0x00;
static const u8 AUD_CONF1 = 0x01;
static const u8 AUD_INT = 0x02;

void halAudioDma_DmaInterruptMask(u16 baseAddress, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWriteByte(mask, baseAddress + AHB_DMA_MASK);
}

void halAudioDma_BufferInterruptMask(u16 baseAddress, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWriteByte(mask, baseAddress + AHB_DMA_BUFFMASK);
}


static void halFrameComposerAudio_PacketSampleFlat(u16 baseAddr, u8 value)
{
	access_CoreWrite(value, (baseAddr + FC_AUDSCONF), 4, 4);
}

static int audio_Mute(u16 baseAddr, u8 state)
{
	/* audio mute priority: AVMUTE, sample flat, validity */
	/* AVMUTE also mutes video */
	halFrameComposerAudio_PacketSampleFlat(baseAddr + FC_BASE_ADDR,
			state ? 0xF : 0);

	return TRUE;
}

int audio_Initialize(u16 baseAddr)
{
	LOG_TRACE();
	halAudioDma_DmaInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, ~0);
	halAudioDma_BufferInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, ~0);
	return audio_Mute(baseAddr, 0);//1);
}

void audioParams_Reset(audioParams_t *params)
{
	params->mInterfaceType = I2S;
	params->mCodingType = PCM;
	params->mChannelAllocation = 0;
	params->mSampleSize = 16;
	params->mSamplingFrequency = 48000;
	params->mLevelShiftValue = 0;
	params->mDownMixInhibitFlag = 0;
	params->mOriginalSamplingFrequency = 0;
	params->mIecCopyright = 1;
	params->mIecCgmsA = 3;
	params->mIecPcmMode = 0;
	params->mIecCategoryCode = 0;
	params->mIecSourceNumber = 1;
	params->mIecClockAccuracy = 0;
	params->mPacketType = AUDIO_SAMPLE;
	params->mClockFsFactor = 512;
	params->mDmaBeatIncrement = DMA_UNSPECIFIED_INCREMENT;
	params->mDmaThreshold = 0;
	params->mDmaHlock = 0;
	params->mGpaInsertPucv = 0;
}

 u8 audioParams_GetChannelAllocation(audioParams_t *params)
{
	return params->mChannelAllocation;
}

void audioParams_SetChannelAllocation(audioParams_t *params, u8 value)
{
	params->mChannelAllocation = value;
}

 u8 audioParams_GetCodingType(audioParams_t *params)
{
	return params->mCodingType;
}

void audioParams_SetCodingType(audioParams_t *params, codingType_t value)
{
	params->mCodingType = value;
}

 u8 audioParams_GetDownMixInhibitFlag(audioParams_t *params)
{
	return params->mDownMixInhibitFlag;
}

void audioParams_SetDownMixInhibitFlag(audioParams_t *params, u8 value)
{
	params->mDownMixInhibitFlag = value;
}

 u8 audioParams_GetIecCategoryCode(audioParams_t *params)
{
	return params->mIecCategoryCode;
}

void audioParams_SetIecCategoryCode(audioParams_t *params, u8 value)
{
	params->mIecCategoryCode = value;
}

 u8 audioParams_GetIecCgmsA(audioParams_t *params)
{
	return params->mIecCgmsA;
}

void audioParams_SetIecCgmsA(audioParams_t *params, u8 value)
{
	params->mIecCgmsA = value;
}

 u8 audioParams_GetIecClockAccuracy(audioParams_t *params)
{
	return params->mIecClockAccuracy;
}

void audioParams_SetIecClockAccuracy(audioParams_t *params, u8 value)
{
	params->mIecClockAccuracy = value;
}

 u8 audioParams_GetIecCopyright(audioParams_t *params)
{
	return params->mIecCopyright;
}

void audioParams_SetIecCopyright(audioParams_t *params, u8 value)
{
	params->mIecCopyright = value;
}

 u8 audioParams_GetIecPcmMode(audioParams_t *params)
{
	return params->mIecPcmMode;
}

void audioParams_SetIecPcmMode(audioParams_t *params, u8 value)
{
	params->mIecPcmMode = value;
}

 u8 audioParams_GetIecSourceNumber(audioParams_t *params)
{
	return params->mIecSourceNumber;
}

void audioParams_SetIecSourceNumber(audioParams_t *params, u8 value)
{
	params->mIecSourceNumber = value;
}

 interfaceType_t audioParams_GetInterfaceType(audioParams_t *params)
{
	return params->mInterfaceType;
}

void audioParams_SetInterfaceType(audioParams_t *params, interfaceType_t value)
{
	params->mInterfaceType = value;
}

 u8 audioParams_GetLevelShiftValue(audioParams_t *params)
{
	return params->mLevelShiftValue;
}

void audioParams_SetLevelShiftValue(audioParams_t *params, u8 value)
{
	params->mLevelShiftValue = value;
}

 u32 audioParams_GetOriginalSamplingFrequency(audioParams_t *params)
{
	return params->mOriginalSamplingFrequency;
}

void audioParams_SetOriginalSamplingFrequency(audioParams_t *params, u32 value)
{
	params->mOriginalSamplingFrequency = value;
}

 u8 audioParams_GetSampleSize(audioParams_t *params)
{
	return params->mSampleSize;
}

void audioParams_SetSampleSize(audioParams_t *params, u8 value)
{
	params->mSampleSize = value;
}

 u32 audioParams_GetSamplingFrequency(audioParams_t *params)
{
	return params->mSamplingFrequency;
}

void audioParams_SetSamplingFrequency(audioParams_t *params, u32 value)
{
	params->mSamplingFrequency = value;
}

 u32 audioParams_AudioClock(audioParams_t *params)
{
	return (params->mClockFsFactor * params->mSamplingFrequency);
}

 u8 audioParams_ChannelCount(audioParams_t *params)
{
	return 1;
}

 u8 audioParams_IecOriginalSamplingFrequency(audioParams_t *params)
{
	/* SPEC values are LSB to MSB */
	return 0xD;
}

 u8 audioParams_IecSamplingFrequency(audioParams_t *params)
{
	/* SPEC values are LSB to MSB */
	return 0x2;
}

 u8 audioParams_IecWordLength(audioParams_t *params)
{
	/* SPEC values are LSB to MSB */
	return 0xB;
}

 u8 audioParams_IsLinearPCM(audioParams_t *params)
{
	return params->mCodingType == PCM;
}

packet_t audioParams_GetPacketType(audioParams_t *params)
{
	return params->mPacketType;
}

void audioParams_SetPacketType(audioParams_t *params, packet_t packetType)
{
	params->mPacketType = packetType;
}

u8 audioParams_IsChannelEn(audioParams_t *params, u8 channel)
{
	return 1;
}

u16 audioParams_GetClockFsFactor(audioParams_t *params)
{
	return params->mClockFsFactor;
}

void audioParams_SetClockFsFactor(audioParams_t *params, u16 clockFsFactor)
{
	params->mClockFsFactor = clockFsFactor;
}

dmaIncrement_t audioParams_GetDmaBeatIncrement(audioParams_t *params)
{
	return params->mDmaBeatIncrement;
}

void audioParams_SetDmaBeatIncrement(audioParams_t *params, dmaIncrement_t value)
{
	params->mDmaBeatIncrement = value;
}

void audioParams_SetDmaThreshold(audioParams_t *params, u8 threshold)
{
	params->mDmaThreshold = threshold;
}

u8 audioParams_GetDmaThreshold(audioParams_t *params)
{
	return params->mDmaThreshold;
}

u8 audioParams_GetDmaHlock(audioParams_t *params)
{
	return params->mDmaHlock;
}

void audioParams_SetDmaHlock(audioParams_t *params, int enable)
{
	params->mDmaHlock = enable;
}

void audioParams_SetGpaSamplePacketInfoSource(audioParams_t *params, int internal)
{
	params->mGpaInsertPucv = internal;
}

int audioParams_GetGpaSamplePacketInfoSource(audioParams_t *params)
{
	return params->mGpaInsertPucv;
}

void halFrameComposerAudio_PacketLayout(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, baseAddr + FC_AUDSCONF, 0, 1);
}

void halFrameComposerAudio_ValidityRight(u16 baseAddr, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		access_CoreWrite(bit, baseAddr + FC_AUDSV, 4 + channel, 1);
}

void halFrameComposerAudio_ValidityLeft(u16 baseAddr, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		access_CoreWrite(bit, baseAddr + FC_AUDSV, channel, 1);
}

void halFrameComposerAudio_UserRight(u16 baseAddr, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		access_CoreWrite(bit, baseAddr + FC_AUDSU, 4 + channel, 1);
}

void halFrameComposerAudio_UserLeft(u16 baseAddr, u8 bit, unsigned channel)
{
	LOG_TRACE2(bit, channel);
	if (channel < 4)
		access_CoreWrite(bit, baseAddr + FC_AUDSU, channel, 1);
}

void halFrameComposerAudio_IecCgmsA(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS0, 4, 2);
}

void halFrameComposerAudio_IecCopyright(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, baseAddr + FC_AUDSCHNLS0, 0, 1);
}

void halFrameComposerAudio_IecCategoryCode(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, baseAddr + FC_AUDSCHNLS1);
}

void halFrameComposerAudio_IecPcmMode(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS2, 4, 3);
}

void halFrameComposerAudio_IecSource(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS2, 0, 4);
}

void halFrameComposerAudio_IecChannelRight(u16 baseAddr, u8 value,
		unsigned channel)
{
		access_CoreWrite(value, baseAddr + FC_AUDSCHNLS3, 4, 4);
}

void halFrameComposerAudio_IecChannelLeft(u16 baseAddr, u8 value,
		unsigned channel)
{
	LOG_TRACE2(value, channel);
		access_CoreWrite(value, baseAddr + FC_AUDSCHNLS5, 4, 4);
}

void halFrameComposerAudio_IecClockAccuracy(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS7, 4, 2);
}

void halFrameComposerAudio_IecSamplingFreq(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS7, 0, 4);
}

void halFrameComposerAudio_IecOriginalSamplingFreq(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS8, 4, 4);
}

void halFrameComposerAudio_IecWordLength(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + FC_AUDSCHNLS8, 0, 4);
}

void halAudioI2s_ResetFifo(u16 baseAddress)
{
	LOG_TRACE();
	access_CoreWrite(1, baseAddress + AUD_CONF0, 7, 1);
}

void halAudioI2s_Select(u16 baseAddress, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, baseAddress + AUD_CONF0, 5, 1);
}

void halAudioI2s_DataEnable(u16 baseAddress, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddress + AUD_CONF0, 0, 4);
}

void halAudioI2s_DataMode(u16 baseAddress, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddress + AUD_CONF1, 5, 3);
}

void halAudioI2s_DataWidth(u16 baseAddress, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddress + AUD_CONF1, 0, 5);
}

void halAudioI2s_InterruptMask(u16 baseAddress, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddress + AUD_INT, 2, 2);
}

void halAudioI2s_InterruptPolarity(u16 baseAddress, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddress + AUD_INT, 0, 2);
}
/* register offsets */
static const u8 AUD_SPDIF0 = 0x00;
static const u8 AUD_SPDIF1 = 0x01;
static const u8 AUD_SPDIFINT = 0x02;

void halAudioSpdif_ResetFifo(u16 baseAddr)
{
	LOG_TRACE();
	access_CoreWrite(1, baseAddr + AUD_SPDIF0, 7, 1);
}

void halAudioSpdif_NonLinearPcm(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, baseAddr + AUD_SPDIF1, 7, 1);
}

void halAudioSpdif_DataWidth(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + AUD_SPDIF1, 0, 5);
}

void halAudioSpdif_InterruptMask(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + AUD_SPDIFINT, 2, 2);
}

void halAudioSpdif_InterruptPolarity(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, baseAddr + AUD_SPDIFINT, 0, 2);
}

/* register offsets */
static const u8 AUD_N1 = 0x00;
static const u8 AUD_N2 = 0x01;
static const u8 AUD_N3 = 0x02;
static const u8 AUD_CTS1 = 0x03;
static const u8 AUD_CTS2 = 0x04;
static const u8 AUD_CTS3 = 0x05;
static const u8 AUD_INPUTCLKFS = 0x06;

void halAudioClock_N(u16 baseAddress, u32 value)
{
	LOG_TRACE();
	/* 19-bit width */
	access_CoreWriteByte((u8) (value >> 0), baseAddress + AUD_N1);
	access_CoreWriteByte((u8) (value >> 8), baseAddress + AUD_N2);
	access_CoreWrite((u8) (value >> 16), baseAddress + AUD_N3, 0, 3);
	/* no shift */
	access_CoreWrite(0, baseAddress + AUD_CTS3, 5, 3);
}

/*
 * @param value: new CTS value or set to zero for automatic mode
 */
void halAudioClock_Cts(u16 baseAddress, u32 value)
{
	LOG_TRACE1(value);
	/* manual or automatic? must be programmed first */
	access_CoreWrite((value != 0) ? 1 : 0, baseAddress + AUD_CTS3, 4, 1);
	/* 19-bit width */
	access_CoreWriteByte((u8) (value >> 0), baseAddress + AUD_CTS1);
	access_CoreWriteByte((u8) (value >> 8), baseAddress + AUD_CTS2);
	access_CoreWrite((u8) (value >> 16), baseAddress + AUD_CTS3, 0, 4);
	//access_CoreWrite((u8) (0), baseAddress + AUD_CTS3, 0, 3);
}

void halAudioClock_F(u16 baseAddress, u8 value)
{
	LOG_TRACE();
	access_CoreWrite(value, baseAddress + AUD_INPUTCLKFS, 0, 3);
}

u16 audio_ComputeN(u16 baseAddr, u32 freq, u16 pixelClk, u16 ratioClk)
{
  return 6144;
}

u32 audio_ComputeCts(u16 baseAddr, u32 freq, u16 pixelClk, u16 ratioClk)
{
	u32 cts = 0;
	switch (pixelClk)
	{
		case 2700:
		case 5400:
		case 7425:
		case 14850:
			cts = pixelClk * 10;
			break;
		default:
			/* All other TMDS clocks are not supported by DWC_hdmi_tx
			 * the TMDS clocks divided or multiplied by 1,001
			 * coefficients are not supported.
			 */
			cts = (pixelClk * 100 * audio_ComputeN(baseAddr, freq, pixelClk, ratioClk)) / (128 * (freq /100));
			break;
	}
	return (cts * ratioClk) / 100;
}

static struct cts_map{
	u16 pixel_clock;
	struct{
		u32 sample_freq;
		u32 cts;
	}val;
}g_cts_map[] = {
	{2700 , {48000 , 27000}},
	{7425 , {48000 , 74250}},
	{14850, {48000 , 148500}},
};

static u32 _get_cts(u16 pixel_clock, u32 sample_freq)
{
	u32 cts = 0;
	int i;

	for(i = 0; i < sizeof(g_cts_map) / sizeof(g_cts_map[0]); i++){
		if(pixel_clock == g_cts_map[i].pixel_clock){
			if(sample_freq == g_cts_map[i].val.sample_freq)
				return g_cts_map[i].val.cts;
		}
	}
	return cts;
}

int audio_Configure(u16 baseAddr, audioParams_t *params, u16 pixelClk, unsigned int ratioClk)
{
	u16 i = 0;
	LOG_TRACE();

	/* more than 2 channels => layout 1 else layout 0 */
	halFrameComposerAudio_PacketLayout(baseAddr + FC_BASE_ADDR,
			((audioParams_ChannelCount(params) + 1) > 2) ? 1 : 0);
	/* iec validity and user bits (IEC 60958-1 */
	for (i = 0; i < 4; i++)
	{
		/* audioParams_IsChannelEn considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		halFrameComposerAudio_ValidityRight(baseAddr + FC_BASE_ADDR,
				!(audioParams_IsChannelEn(params, (2 * i))), i);
		halFrameComposerAudio_ValidityLeft(baseAddr + FC_BASE_ADDR,
				!(audioParams_IsChannelEn(params, (2 * i) + 1)), i);
		halFrameComposerAudio_UserRight(baseAddr + FC_BASE_ADDR,
				USER_BIT, i);
		halFrameComposerAudio_UserLeft(baseAddr + FC_BASE_ADDR,
				USER_BIT, i);
	}
	/* IEC - not needed if non-linear PCM */
	halFrameComposerAudio_IecCgmsA(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCgmsA(params));
	halFrameComposerAudio_IecCopyright(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCopyright(params) ? 0 : 1);
	halFrameComposerAudio_IecCategoryCode(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCategoryCode(params));
	halFrameComposerAudio_IecPcmMode(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecPcmMode(params));
	halFrameComposerAudio_IecSource(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecSourceNumber(params));
	for (i = 0; i < 4; i++)
	{ 	/* 0, 1, 2, 3 */
		halFrameComposerAudio_IecChannelLeft(baseAddr + FC_BASE_ADDR,
				2* i + 1, i); /* 1, 3, 5, 7 */
		halFrameComposerAudio_IecChannelRight(baseAddr + FC_BASE_ADDR,
				2* (i + 1), i); /* 2, 4, 6, 8 */
	}
	halFrameComposerAudio_IecClockAccuracy(baseAddr + FC_BASE_ADDR, audioParams_GetIecClockAccuracy(params));
    halFrameComposerAudio_IecSamplingFreq(baseAddr + FC_BASE_ADDR, audioParams_IecSamplingFrequency(params));
    halFrameComposerAudio_IecOriginalSamplingFreq(baseAddr + FC_BASE_ADDR, audioParams_IecOriginalSamplingFrequency(params));
    halFrameComposerAudio_IecWordLength(baseAddr + FC_BASE_ADDR, audioParams_IecWordLength(params));

	halAudioI2s_Select(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 1);
 	/*
 	 * ATTENTION: fixed I2S data enable configuration
 	 * is equivalent to 0x1 for 1 or 2 channels
 	 * is equivalent to 0x3 for 3 or 4 channels
 	 * is equivalent to 0x7 for 5 or 6 channels
 	 * is equivalent to 0xF for 7 or 8 channels
 	 */
 	halAudioI2s_DataEnable(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 0xF);
 	/* ATTENTION: fixed I2S data mode (standard) */
 	halAudioI2s_DataMode(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 0);
 	halAudioI2s_DataWidth(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 16);
 	halAudioI2s_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 3);
 	halAudioI2s_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 3);
 	halAudioI2s_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_I2S);

 	halAudioSpdif_NonLinearPcm(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 0 );
 	halAudioSpdif_DataWidth(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF,16);
 	halAudioSpdif_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 3);
 	halAudioSpdif_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 3);
 	halAudioSpdif_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF);

 	halAudioClock_N(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeN(baseAddr, audioParams_GetSamplingFrequency(params), pixelClk, ratioClk));

	u16 fsFactor = 0;
	/* if NOT GPA interface, use automatic mode CTS */
	halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, _get_cts(pixelClk, audioParams_GetSamplingFrequency(params)));
	fsFactor = 64;

 	return TRUE;
 }
/* register offsets */
#define FC_INVIDCONF  0x00
#define FC_INHACTV0  0x01
#define FC_INHACTV1  0x02
#define FC_INHBLANK0  0x03
#define FC_INHBLANK1  0x04
#define FC_INVACTV0  0x05
#define FC_INVACTV1  0x06
#define FC_INVBLANK  0x07
#define FC_HSYNCINDELAY0  0x08
#define FC_HSYNCINDELAY1  0x09
#define FC_HSYNCINWIDTH0  0x0A
#define FC_HSYNCINWIDTH1  0x0B
#define FC_VSYNCINDELAY  0x0C
#define FC_VSYNCINWIDTH  0x0D
#define FC_CTRLDUR  0x11
#define FC_EXCTRLDUR  0x12
#define FC_EXCTRLSPAC  0x13
#define FC_CH0PREAM  0x14
#define FC_CH1PREAM  0x15
#define FC_CH2PREAM  0x16
#define FC_PRCONF  0xE0

static void halFrameComposerVideo_HdcpKeepout(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 7, 1);
}

static void halFrameComposerVideo_VSyncPolarity(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 6, 1);
}

static u8 dtd_GetVSyncPolarity(const dtd_t *dtd)
{
	return dtd->mVSyncPolarity;
}

static void halFrameComposerVideo_HSyncPolarity(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 5, 1);
}

static void halFrameComposerVideo_DataEnablePolarity(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 4, 1);
}

static void halFrameComposerVideo_DviOrHdmi(u16 baseAddr, u8 bit)
{
	/* 1: HDMI; 0: DVI */
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 3, 1);
}

static void halFrameComposerVideo_VBlankOsc(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 1, 1);
}

static void halFrameComposerVideo_Interlaced(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + FC_INVIDCONF), 0, 1);
}

static u8 dtd_GetInterlaced(const dtd_t *dtd)
{
	return dtd->mInterlaced;
}

static void halFrameComposerVideo_VActive(u16 baseAddr, u16 value)
{
	/* 11-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_INVACTV0));
	access_CoreWrite((u8) (value >> 8), (baseAddr + FC_INVACTV1), 0, 4);
}

static u16 dtd_GetVActive(const dtd_t *dtd)
{
	return dtd->mVActive;
}

static u16 dtd_GetVBlanking(const dtd_t *dtd)
{
	return dtd->mVBlanking;
}

static void halFrameComposerVideo_HActive(u16 baseAddr, u16 value)
{
	/* 12-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_INHACTV0));
	access_CoreWrite((u8) (value >> 8), (baseAddr + FC_INHACTV1), 0, 5);
}

static void halFrameComposerVideo_HBlank(u16 baseAddr, u16 value)
{
	/* 10-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_INHBLANK0));
	access_CoreWrite((u8) (value >> 8), (baseAddr + FC_INHBLANK1), 0, 5);
}

static void halFrameComposerVideo_VBlank(u16 baseAddr, u16 value)
{
	/* 8-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_INVBLANK));
}

static void halFrameComposerVideo_HSyncEdgeDelay(u16 baseAddr, u16 value)
{
	/* 11-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_HSYNCINDELAY0));
	access_CoreWrite((u8) (value >> 8), (baseAddr + FC_HSYNCINDELAY1), 0, 4);
}

static u16 dtd_GetHSyncOffset(const dtd_t *dtd)
{
	return dtd->mHSyncOffset;
}

static void halFrameComposerVideo_HSyncPulseWidth(u16 baseAddr, u16 value)
{
	/* 9-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_HSYNCINWIDTH0));
	access_CoreWrite((u8) (value >> 8), (baseAddr + FC_HSYNCINWIDTH1), 0, 1);
}

static u16 dtd_GetHSyncPulseWidth(const dtd_t *dtd)
{
	return dtd->mHSyncPulseWidth;
}

static void halFrameComposerVideo_VSyncEdgeDelay(u16 baseAddr, u16 value)
{
	/* 8-bit width */
	access_CoreWriteByte((u8) (value), (baseAddr + FC_VSYNCINDELAY));
}

static u16 dtd_GetVSyncOffset(const dtd_t *dtd)
{
	return dtd->mVSyncOffset;
}

static void halFrameComposerVideo_VSyncPulseWidth(u16 baseAddr, u16 value)
{
	access_CoreWrite((u8) (value), (baseAddr + FC_VSYNCINWIDTH), 0, 6);
}

static u16 dtd_GetVSyncPulseWidth(const dtd_t *dtd)
{
	return dtd->mVSyncPulseWidth;
}

static void halFrameComposerVideo_ControlPeriodMinDuration(u16 baseAddr, u8 value)
{
	access_CoreWriteByte(value, (baseAddr + FC_CTRLDUR));
}

static void halFrameComposerVideo_ExtendedControlPeriodMinDuration(u16 baseAddr,
		u8 value)
{
	access_CoreWriteByte(value, (baseAddr + FC_EXCTRLDUR));
}

static void halFrameComposerVideo_ExtendedControlPeriodMaxSpacing(u16 baseAddr,
		u8 value)
{
	access_CoreWriteByte(value, (baseAddr + FC_EXCTRLSPAC));
}

static void halFrameComposerVideo_PreambleFilter(u16 baseAddr, u8 value,
		unsigned channel)
{
		access_CoreWrite(value, (baseAddr + FC_CH1PREAM), 0, 6);
}

static void halFrameComposerVideo_PixelRepetitionInput(u16 baseAddr, u8 value)
{
	access_CoreWrite(value, (baseAddr + FC_PRCONF), 4, 4);
}

static u8 dtd_GetHSyncPolarity(const dtd_t *dtd)
{
	return dtd->mHSyncPolarity;
}

static u16 dtd_GetHActive(const dtd_t *dtd)
{
	return dtd->mHActive;
}

static u16 dtd_GetHBlanking(const dtd_t *dtd)
{
	return dtd->mHBlanking;
}

static int video_FrameComposer(u16 baseAddr, videoParams_t *params, u8 dataEnablePolarity, u8 hdcp)
{
	const dtd_t *dtd = videoParams_GetDtd(params);
	u16 i = 0;

	/* HDCP support was checked previously */
	halFrameComposerVideo_HdcpKeepout(baseAddr + FC_BASE_ADDR, 1);//hdcp);
	halFrameComposerVideo_VSyncPolarity(baseAddr + FC_BASE_ADDR,
			dtd_GetVSyncPolarity(dtd));
	halFrameComposerVideo_HSyncPolarity(baseAddr + FC_BASE_ADDR,
			dtd_GetHSyncPolarity(dtd));
	halFrameComposerVideo_DataEnablePolarity(baseAddr + FC_BASE_ADDR,
			dataEnablePolarity);
	halFrameComposerVideo_DviOrHdmi(baseAddr + FC_BASE_ADDR,
			videoParams_GetHdmi(params));
	halFrameComposerVideo_VBlankOsc(baseAddr + FC_BASE_ADDR,
			dtd_GetInterlaced(dtd));
	halFrameComposerVideo_Interlaced(baseAddr + FC_BASE_ADDR,
			dtd_GetInterlaced(dtd));
	halFrameComposerVideo_VActive(baseAddr + FC_BASE_ADDR,
			dtd_GetVActive(dtd));
	halFrameComposerVideo_HActive(baseAddr + FC_BASE_ADDR,
			dtd_GetHActive(dtd));
	halFrameComposerVideo_HBlank(baseAddr + FC_BASE_ADDR,
			dtd_GetHBlanking(dtd));
	halFrameComposerVideo_VBlank(baseAddr + FC_BASE_ADDR,
			dtd_GetVBlanking(dtd));
	halFrameComposerVideo_HSyncEdgeDelay(baseAddr + FC_BASE_ADDR,
			dtd_GetHSyncOffset(dtd));
	halFrameComposerVideo_HSyncPulseWidth(baseAddr + FC_BASE_ADDR,
			dtd_GetHSyncPulseWidth(dtd));
	halFrameComposerVideo_VSyncEdgeDelay(baseAddr + FC_BASE_ADDR,
			dtd_GetVSyncOffset(dtd));
	halFrameComposerVideo_VSyncPulseWidth(baseAddr + FC_BASE_ADDR,
			dtd_GetVSyncPulseWidth(dtd));
	halFrameComposerVideo_ControlPeriodMinDuration(baseAddr
			+ FC_BASE_ADDR, 12);
	halFrameComposerVideo_ExtendedControlPeriodMinDuration(baseAddr
			+ FC_BASE_ADDR, 32);
	/* spacing < 256^2 * config / tmdsClock, spacing <= 50ms
	 * worst case: tmdsClock == 25MHz => config <= 19
	 */
	halFrameComposerVideo_ExtendedControlPeriodMaxSpacing(baseAddr
			+ FC_BASE_ADDR, 1);
	for (i = 0; i < 3; i++)
		halFrameComposerVideo_PreambleFilter(baseAddr + FC_BASE_ADDR,
				(i + 1) * 11, i);
	halFrameComposerVideo_PixelRepetitionInput(
			baseAddr + FC_BASE_ADDR, dtd_GetPixelRepetitionInput(dtd)
					+ 1);
	return TRUE;
}

/* register offsets */
#define VP_PR_CD  0x01
#define VP_STUFF  0x02
#define VP_REMAP  0x03
#define VP_CONF  0x04

static void halVideoPacketizer_PixelRepetitionFactor(u16 baseAddr, u8 value)
{
	/* desired factor */
	access_CoreWrite(value, (baseAddr + VP_PR_CD), 0, 4);
	/* enable stuffing */
	access_CoreWrite(1, (baseAddr + VP_STUFF), 0, 1);
	/* enable block */
	access_CoreWrite((value > 1) ? 1 : 0, (baseAddr + VP_CONF), 4, 1);
	/* bypass block */
	access_CoreWrite((value > 1) ? 0 : 1, (baseAddr + VP_CONF), 2, 1);
}

static void halVideoPacketizer_ColorDepth(u16 baseAddr, u8 value)
{
	/* color depth */
	access_CoreWrite(value, (baseAddr + VP_PR_CD), 4, 4);
}

static void halVideoPacketizer_PixelPackingDefaultPhase(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + VP_STUFF), 5, 1);
}

static void halVideoPacketizer_Ycc422RemapSize(u16 baseAddr, u8 value)
{
	access_CoreWrite(value, (baseAddr + VP_REMAP), 0, 2);
}

static void halVideoPacketizer_OutputSelector(u16 baseAddr, u8 value)
{
	if (value == 0)
	{ /* pixel packing */
		access_CoreWrite(0, (baseAddr + VP_CONF), 6, 1);
		/* enable pixel packing */
		access_CoreWrite(1, (baseAddr + VP_CONF), 5, 1);
		access_CoreWrite(0, (baseAddr + VP_CONF), 3, 1);
	}
	else if (value == 1)
	{ /* YCC422 */
		access_CoreWrite(0, (baseAddr + VP_CONF), 6, 1);
		access_CoreWrite(0, (baseAddr + VP_CONF), 5, 1);
		/* enable YCC422 */
		access_CoreWrite(1, (baseAddr + VP_CONF), 3, 1);
	}
	else if (value == 2 || value == 3)
	{ /* bypass */
		/* enable bypass */
		access_CoreWrite(1, (baseAddr + VP_CONF), 6, 1);
		access_CoreWrite(0, (baseAddr + VP_CONF), 5, 1);
		access_CoreWrite(0, (baseAddr + VP_CONF), 3, 1);
	}
	else
	{
		return;
	}

	/* YCC422 stuffing */
	access_CoreWrite(1, (baseAddr + VP_STUFF), 2, 1);
	/* pixel packing stuffing */
	access_CoreWrite(1, (baseAddr + VP_STUFF), 1, 1);

	/* ouput selector */
	access_CoreWrite(value, (baseAddr + VP_CONF), 0, 2);
}
static int video_VideoPacketizer(u16 baseAddr, videoParams_t *params)
{
	unsigned color_depth = 0;
	unsigned remap_size = 0;
	unsigned output_select = 0;

	if (videoParams_GetEncodingOut(params) == RGB
			|| videoParams_GetEncodingOut(params) == YCC444)
	{
			color_depth = 4;
			output_select = 3;
	}
	else
	{
		return FALSE;
	}

	halVideoPacketizer_PixelRepetitionFactor(baseAddr + VP_BASE_ADDR,
			videoParams_GetPixelRepetitionFactor(params));
	halVideoPacketizer_ColorDepth(baseAddr + VP_BASE_ADDR, color_depth);
	halVideoPacketizer_PixelPackingDefaultPhase(baseAddr
			+ VP_BASE_ADDR, videoParams_GetPixelPackingDefaultPhase(params));
	halVideoPacketizer_Ycc422RemapSize(baseAddr + VP_BASE_ADDR,
			remap_size);
	halVideoPacketizer_OutputSelector(baseAddr + VP_BASE_ADDR,
			output_select);
	return TRUE;
}

/* Color Space Converter register offsets */
#define CSC_CFG  0x00
#define CSC_SCALE  0x01
#define CSC_COEF_A1_MSB  0x02
#define CSC_COEF_A1_LSB  0x03
#define CSC_COEF_A2_MSB  0x04
#define CSC_COEF_A2_LSB  0x05
#define CSC_COEF_A3_MSB  0x06
#define CSC_COEF_A3_LSB  0x07
#define CSC_COEF_A4_MSB  0x08
#define CSC_COEF_A4_LSB  0x09
#define CSC_COEF_B1_MSB  0x0A
#define CSC_COEF_B1_LSB  0x0B
#define CSC_COEF_B2_MSB  0x0C
#define CSC_COEF_B2_LSB  0x0D
#define CSC_COEF_B3_MSB  0x0E
#define CSC_COEF_B3_LSB  0x0F
#define CSC_COEF_B4_MSB  0x10
#define CSC_COEF_B4_LSB  0x11
#define CSC_COEF_C1_MSB  0x12
#define CSC_COEF_C1_LSB  0x13
#define CSC_COEF_C2_MSB  0x14
#define CSC_COEF_C2_LSB  0x15
#define CSC_COEF_C3_MSB  0x16
#define CSC_COEF_C3_LSB  0x17
#define CSC_COEF_C4_MSB  0x18
#define CSC_COEF_C4_LSB  0x19
static void halColorSpaceConverter_Interpolation(u16 baseAddr, u8 value)
{
	/* 2-bit width */
	access_CoreWrite(value, baseAddr + CSC_CFG, 4, 2);
}

static void halColorSpaceConverter_Decimation(u16 baseAddr, u8 value)
{
	/* 2-bit width */
	access_CoreWrite(value, baseAddr + CSC_CFG, 0, 2);
}

static void halColorSpaceConverter_CoefficientA1(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_A1_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_A1_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientA2(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_A2_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_A2_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientA3(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_A3_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_A3_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientA4(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_A4_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_A4_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientB1(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_B1_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_B1_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientB2(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_B2_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_B2_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientB3(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_B3_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_B3_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientB4(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_B4_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_B4_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientC1(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_C1_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_C1_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientC2(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_C2_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_C2_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientC3(u16 baseAddr, u16 value)
{
	/* 15-bit width */
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_C3_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_C3_MSB, 0, 7);
}

static void halColorSpaceConverter_CoefficientC4(u16 baseAddr, u16 value)
{
	access_CoreWriteByte((u8) (value), baseAddr + CSC_COEF_C4_LSB);
	access_CoreWrite((u8) (value >> 8), baseAddr + CSC_COEF_C4_MSB, 0, 7);
}

static void halColorSpaceConverter_ScaleFactor(u16 baseAddr, u8 value)
{
	/* 2-bit width */
	access_CoreWrite(value, baseAddr + CSC_SCALE, 0, 2);
}

static void halColorSpaceConverter_ColorDepth(u16 baseAddr, u8 value)
{
	/* 4-bit width */
	access_CoreWrite(value, baseAddr + CSC_SCALE, 4, 4);
}

static int video_ColorSpaceConverter(u16 baseAddr, videoParams_t *params)
{
	unsigned interpolation = 0;
	unsigned decimation = 0;
	unsigned color_depth = 0;

	if (videoParams_IsColorSpaceInterpolation(params))//插值
	{
		if (videoParams_GetCscFilter(params) > 1)
		{
			return FALSE;
		}
		interpolation = 1 + videoParams_GetCscFilter(params);
	}
	else if (videoParams_IsColorSpaceDecimation(params))//抽样
	{
		if (videoParams_GetCscFilter(params) > 2)
		{
			return FALSE;
		}
		decimation = 1 + videoParams_GetCscFilter(params);
	}

	if ((videoParams_GetColorResolution(params) == 8)
			|| (videoParams_GetColorResolution(params) == 0))
		color_depth = 4;
	else if (videoParams_GetColorResolution(params) == 10)
		color_depth = 5;
	else if (videoParams_GetColorResolution(params) == 12)
		color_depth = 6;
	else if (videoParams_GetColorResolution(params) == 16)
		color_depth = 7;
	else
	{
		return FALSE;
	}

	halColorSpaceConverter_Interpolation(baseAddr + CSC_BASE_ADDR,
			interpolation);
	halColorSpaceConverter_Decimation(baseAddr + CSC_BASE_ADDR,
			decimation);
	halColorSpaceConverter_CoefficientA1(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscA(params)[0]);
	halColorSpaceConverter_CoefficientA2(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscA(params)[1]);
	halColorSpaceConverter_CoefficientA3(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscA(params)[2]);
	halColorSpaceConverter_CoefficientA4(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscA(params)[3]);
	halColorSpaceConverter_CoefficientB1(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscB(params)[0]);
	halColorSpaceConverter_CoefficientB2(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscB(params)[1]);
	halColorSpaceConverter_CoefficientB3(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscB(params)[2]);
	halColorSpaceConverter_CoefficientB4(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscB(params)[3]);
	halColorSpaceConverter_CoefficientC1(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscC(params)[0]);
	halColorSpaceConverter_CoefficientC2(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscC(params)[1]);
	halColorSpaceConverter_CoefficientC3(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscC(params)[2]);
	halColorSpaceConverter_CoefficientC4(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscC(params)[3]);
	halColorSpaceConverter_ScaleFactor(baseAddr + CSC_BASE_ADDR,
			videoParams_GetCscScale(params));
	halColorSpaceConverter_ColorDepth(baseAddr + CSC_BASE_ADDR,
			color_depth);
	return TRUE;
}

/* register offsets */
#define TX_INVID0  0x00
#define TX_INSTUFFING  0x01
#define TX_GYDATA0  0x02
#define TX_GYDATA1  0x03
#define TX_RCRDATA0  0x04
#define TX_RCRDATA1  0x05
#define TX_BCBDATA0  0x06
#define TX_BCBDATA1  0x07
static void halVideoSampler_InternalDataEnableGenerator(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + TX_INVID0), 7, 1);
}

static void halVideoSampler_VideoMapping(u16 baseAddr, u8 value)
{
	access_CoreWrite(value, (baseAddr + TX_INVID0), 0, 5);
}

static void halVideoSampler_StuffingGy(u16 baseAddr, u16 value)
{
	access_CoreWriteByte((u8) (value >> 0), (baseAddr + TX_GYDATA0));
	access_CoreWriteByte((u8) (value >> 8), (baseAddr + TX_GYDATA1));
	access_CoreWrite(1, (baseAddr + TX_INSTUFFING), 0, 1);
}

static void halVideoSampler_StuffingRcr(u16 baseAddr, u16 value)
{
	access_CoreWriteByte((u8) (value >> 0), (baseAddr + TX_RCRDATA0));
	access_CoreWriteByte((u8) (value >> 8), (baseAddr + TX_RCRDATA1));
	access_CoreWrite(1, (baseAddr + TX_INSTUFFING), 1, 1);
}

static void halVideoSampler_StuffingBcb(u16 baseAddr, u16 value)
{
	access_CoreWriteByte((u8) (value >> 0), (baseAddr + TX_BCBDATA0));
	access_CoreWriteByte((u8) (value >> 8), (baseAddr + TX_BCBDATA1));
	access_CoreWrite(1, (baseAddr + TX_INSTUFFING), 2, 1);
}
static int video_VideoSampler(u16 baseAddr, videoParams_t *params)
{
	unsigned map_code = 0;

	if (videoParams_GetEncodingIn(params) == RGB || videoParams_GetEncodingIn(
			params) == YCC444)
	{
		if ((videoParams_GetColorResolution(params) == 8)
				|| (videoParams_GetColorResolution(params) == 0))
			map_code = 1;
		else if (videoParams_GetColorResolution(params) == 10)
			map_code = 3;
		else if (videoParams_GetColorResolution(params) == 12)
			map_code = 5;
		else if (videoParams_GetColorResolution(params) == 16)
			map_code = 7;
		else
		{
			return FALSE;
		}
		map_code += (videoParams_GetEncodingIn(params) == YCC444) ? 8 : 0;
	}
	else
	{
		return FALSE;
	}

	halVideoSampler_InternalDataEnableGenerator(baseAddr
			+ TX_BASE_ADDR, 0);//使用外部数据使能信息
	halVideoSampler_VideoMapping(baseAddr + TX_BASE_ADDR, map_code);//map_code = 9
	halVideoSampler_StuffingGy(baseAddr + TX_BASE_ADDR, 0);
	halVideoSampler_StuffingRcr(baseAddr + TX_BASE_ADDR, 0);
	halVideoSampler_StuffingBcb(baseAddr + TX_BASE_ADDR, 0);
	return TRUE;
}
static int video_Configure(u16 baseAddr, videoParams_t *params, u8 dataEnablePolarity, u8 hdcp)
{

	/* DVI mode does not support pixel repetition */
#if 0
	if (!videoParams_GetHdmi(params) && videoParams_IsPixelRepetition(params))
		return FALSE;
#endif
	if (video_ForceOutput(baseAddr, 1) != TRUE)
		return FALSE;
	if (video_FrameComposer(baseAddr, params, dataEnablePolarity, hdcp) != TRUE)
		return FALSE;
	if (video_VideoPacketizer(baseAddr, params) != TRUE)
		return FALSE;
	if (video_ColorSpaceConverter(baseAddr, params) != TRUE)
		return FALSE;
	if (video_VideoSampler(baseAddr, params) != TRUE)
		return FALSE;
	return TRUE;
}

/* register offsets */
#define MC_CLKDIS  0x01
#define MC_FLOWCTRL  0x04
#define MC_PHYRSTZ  0x05
#define MC_HEACPHY_RST  0x07
static void halMainController_VideoFeedThroughOff(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_FLOWCTRL), 0, 1);
}

static void halMainController_PixelClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 0, 1);
}

static void halMainController_TmdsClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 1, 1);
}

static void halMainController_PixelRepetitionClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 2, 1);
}

static void halMainController_ColorSpaceConverterClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 4, 1);
}

static void halMainController_AudioSamplerClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 3, 1);
}

static void halMainController_CecClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 5, 1);
}

static void halMainController_HdcpClockGate(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + MC_CLKDIS), 6, 1);
}

static int control_Configure(u16 baseAddr, u16 pClk, u8 pRep, u8 cRes, int cscOn, int audioOn,
		int cecOn, int hdcpOn)
{
	halMainController_VideoFeedThroughOff(baseAddr + MC_BASE_ADDR, cscOn ? 1 : 0);
	/*  clock gate == 0 => turn on modules */
	halMainController_PixelClockGate(baseAddr + MC_BASE_ADDR, 0);
	halMainController_TmdsClockGate(baseAddr + MC_BASE_ADDR, 0);
	halMainController_PixelRepetitionClockGate(baseAddr
			+ MC_BASE_ADDR, (pRep > 0) ? 0 : 1);
	halMainController_ColorSpaceConverterClockGate(baseAddr
			+ MC_BASE_ADDR, cscOn ? 0 : 1);
	halMainController_AudioSamplerClockGate(baseAddr + MC_BASE_ADDR,
			audioOn ? 0 : 1);
	halMainController_CecClockGate(baseAddr + MC_BASE_ADDR,
			cecOn ? 0 : 1);
	halMainController_HdcpClockGate(baseAddr + MC_BASE_ADDR,
			hdcpOn ? 0 : 1);
	return TRUE;
}

#define PHY_TIMEOUT (3000*5)

#ifdef ENABLE_HDMI_PHY_V100
static u8 access_CoreRead(u16 addr, u8 shift, u8 width)
{
	if (width <= 0)
	{
		return 0;
	}
	return (access_CoreReadByte(addr) >> shift) & (BIT(width) - 1);
}

static void halMainController_PhyReset(u16 baseAddr, u8 bit)
{
	/* active low */
	access_CoreWrite(bit ? 0 : 1, (baseAddr + MC_PHYRSTZ), 0, 1);
}

static void halMainController_HeacPhyReset(u16 baseAddr, u8 bit)
{
	/* active high */
	access_CoreWrite(bit ? 1 : 0, (baseAddr + MC_HEACPHY_RST), 0, 1);
}

#define IH_MUTE_I2CMPHY_STAT0 0x88

u8 halInterrupt_MutePhyI2cState(u16 baseAddr)
{
    return access_CoreReadByte(baseAddr + IH_MUTE_I2CMPHY_STAT0);
}

void halInterrupt_MutePhyI2cClear(u16 baseAddr, u8 value)
{
    access_CoreWriteByte(value, (baseAddr + IH_MUTE_I2CMPHY_STAT0));
}

#define IH_BASE_ADDR     0x0100
#define IH_I2CMPHY_STAT0 0x08

u8 halInterrupt_I2cPhyState(u16 baseAddr)
{
    return access_CoreReadByte(baseAddr + IH_I2CMPHY_STAT0);
}

void halInterrupt_I2cPhyClear(u16 baseAddr, u8 value)
{
    access_CoreWriteByte(value, (baseAddr + IH_I2CMPHY_STAT0));
}

u8 control_InterruptI2cPhyState(u16 baseAddr)
{
    return halInterrupt_I2cPhyState(baseAddr + IH_BASE_ADDR);
}

int control_InterruptI2cPhyClear(u16 baseAddr, u8 value)
{
    halInterrupt_I2cPhyClear(baseAddr + IH_BASE_ADDR, value);
    return TRUE;
}

/* register offsets */
#define PHY_CONF0  0x00
#define PHY_TST0  0x01
#define PHY_STAT0  0x04

static void gx3211_halSourcePhy_Gen2PDDQ(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + PHY_CONF0), 4, 1);
}

static void gx3211_halSourcePhy_Gen2TxPowerOn(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + PHY_CONF0), 3, 1);
}

static void gx3211_halSourcePhy_Gen2EnHpdRxSense(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + PHY_CONF0), 2, 1);
}

static void gx3211_halSourcePhy_TestClear(u16 baseAddr, u8 bit)
{
	access_CoreWrite(bit, (baseAddr + PHY_TST0), 5, 1);
}

static u8 gx3211_halSourcePhy_PhaseLockLoopState(u16 baseAddr)
{
	return access_CoreRead((baseAddr + PHY_STAT0), 0, 1);
}

/* register offsets */
#define PHY_I2CM_SLAVE  0x00
#define PHY_I2CM_ADDRESS  0x01
#define PHY_I2CM_DATAO  0x02
#define PHY_I2CM_DATAI  0x04
#define PHY_I2CM_OPERATION  0x06
#define PHY_I2CM_INT 0x07
#define PHY_I2CM_CTLINT 0x08

static void gx3211_halI2cMasterPhy_SlaveAddress(u16 baseAddr, u8 value)
{
	access_CoreWrite(value, (baseAddr + PHY_I2CM_SLAVE), 0, 7);
}

static void gx3211_halI2cMasterPhy_RegisterAddress(u16 baseAddr, u8 value)
{
	access_CoreWriteByte(value, (baseAddr + PHY_I2CM_ADDRESS));
}

static void gx3211_halI2cMasterPhy_WriteData(u16 baseAddr, u16 value)
{
	access_CoreWriteByte((u8) (value >> 8), (baseAddr + PHY_I2CM_DATAO + 0));
	access_CoreWriteByte((u8) (value >> 0), (baseAddr + PHY_I2CM_DATAO + 1));
}

u16 gx3211_halI2cMasterPhy_ReadData(u16 baseAddr)
{
    return ((u16)(access_CoreReadByte(baseAddr + PHY_I2CM_DATAI + 0) << 8) | access_CoreReadByte(baseAddr + PHY_I2CM_DATAI + 1));
}

void gx3211_halI2cMasterPhy_ReadRequest(u16 baseAddr)
{
    access_CoreWrite(1, (baseAddr + PHY_I2CM_OPERATION), 0, 1);
}

static void gx3211_halI2cMasterPhy_WriteRequest(u16 baseAddr)
{
	access_CoreWrite(1, (baseAddr + PHY_I2CM_OPERATION), 4, 1);
}

void gx3211_halI2cMasterPhy_MaskInterrupts(u16 baseAddr, u8 mask)
{
    //access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_INT), 2, 1);
    //access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 2, 1);
    //access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 6, 1);
	access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_INT), 3, 1);
	access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 3, 1);
	access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 7, 1);
}

#define PHY_BASE_ADDR  0x3000
#define PHY_I2CM_BASE_ADDR  0x3020
#define PHY_I2C_SLAVE_ADDR  0x69

static int gx3211_phy_I2cWrite(u16 baseAddr, u16 data, u8 addr)
{
#if 0
	udelay(100);
	gx3211_halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
	gx3211_halI2cMasterPhy_WriteData(baseAddr + PHY_I2CM_BASE_ADDR, data);
	gx3211_halI2cMasterPhy_WriteRequest(baseAddr + PHY_I2CM_BASE_ADDR);
	return TRUE;
#else
	int cnt = 0;
	int state = 0;

	gx3211_halI2cMasterPhy_MaskInterrupts(baseAddr + PHY_I2CM_BASE_ADDR, 1);
	halInterrupt_MutePhyI2cClear(baseAddr+0x0100, 0);

	do {
		udelay(1000);
		gx3211_halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
		udelay(1000);
		gx3211_halI2cMasterPhy_WriteData(baseAddr + PHY_I2CM_BASE_ADDR, data);
		udelay(1000);
		gx3211_halI2cMasterPhy_WriteRequest(baseAddr + PHY_I2CM_BASE_ADDR);
		udelay(1000);

		state = control_InterruptI2cPhyState(baseAddr);
		if (state == 2) {
			//printf("\n### phy I2C W/R Success\n");
			control_InterruptI2cPhyClear(baseAddr, state);
			break;
		} else {
			printf("\n### phy I2C W/R Failed\n");
			control_InterruptI2cPhyClear(baseAddr, state);
			continue;
		}
	} while(cnt++ < 5);

	return TRUE;
#endif
}

void gx3211_phy_I2cReadRequest(u16 baseAddr, u8 addr)
{
    gx3211_halI2cMasterPhy_MaskInterrupts(baseAddr + PHY_I2CM_BASE_ADDR, 0);
    gx3211_halI2cMasterPhy_RegisterAddress(baseAddr + PHY_I2CM_BASE_ADDR, addr);
    gx3211_halI2cMasterPhy_ReadRequest(baseAddr + PHY_I2CM_BASE_ADDR);
}

u16 gx3211_phy_I2cRead(u16 baseAddr)
{
    gx3211_halI2cMasterPhy_MaskInterrupts(baseAddr + PHY_I2CM_BASE_ADDR, 1);
    return gx3211_halI2cMasterPhy_ReadData(baseAddr + PHY_I2CM_BASE_ADDR);
}

static int gx3211_phy_Configure (u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model)
{
#ifndef PHY_THIRD_PARTY
	u32 i = 0;
	/*  colour resolution 0 is 8 bit colour depth */
	if (cRes == 0)
		cRes = 8;

	//if (pRep != 0)
	//{
	//	return FALSE;
	//}

	/* PHY_GEN2_TSMC_40G_1_8V */ /*E102*/
	if(phy_model == 102)
	{
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 0);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 1);
		halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 0);
		halMainController_PhyReset(baseAddr + MC_BASE_ADDR, 1);
		halMainController_HeacPhyReset(baseAddr + MC_BASE_ADDR, 1);
		gx3211_halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 1);
		gx3211_halI2cMasterPhy_SlaveAddress(baseAddr + PHY_I2CM_BASE_ADDR, PHY_I2C_SLAVE_ADDR);
		gx3211_halSourcePhy_TestClear(baseAddr + PHY_BASE_ADDR, 0);

#ifdef CONFIG_ARCH_CKMMU_TAURUS
	#ifdef CHIP_BOARD_6615H1
		gx3211_phy_I2cWrite(baseAddr, 0, 0x1e);      // 数据NP正相
	#else
		gx3211_phy_I2cWrite(baseAddr, 0x0070, 0x1e); // 数据NP反相
	#endif
#else
		gx3211_phy_I2cWrite(baseAddr, 0x0070, 0x1e); // 数据NP反相
#endif

#if defined(CONFIG_ARCH_CKMMU_GX3211)
		gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x13);
		gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x19);
		gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x17);
#else
		gx3211_phy_I2cWrite(baseAddr, 0x0800, 0x13);
		gx3211_phy_I2cWrite(baseAddr, 0x0006, 0x19);
		gx3211_phy_I2cWrite(baseAddr, 0x0004, 0x17);
#endif
		gx3211_phy_I2cWrite(baseAddr, 0x8000, 0x05);
		switch (pClk)
		{
		case 2700:
			gx3211_phy_I2cWrite(baseAddr, 0x01e0, 0x06);
			gx3211_phy_I2cWrite(baseAddr, 0x0000, 0x15);
			gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
#if defined(CONFIG_ARCH_ARM_SIRIUS)
			gx3211_phy_I2cWrite(baseAddr, 0x08da, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x01ef, 0x0E);
#else
			gx3211_phy_I2cWrite(baseAddr, 0x091c, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x0210, 0x0E);
#endif
			break;
		case 7425:
			gx3211_phy_I2cWrite(baseAddr, 0x0140, 0x06);
			gx3211_phy_I2cWrite(baseAddr, 0x0005, 0x15);
			gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
#if defined(CONFIG_ARCH_ARM_SIRIUS)
			gx3211_phy_I2cWrite(baseAddr, 0x079a, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x01ef, 0x0E);
#else
			gx3211_phy_I2cWrite(baseAddr, 0x06dc, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x0210, 0x0E);
#endif
			break;
		case 14850:
			gx3211_phy_I2cWrite(baseAddr, 0x00a0, 0x06);
			gx3211_phy_I2cWrite(baseAddr, 0x000a, 0x15);
			gx3211_phy_I2cWrite(baseAddr, 0x8009, 0x09);
#if defined(CONFIG_ARCH_ARM_SIRIUS)
			gx3211_phy_I2cWrite(baseAddr, 0x079a, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x0108, 0x0E);
#else
			gx3211_phy_I2cWrite(baseAddr, 0x06dc, 0x10);
			gx3211_phy_I2cWrite(baseAddr, 0x0210, 0x0E);
#endif
			break;
		default:
			return FALSE;
		}
		gx3211_halSourcePhy_Gen2EnHpdRxSense(baseAddr + PHY_BASE_ADDR, 1);
		gx3211_halSourcePhy_Gen2TxPowerOn(baseAddr + PHY_BASE_ADDR, 1);
		gx3211_halSourcePhy_Gen2PDDQ(baseAddr + PHY_BASE_ADDR, 0);
	}
	/* wait PHY_TIMEOUT no of cycles at most for the pll lock signal to raise ~around 20us max */
	for (i = 0; i < PHY_TIMEOUT; i++)
	{
		if ((i % 100) == 0)
		{
			mdelay(1);
			if (gx3211_halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) == TRUE)
			{
				break;
			}
		}
	}
	if (gx3211_halSourcePhy_PhaseLockLoopState(baseAddr + PHY_BASE_ADDR) != TRUE)
	{
		return FALSE;
	}
#endif
	return TRUE;
}
#endif

#ifdef ENABLE_HDMI_PHY_V200
#ifdef CONFIG_ARCH_CKMMU_CYGNUS
static volatile u8 *PHY_VIRTUAL_BASE_ADDR = (volatile u8*)0xa0700000;
#else
static volatile u8 *PHY_VIRTUAL_BASE_ADDR = (volatile u8*)0x8ae08000;
#endif

static int phy_ApbWrite(u16 device_reg_addr, u8 wr_data)
{
	PHY_VIRTUAL_BASE_ADDR[(u32)(device_reg_addr<<2)] = wr_data;
	udelay(200); // 解决 O2 编译 phy 无法锁定问题，对 phy 的寄存器操作太快，需要添加适当延时

	return TRUE;
}

static u8 phy_ApbRead(u16 device_reg_addr)
{
	u32 x= (u32)(device_reg_addr<< 2);
	return PHY_VIRTUAL_BASE_ADDR[x];
}

static void phy_ApbRW(u16 device_reg_addr, char bitwise_operators, u8 value)
{
	u8 rvalue = phy_ApbRead(device_reg_addr);

	if (bitwise_operators == '&')
		phy_ApbWrite(device_reg_addr, rvalue&value);
	else if (bitwise_operators == '|')
		phy_ApbWrite(device_reg_addr, rvalue|value);
}

static int cygnus_phy_Configure(u16 baseAddr, u16 pClk, u8 cRes, u8 pRep, u8 phy_model)
{
	u16 i = 0;
#ifdef CONFIG_ARCH_CKMMU_CYGNUS
	unsigned int sysclk = 24000000;
#else
	unsigned int sysclk = 27000000;
#endif
	LOG_TRACE();

	/* color resolution 0 is 8 bit color depth */
	if (cRes == 0)
		cRes = 8;

	/* reset PHY */
	phy_ApbRW(0xbe, '&', 0x8f);
	phy_ApbRW(0x00, '&', 0x3f);
	phy_ApbRW(0x00, '|', 0xc0);

	phy_ApbRW(0xb4, '|', 0x07);
	phy_ApbRW(0xcc, '&', 0xf0); // Rxsense detect disable TMDS
	phy_ApbRW(0xb2, '&', 0xf0); // driver disable TMDS

	phy_ApbWrite(0xa0, 0x01);
	phy_ApbWrite(0xaa, 0x0f);

	switch (pClk) {
		case 2700:
			switch (cRes) {
				case 8: {
					phy_ApbWrite(0xa1,0x01);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x5a);
					} else {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x50);
					}
					phy_ApbWrite(0xa4,0x3a);
					phy_ApbWrite(0xa5,0x6a);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x28);
					phy_ApbWrite(0xad,0x03);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					 break;
				}
				case 10: {
					phy_ApbWrite(0xa1,0x01);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x5a);
					} else {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x50);
					}
					phy_ApbWrite(0xa4,0x1f);
					phy_ApbWrite(0xa5,0x6a);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x28);
					phy_ApbWrite(0xad,0x03);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					 break;
				}
				case 12: {
					phy_ApbWrite(0xa1,0x02);
					phy_ApbWrite(0xa2,0xf0);
					phy_ApbWrite(0xa3,0x87);
					phy_ApbWrite(0xa4,0x35);
					phy_ApbWrite(0xa5,0x4f);
					phy_ApbWrite(0xa6,0x42);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x28);
					phy_ApbWrite(0xad,0x03);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					 break;
				}
				default:
					 printf("color depth not supported\n");
					 return FALSE;
			}
			break;
		case 7425:
			switch (cRes) {
				case 8: {
					phy_ApbWrite(0xa1,0x04);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xef);
					} else {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xb8);
					}
					phy_ApbWrite(0xa4,0x35);
					phy_ApbWrite(0xa5,0x61);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x14);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					break;
				}
				case 10: {
					phy_ApbWrite(0xa1,0x04);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xef);
					} else {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xb8);
					}
					phy_ApbWrite(0xa4,0x1a);
					phy_ApbWrite(0xa5,0x61);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x14);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					 break;
				}
				case 12: {
					phy_ApbWrite(0xa1,0x04);
					phy_ApbWrite(0xa2,0xf1);
					phy_ApbWrite(0xa3,0x29);
					phy_ApbWrite(0xa4,0x15);
					phy_ApbWrite(0xa5,0x21);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x01);
					phy_ApbWrite(0xac,0x14);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x11);
					phy_ApbWrite(0xc0,0x11);
					 break;
				}
				default:
					 printf("color depth not supported\n");
					 return FALSE;
			}
			break;
		case 14850:
			switch (cRes) {
				case 8: {
					phy_ApbWrite(0xa1,0x01);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x63);
					} else {
						phy_ApbWrite(0xa2,0xf0);
						phy_ApbWrite(0xa3,0x58);
					}
					phy_ApbWrite(0xa4,0x15);
					phy_ApbWrite(0xa5,0x41);
					phy_ApbWrite(0xa6,0x42);
					phy_ApbWrite(0xab,0x04);
					phy_ApbWrite(0xac,0x50);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x22);
					phy_ApbWrite(0xc0,0x22);
					break;
				}
				case 10: {
					phy_ApbWrite(0xa1,0x04);
					if (sysclk == 24000000) {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xef);
					} else {
						phy_ApbWrite(0xa2,0xf1);
						phy_ApbWrite(0xa3,0xb8);
					}
					phy_ApbWrite(0xa4,0x0a);
					phy_ApbWrite(0xa5,0x61);
					phy_ApbWrite(0xa6,0x42);
					phy_ApbWrite(0xab,0x04);
					phy_ApbWrite(0xac,0x50);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x22);
					phy_ApbWrite(0xc0,0x22);
					 break;
				}
				case 12: {
					phy_ApbWrite(0xa1,0x02);
					phy_ApbWrite(0xa2,0xf1);
					phy_ApbWrite(0xa3,0x29);
					phy_ApbWrite(0xa4,0x0a);
					phy_ApbWrite(0xa5,0x21);
					phy_ApbWrite(0xa6,0x64);
					phy_ApbWrite(0xab,0x04);
					phy_ApbWrite(0xac,0x50);
					phy_ApbWrite(0xad,0x01);
					phy_ApbWrite(0xbf,0x22);
					phy_ApbWrite(0xc0,0x22);
					 break;
				}
				default:
					 printf("color depth not supported\n");
					 return FALSE;
			}
			break;
		default:
			printf("pixel clock not supported\n");
			return FALSE;
	}

	phy_ApbWrite(0xa0, 0x00);
	phy_ApbWrite(0xaa, 0x0e);

	phy_ApbRW(0xb2, '|', 0x0f); // driver enable TMDS
	phy_ApbRW(0xcc, '|', 0x0f); // Rxsense detect enable TMDS
	phy_ApbRW(0xb0, '|', 0x0e);

	for (i = 0; i < PHY_TIMEOUT; i++) {
		if ((i % 100) == 0) {
			/* Pre-PLL&Post-PLL lock state */
			if (((phy_ApbRead(0xa9) & 0x01) == 1 ) && ((phy_ApbRead(0xaf) & 0x01) == 1 ))  {
				mdelay(10);
				printf("PHY PLL locked\n");
				break;
			}
		}
	}

	/* Pre-PLL&Post-PLL  unlock state */
	if (((phy_ApbRead(0xa9) & 0x01) == 0 ) || ((phy_ApbRead(0xaf) & 0x01) == 0 ))  {
		printf("PHY PLL not locked\n");
		return FALSE;
	}

	mdelay(20);
	phy_ApbRW(0xbe, '|', 0x70);
	phy_ApbRW(0x02, '&', 0xfe); // 开起 phy 数据传输之后，phy 的数据通道复位一下
	phy_ApbRW(0x02, '|', 0x01);

	return TRUE;
}
#endif

/* register offsets */
static const u8 FC_AVICONF0 = 0x19;
static const u8 FC_AVICONF1 = 0x1A;
static const u8 FC_AVICONF2 = 0x1B;
static const u8 FC_AVIVID = 0x1C;
static const u8 FC_AVIETB0 = 0x1D;
static const u8 FC_AVIETB1 = 0x1E;
static const u8 FC_AVISBB0 = 0x1F;
static const u8 FC_AVISBB1 = 0x20;
static const u8 FC_AVIELB0 = 0x21;
static const u8 FC_AVIELB1 = 0x22;
static const u8 FC_AVISRB0 = 0x23;
static const u8 FC_AVISRB1 = 0x24;
/* bit shifts */
static const u8 RGBYCC = 0;
static const u8 SCAN_INFO = 4;
static const u8 COLORIMETRY = 6;
static const u8 PIC_ASPECT_RATIO = 4;
static const u8 ACTIVE_FORMAT_ASPECT_RATIO = 0;
static const u8 ACTIVE_FORMAT_AR_VALID = 6;
static const u8 IT_CONTENT = 7;
static const u8 EXT_COLORIMETRY = 4;
static const u8 QUANTIZATION_RANGE = 2;
static const u8 NON_UNIFORM_PIC_SCALING = 0;
static const u8 H_BAR_INFO = 3;
static const u8 V_BAR_INFO = 2;
static const u8 PIXEL_REP_OUT = 0;

void halFrameComposerAvi_RgbYcc(u16 baseAddr, u8 type)
{
	LOG_TRACE1(type);
	access_CoreWrite(type, baseAddr + FC_AVICONF0, RGBYCC, 2);
}

void halFrameComposerAvi_ScanInfo(u16 baseAddr, u8 left)
{
	LOG_TRACE1(left);
	access_CoreWrite(left, baseAddr + FC_AVICONF0, SCAN_INFO, 2);
}

void halFrameComposerAvi_Colorimetry(u16 baseAddr, unsigned cscITU)
{
	LOG_TRACE1(cscITU);
	access_CoreWrite(cscITU, baseAddr + FC_AVICONF1, COLORIMETRY, 2);
}

void halFrameComposerAvi_PicAspectRatio(u16 baseAddr, u8 ar)
{
	LOG_TRACE1(ar);
	access_CoreWrite(ar, baseAddr + FC_AVICONF1, PIC_ASPECT_RATIO, 2);
}

void halFrameComposerAvi_ActiveAspectRatioValid(u16 baseAddr, u8 valid)
{
	LOG_TRACE1(valid);
	access_CoreWrite(valid, baseAddr + FC_AVICONF0, ACTIVE_FORMAT_AR_VALID, 1); /* data valid flag */
}

void halFrameComposerAvi_ActiveFormatAspectRatio(u16 baseAddr, u8 left)
{
	LOG_TRACE1(left);
	access_CoreWrite(left, baseAddr + FC_AVICONF1, ACTIVE_FORMAT_ASPECT_RATIO,
			4);
}

void halFrameComposerAvi_IsItContent(u16 baseAddr, u8 it)
{
	LOG_TRACE1(it);
	access_CoreWrite((it ? 1 : 0), baseAddr + FC_AVICONF2, IT_CONTENT, 1);
}

void halFrameComposerAvi_ExtendedColorimetry(u16 baseAddr, u8 extColor)
{
	LOG_TRACE1(extColor);
	access_CoreWrite(extColor, baseAddr + FC_AVICONF2, EXT_COLORIMETRY, 3);
	access_CoreWrite(0x3, baseAddr + FC_AVICONF1, COLORIMETRY, 2); /*data valid flag */
}

void halFrameComposerAvi_QuantizationRange(u16 baseAddr, u8 range)
{
	LOG_TRACE1(range);
	access_CoreWrite(range, baseAddr + FC_AVICONF2, QUANTIZATION_RANGE, 2);
}

void halFrameComposerAvi_NonUniformPicScaling(u16 baseAddr, u8 scale)
{
	LOG_TRACE1(scale);
	access_CoreWrite(scale, baseAddr + FC_AVICONF2, NON_UNIFORM_PIC_SCALING, 2);
}

void halFrameComposerAvi_VideoCode(u16 baseAddr, u8 code)
{
	LOG_TRACE1(code);
	access_CoreWriteByte(code, baseAddr + FC_AVIVID);
}

void halFrameComposerAvi_HorizontalBarsValid(u16 baseAddr, u8 validity)
{
	access_CoreWrite((validity ? 1 : 0), baseAddr + FC_AVICONF0, H_BAR_INFO, 1); /*data valid flag */
}

void halFrameComposerAvi_HorizontalBars(u16 baseAddr, u16 endTop,
		u16 startBottom)
{
	LOG_TRACE2(endTop, startBottom);
	access_CoreWriteByte((u8) (endTop), baseAddr + FC_AVIETB0);
	access_CoreWriteByte((u8) (endTop >> 8), baseAddr + FC_AVIETB1);
	access_CoreWriteByte((u8) (startBottom), baseAddr + FC_AVISBB0);
	access_CoreWriteByte((u8) (startBottom >> 8), baseAddr + FC_AVISBB1);
}

void halFrameComposerAvi_VerticalBarsValid(u16 baseAddr, u8 validity)
{
	access_CoreWrite((validity ? 1 : 0), baseAddr + FC_AVICONF0, V_BAR_INFO, 1); /*data valid flag */
}

void halFrameComposerAvi_VerticalBars(u16 baseAddr, u16 endLeft, u16 startRight)
{
	LOG_TRACE2(endLeft, startRight);
	access_CoreWriteByte((u8) (endLeft), baseAddr + FC_AVIELB0);
	access_CoreWriteByte((u8) (endLeft >> 8), baseAddr + FC_AVIELB1);
	access_CoreWriteByte((u8) (startRight), baseAddr + FC_AVISRB0);
	access_CoreWriteByte((u8) (startRight >> 8), baseAddr + FC_AVISRB1);
}

void halFrameComposerAvi_OutPixelRepetition(u16 baseAddr, u8 pr)
{
	LOG_TRACE1(pr);
	access_CoreWrite(pr, baseAddr + FC_PRCONF, PIXEL_REP_OUT, 4);
}

void packets_AuxiliaryVideoInfoFrame(u16 baseAddr, videoParams_t *videoParams)
{
	u16 endTop = 0;
	u16 startBottom = 0;
	u16 endLeft = 0;
	u16 startRight = 0;
	LOG_TRACE();
	if (videoParams_GetEncodingOut(videoParams) == RGB)
	{
		halFrameComposerAvi_RgbYcc(baseAddr + FC_BASE_ADDR, 0);
	}
	else if (videoParams_GetEncodingOut(videoParams) == YCC422)
	{
		halFrameComposerAvi_RgbYcc(baseAddr + FC_BASE_ADDR, 1);
	}
	else if (videoParams_GetEncodingOut(videoParams) == YCC444)
	{
		halFrameComposerAvi_RgbYcc(baseAddr + FC_BASE_ADDR, 2);
	}
	halFrameComposerAvi_ScanInfo(baseAddr + FC_BASE_ADDR, videoParams_GetScanInfo(videoParams));
	if (dtd_GetHImageSize(videoParams_GetDtd(videoParams)) != 0
			|| dtd_GetVImageSize(videoParams_GetDtd(videoParams)) != 0)
	{
		u8 pic = (dtd_GetHImageSize(videoParams_GetDtd(videoParams)) * 10)
				% dtd_GetVImageSize(videoParams_GetDtd(videoParams));
		halFrameComposerAvi_PicAspectRatio(baseAddr + FC_BASE_ADDR, (pic > 5) ? 2 : 1); /* 16:9 or 4:3 */
	}
	else
	{
		halFrameComposerAvi_PicAspectRatio(baseAddr + FC_BASE_ADDR, 0); /* No Data */
	}
	halFrameComposerAvi_IsItContent(baseAddr + FC_BASE_ADDR, videoParams_GetItContent(
			videoParams));

	halFrameComposerAvi_QuantizationRange(baseAddr + FC_BASE_ADDR,
			videoParams_GetRgbQuantizationRange(videoParams));
	halFrameComposerAvi_NonUniformPicScaling(baseAddr + FC_BASE_ADDR,
			videoParams_GetNonUniformScaling(videoParams));
	if (dtd_GetCode(videoParams_GetDtd(videoParams)) != (u8) (-1))
	{
		halFrameComposerAvi_VideoCode(baseAddr + FC_BASE_ADDR, dtd_GetCode(
		videoParams_GetDtd(videoParams)));
	}
	else
	{
		halFrameComposerAvi_VideoCode(baseAddr + FC_BASE_ADDR, 0);
	}
	if (videoParams_GetColorimetry(videoParams) == EXTENDED_COLORIMETRY)
	{ /* ext colorimetry valid */
		if (videoParams_GetExtColorimetry(videoParams) != (u8) (-1))
		{
			halFrameComposerAvi_ExtendedColorimetry(baseAddr + FC_BASE_ADDR,
					videoParams_GetExtColorimetry(videoParams));
			halFrameComposerAvi_Colorimetry(baseAddr + FC_BASE_ADDR,
					videoParams_GetColorimetry(videoParams)); /* EXT-3 */
		}
		else
		{
			halFrameComposerAvi_Colorimetry(baseAddr + FC_BASE_ADDR, 0); /* No Data */
		}
	}
	else
	{
		halFrameComposerAvi_Colorimetry(baseAddr + FC_BASE_ADDR, videoParams_GetColorimetry(
				videoParams)); /* NODATA-0/ 601-1/ 709-2/ EXT-3 */
	}
	if (videoParams_GetActiveFormatAspectRatio(videoParams) != 0)
	{
		halFrameComposerAvi_ActiveFormatAspectRatio(baseAddr + FC_BASE_ADDR,
				videoParams_GetActiveFormatAspectRatio(videoParams));
		halFrameComposerAvi_ActiveAspectRatioValid(baseAddr + FC_BASE_ADDR, 1);
	}
	else
	{
		halFrameComposerAvi_ActiveAspectRatioValid(baseAddr + FC_BASE_ADDR, 0);
	}
	if (videoParams_GetEndTopBar(videoParams) != (u16) (-1)
			|| videoParams_GetStartBottomBar(videoParams) != (u16) (-1))
	{
		if (videoParams_GetEndTopBar(videoParams) != (u16) (-1))
		{
			endTop = videoParams_GetEndTopBar(videoParams);
		}
		if (videoParams_GetStartBottomBar(videoParams) != (u16) (-1))
		{
			startBottom = videoParams_GetStartBottomBar(videoParams);
		}
		halFrameComposerAvi_HorizontalBars(baseAddr + FC_BASE_ADDR, endTop,
		startBottom);
		halFrameComposerAvi_HorizontalBarsValid(baseAddr + FC_BASE_ADDR, 1);
	}
	else
	{
		halFrameComposerAvi_HorizontalBarsValid(baseAddr + FC_BASE_ADDR, 0);
	}
	if (videoParams_GetEndLeftBar(videoParams) != (u16) (-1)
			|| videoParams_GetStartRightBar(videoParams) != (u16) (-1))
	{
		if (videoParams_GetEndLeftBar(videoParams) != (u16) (-1))
		{
			endLeft = videoParams_GetEndLeftBar(videoParams);
		}
		if (videoParams_GetStartRightBar(videoParams) != (u16) (-1))
		{
			startRight = videoParams_GetStartRightBar(videoParams);
		}
		halFrameComposerAvi_VerticalBars(baseAddr + FC_BASE_ADDR, endLeft, startRight);
		halFrameComposerAvi_VerticalBarsValid(baseAddr + FC_BASE_ADDR, 1);
	}
	else
	{
		halFrameComposerAvi_VerticalBarsValid(baseAddr + FC_BASE_ADDR, 0);
	}
	halFrameComposerAvi_OutPixelRepetition(baseAddr + FC_BASE_ADDR,
			(dtd_GetPixelRepetitionInput(videoParams_GetDtd(videoParams)) + 1)
					* (videoParams_GetPixelRepetitionFactor(videoParams) + 1)
					- 1);
}
/* register offset */
static const u8 FC_GCP = 0x18;
/* bit shifts */
static const u8 DEFAULT_PHASE = 2;
static const u8 CLEAR_AVMUTE = 0;
static const u8 SET_AVMUTE = 1;

void halFrameComposerGcp_DefaultPhase(u16 baseAddr, u8 uniform)
{
	LOG_TRACE1(uniform);
	access_CoreWrite((uniform ? 1 : 0), (baseAddr + FC_GCP), DEFAULT_PHASE, 1);
}
/* register offsets */
static const u8 FC_GMD_STAT = 0x00;
static const u8 FC_GMD_EN = 0x01;
static const u8 FC_GMD_UP = 0x02;
static const u8 FC_GMD_CONF = 0x03;
static const u8 FC_GMD_HB = 0x04;
static const u8 FC_GMD_PB0 = 0x05;
static const u8 FC_GMD_PB27 = 0x20;
/* bit shifts */
static const u8 GMD_ENABLE_TX = 0;
static const u8 GMD_ENABLE_TX_WIDTH = 1;
static const u8 GMD_UPDATE = 0;
static const u8 GMD_UPDATE_WIDTH = 1;
static const u8 PACKET_LINE_SPACING = 0;
static const u8 PACKET_LINE_SPACING_WIDTH = 4;
static const u8 PACKETS_PER_FRAME = 4;
static const u8 PACKETS_PER_FRAME_WIDTH = 4;
static const u8 AFFECTED_SEQ_NO = 0;
static const u8 AFFECTED_SEQ_NO_WIDTH = 4;
static const u8 GMD_PROFILE = 4;
static const u8 GMD_PROFILE_WIDTH = 3;
static const u8 GMD_CURR_SEQ = 0;
static const u8 GMD_PACK_SEQ = 4;
static const u8 GMD_NO_CURR_GBD = 7;


void halFrameComposerGamut_Profile(u16 baseAddr, u8 profile)
{
	LOG_TRACE1(profile);
	access_CoreWrite(profile, baseAddr + FC_GMD_HB, GMD_PROFILE,
			GMD_PROFILE_WIDTH);
}

void halFrameComposerGamut_AffectedSeqNo(u16 baseAddr, u8 no)
{
	LOG_TRACE1(no);
	access_CoreWrite(no, baseAddr + FC_GMD_HB, AFFECTED_SEQ_NO,
			AFFECTED_SEQ_NO_WIDTH);
}

void halFrameComposerGamut_PacketsPerFrame(u16 baseAddr, u8 packets)
{
	LOG_TRACE1(packets);
	access_CoreWrite(packets, baseAddr + FC_GMD_CONF, PACKETS_PER_FRAME,
			PACKETS_PER_FRAME_WIDTH);
}

void halFrameComposerGamut_PacketLineSpacing(u16 baseAddr, u8 lineSpacing)
{
	LOG_TRACE1(lineSpacing);
	access_CoreWrite(lineSpacing, baseAddr + FC_GMD_CONF, PACKET_LINE_SPACING,
			PACKET_LINE_SPACING_WIDTH);
}

/* register offsets */
static const u8 FC_CTRLQHIGH = 0x73;
static const u8 FC_CTRLQLOW = 0x74;
static const u8 FC_DATAUTO1 = 0xB4;
static const u8 FC_DATAUTO2 = 0xB5;
static const u8 FC_DATAUTO3 = 0xB7;
static const u8 FC_DATMAN = 0xB6;
static const u8 FC_DATAUTO0 = 0xB3;
/* bit shifts */
static const u8 FRAME_INTERPOLATION = 0;
static const u8 FRAME_PER_PACKETS = 4;
static const u8 LINE_SPACING = 0;

#define ACP_TX  	0
#define ISRC1_TX 	1
#define ISRC2_TX 	2
#define SPD_TX 		4
#define VSD_TX 		3

void halFrameComposerPackets_DisableAll(u16 baseAddr)
{
	LOG_TRACE();
	access_CoreWriteByte((~(BIT(ACP_TX) | BIT(ISRC1_TX) | BIT(ISRC2_TX)
			| BIT(SPD_TX) | BIT(VSD_TX))) & access_CoreReadByte
			(baseAddr + FC_DATAUTO0), (baseAddr + FC_DATAUTO0));
	access_CoreWriteByte((~(BIT(ACP_TX))) & access_CoreReadByte
			(baseAddr + FC_DATAUTO3), (baseAddr + FC_DATAUTO3));
}

void packets_DisableAllPackets(u16 baseAddr)
{
	LOG_TRACE();
	halFrameComposerPackets_DisableAll(baseAddr + FC_BASE_ADDR);
}

int packets_Initialize(u16 baseAddr)
{
	LOG_TRACE();
	//packets_DisableAllPackets(baseAddr + FC_BASE_ADDR);
	packets_DisableAllPackets(baseAddr);
	return TRUE;
}

static const u16 HDCP_BASE_ADDR = 0x5000;
const u8 A_VIDPOLCFG = 0x09;
const u8 A_HDCPCFG0 = 0x00;
const u8 A_HDCPCFG1 = 0x01;

void halHdcp_DataEnablePolarity(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_VIDPOLCFG), 4, 1);
}

void halHdcp_DisableEncryption(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG1), 1, 1);
}

void halHdcp_RxDetected(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + A_HDCPCFG0), 2, 1);
}

int hdcp_Initialize(u16 baseAddr, int dataEnablePolarity)
{
	LOG_TRACE();
	halHdcp_RxDetected(baseAddr + HDCP_BASE_ADDR, 0);
	halHdcp_DataEnablePolarity(baseAddr + HDCP_BASE_ADDR, (dataEnablePolarity
			== TRUE) ? 1 : 0);
	halHdcp_DisableEncryption(baseAddr + HDCP_BASE_ADDR, 1);
	return TRUE;
}

void halFrameComposerPackets_MetadataFrameInterpolation(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, (baseAddr + FC_DATAUTO1), FRAME_INTERPOLATION, 4);
}

void halFrameComposerPackets_MetadataFramesPerPacket(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, (baseAddr + FC_DATAUTO2), FRAME_PER_PACKETS, 4);
}

void halFrameComposerPackets_MetadataLineSpacing(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, (baseAddr + FC_DATAUTO2), LINE_SPACING, 4);
}

int packets_Configure(u16 baseAddr, videoParams_t * video,
		productParams_t * prod)
{
	halFrameComposerPackets_MetadataFrameInterpolation(baseAddr + FC_BASE_ADDR,
			1);
	halFrameComposerPackets_MetadataFramesPerPacket(baseAddr + FC_BASE_ADDR, 1);
	halFrameComposerPackets_MetadataLineSpacing(baseAddr + FC_BASE_ADDR, 1);
	halFrameComposerGcp_DefaultPhase(baseAddr + FC_BASE_ADDR,
			videoParams_GetPixelPackingDefaultPhase(video) == 1); /* default phase 1 = true */
	halFrameComposerGamut_Profile(baseAddr + FC_BASE_ADDR + 0x100, 0x0); /* P0 */
	halFrameComposerGamut_PacketsPerFrame(baseAddr + FC_BASE_ADDR + 0x100, 0x1); /* P0 */
	halFrameComposerGamut_PacketLineSpacing(baseAddr + FC_BASE_ADDR + 0x100,
			0x1);
	packets_AuxiliaryVideoInfoFrame(baseAddr, video);

	return 0;
}

static const u8 FC_AUDICONF0 = 0X25;
static const u8 FC_AUDICONF1 = 0X26;
static const u8 FC_AUDICONF2 = 0X27;
static const u8 FC_AUDICONF3 = 0X28;
static const u8 CHANNEL_COUNT = 4;
static const u8 CODING_TYPE = 0;
static const u8 SAMPLING_SIZE = 4;
static const u8 SAMPLING_FREQ = 0;
static const u8 CHANNEL_ALLOCATION = 0;
static const u8 LEVEL_SHIFT_VALUE = 0;
static const u8 DOWN_MIX_INHIBIT = 4;

void halFrameComposerAudioInfo_ChannelCount(u16 baseAddr, u8 noOfChannels)
{
	LOG_TRACE1(noOfChannels);
	access_CoreWrite(noOfChannels, baseAddr + FC_AUDICONF0, CHANNEL_COUNT, 3);
}

void halFrameComposerAudioInfo_SampleFreq(u16 baseAddr, u8 sf)
{
	LOG_TRACE1(sf);
	access_CoreWrite(sf, baseAddr + FC_AUDICONF1, SAMPLING_FREQ, 3);
}

void halFrameComposerAudioInfo_AllocateChannels(u16 baseAddr, u8 ca)
{
	LOG_TRACE1(ca);
	access_CoreWriteByte(ca, baseAddr + FC_AUDICONF2);
}

void halFrameComposerAudioInfo_LevelShiftValue(u16 baseAddr, u8 lsv)
{
	LOG_TRACE1(lsv);
	access_CoreWrite(lsv, baseAddr + FC_AUDICONF3, LEVEL_SHIFT_VALUE, 4);
}

void halFrameComposerAudioInfo_DownMixInhibit(u16 baseAddr, u8 prohibited)
{
	LOG_TRACE1(prohibited);
	access_CoreWrite((prohibited ? 1 : 0), baseAddr + FC_AUDICONF3,
			DOWN_MIX_INHIBIT, 1);
}

void halFrameComposerAudioInfo_CodingType(u16 baseAddr, u8 codingType)
{
	LOG_TRACE1(codingType);
	access_CoreWrite(codingType, baseAddr + FC_AUDICONF0, CODING_TYPE, 4);
}

void halFrameComposerAudioInfo_SamplingSize(u16 baseAddr, u8 ss)
{
	LOG_TRACE1(ss);
	access_CoreWrite(ss, baseAddr + FC_AUDICONF1, SAMPLING_SIZE, 2);
}

void packets_AudioInfoFrame(u16 baseAddr, audioParams_t *params)
{
	LOG_TRACE();
	halFrameComposerAudioInfo_ChannelCount(baseAddr + FC_BASE_ADDR, audioParams_ChannelCount(
			params));
	halFrameComposerAudioInfo_AllocateChannels(baseAddr + FC_BASE_ADDR,
			audioParams_GetChannelAllocation(params));
	halFrameComposerAudioInfo_LevelShiftValue(baseAddr + FC_BASE_ADDR,
			audioParams_GetLevelShiftValue(params));
	halFrameComposerAudioInfo_DownMixInhibit(baseAddr + FC_BASE_ADDR,
			audioParams_GetDownMixInhibitFlag(params));
	if ((audioParams_GetCodingType(params) == ONE_BIT_AUDIO)
			|| (audioParams_GetCodingType(params) == DST))
	{
		/* Audio InfoFrame sample frequency when OBA or DST */
			halFrameComposerAudioInfo_SampleFreq(baseAddr + FC_BASE_ADDR, 3);
	}
	else
	{
		halFrameComposerAudioInfo_SampleFreq(baseAddr + FC_BASE_ADDR, 0); /* otherwise refer to stream header (0) */
	}
	halFrameComposerAudioInfo_CodingType(baseAddr + FC_BASE_ADDR, 0); /* for HDMI refer to stream header  (0) */
	halFrameComposerAudioInfo_SamplingSize(baseAddr + FC_BASE_ADDR, 0); /* for HDMI refer to stream header  (0) */
}


static int dw_api_Configure(u16 cn_hdmi_base_addr, videoParams_t * video, audioParams_t *audio, productParams_t * product, u8 phy_model)
{
	int audioOn = 1;
	int hdcpOn = 0;
	int api_mDataEnablePolarity = 1;
	u16 api_mBaseAddress = cn_hdmi_base_addr;

#ifdef ENABLE_HDMI_PHY_V200
	if (cygnus_phy_Configure(api_mBaseAddress, videoParams_GetPixelClock(video),
				videoParams_GetColorResolution(video),
				videoParams_GetPixelRepetitionFactor(video), phy_model) != TRUE)
	{
			return FALSE;
	}
	mdelay(20);
#endif

	control_InterruptMute(api_mBaseAddress, ~0); /* disable interrupts */
	//关掉所有音频中断
	if (audio_Initialize(api_mBaseAddress) != TRUE)
	{
		return FALSE;
	}
	//关掉所有包的发送
	if (packets_Initialize(api_mBaseAddress) != TRUE)
	{
		return FALSE;
	}
	//关掉HDCP
	if (hdcp_Initialize(api_mBaseAddress, api_mDataEnablePolarity)
			!= TRUE)
	{
		return FALSE;
	}

	if (video_Configure(api_mBaseAddress, video, 1, hdcpOn) != TRUE)
	{
			return FALSE;
	}

	audioParams_Reset(audio);

	if (audio_Configure(api_mBaseAddress, audio, videoParams_GetPixelClock(
			video), videoParams_GetRatioClock(video)) != TRUE)
	{
		return FALSE;
	}

	packets_AudioInfoFrame(api_mBaseAddress, audio);

	if (control_Configure(api_mBaseAddress, videoParams_GetPixelClock(video),
				videoParams_GetPixelRepetitionFactor(video),
				videoParams_GetColorResolution(video),
				videoParams_IsColorSpaceConversion(video), audioOn, FALSE, hdcpOn)
			!= TRUE)
	{
			return FALSE;
	}

	packets_Configure(api_mBaseAddress, video, product);

#ifdef ENABLE_HDMI_PHY_V100
	if (gx3211_phy_Configure(api_mBaseAddress, videoParams_GetPixelClock(video),
				videoParams_GetColorResolution(video),
				videoParams_GetPixelRepetitionFactor(video), phy_model) != TRUE)
	{
			return FALSE;
	}
#endif
#ifdef ENABLE_HDMI_PHY_V200
	access_Write(access_Read(FC_BASE_ADDR), FC_BASE_ADDR);//regupdatearithunit start
#endif

	/* disable blue screen transmission after turning on all necessary blocks (e.g. HDCP) */
	if (video_ForceOutput(api_mBaseAddress, FALSE) != TRUE)
	{
			return FALSE;
	}
	/* send AVMUTE CLEAR (optional) */
	return TRUE;
}

int dw_hdmi_configure(videoParams_t *video, audioParams_t *audio, u16 ceacode)
{
	dtd_t tmp_dtd;

	videoParams_Reset(video);
	videoParams_SetEncodingIn(video, YCC444);
	videoParams_SetEncodingOut(video, RGB);
	videoParams_SetColorResolution(video, 0);

	dtd_Fill(&tmp_dtd, ceacode);
	videoParams_SetDtd(video, &tmp_dtd);
	videoParams_SetHdmi(video, TRUE);
	videoParams_SetColorResolution(video, 8);

	if (ceacode == 2 || ceacode == 6 || ceacode == 21 || ceacode == 17){
		videoParams_SetColorimetry(video, ITU601);
	}else{
		videoParams_SetColorimetry(video, ITU709);
	}

	dw_api_Configure(0,video,audio, 0, 102);

	access_Write(access_Read(0x10b7) | 0x1, 0x10b7);

	//same as api_AvMute(0)
	access_Write(access_Read(0x1018) | 0x1, 0x1018);

	return TRUE;
}
