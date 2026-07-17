
#ifndef _DW_HDMI_H_
#define _DW_HDMI_H_

#include "sys/types.h"

#define TRUE  1
#define FALSE 0

typedef enum
{
	RGB = 0,
	YCC444,
	YCC422

} encoding_t;

typedef enum
{

	ITU601 = 1,
	ITU709,
	EXTENDED_COLORIMETRY

} colorimetry_t;

/**
 * @file
 * For detailed handling of this structure, refer to documentation of the functions
 */
typedef struct
{
	/** VIC code */
	u8 mCode;

	u16 mPixelRepetitionInput;
	/** in units of 10KHz */
	u16 mPixelClock;
	/** 1 for interlaced, 0 progressive */
	u8 mInterlaced;

	u16 mHActive;

	u16 mHBlanking;

	u16 mHBorder;

	u16 mHImageSize;

	u16 mHSyncOffset;

	u16 mHSyncPulseWidth;
	/** 0 for Active low, 1 active high */
	u8 mHSyncPolarity;

	u16 mVActive;

	u16 mVBlanking;

	u16 mVBorder;

	u16 mVImageSize;

	u16 mVSyncOffset;

	u16 mVSyncPulseWidth;
	/** 0 for Active low, 1 active high */
	u8 mVSyncPolarity;

} dtd_t;

typedef struct
{
	u8 mHdmi;
	encoding_t mEncodingOut;
	encoding_t mEncodingIn;
	u8 mColorResolution;
	u8 mPixelRepetitionFactor;
	dtd_t mDtd;
	u8 mRgbQuantizationRange;
	u8 mPixelPackingDefaultPhase;
	u8 mColorimetry;
	u8 mScanInfo;
	u8 mActiveFormatAspectRatio;
	u8 mNonUniformScaling;
	u8 mExtColorimetry;
	u8 mItContent;
	u16 mEndTopBar;
	u16 mStartBottomBar;
	u16 mEndLeftBar;
	u16 mStartRightBar;
	u16 mCscFilter;
	u16 mCscA[4];
	u16 mCscC[4];
	u16 mCscB[4];
	u16 mCscScale;
	u8 mHdmiVideoFormat;
	u8 m3dStructure;
	u8 m3dExtData;
	u8 mHdmiVic;

} videoParams_t;

typedef enum
{
	I2S = 0, SPDIF, HBR, GPA, DMA
} interfaceType_t;

typedef enum
{
	PCM = 1,
	AC3,
	MPEG1,
	MP3,
	MPEG2,
	AAC,
	DTS,
	ATRAC,
	ONE_BIT_AUDIO,
	DOLBY_DIGITAL_PLUS,
	DTS_HD,
	MAT,
	DST,
	WMAPRO
} codingType_t;

typedef enum
{
	AUDIO_SAMPLE = 1, HBR_STREAM
} packet_t;

typedef enum
{
	DMA_4_BEAT_INCREMENT = 0,
	DMA_8_BEAT_INCREMENT,
	DMA_16_BEAT_INCREMENT,
	DMA_UNUSED_BEAT_INCREMENT,
	DMA_UNSPECIFIED_INCREMENT
} dmaIncrement_t;
/** For detailed handling of this structure, refer to documentation of the 
functions */
typedef struct
{
	interfaceType_t mInterfaceType;

	codingType_t mCodingType; /** (audioParams_t *params, see InfoFrame) */

	u8 mChannelAllocation; /** channel allocation (audioParams_t *params, 
						   see InfoFrame) */

	u8 mSampleSize; /**  sample size (audioParams_t *params, 16 to 24) */

	u32 mSamplingFrequency; /** sampling frequency (audioParams_t *params, Hz) */

	u8 mLevelShiftValue; /** level shift value (audioParams_t *params, 
						 see InfoFrame) */

	u8 mDownMixInhibitFlag; /** down-mix inhibit flag (audioParams_t *params, 
							see InfoFrame) */

	u32 mOriginalSamplingFrequency; /** Original sampling frequency */

	u8 mIecCopyright; /** IEC copyright */

	u8 mIecCgmsA; /** IEC CGMS-A */

	u8 mIecPcmMode; /** IEC PCM mode */

	u8 mIecCategoryCode; /** IEC category code */

	u8 mIecSourceNumber; /** IEC source number */

	u8 mIecClockAccuracy; /** IEC clock accuracy */ 

	packet_t mPacketType; /** packet type. currently only Audio Sample (AUDS) 
						  and High Bit Rate (HBR) are supported */

	u16 mClockFsFactor; /** Input audio clock Fs factor used at the audop 
						packetizer to calculate the CTS value and ACR packet
						insertion rate */

	dmaIncrement_t mDmaBeatIncrement; /** Incremental burst modes: unspecified
									lengths (upper limit is 1kB boundary) and
									INCR4, INCR8, and INCR16 fixed-beat burst */

	u8 mDmaThreshold; /** When the number of samples in the Audio FIFO is lower
						than the threshold, the DMA engine requests a new burst
						request to the AHB master interface */

	u8 mDmaHlock; /** Master burst lock mechanism */

	u8 mGpaInsertPucv; /* discard incoming (Parity, Channel status, User bit,
					      Valid and B bit) data and insert data configured in 
					      controller instead */
} audioParams_t;
/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct
{
	/* Vendor Name of eight 7-bit ASCII characters */
	u8 mVendorName[8];

	u8 mVendorNameLength;

	/* Product name or description, consists of sixteen 7-bit ASCII characters */
	u8 mProductName[16];

	u8 mProductNameLength;

	/* Code that classifies the source device (CEA Table 15) */
	u8 mSourceType;

	/* oui 24 bit IEEE Registration Identifier */
	u32 mOUI;

	u8 mVendorPayload[24];

	u8 mVendorPayloadLength;

} productParams_t;

int dw_hdmi_configure(videoParams_t *video, audioParams_t *audio, u16 ceacode);

/**
 * This method should be called before setting any parameters
 * to put the state of the strucutre to default
 * @param params pointer to the video parameters structure
 */
void videoParams_Reset(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return active format aspect ratio code in Table 9 in CEA-D/E
 */
u8 videoParams_GetActiveFormatAspectRatio(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return colorimetry code in Table 9 in CEA-D/E
 */
u8 videoParams_GetColorimetry(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return color resolution/depth (8/10/12/16-bits per pixel)
 */
u8 videoParams_GetColorResolution(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the colour space convertion filter
 */
u8 videoParams_GetCscFilter(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return pointer to the DTD structure that describes the video output stream
 */
dtd_t* videoParams_GetDtd(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the input video encoding type
 */
encoding_t videoParams_GetEncodingIn(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the output video encoding type
 */
encoding_t videoParams_GetEncodingOut(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return the pixel number of end of left bar
 */
u16 videoParams_GetEndLeftBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return line number of end of top bar
 */
u16 videoParams_GetEndTopBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return the extended colorimetry code in Table 11 in CEA-D/E
 */
u8 videoParams_GetExtColorimetry(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if output stream is HDMI
 */
u8 videoParams_GetHdmi(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if stream is IT content, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetItContent(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetNonUniformScaling(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return default phase (0 or 1)
 */
u8 videoParams_GetPixelPackingDefaultPhase(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return desired pixel repetition factor
 */
u8 videoParams_GetPixelRepetitionFactor(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the quantisation range of the video stream,
 * refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetRgbQuantizationRange(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the scan info code, refer to Table 8 in CEA-D/E
 */
u8 videoParams_GetScanInfo(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the line number of start of bottom bar
 */
u16 videoParams_GetStartBottomBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the pixel number of start of right bar
 */
u16 videoParams_GetStartRightBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return custom csc scale
 */
u16 videoParams_GetCscScale(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @param value active format aspect ratio code in Table 9 in CEA-D/E
 */
void videoParams_SetActiveFormatAspectRatio(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value colorimetry code in Table 9 in CEA-D/E
 */
void videoParams_SetColorimetry(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value color resolution/depth (8/10/12/16-bits per pixel)
 */
void videoParams_SetColorResolution(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value colour space convertion chroma filter
 * check Color Space Converter configuration in the
 * HDMI Transmitter Controller Databook
 */
void videoParams_SetCscFilter(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param dtd pointer to dtd_t structure describing the video output stream
 */
void videoParams_SetDtd(videoParams_t *params, dtd_t *dtd);
/**
 * @param params pointer to the video parameters structure
 * @param value of thie input video one of enumurators: RGB/YCC444/YCC422 of type encoding_t
 */
void videoParams_SetEncodingIn(videoParams_t *params, encoding_t value);
/**
 * @param params pointer to the video parameters structure
 * @param value one of enumurators: RGB/YCC444/YCC422 of type encoding_t
 */
void videoParams_SetEncodingOut(videoParams_t *params, encoding_t value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetEndLeftBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetEndTopBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value extended colorimetry code in Table 11 in CEA-D/E
 */
void videoParams_SetExtColorimetry(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value HDMI TRUE, DVI FALSE
 */
void videoParams_SetHdmi(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetItContent(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetNonUniformScaling(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetPixelPackingDefaultPhase(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetPixelRepetitionFactor(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetRgbQuantizationRange(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetScanInfo(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetStartBottomBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetStartRightBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients A
 */
u16 * videoParams_GetCscA(videoParams_t *params);

void videoParams_SetCscA(videoParams_t *params, u16 value[4]);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients B
 */
u16 * videoParams_GetCscB(videoParams_t *params);

void videoParams_SetCscB(videoParams_t *params, u16 value[4]);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients C
 */
u16 * videoParams_GetCscC(videoParams_t *params);

void videoParams_SetCscC(videoParams_t *params, u16 value[4]);

void videoParams_SetCscScale(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @return Video PixelClock in [0.01 MHz]
 */
u16 videoParams_GetPixelClock(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TMDS Clock in [0.01 MHz]
 */
u16 videoParams_GetTmdsClock(videoParams_t *params);
/** 
 * @param params pointer to the video parameters structure
 * @return Ration clock x 100 (should be multiplied by x 0.01 afterwards)
 */
unsigned videoParams_GetRatioClock(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if csc is needed
 */
int videoParams_IsColorSpaceConversion(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if color space decimation is needed
 */
int videoParams_IsColorSpaceDecimation(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video is interpolated
 */
int videoParams_IsColorSpaceInterpolation(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video has pixel repetition
 */
int videoParams_IsPixelRepetition(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetHdmiVideoFormat(videoParams_t *params);

void videoParams_SetHdmiVideoFormat(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_Get3dStructure(videoParams_t *params);

void videoParams_Set3dStructure(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_Get3dExtData(videoParams_t *params);

void videoParams_Set3dExtData(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return HDMI Video Format Identification Code (CEA-D/E and HDMI 1.4 Table 8-14)
 */
u8 videoParams_GetHdmiVic(videoParams_t *params);

void videoParams_SetHdmiVic(videoParams_t *params, u8 value);

void videoParams_UpdateCscCoefficients(videoParams_t *params);

#define LOG_TRACE(a)  ((void)0)
#define LOG_TRACE1(a) ((void)0)
#define LOG_TRACE2(a,b) ((void)0)




#endif /* _DW_HDMI_H_ */
