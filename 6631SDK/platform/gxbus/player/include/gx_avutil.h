#ifndef _GX_AVUTIL_H_
#define _GX_AVUTIL_H_

#ifndef MKBETAG
#define MKBETAG(a,b,c,d) (d | (c << 8) | (b << 16) | (a << 24))
#endif
/**
 * Pixel format. Notes:
 *
 * PIX_FMT_RGB32 is handled in an endian-specific manner. A RGBA
 * color is put together as:
 *  (A << 24) | (R << 16) | (G << 8) | B
 * This is stored as BGRA on little endian CPU architectures and ARGB on
 * big endian CPUs.
 *
 * When the pixel format is palettized RGB (PIX_FMT_PAL8), the palettized
 * image data is stored in AVFrame.data[0]. The palette is transported in
 * AVFrame.data[1] and, is 1024 bytes long (256 4-byte entries) and is
 * formatted the same as in PIX_FMT_RGB32 described above (i.e., it is
 * also endian-specific). Note also that the individual RGB palette
 * components stored in AVFrame.data[1] should be in the range 0..255.
 * This is important as many custom PAL8 video codecs that were designed
 * to run on the IBM VGA graphics adapter use 6-bit palette components.
 */
enum PixelFormat {
    PIX_FMT_NONE= -1,
    PIX_FMT_YUV420P,   ///< Planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    PIX_FMT_YUYV422,   ///< Packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    PIX_FMT_RGB24,     ///< Packed RGB 8:8:8, 24bpp, RGBRGB...
    PIX_FMT_BGR24,     ///< Packed RGB 8:8:8, 24bpp, BGRBGR...
    PIX_FMT_YUV422P,   ///< Planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    PIX_FMT_YUV444P,   ///< Planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    PIX_FMT_RGB32,     ///< Packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in cpu endianness
    PIX_FMT_YUV410P,   ///< Planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    PIX_FMT_YUV411P,   ///< Planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    PIX_FMT_RGB565,    ///< Packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), in cpu endianness
    PIX_FMT_RGB555,    ///< Packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in cpu endianness most significant bit to 0
    PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
    PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black
    PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white
    PIX_FMT_PAL8,      ///< 8 bit with PIX_FMT_RGB32 palette
    PIX_FMT_YUVJ420P,  ///< Planar YUV 4:2:0, 12bpp, full scale (jpeg)
    PIX_FMT_YUVJ422P,  ///< Planar YUV 4:2:2, 16bpp, full scale (jpeg)
    PIX_FMT_YUVJ444P,  ///< Planar YUV 4:4:4, 24bpp, full scale (jpeg)
    PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
    PIX_FMT_XVMC_MPEG2_IDCT,
    PIX_FMT_UYVY422,   ///< Packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    PIX_FMT_UYYVYY411, ///< Packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    PIX_FMT_BGR32,     ///< Packed RGB 8:8:8, 32bpp, (msb)8A 8B 8G 8R(lsb), in cpu endianness
    PIX_FMT_BGR565,    ///< Packed RGB 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), in cpu endianness
    PIX_FMT_BGR555,    ///< Packed RGB 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), in cpu endianness most significant bit to 1
    PIX_FMT_BGR8,      ///< Packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    PIX_FMT_BGR4,      ///< Packed RGB 1:2:1,  4bpp, (msb)1B 2G 1R(lsb)
    PIX_FMT_BGR4_BYTE, ///< Packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    PIX_FMT_RGB8,      ///< Packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    PIX_FMT_RGB4,      ///< Packed RGB 1:2:1,  4bpp, (msb)2R 3G 3B(lsb)
    PIX_FMT_RGB4_BYTE, ///< Packed RGB 1:2:1,  8bpp, (msb)2R 3G 3B(lsb)
    PIX_FMT_NV12,      ///< Planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 for UV
    PIX_FMT_NV21,      ///< as above, but U and V bytes are swapped

    PIX_FMT_RGB32_1,   ///< Packed RGB 8:8:8, 32bpp, (msb)8R 8G 8B 8A(lsb), in cpu endianness
    PIX_FMT_BGR32_1,   ///< Packed RGB 8:8:8, 32bpp, (msb)8B 8G 8R 8A(lsb), in cpu endianness

    PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
    PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
    PIX_FMT_YUV440P,   ///< Planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    PIX_FMT_YUVJ440P,  ///< Planar YUV 4:4:0 full scale (jpeg)
    PIX_FMT_YUVA420P,  ///< Planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    PIX_FMT_NB,        ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions

#define AV_PIX_FMT_NONE            PIX_FMT_NONE
#define AV_PIX_FMT_YUV420P         PIX_FMT_YUV420P
#define AV_PIX_FMT_YUYV422         PIX_FMT_YUYV422
#define AV_PIX_FMT_RGB24           PIX_FMT_RGB24
#define AV_PIX_FMT_BGR24           PIX_FMT_BGR24
#define AV_PIX_FMT_YUV422P         PIX_FMT_YUV422P
#define AV_PIX_FMT_YUV444P         PIX_FMT_YUV444P
#define AV_PIX_FMT_RGB32           PIX_FMT_RGB32
#define AV_PIX_FMT_YUV410P         PIX_FMT_YUV410P
#define AV_PIX_FMT_YUV411P         PIX_FMT_YUV411P
#define AV_PIX_FMT_RGB565          PIX_FMT_RGB565
#define AV_PIX_FMT_RGB555          PIX_FMT_RGB555
#define AV_PIX_FMT_GRAY8           PIX_FMT_GRAY8
#define AV_PIX_FMT_MONOWHITE       PIX_FMT_MONOWHITE
#define AV_PIX_FMT_MONOBLACK       PIX_FMT_MONOBLACK
#define AV_PIX_FMT_PAL8            PIX_FMT_PAL8
#define AV_PIX_FMT_YUVJ420P        PIX_FMT_YUVJ420P
#define AV_PIX_FMT_YUVJ422P        PIX_FMT_YUVJ422P
#define AV_PIX_FMT_YUVJ444P        PIX_FMT_YUVJ444P
#define AV_PIX_FMT_XVMC_MPEG2_MC   PIX_FMT_XVMC_MPEG2_
#define AV_PIX_FMT_XVMC_MPEG2_IDCT PIX_FMT_XVMC_MPEG2_
#define AV_PIX_FMT_UYVY422         PIX_FMT_UYVY422
#define AV_PIX_FMT_UYYVYY411       PIX_FMT_UYYVYY411
#define AV_PIX_FMT_BGR32           PIX_FMT_BGR32
#define AV_PIX_FMT_BGR565          PIX_FMT_BGR565
#define AV_PIX_FMT_BGR555          PIX_FMT_BGR555
#define AV_PIX_FMT_BGR8            PIX_FMT_BGR8
#define AV_PIX_FMT_BGR4            PIX_FMT_BGR4
#define AV_PIX_FMT_BGR4_BYTE       PIX_FMT_BGR4_BYTE
#define AV_PIX_FMT_RGB8            PIX_FMT_RGB8
#define AV_PIX_FMT_RGB4            PIX_FMT_RGB4
#define AV_PIX_FMT_RGB4_BYTE       PIX_FMT_RGB4_BYTE
#define AV_PIX_FMT_NV12            PIX_FMT_NV12
#define AV_PIX_FMT_NV21            PIX_FMT_NV21
#define AV_
#define AV_PIX_FMT_RGB32_1         PIX_FMT_RGB32_1
#define AV_PIX_FMT_BGR32_1         PIX_FMT_BGR32_1
#define AV_
#define AV_PIX_FMT_GRAY16BE        PIX_FMT_GRAY16BE
#define AV_PIX_FMT_GRAY16LE        PIX_FMT_GRAY16LE
#define AV_PIX_FMT_YUV440P         PIX_FMT_YUV440P
#define AV_PIX_FMT_YUVJ440P        PIX_FMT_YUVJ440P
#define AV_PIX_FMT_YUVA420P        PIX_FMT_YUVA420P
#define AV_PIX_FMT_NB              PIX_FMT_NB

};
#define AVPixelFormat PixelFormat

#ifdef WORDS_BIGENDIAN
#define PIX_FMT_RGBA PIX_FMT_RGB32_1
#define PIX_FMT_BGRA PIX_FMT_BGR32_1
#define PIX_FMT_ARGB PIX_FMT_RGB32
#define PIX_FMT_ABGR PIX_FMT_BGR32
#define PIX_FMT_GRAY16 PIX_FMT_GRAY16BE
#else
#define PIX_FMT_RGBA PIX_FMT_BGR32
#define PIX_FMT_BGRA PIX_FMT_RGB32
#define PIX_FMT_ARGB PIX_FMT_BGR32_1
#define PIX_FMT_ABGR PIX_FMT_RGB32_1
#define PIX_FMT_GRAY16 PIX_FMT_GRAY16LE
#endif

#define PIX_FMT_UYVY411 PIX_FMT_UYYVYY411
#define PIX_FMT_RGBA32  PIX_FMT_RGB32
#define PIX_FMT_YUV422  PIX_FMT_YUYV422



/**
 * Identifies the syntax and semantics of the bitstream.
 * The principle is roughly:
 * Two decoders with the same ID can decode the same streams.
 * Two encoders with the same ID can encode compatible streams.
 * There may be slight deviations from the principle due to implementation
 * details.
 *
 * If you add a codec ID to this list, add it so that
 * 1. no value of a existing codec ID changes (that would break ABI),
 * 2. it is as close as possible to similar codecs.
 */
enum CodecID {
    CODEC_ID_NONE,

    /* video codecs */
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
//#if FF_API_XVMC
    CODEC_ID_MPEG2VIDEO_XVMC,
//#endif /* FF_API_XVMC */
    CODEC_ID_H261,
    CODEC_ID_H263,
    CODEC_ID_RV10,
    CODEC_ID_RV20,
    CODEC_ID_MJPEG,
    CODEC_ID_MJPEGB,
    CODEC_ID_LJPEG,
    CODEC_ID_SP5X,
    CODEC_ID_JPEGLS,
    CODEC_ID_MPEG4,
    CODEC_ID_RAWVIDEO,
    CODEC_ID_MSMPEG4V1,
    CODEC_ID_MSMPEG4V2,
    CODEC_ID_MSMPEG4V3,
    CODEC_ID_WMV1,
    CODEC_ID_WMV2,
    CODEC_ID_H263P,
    CODEC_ID_H263I,
    CODEC_ID_FLV1,
    CODEC_ID_SVQ1,
    CODEC_ID_SVQ3,
    CODEC_ID_DVVIDEO,
    CODEC_ID_HUFFYUV,
    CODEC_ID_CYUV,
    CODEC_ID_H264,
    CODEC_ID_INDEO3,
    CODEC_ID_VP3,
    CODEC_ID_THEORA,
    CODEC_ID_ASV1,
    CODEC_ID_ASV2,
    CODEC_ID_FFV1,
    CODEC_ID_4XM,
    CODEC_ID_VCR1,
    CODEC_ID_CLJR,
    CODEC_ID_MDEC,
    CODEC_ID_ROQ,
    CODEC_ID_INTERPLAY_VIDEO,
    CODEC_ID_XAN_WC3,
    CODEC_ID_XAN_WC4,
    CODEC_ID_RPZA,
    CODEC_ID_CINEPAK,
    CODEC_ID_WS_VQA,
    CODEC_ID_MSRLE,
    CODEC_ID_MSVIDEO1,
    CODEC_ID_IDCIN,
    CODEC_ID_8BPS,
    CODEC_ID_SMC,
    CODEC_ID_FLIC,
    CODEC_ID_TRUEMOTION1,
    CODEC_ID_VMDVIDEO,
    CODEC_ID_MSZH,
    CODEC_ID_ZLIB,
    CODEC_ID_QTRLE,
    CODEC_ID_TSCC,
    CODEC_ID_ULTI,
    CODEC_ID_QDRAW,
    CODEC_ID_VIXL,
    CODEC_ID_QPEG,
    CODEC_ID_PNG,
    CODEC_ID_PPM,
    CODEC_ID_PBM,
    CODEC_ID_PGM,
    CODEC_ID_PGMYUV,
    CODEC_ID_PAM,
    CODEC_ID_FFVHUFF,
    CODEC_ID_RV30,
    CODEC_ID_RV40,
    CODEC_ID_VC1,
    CODEC_ID_WMV3,
    CODEC_ID_LOCO,
    CODEC_ID_WNV1,
    CODEC_ID_AASC,
    CODEC_ID_INDEO2,
    CODEC_ID_FRAPS,
    CODEC_ID_TRUEMOTION2,
    CODEC_ID_BMP,
    CODEC_ID_CSCD,
    CODEC_ID_MMVIDEO,
    CODEC_ID_ZMBV,
    CODEC_ID_AVS,
    CODEC_ID_SMACKVIDEO,
    CODEC_ID_NUV,
    CODEC_ID_KMVC,
    CODEC_ID_FLASHSV,
    CODEC_ID_CAVS,
    CODEC_ID_JPEG2000,
    CODEC_ID_VMNC,
    CODEC_ID_VP5,
    CODEC_ID_VP6,
    CODEC_ID_VP6F,
    CODEC_ID_TARGA,
    CODEC_ID_DSICINVIDEO,
    CODEC_ID_TIERTEXSEQVIDEO,
    CODEC_ID_TIFF,
    CODEC_ID_GIF,
    CODEC_ID_DXA,
    CODEC_ID_DNXHD,
    CODEC_ID_THP,
    CODEC_ID_SGI,
    CODEC_ID_C93,
    CODEC_ID_BETHSOFTVID,
    CODEC_ID_PTX,
    CODEC_ID_TXD,
    CODEC_ID_VP6A,
    CODEC_ID_AMV,
    CODEC_ID_VB,
    CODEC_ID_PCX,
    CODEC_ID_SUNRAST,
    CODEC_ID_INDEO4,
    CODEC_ID_INDEO5,
    CODEC_ID_MIMIC,
    CODEC_ID_RL2,
    CODEC_ID_ESCAPE124,
    CODEC_ID_DIRAC,
    CODEC_ID_BFI,
    CODEC_ID_CMV,
    CODEC_ID_MOTIONPIXELS,
    CODEC_ID_TGV,
    CODEC_ID_TGQ,
    CODEC_ID_TQI,
    CODEC_ID_AURA,
    CODEC_ID_AURA2,
    CODEC_ID_V210X,
    CODEC_ID_TMV,
    CODEC_ID_V210,
    CODEC_ID_DPX,
    CODEC_ID_MAD,
    CODEC_ID_FRWU,
    CODEC_ID_FLASHSV2,
    CODEC_ID_CDGRAPHICS,
    CODEC_ID_R210,
    CODEC_ID_ANM,
    CODEC_ID_BINKVIDEO,
    CODEC_ID_IFF_ILBM,
    CODEC_ID_IFF_BYTERUN1,
    CODEC_ID_KGV1,
    CODEC_ID_YOP,
    CODEC_ID_VP8,
    CODEC_ID_PICTOR,
    CODEC_ID_ANSI,
    CODEC_ID_A64_MULTI,
    CODEC_ID_A64_MULTI5,
    CODEC_ID_R10K,
    CODEC_ID_MXPEG,
    CODEC_ID_LAGARITH,
    CODEC_ID_PRORES,
    CODEC_ID_JV,
    CODEC_ID_DFA,
    CODEC_ID_WMV3IMAGE,
    CODEC_ID_VC1IMAGE,
    CODEC_ID_UTVIDEO,
    CODEC_ID_BMV_VIDEO,
    CODEC_ID_VBLE,
    CODEC_ID_DXTORY,
    CODEC_ID_V410,
    CODEC_ID_XWD,
    CODEC_ID_CDXL,
    CODEC_ID_XBM,
    CODEC_ID_ZEROCODEC,
    CODEC_ID_MSS1,
    CODEC_ID_MSA1,
    CODEC_ID_TSCC2,
    CODEC_ID_MTS2,
    CODEC_ID_CLLC,
    CODEC_ID_MSS2,
    CODEC_ID_VP9,
    CODEC_ID_AIC,
    CODEC_ID_ESCAPE130_DEPRECATED,
    CODEC_ID_G2M_DEPRECATED,
    CODEC_ID_WEBP_DEPRECATED,
    CODEC_ID_HNM4_VIDEO,
    CODEC_ID_HEVC_DEPRECATED,
    CODEC_ID_FIC,

    CODEC_ID_BRENDER_PIX= MKBETAG('B','P','I','X'),
    CODEC_ID_Y41P       = MKBETAG('Y','4','1','P'),
    CODEC_ID_ESCAPE130  = MKBETAG('E','1','3','0'),
    CODEC_ID_EXR        = MKBETAG('0','E','X','R'),
    CODEC_ID_AVRP       = MKBETAG('A','V','R','P'),

    CODEC_ID_012V       = MKBETAG('0','1','2','V'),
    CODEC_ID_G2M        = MKBETAG( 0 ,'G','2','M'),
    CODEC_ID_AVUI       = MKBETAG('A','V','U','I'),
    CODEC_ID_AYUV       = MKBETAG('A','Y','U','V'),
    CODEC_ID_TARGA_Y216 = MKBETAG('T','2','1','6'),
    CODEC_ID_V308       = MKBETAG('V','3','0','8'),
    CODEC_ID_V408       = MKBETAG('V','4','0','8'),
    CODEC_ID_YUV4       = MKBETAG('Y','U','V','4'),
    CODEC_ID_SANM       = MKBETAG('S','A','N','M'),
    CODEC_ID_PAF_VIDEO  = MKBETAG('P','A','F','V'),
    CODEC_ID_AVRN       = MKBETAG('A','V','R','n'),
    CODEC_ID_CPIA       = MKBETAG('C','P','I','A'),
    CODEC_ID_XFACE      = MKBETAG('X','F','A','C'),
    CODEC_ID_SGIRLE     = MKBETAG('S','G','I','R'),
    CODEC_ID_MVC1       = MKBETAG('M','V','C','1'),
    CODEC_ID_MVC2       = MKBETAG('M','V','C','2'),
    CODEC_ID_SNOW       = MKBETAG('S','N','O','W'),
    CODEC_ID_WEBP       = MKBETAG('W','E','B','P'),
    CODEC_ID_SMVJPEG    = MKBETAG('S','M','V','J'),
    CODEC_ID_HEVC       = MKBETAG('H','2','6','5'),
    CODEC_ID_DRA        = MKBETAG('D','R','A','1'),
#define CODEC_ID_H265 CODEC_ID_HEVC
	CODEC_ID_AV1        = MKBETAG('a','v','0','1'),
    /* various PCM "codecs" */
    CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
    CODEC_ID_PCM_S16LE = 0x10000,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_PCM_U16LE,
    CODEC_ID_PCM_U16BE,
    CODEC_ID_PCM_S8,
    CODEC_ID_PCM_U8,
    CODEC_ID_PCM_MULAW,
    CODEC_ID_PCM_ALAW,
    CODEC_ID_PCM_S32LE,
    CODEC_ID_PCM_S32BE,
    CODEC_ID_PCM_U32LE,
    CODEC_ID_PCM_U32BE,
    CODEC_ID_PCM_S24LE,
    CODEC_ID_PCM_S24BE,
    CODEC_ID_PCM_U24LE,
    CODEC_ID_PCM_U24BE,
    CODEC_ID_PCM_S24DAUD,
    CODEC_ID_PCM_ZORK,
    CODEC_ID_PCM_S16LE_PLANAR,
    CODEC_ID_PCM_DVD,
    CODEC_ID_PCM_F32BE,
    CODEC_ID_PCM_F32LE,
    CODEC_ID_PCM_F64BE,
    CODEC_ID_PCM_F64LE,
    CODEC_ID_PCM_BLURAY,
    CODEC_ID_PCM_LXF,
    CODEC_ID_S302M,
    CODEC_ID_PCM_S8_PLANAR,
    CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED,
    CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED,
    CODEC_ID_PCM_S24LE_PLANAR = MKBETAG(24,'P','S','P'),
    CODEC_ID_PCM_S32LE_PLANAR = MKBETAG(32,'P','S','P'),
    CODEC_ID_PCM_S16BE_PLANAR = MKBETAG('P','S','P',16),

    /* various ADPCM codecs */
    CODEC_ID_ADPCM_IMA_QT = 0x11000,
    CODEC_ID_ADPCM_IMA_WAV,
    CODEC_ID_ADPCM_IMA_DK3,
    CODEC_ID_ADPCM_IMA_DK4,
    CODEC_ID_ADPCM_IMA_WS,
    CODEC_ID_ADPCM_IMA_SMJPEG,
    CODEC_ID_ADPCM_MS,
    CODEC_ID_ADPCM_4XM,
    CODEC_ID_ADPCM_XA,
    CODEC_ID_ADPCM_ADX,
    CODEC_ID_ADPCM_EA,
    CODEC_ID_ADPCM_G726,
    CODEC_ID_ADPCM_CT,
    CODEC_ID_ADPCM_SWF,
    CODEC_ID_ADPCM_YAMAHA,
    CODEC_ID_ADPCM_SBPRO_4,
    CODEC_ID_ADPCM_SBPRO_3,
    CODEC_ID_ADPCM_SBPRO_2,
    CODEC_ID_ADPCM_THP,
    CODEC_ID_ADPCM_IMA_AMV,
    CODEC_ID_ADPCM_EA_R1,
    CODEC_ID_ADPCM_EA_R3,
    CODEC_ID_ADPCM_EA_R2,
    CODEC_ID_ADPCM_IMA_EA_SEAD,
    CODEC_ID_ADPCM_IMA_EA_EACS,
    CODEC_ID_ADPCM_EA_XAS,
    CODEC_ID_ADPCM_EA_MAXIS_XA,
    CODEC_ID_ADPCM_IMA_ISS,
    CODEC_ID_ADPCM_G722,
    CODEC_ID_ADPCM_IMA_APC,
    CODEC_ID_VIMA       = MKBETAG('V','I','M','A'),
    CODEC_ID_ADPCM_AFC  = MKBETAG('A','F','C',' '),
    CODEC_ID_ADPCM_IMA_OKI = MKBETAG('O','K','I',' '),
    CODEC_ID_ADPCM_DTK  = MKBETAG('D','T','K',' '),
    CODEC_ID_ADPCM_IMA_RAD = MKBETAG('R','A','D',' '),
    CODEC_ID_ADPCM_G726LE = MKBETAG('6','2','7','G'),

    /* AMR */
    CODEC_ID_AMR_NB = 0x12000,
    CODEC_ID_AMR_WB,

    /* RealAudio codecs*/
    CODEC_ID_RA_144 = 0x13000,
    CODEC_ID_RA_288,

    /* various DPCM codecs */
    CODEC_ID_ROQ_DPCM = 0x14000,
    CODEC_ID_INTERPLAY_DPCM,
    CODEC_ID_XAN_DPCM,
    CODEC_ID_SOL_DPCM,

    /* audio codecs */
    CODEC_ID_MP2 = 0x15000,
    CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,
    CODEC_ID_VORBIS,
    CODEC_ID_DVAUDIO,
    CODEC_ID_WMAV1,
    CODEC_ID_WMAV2,
    CODEC_ID_BITPACKED,
    CODEC_ID_MACE3,
    CODEC_ID_MACE6,
    CODEC_ID_VMDAUDIO,
    CODEC_ID_FLAC,
    CODEC_ID_MP3ADU,
    CODEC_ID_MP3ON4,
    CODEC_ID_SHORTEN,
    CODEC_ID_ALAC,
    CODEC_ID_WESTWOOD_SND1,
    CODEC_ID_GSM, ///< as in Berlin toast format
    CODEC_ID_QDM2,
    CODEC_ID_COOK,
    CODEC_ID_TRUESPEECH,
    CODEC_ID_TTA,
    CODEC_ID_SMACKAUDIO,
    CODEC_ID_QCELP,
    CODEC_ID_WAVPACK,
    CODEC_ID_DSICINAUDIO,
    CODEC_ID_IMC,
    CODEC_ID_MUSEPACK7,
    CODEC_ID_MLP,
    CODEC_ID_GSM_MS, /* as found in WAV */
    CODEC_ID_ATRAC3,
//#if FF_API_VOXWARE
    CODEC_ID_VOXWARE,
//#endif
    CODEC_ID_APE,
    CODEC_ID_NELLYMOSER,
    CODEC_ID_MUSEPACK8,
    CODEC_ID_SPEEX,
    CODEC_ID_WMAVOICE,
    CODEC_ID_WMAPRO,
    CODEC_ID_WMALOSSLESS,
    CODEC_ID_ATRAC3P,
    CODEC_ID_EAC3,
    CODEC_ID_SIPR,
    CODEC_ID_MP1,
    CODEC_ID_TWINVQ,
    CODEC_ID_TRUEHD,
    CODEC_ID_MP4ALS,
    CODEC_ID_ATRAC1,
    CODEC_ID_BINKAUDIO_RDFT,
    CODEC_ID_BINKAUDIO_DCT,
    CODEC_ID_AAC_LATM,
    CODEC_ID_QDMC,
    CODEC_ID_CELT,
    CODEC_ID_G723_1,
    CODEC_ID_G729,
    CODEC_ID_8SVX_EXP,
    CODEC_ID_8SVX_FIB,
    CODEC_ID_BMV_AUDIO,
    CODEC_ID_RALF,
    CODEC_ID_IAC,
    CODEC_ID_ILBC,
    CODEC_ID_OPUS_DEPRECATED,
    CODEC_ID_COMFORT_NOISE,
    CODEC_ID_TAK_DEPRECATED,
    CODEC_ID_METASOUND,
    CODEC_ID_XMA2,
    CODEC_ID_FFWAVESYNTH = MKBETAG('F','F','W','S'),
    CODEC_ID_SONIC       = MKBETAG('S','O','N','C'),
    CODEC_ID_SONIC_LS    = MKBETAG('S','O','N','L'),
    CODEC_ID_PAF_AUDIO   = MKBETAG('P','A','F','A'),
    CODEC_ID_OPUS        = MKBETAG('O','P','U','S'),
    CODEC_ID_TAK         = MKBETAG('t','B','a','K'),
    CODEC_ID_EVRC        = MKBETAG('s','e','v','c'),
    CODEC_ID_SMV         = MKBETAG('s','s','m','v'),

	CODEC_ID_SBC         = 0x15813,

    /* subtitle codecs */
    CODEC_ID_FIRST_SUBTITLE = 0x17000,          ///< A dummy ID pointing at the start of subtitle codecs.
    CODEC_ID_DVD_SUBTITLE = 0x17000,
    CODEC_ID_DVB_SUBTITLE,
    CODEC_ID_TEXT,  ///< raw UTF-8 text
    CODEC_ID_XSUB,
    CODEC_ID_SSA,
    CODEC_ID_MOV_TEXT,
    CODEC_ID_HDMV_PGS_SUBTITLE,
    CODEC_ID_DVB_TELETEXT,
    CODEC_ID_SRT,
    CODEC_ID_TTML,
    CODEC_ID_MICRODVD   = MKBETAG('m','D','V','D'),
    CODEC_ID_EIA_608    = MKBETAG('c','6','0','8'),
    CODEC_ID_JACOSUB    = MKBETAG('J','S','U','B'),
    CODEC_ID_SAMI       = MKBETAG('S','A','M','I'),
    CODEC_ID_REALTEXT   = MKBETAG('R','T','X','T'),
    CODEC_ID_SUBVIEWER1 = MKBETAG('S','b','V','1'),
    CODEC_ID_SUBVIEWER  = MKBETAG('S','u','b','V'),
    CODEC_ID_SUBRIP     = MKBETAG('S','R','i','p'),
    CODEC_ID_WEBVTT     = MKBETAG('W','V','T','T'),
    CODEC_ID_MPL2       = MKBETAG('M','P','L','2'),
    CODEC_ID_VPLAYER    = MKBETAG('V','P','l','r'),
    CODEC_ID_PJS        = MKBETAG('P','h','J','S'),
    CODEC_ID_ASS        = MKBETAG('A','S','S',' '),  ///< ASS as defined in Matroska

    /* other specific kind of codecs (generally used for attachments) */
    CODEC_ID_FIRST_UNKNOWN = 0x18000,           ///< A dummy ID pointing at the start of various fake codecs.
    CODEC_ID_TTF = 0x18000,
    CODEC_ID_BIN_DATA,
    CODEC_ID_BINTEXT    = MKBETAG('B','T','X','T'),
    CODEC_ID_XBIN       = MKBETAG('X','B','I','N'),
    CODEC_ID_IDF        = MKBETAG( 0 ,'I','D','F'),
    CODEC_ID_OTF        = MKBETAG( 0 ,'O','T','F'),
    CODEC_ID_SMPTE_KLV  = MKBETAG('K','L','V','A'),
    CODEC_ID_DVD_NAV    = MKBETAG('D','N','A','V'),
    CODEC_ID_TIMED_ID3  = MKBETAG('T','I','D','3'),

    CODEC_ID_PROBE = 0x19000, ///< codec_id is not known (like CODEC_ID_NONE) but lavf should attempt to identify it

    CODEC_ID_MPEG2TS = 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
    CODEC_ID_MPEG4SYSTEMS = 0x20001, /**< _FAKE_ codec to indicate a MPEG-4 Systems
                                * stream (only used by libavformat) */
    CODEC_ID_FFMETADATA = 0x21000,   ///< Dummy codec for streams containing only metadata information.

///nationalchip codec type

    CODEC_ID_MPEG1,
    CODEC_ID_MPEG2,

#define AV_CODEC_ID_MPEG1VIDEO                       CODEC_ID_MPEG1VIDEO
#define AV_CODEC_ID_MPEG2VIDEO                       CODEC_ID_MPEG2VIDEO
#define AV_CODEC_ID_MPEG2VIDEO_XVMC                  CODEC_ID_MPEG2VIDEO_XVMC
#define AV_CODEC_ID_H261                             CODEC_ID_H261
#define AV_CODEC_ID_H263                             CODEC_ID_H263
#define AV_CODEC_ID_RV10                             CODEC_ID_RV10
#define AV_CODEC_ID_RV20                             CODEC_ID_RV20
#define AV_CODEC_ID_MJPEG                            CODEC_ID_MJPEG
#define AV_CODEC_ID_MJPEGB                           CODEC_ID_MJPEGB
#define AV_CODEC_ID_LJPEG                            CODEC_ID_LJPEG
#define AV_CODEC_ID_SP5X                             CODEC_ID_SP5X
#define AV_CODEC_ID_JPEGLS                           CODEC_ID_JPEGLS
#define AV_CODEC_ID_MPEG4                            CODEC_ID_MPEG4
#define AV_CODEC_ID_RAWVIDEO                         CODEC_ID_RAWVIDEO
#define AV_CODEC_ID_MSMPEG4V1                        CODEC_ID_MSMPEG4V1
#define AV_CODEC_ID_MSMPEG4V2                        CODEC_ID_MSMPEG4V2
#define AV_CODEC_ID_MSMPEG4V3                        CODEC_ID_MSMPEG4V3
#define AV_CODEC_ID_WMV1                             CODEC_ID_WMV1
#define AV_CODEC_ID_WMV2                             CODEC_ID_WMV2
#define AV_CODEC_ID_H263P                            CODEC_ID_H263P
#define AV_CODEC_ID_H263I                            CODEC_ID_H263I
#define AV_CODEC_ID_FLV1                             CODEC_ID_FLV1
#define AV_CODEC_ID_SVQ1                             CODEC_ID_SVQ1
#define AV_CODEC_ID_SVQ3                             CODEC_ID_SVQ3
#define AV_CODEC_ID_DVVIDEO                          CODEC_ID_DVVIDEO
#define AV_CODEC_ID_HUFFYUV                          CODEC_ID_HUFFYUV
#define AV_CODEC_ID_CYUV                             CODEC_ID_CYUV
#define AV_CODEC_ID_H264                             CODEC_ID_H264
#define AV_CODEC_ID_INDEO3                           CODEC_ID_INDEO3
#define AV_CODEC_ID_VP3                              CODEC_ID_VP3
#define AV_CODEC_ID_THEORA                           CODEC_ID_THEORA
#define AV_CODEC_ID_ASV1                             CODEC_ID_ASV1
#define AV_CODEC_ID_ASV2                             CODEC_ID_ASV2
#define AV_CODEC_ID_FFV1                             CODEC_ID_FFV1
#define AV_CODEC_ID_4XM                              CODEC_ID_4XM
#define AV_CODEC_ID_VCR1                             CODEC_ID_VCR1
#define AV_CODEC_ID_CLJR                             CODEC_ID_CLJR
#define AV_CODEC_ID_MDEC                             CODEC_ID_MDEC
#define AV_CODEC_ID_ROQ                              CODEC_ID_ROQ
#define AV_CODEC_ID_INTERPLAY_VIDEO                  CODEC_ID_INTERPLAY_VIDEO
#define AV_CODEC_ID_XAN_WC3                          CODEC_ID_XAN_WC3
#define AV_CODEC_ID_XAN_WC4                          CODEC_ID_XAN_WC4
#define AV_CODEC_ID_RPZA                             CODEC_ID_RPZA
#define AV_CODEC_ID_CINEPAK                          CODEC_ID_CINEPAK
#define AV_CODEC_ID_WS_VQA                           CODEC_ID_WS_VQA
#define AV_CODEC_ID_MSRLE                            CODEC_ID_MSRLE
#define AV_CODEC_ID_MSVIDEO1                         CODEC_ID_MSVIDEO1
#define AV_CODEC_ID_IDCIN                            CODEC_ID_IDCIN
#define AV_CODEC_ID_8BPS                             CODEC_ID_8BPS
#define AV_CODEC_ID_SMC                              CODEC_ID_SMC
#define AV_CODEC_ID_FLIC                             CODEC_ID_FLIC
#define AV_CODEC_ID_TRUEMOTION1                      CODEC_ID_TRUEMOTION1
#define AV_CODEC_ID_VMDVIDEO                         CODEC_ID_VMDVIDEO
#define AV_CODEC_ID_MSZH                             CODEC_ID_MSZH
#define AV_CODEC_ID_ZLIB                             CODEC_ID_ZLIB
#define AV_CODEC_ID_QTRLE                            CODEC_ID_QTRLE
#define AV_CODEC_ID_TSCC                             CODEC_ID_TSCC
#define AV_CODEC_ID_ULTI                             CODEC_ID_ULTI
#define AV_CODEC_ID_QDRAW                            CODEC_ID_QDRAW
#define AV_CODEC_ID_VIXL                             CODEC_ID_VIXL
#define AV_CODEC_ID_QPEG                             CODEC_ID_QPEG
#define AV_CODEC_ID_PNG                              CODEC_ID_PNG
#define AV_CODEC_ID_PPM                              CODEC_ID_PPM
#define AV_CODEC_ID_PBM                              CODEC_ID_PBM
#define AV_CODEC_ID_PGM                              CODEC_ID_PGM
#define AV_CODEC_ID_PGMYUV                           CODEC_ID_PGMYUV
#define AV_CODEC_ID_PAM                              CODEC_ID_PAM
#define AV_CODEC_ID_FFVHUFF                          CODEC_ID_FFVHUFF
#define AV_CODEC_ID_RV30                             CODEC_ID_RV30
#define AV_CODEC_ID_RV40                             CODEC_ID_RV40
#define AV_CODEC_ID_VC1                              CODEC_ID_VC1
#define AV_CODEC_ID_WMV3                             CODEC_ID_WMV3
#define AV_CODEC_ID_LOCO                             CODEC_ID_LOCO
#define AV_CODEC_ID_WNV1                             CODEC_ID_WNV1
#define AV_CODEC_ID_AASC                             CODEC_ID_AASC
#define AV_CODEC_ID_INDEO2                           CODEC_ID_INDEO2
#define AV_CODEC_ID_FRAPS                            CODEC_ID_FRAPS
#define AV_CODEC_ID_TRUEMOTION2                      CODEC_ID_TRUEMOTION2
#define AV_CODEC_ID_BMP                              CODEC_ID_BMP
#define AV_CODEC_ID_CSCD                             CODEC_ID_CSCD
#define AV_CODEC_ID_MMVIDEO                          CODEC_ID_MMVIDEO
#define AV_CODEC_ID_ZMBV                             CODEC_ID_ZMBV
#define AV_CODEC_ID_AVS                              CODEC_ID_AVS
#define AV_CODEC_ID_SMACKVIDEO                       CODEC_ID_SMACKVIDEO
#define AV_CODEC_ID_NUV                              CODEC_ID_NUV
#define AV_CODEC_ID_KMVC                             CODEC_ID_KMVC
#define AV_CODEC_ID_FLASHSV                          CODEC_ID_FLASHSV
#define AV_CODEC_ID_CAVS                             CODEC_ID_CAVS
#define AV_CODEC_ID_JPEG2000                         CODEC_ID_JPEG2000
#define AV_CODEC_ID_VMNC                             CODEC_ID_VMNC
#define AV_CODEC_ID_VP5                              CODEC_ID_VP5
#define AV_CODEC_ID_VP6                              CODEC_ID_VP6
#define AV_CODEC_ID_VP6F                             CODEC_ID_VP6F
#define AV_CODEC_ID_TARGA                            CODEC_ID_TARGA
#define AV_CODEC_ID_DSICINVIDEO                      CODEC_ID_DSICINVIDEO
#define AV_CODEC_ID_TIERTEXSEQVIDEO                  CODEC_ID_TIERTEXSEQVIDEO
#define AV_CODEC_ID_TIFF                             CODEC_ID_TIFF
#define AV_CODEC_ID_GIF                              CODEC_ID_GIF
#define AV_CODEC_ID_DXA                              CODEC_ID_DXA
#define AV_CODEC_ID_DNXHD                            CODEC_ID_DNXHD
#define AV_CODEC_ID_THP                              CODEC_ID_THP
#define AV_CODEC_ID_SGI                              CODEC_ID_SGI
#define AV_CODEC_ID_C93                              CODEC_ID_C93
#define AV_CODEC_ID_BETHSOFTVID                      CODEC_ID_BETHSOFTVID
#define AV_CODEC_ID_PTX                              CODEC_ID_PTX
#define AV_CODEC_ID_TXD                              CODEC_ID_TXD
#define AV_CODEC_ID_VP6A                             CODEC_ID_VP6A
#define AV_CODEC_ID_AMV                              CODEC_ID_AMV
#define AV_CODEC_ID_VB                               CODEC_ID_VB
#define AV_CODEC_ID_PCX                              CODEC_ID_PCX
#define AV_CODEC_ID_SUNRAST                          CODEC_ID_SUNRAST
#define AV_CODEC_ID_INDEO4                           CODEC_ID_INDEO4
#define AV_CODEC_ID_INDEO5                           CODEC_ID_INDEO5
#define AV_CODEC_ID_MIMIC                            CODEC_ID_MIMIC
#define AV_CODEC_ID_RL2                              CODEC_ID_RL2
#define AV_CODEC_ID_ESCAPE124                        CODEC_ID_ESCAPE124
#define AV_CODEC_ID_DIRAC                            CODEC_ID_DIRAC
#define AV_CODEC_ID_BFI                              CODEC_ID_BFI
#define AV_CODEC_ID_CMV                              CODEC_ID_CMV
#define AV_CODEC_ID_MOTIONPIXELS                     CODEC_ID_MOTIONPIXELS
#define AV_CODEC_ID_TGV                              CODEC_ID_TGV
#define AV_CODEC_ID_TGQ                              CODEC_ID_TGQ
#define AV_CODEC_ID_TQI                              CODEC_ID_TQI
#define AV_CODEC_ID_AURA                             CODEC_ID_AURA
#define AV_CODEC_ID_AURA2                            CODEC_ID_AURA2
#define AV_CODEC_ID_V210X                            CODEC_ID_V210X
#define AV_CODEC_ID_TMV                              CODEC_ID_TMV
#define AV_CODEC_ID_V210                             CODEC_ID_V210
#define AV_CODEC_ID_DPX                              CODEC_ID_DPX
#define AV_CODEC_ID_MAD                              CODEC_ID_MAD
#define AV_CODEC_ID_FRWU                             CODEC_ID_FRWU
#define AV_CODEC_ID_FLASHSV2                         CODEC_ID_FLASHSV2
#define AV_CODEC_ID_CDGRAPHICS                       CODEC_ID_CDGRAPHICS
#define AV_CODEC_ID_R210                             CODEC_ID_R210
#define AV_CODEC_ID_ANM                              CODEC_ID_ANM
#define AV_CODEC_ID_BINKVIDEO                        CODEC_ID_BINKVIDEO
#define AV_CODEC_ID_IFF_ILBM                         CODEC_ID_IFF_ILBM
#define AV_CODEC_ID_IFF_BYTERUN1                     CODEC_ID_IFF_BYTERUN1
#define AV_CODEC_ID_KGV1                             CODEC_ID_KGV1
#define AV_CODEC_ID_YOP                              CODEC_ID_YOP
#define AV_CODEC_ID_VP8                              CODEC_ID_VP8
#define AV_CODEC_ID_PICTOR                           CODEC_ID_PICTOR
#define AV_CODEC_ID_ANSI                             CODEC_ID_ANSI
#define AV_CODEC_ID_A64_MULTI                        CODEC_ID_A64_MULTI
#define AV_CODEC_ID_A64_MULTI5                       CODEC_ID_A64_MULTI5
#define AV_CODEC_ID_R10K                             CODEC_ID_R10K
#define AV_CODEC_ID_MXPEG                            CODEC_ID_MXPEG
#define AV_CODEC_ID_LAGARITH                         CODEC_ID_LAGARITH
#define AV_CODEC_ID_PRORES                           CODEC_ID_PRORES
#define AV_CODEC_ID_JV                               CODEC_ID_JV
#define AV_CODEC_ID_DFA                              CODEC_ID_DFA
#define AV_CODEC_ID_WMV3IMAGE                        CODEC_ID_WMV3IMAGE
#define AV_CODEC_ID_VC1IMAGE                         CODEC_ID_VC1IMAGE
#define AV_CODEC_ID_UTVIDEO                          CODEC_ID_UTVIDEO
#define AV_CODEC_ID_BMV_VIDEO                        CODEC_ID_BMV_VIDEO
#define AV_CODEC_ID_VBLE                             CODEC_ID_VBLE
#define AV_CODEC_ID_DXTORY                           CODEC_ID_DXTORY
#define AV_CODEC_ID_V410                             CODEC_ID_V410
#define AV_CODEC_ID_XWD                              CODEC_ID_XWD
#define AV_CODEC_ID_CDXL                             CODEC_ID_CDXL
#define AV_CODEC_ID_XBM                              CODEC_ID_XBM
#define AV_CODEC_ID_ZEROCODEC                        CODEC_ID_ZEROCODEC
#define AV_CODEC_ID_MSS1                             CODEC_ID_MSS1
#define AV_CODEC_ID_MSA1                             CODEC_ID_MSA1
#define AV_CODEC_ID_TSCC2                            CODEC_ID_TSCC2
#define AV_CODEC_ID_MTS2                             CODEC_ID_MTS2
#define AV_CODEC_ID_CLLC                             CODEC_ID_CLLC
#define AV_CODEC_ID_MSS2                             CODEC_ID_MSS2
#define AV_CODEC_ID_VP9                              CODEC_ID_VP9
#define AV_CODEC_ID_AIC                              CODEC_ID_AIC
#define AV_CODEC_ID_ESCAPE130_DEPRECATED             CODEC_ID_ESCAPE130_DEPRECATED
#define AV_CODEC_ID_G2M_DEPRECATED                   CODEC_ID_G2M_DEPRECATED
#define AV_CODEC_ID_WEBP_DEPRECATED                  CODEC_ID_WEBP_DEPRECATED
#define AV_CODEC_ID_HNM4_VIDEO                       CODEC_ID_HNM4_VIDEO
#define AV_CODEC_ID_HEVC_DEPRECATED                  CODEC_ID_HEVC_DEPRECATED
#define AV_CODEC_ID_FIC                              CODEC_ID_FIC
#define AV_CODEC_ID_BRENDER_PIX                      CODEC_ID_BRENDER_PIX
#define AV_CODEC_ID_Y41P                             CODEC_ID_Y41P
#define AV_CODEC_ID_ESCAPE130                        CODEC_ID_ESCAPE130
#define AV_CODEC_ID_EXR                              CODEC_ID_EXR
#define AV_CODEC_ID_AVRP                             CODEC_ID_AVRP

#define AV_CODEC_ID_012V                             CODEC_ID_012V
#define AV_CODEC_ID_G2M                              CODEC_ID_G2M
#define AV_CODEC_ID_AVUI                             CODEC_ID_AVUI
#define AV_CODEC_ID_AYUV                             CODEC_ID_AYUV
#define AV_CODEC_ID_TARGA_Y216                       CODEC_ID_TARGA_Y216
#define AV_CODEC_ID_V308                             CODEC_ID_V308
#define AV_CODEC_ID_V408                             CODEC_ID_V408
#define AV_CODEC_ID_YUV4                             CODEC_ID_YUV4
#define AV_CODEC_ID_SANM                             CODEC_ID_SANM
#define AV_CODEC_ID_PAF_VIDEO                        CODEC_ID_PAF_VIDEO
#define AV_CODEC_ID_AVRN                             CODEC_ID_AVRN
#define AV_CODEC_ID_CPIA                             CODEC_ID_CPIA
#define AV_CODEC_ID_XFACE                            CODEC_ID_XFACE
#define AV_CODEC_ID_SGIRLE                           CODEC_ID_SGIRLE
#define AV_CODEC_ID_MVC1                             CODEC_ID_MVC1
#define AV_CODEC_ID_MVC2                             CODEC_ID_MVC2
#define AV_CODEC_ID_SNOW                             CODEC_ID_SNOW
#define AV_CODEC_ID_WEBP                             CODEC_ID_WEBP
#define AV_CODEC_ID_SMVJPEG                          CODEC_ID_SMVJPEG
#define AV_CODEC_ID_HEVC                             CODEC_ID_HEVC
#define AV_CODEC_ID_DRA                              CODEC_ID_DRA

#define AV_CODEC_ID_FIRST_AUDIO                      CODEC_ID_FIRST_AUDIO
#define AV_CODEC_ID_PCM_S16LE                        CODEC_ID_PCM_S16LE
#define AV_CODEC_ID_PCM_S16BE                        CODEC_ID_PCM_S16BE
#define AV_CODEC_ID_PCM_U16LE                        CODEC_ID_PCM_U16LE
#define AV_CODEC_ID_PCM_U16BE                        CODEC_ID_PCM_U16BE
#define AV_CODEC_ID_PCM_S8                           CODEC_ID_PCM_S8
#define AV_CODEC_ID_PCM_U8                           CODEC_ID_PCM_U8
#define AV_CODEC_ID_PCM_MULAW                        CODEC_ID_PCM_MULAW
#define AV_CODEC_ID_PCM_ALAW                         CODEC_ID_PCM_ALAW
#define AV_CODEC_ID_PCM_S32LE                        CODEC_ID_PCM_S32LE
#define AV_CODEC_ID_PCM_S32BE                        CODEC_ID_PCM_S32BE
#define AV_CODEC_ID_PCM_U32LE                        CODEC_ID_PCM_U32LE
#define AV_CODEC_ID_PCM_U32BE                        CODEC_ID_PCM_U32BE
#define AV_CODEC_ID_PCM_S24LE                        CODEC_ID_PCM_S24LE
#define AV_CODEC_ID_PCM_S24BE                        CODEC_ID_PCM_S24BE
#define AV_CODEC_ID_PCM_U24LE                        CODEC_ID_PCM_U24LE
#define AV_CODEC_ID_PCM_U24BE                        CODEC_ID_PCM_U24BE
#define AV_CODEC_ID_PCM_S24DAUD                      CODEC_ID_PCM_S24DAUD
#define AV_CODEC_ID_PCM_ZORK                         CODEC_ID_PCM_ZORK
#define AV_CODEC_ID_PCM_S16LE_PLANAR                 CODEC_ID_PCM_S16LE_PLANAR
#define AV_CODEC_ID_PCM_DVD                          CODEC_ID_PCM_DVD
#define AV_CODEC_ID_PCM_F32BE                        CODEC_ID_PCM_F32BE
#define AV_CODEC_ID_PCM_F32LE                        CODEC_ID_PCM_F32LE
#define AV_CODEC_ID_PCM_F64BE                        CODEC_ID_PCM_F64BE
#define AV_CODEC_ID_PCM_F64LE                        CODEC_ID_PCM_F64LE
#define AV_CODEC_ID_PCM_BLURAY                       CODEC_ID_PCM_BLURAY
#define AV_CODEC_ID_PCM_LXF                          CODEC_ID_PCM_LXF
#define AV_CODEC_ID_S302M                            CODEC_ID_S302M
#define AV_CODEC_ID_PCM_S8_PLANAR                    CODEC_ID_PCM_S8_PLANAR
#define AV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED      CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED
#define AV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED      CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED
#define AV_CODEC_ID_PCM_S24LE_PLANAR                 CODEC_ID_PCM_S24LE_PLANAR
#define AV_CODEC_ID_PCM_S32LE_PLANAR                 CODEC_ID_PCM_S32LE_PLANAR
#define AV_CODEC_ID_PCM_S16BE_PLANAR                 CODEC_ID_PCM_S16BE_PLANAR

#define AV_CODEC_ID_ADPCM_IMA_QT                     CODEC_ID_ADPCM_IMA_QT
#define AV_CODEC_ID_ADPCM_IMA_WAV                    CODEC_ID_ADPCM_IMA_WAV
#define AV_CODEC_ID_ADPCM_IMA_DK3                    CODEC_ID_ADPCM_IMA_DK3
#define AV_CODEC_ID_ADPCM_IMA_DK4                    CODEC_ID_ADPCM_IMA_DK4
#define AV_CODEC_ID_ADPCM_IMA_WS                     CODEC_ID_ADPCM_IMA_WS
#define AV_CODEC_ID_ADPCM_IMA_SMJPEG                 CODEC_ID_ADPCM_IMA_SMJPEG
#define AV_CODEC_ID_ADPCM_MS                         CODEC_ID_ADPCM_MS
#define AV_CODEC_ID_ADPCM_4XM                        CODEC_ID_ADPCM_4XM
#define AV_CODEC_ID_ADPCM_XA                         CODEC_ID_ADPCM_XA
#define AV_CODEC_ID_ADPCM_ADX                        CODEC_ID_ADPCM_ADX
#define AV_CODEC_ID_ADPCM_EA                         CODEC_ID_ADPCM_EA
#define AV_CODEC_ID_ADPCM_G726                       CODEC_ID_ADPCM_G726
#define AV_CODEC_ID_ADPCM_CT                         CODEC_ID_ADPCM_CT
#define AV_CODEC_ID_ADPCM_SWF                        CODEC_ID_ADPCM_SWF
#define AV_CODEC_ID_ADPCM_YAMAHA                     CODEC_ID_ADPCM_YAMAHA
#define AV_CODEC_ID_ADPCM_SBPRO_4                    CODEC_ID_ADPCM_SBPRO_4
#define AV_CODEC_ID_ADPCM_SBPRO_3                    CODEC_ID_ADPCM_SBPRO_3
#define AV_CODEC_ID_ADPCM_SBPRO_2                    CODEC_ID_ADPCM_SBPRO_2
#define AV_CODEC_ID_ADPCM_THP                        CODEC_ID_ADPCM_THP
#define AV_CODEC_ID_ADPCM_IMA_AMV                    CODEC_ID_ADPCM_IMA_AMV
#define AV_CODEC_ID_ADPCM_EA_R1                      CODEC_ID_ADPCM_EA_R1
#define AV_CODEC_ID_ADPCM_EA_R3                      CODEC_ID_ADPCM_EA_R3
#define AV_CODEC_ID_ADPCM_EA_R2                      CODEC_ID_ADPCM_EA_R2
#define AV_CODEC_ID_ADPCM_IMA_EA_SEAD                CODEC_ID_ADPCM_IMA_EA_SEAD
#define AV_CODEC_ID_ADPCM_IMA_EA_EACS                CODEC_ID_ADPCM_IMA_EA_EACS
#define AV_CODEC_ID_ADPCM_EA_XAS                     CODEC_ID_ADPCM_EA_XAS
#define AV_CODEC_ID_ADPCM_EA_MAXIS_XA                CODEC_ID_ADPCM_EA_MAXIS_XA
#define AV_CODEC_ID_ADPCM_IMA_ISS                    CODEC_ID_ADPCM_IMA_ISS
#define AV_CODEC_ID_ADPCM_G722                       CODEC_ID_ADPCM_G722
#define AV_CODEC_ID_ADPCM_IMA_APC                    CODEC_ID_ADPCM_IMA_APC
#define AV_CODEC_ID_VIMA                             CODEC_ID_VIMA
#define AV_CODEC_ID_ADPCM_AFC                        CODEC_ID_ADPCM_AFC
#define AV_CODEC_ID_ADPCM_IMA_OKI                    CODEC_ID_ADPCM_IMA_OKI
#define AV_CODEC_ID_ADPCM_DTK                        CODEC_ID_ADPCM_DTK
#define AV_CODEC_ID_ADPCM_IMA_RAD                    CODEC_ID_ADPCM_IMA_RAD
#define AV_CODEC_ID_ADPCM_G726LE                     CODEC_ID_ADPCM_G726LE

#define AV_CODEC_ID_AMR_NB                           CODEC_ID_AMR_NB
#define AV_CODEC_ID_AMR_WB                           CODEC_ID_AMR_WB

#define AV_CODEC_ID_RA_144                           CODEC_ID_RA_144
#define AV_CODEC_ID_RA_288                           CODEC_ID_RA_288

#define AV_CODEC_ID_ROQ_DPCM                         CODEC_ID_ROQ_DPCM
#define AV_CODEC_ID_INTERPLAY_DPCM                   CODEC_ID_INTERPLAY_DPCM
#define AV_CODEC_ID_XAN_DPCM                         CODEC_ID_XAN_DPCM
#define AV_CODEC_ID_SOL_DPCM                         CODEC_ID_SOL_DPCM

#define AV_CODEC_ID_MP2                              CODEC_ID_MP2
#define AV_CODEC_ID_MP3                              CODEC_ID_MP3
#define AV_CODEC_ID_AAC                              CODEC_ID_AAC
#define AV_CODEC_ID_AC3                              CODEC_ID_AC3
#define AV_CODEC_ID_DTS                              CODEC_ID_DTS
#define AV_CODEC_ID_VORBIS                           CODEC_ID_VORBIS
#define AV_CODEC_ID_DVAUDIO                          CODEC_ID_DVAUDIO
#define AV_CODEC_ID_WMAV1                            CODEC_ID_WMAV1
#define AV_CODEC_ID_WMAV2                            CODEC_ID_WMAV2
#define AV_CODEC_ID_MACE3                            CODEC_ID_MACE3
#define AV_CODEC_ID_MACE6                            CODEC_ID_MACE6
#define AV_CODEC_ID_VMDAUDIO                         CODEC_ID_VMDAUDIO
#define AV_CODEC_ID_FLAC                             CODEC_ID_FLAC
#define AV_CODEC_ID_MP3ADU                           CODEC_ID_MP3ADU
#define AV_CODEC_ID_MP3ON4                           CODEC_ID_MP3ON4
#define AV_CODEC_ID_SHORTEN                          CODEC_ID_SHORTEN
#define AV_CODEC_ID_ALAC                             CODEC_ID_ALAC
#define AV_CODEC_ID_WESTWOOD_SND1                    CODEC_ID_WESTWOOD_SND1
#define AV_CODEC_ID_GSM                              CODEC_ID_GSM
#define AV_CODEC_ID_QDM2                             CODEC_ID_QDM2
#define AV_CODEC_ID_COOK                             CODEC_ID_COOK
#define AV_CODEC_ID_TRUESPEECH                       CODEC_ID_TRUESPEECH
#define AV_CODEC_ID_TTA                              CODEC_ID_TTA
#define AV_CODEC_ID_SMACKAUDIO                       CODEC_ID_SMACKAUDIO
#define AV_CODEC_ID_QCELP                            CODEC_ID_QCELP
#define AV_CODEC_ID_WAVPACK                          CODEC_ID_WAVPACK
#define AV_CODEC_ID_DSICINAUDIO                      CODEC_ID_DSICINAUDIO
#define AV_CODEC_ID_IMC                              CODEC_ID_IMC
#define AV_CODEC_ID_MUSEPACK7                        CODEC_ID_MUSEPACK7
#define AV_CODEC_ID_MLP                              CODEC_ID_MLP
#define AV_CODEC_ID_GSM_MS                           CODEC_ID_GSM_MS
#define AV_CODEC_ID_ATRAC3                           CODEC_ID_ATRAC3
#define AV_CODEC_ID_VOXWARE                          CODEC_ID_VOXWARE
#define AV_CODEC_ID_APE                              CODEC_ID_APE
#define AV_CODEC_ID_NELLYMOSER                       CODEC_ID_NELLYMOSER
#define AV_CODEC_ID_MUSEPACK8                        CODEC_ID_MUSEPACK8
#define AV_CODEC_ID_SPEEX                            CODEC_ID_SPEEX
#define AV_CODEC_ID_WMAVOICE                         CODEC_ID_WMAVOICE
#define AV_CODEC_ID_WMAPRO                           CODEC_ID_WMAPRO
#define AV_CODEC_ID_WMALOSSLESS                      CODEC_ID_WMALOSSLESS
#define AV_CODEC_ID_ATRAC3P                          CODEC_ID_ATRAC3P
#define AV_CODEC_ID_EAC3                             CODEC_ID_EAC3
#define AV_CODEC_ID_SIPR                             CODEC_ID_SIPR
#define AV_CODEC_ID_MP1                              CODEC_ID_MP1
#define AV_CODEC_ID_TWINVQ                           CODEC_ID_TWINVQ
#define AV_CODEC_ID_TRUEHD                           CODEC_ID_TRUEHD
#define AV_CODEC_ID_MP4ALS                           CODEC_ID_MP4ALS
#define AV_CODEC_ID_ATRAC1                           CODEC_ID_ATRAC1
#define AV_CODEC_ID_BINKAUDIO_RDFT                   CODEC_ID_BINKAUDIO_RDFT
#define AV_CODEC_ID_BINKAUDIO_DCT                    CODEC_ID_BINKAUDIO_DCT
#define AV_CODEC_ID_AAC_LATM                         CODEC_ID_AAC_LATM
#define AV_CODEC_ID_QDMC                             CODEC_ID_QDMC
#define AV_CODEC_ID_CELT                             CODEC_ID_CELT
#define AV_CODEC_ID_G723_1                           CODEC_ID_G723_1
#define AV_CODEC_ID_G729                             CODEC_ID_G729
#define AV_CODEC_ID_8SVX_EXP                         CODEC_ID_8SVX_EXP
#define AV_CODEC_ID_8SVX_FIB                         CODEC_ID_8SVX_FIB
#define AV_CODEC_ID_BMV_AUDIO                        CODEC_ID_BMV_AUDIO
#define AV_CODEC_ID_RALF                             CODEC_ID_RALF
#define AV_CODEC_ID_IAC                              CODEC_ID_IAC
#define AV_CODEC_ID_ILBC                             CODEC_ID_ILBC
#define AV_CODEC_ID_OPUS_DEPRECATED                  CODEC_ID_OPUS_DEPRECATED
#define AV_CODEC_ID_COMFORT_NOISE                    CODEC_ID_COMFORT_NOISE
#define AV_CODEC_ID_TAK_DEPRECATED                   CODEC_ID_TAK_DEPRECATED
#define AV_CODEC_ID_METASOUND                        CODEC_ID_METASOUND
#define AV_CODEC_ID_FFWAVESYNTH                      CODEC_ID_FFWAVESYNTH
#define AV_CODEC_ID_SONIC                            CODEC_ID_SONIC
#define AV_CODEC_ID_SONIC_LS                         CODEC_ID_SONIC_LS
#define AV_CODEC_ID_PAF_AUDIO                        CODEC_ID_PAF_AUDIO
#define AV_CODEC_ID_OPUS                             CODEC_ID_OPUS
#define AV_CODEC_ID_TAK                              CODEC_ID_TAK
#define AV_CODEC_ID_EVRC                             CODEC_ID_EVRC
#define AV_CODEC_ID_SMV                              CODEC_ID_SMV

#define AV_CODEC_ID_SBC                              CODEC_ID_SBC

#define AV_CODEC_ID_FIRST_SUBTITLE                   CODEC_ID_FIRST_SUBTITLE
#define AV_CODEC_ID_DVD_SUBTITLE                     CODEC_ID_DVD_SUBTITLE
#define AV_CODEC_ID_DVB_SUBTITLE                     CODEC_ID_DVB_SUBTITLE
#define AV_CODEC_ID_TEXT                             CODEC_ID_TEXT
#define AV_CODEC_ID_XSUB                             CODEC_ID_XSUB
#define AV_CODEC_ID_SSA                              CODEC_ID_SSA
#define AV_CODEC_ID_MOV_TEXT                         CODEC_ID_MOV_TEXT
#define AV_CODEC_ID_HDMV_PGS_SUBTITLE                CODEC_ID_HDMV_PGS_SUBTITLE
#define AV_CODEC_ID_DVB_TELETEXT                     CODEC_ID_DVB_TELETEXT
#define AV_CODEC_ID_SRT                              CODEC_ID_SRT
#define AV_CODEC_ID_MICRODVD                         CODEC_ID_MICRODVD
#define AV_CODEC_ID_EIA_608                          CODEC_ID_EIA_608
#define AV_CODEC_ID_JACOSUB                          CODEC_ID_JACOSUB
#define AV_CODEC_ID_SAMI                             CODEC_ID_SAMI
#define AV_CODEC_ID_REALTEXT                         CODEC_ID_REALTEXT
#define AV_CODEC_ID_SUBVIEWER1                       CODEC_ID_SUBVIEWER1
#define AV_CODEC_ID_SUBVIEWER                        CODEC_ID_SUBVIEWER
#define AV_CODEC_ID_SUBRIP                           CODEC_ID_SUBRIP
#define AV_CODEC_ID_WEBVTT                           CODEC_ID_WEBVTT
#define AV_CODEC_ID_MPL2                             CODEC_ID_MPL2
#define AV_CODEC_ID_VPLAYER                          CODEC_ID_VPLAYER
#define AV_CODEC_ID_PJS                              CODEC_ID_PJS
#define AV_CODEC_ID_ASS                              CODEC_ID_ASS
#define AV_CODEC_ID_TTML                             CODEC_ID_TTML

#define AV_CODEC_ID_FIRST_UNKNOWN                    CODEC_ID_FIRST_UNKNOWN
#define AV_CODEC_ID_TTF                              CODEC_ID_TTF
#define AV_CODEC_ID_BINTEXT                          CODEC_ID_BINTEXT
#define AV_CODEC_ID_XBIN                             CODEC_ID_XBIN
#define AV_CODEC_ID_IDF                              CODEC_ID_IDF
#define AV_CODEC_ID_OTF                              CODEC_ID_OTF
#define AV_CODEC_ID_SMPTE_KLV                        CODEC_ID_SMPTE_KLV
#define AV_CODEC_ID_DVD_NAV                          CODEC_ID_DVD_NAV
#define AV_CODEC_ID_TIMED_ID3                        CODEC_ID_TIMED_ID3
#define AV_CODEC_ID_BIN_DATA                         CODEC_ID_BIN_DATA
#define AV_CODEC_ID_NONE                             CODEC_ID_NONE
#define AV_CODEC_ID_MPEG2TS                          CODEC_ID_MPEG2TS
#define AV_CODEC_ID_BITPACKED                        CODEC_ID_BITPACKED
};
#define AVCodecID CodecID

#define CODEC_ID_MP3LAME CODEC_ID_MP3
#define CODEC_ID_MPEG4AAC CODEC_ID_AAC


enum CodecType {
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_VIDEO,
    CODEC_TYPE_AUDIO,
    CODEC_TYPE_DATA,
    CODEC_TYPE_SUBTITLE,
    CODEC_TYPE_ATTACHMENT,
    CODEC_TYPE_HW
#define AVMEDIA_TYPE_UNKNOWN     CODEC_TYPE_UNKNOWN
#define AVMEDIA_TYPE_VIDEO       CODEC_TYPE_VIDEO
#define AVMEDIA_TYPE_AUDIO       CODEC_TYPE_AUDIO
#define AVMEDIA_TYPE_DATA        CODEC_TYPE_DATA
#define AVMEDIA_TYPE_SUBTITLE    CODEC_TYPE_SUBTITLE
#define AVMEDIA_TYPE_ATTACHMENT  CODEC_TYPE_ATTACHMENT
#define AVMEDIA_TYPE_NB          CODEC_TYPE_HW
};
#define AVMediaType CodecType

enum lavf_control_type {
    HLS_SEAMLESS_BANDWIDTH_SWITCH,//0
    HLS_IS_MULIT_PROGRAM,// 1
    HLS_MULIT_PROGRAM, // 2
    HLS_MULIT_VIDEO_TRACK, // 3
    HLS_IS_MULIT_VIDEO_TRACK, // 4
    HLS_SWITCH_AUDIO_MULITPROGRAM_TRACK, // 5
    HLS_IS_MULIT_AUDIO_TRACK, // 6
    HLS_MULIT_AUDIO_TRACK_STREAM_INDEX, // 7
    HLS_MULIT_SUBTITLE, // 8
    HLS_IS_MULIT_SUBTITLE, // 9
    DASH_SWITCH_VIDEO_MULITPROGRAM_TRACK, // 10
    DASH_SWITCH_AUDIO_MULITPROGRAM_TRACK, // 11
    DASH_SWITCH_SUBTILE_MULITPROGRAM_TRACK, // 12
};

/**
 * This struct describes the properties of a single codec described by an
 * AVCodecID.
 * @see avcodec_get_descriptor()
 */
typedef struct AVCodecDescriptor {
    enum CodecID     id;
    enum CodecType type;
    /**
     * Name of the codec described by this descriptor. It is non-empty and
     * unique for each codec descriptor. It should contain alphanumeric
     * characters and '_' only.
     */
    const char      *name;
    /**
     * Codec properties, a combination of AV_CODEC_PROP_* flags.
     */
    int             props;
} AVCodecDescriptor;

/**
 * Codec uses only intra compression.
 * Video codecs only.
 */
#define AV_CODEC_PROP_INTRA_ONLY    (1 << 0)
/**
 * Codec supports lossy compression. Audio and video codecs only.
 * @note a codec may support both lossy and lossless
 * compression modes
 */
#define AV_CODEC_PROP_LOSSY         (1 << 1)
/**
 * Codec supports lossless compression. Audio and video codecs only.
 */
#define AV_CODEC_PROP_LOSSLESS      (1 << 2)
/**
 * Subtitle codec is bitmap based
 * Decoded AVSubtitle data can be read from the AVSubtitleRect->pict field.
 */
#define AV_CODEC_PROP_BITMAP_SUB    (1 << 16)
/**
 * Subtitle codec is text based.
 * Decoded AVSubtitle data can be read from the AVSubtitleRect->ass field.
 */
#define AV_CODEC_PROP_TEXT_SUB      (1 << 17)


const AVCodecDescriptor *avcodec_descriptor_get(enum CodecID id);
const AVCodecDescriptor *avcodec_descriptor_next(const AVCodecDescriptor *prev);
const AVCodecDescriptor *avcodec_descriptor_get_by_name(const char *name);
#endif
