#ifndef __GX3201_GDI_H_
#define __GX3201_GDI_H_

#include "common/io.h"

typedef enum   {
	GX_COLOR_FMT_CLUT1 = 0, //0
	GX_COLOR_FMT_CLUT2,     //1
	GX_COLOR_FMT_CLUT4,     //2
	GX_COLOR_FMT_CLUT8,     //3
	GX_COLOR_FMT_RGBA4444,  //4
	GX_COLOR_FMT_RGBA5551,  //5
	GX_COLOR_FMT_RGB565,    //6
	GX_COLOR_FMT_RGBA8888,  //7
	GX_COLOR_FMT_RGB888,    //8
	GX_COLOR_FMT_BGR888,    //9

	GX_COLOR_FMT_ARGB4444, //10
	GX_COLOR_FMT_ARGB1555,  //11
	GX_COLOR_FMT_ARGB8888,  //12

	GX_COLOR_FMT_YCBCR422,  //13
	GX_COLOR_FMT_YCBCRA6442,//14
	GX_COLOR_FMT_YCBCR420,  //15

	GX_COLOR_FMT_YCBCR420_Y_UV, //16
	GX_COLOR_FMT_YCBCR420_Y_U_V,//17
	GX_COLOR_FMT_YCBCR420_Y,    //18
	GX_COLOR_FMT_YCBCR420_U,    //19
	GX_COLOR_FMT_YCBCR420_V,    //20
	GX_COLOR_FMT_YCBCR420_UV,   //21

	GX_COLOR_FMT_YCBCR422_Y_UV, //22
	GX_COLOR_FMT_YCBCR422_Y_U_V,//23
	GX_COLOR_FMT_YCBCR422_Y,    //24
	GX_COLOR_FMT_YCBCR422_U,    //25
	GX_COLOR_FMT_YCBCR422_V,    //26
	GX_COLOR_FMT_YCBCR422_UV,   //27

	GX_COLOR_FMT_YCBCR444,      //28
	GX_COLOR_FMT_YCBCR444_Y_UV, //29
	GX_COLOR_FMT_YCBCR444_Y_U_V,//30
	GX_COLOR_FMT_YCBCR444_Y,    //31
	GX_COLOR_FMT_YCBCR444_U,    //32
	GX_COLOR_FMT_YCBCR444_V,    //33
	GX_COLOR_FMT_YCBCR444_UV,   //34

	GX_COLOR_FMT_YCBCR400,      //35
	GX_COLOR_FMT_A8,            //36
	GX_COLOR_FMT_ABGR4444,      //37
	GX_COLOR_FMT_ABGR1555,      //39
	GX_COLOR_FMT_Y8,            //40
	GX_COLOR_FMT_UV16,          //41
	GX_COLOR_FMT_YCBCR422v,     //42
	GX_COLOR_FMT_YCBCR422h,     //43
} GxColorFormat;

#define gx_ioread32 __raw_readl
#define gx_iowrite32 __raw_writel
#define gx_clear_bit(nr, addr) REG_CLR_BIT(addr, nr)
#define gx_set_bit(nr, addr) REG_SET_BIT(addr, nr)
#define gx_test_bit(nr, addr) REG_GET_BIT(addr, nr)

//reference from gxavdev->gxav_bitops.h
#ifndef __GXAV_BITOPS_H__
#define __GXAV_BITOPS_H__

#define REG_SET_BIT64(reg, bit) do {            \
	gx_set_bit(reg[bit >> 5], bit % 32);    \
}while(0)

#define REG_CLR_BIT64(reg, bit) do {            \
	gx_clear_bit(reg[bit >> 5], bit % 32);  \
}while(0)

#define REG_SET_BITS(reg,bits) do {             \
	unsigned int Reg = gx_ioread32(reg);    \
	Reg   |=  (bits);                       \
	gx_iowrite32(Reg, reg);                 \
}while(0)

#define REG_CLR_BITS(reg,bits) do {             \
	unsigned int Reg = gx_ioread32(reg);    \
	Reg   &=  ~(bits);                      \
	gx_iowrite32(Reg, reg);                 \
}while(0)

#define REG_GET_BYTE0(reg)  ((reg)  &  0xFF)
#define REG_GET_BYTE1(reg)  (((reg) >> 8) & 0xFF)
#define REG_GET_BYTE2(reg)  (((reg) >> 16) & 0xFF)
#define REG_GET_BYTE3(reg)  (((reg) >> 24) & 0xFF)

#if 1 //#if (defined LITTLE_ENDIAN)||(defined __KERNEL__)

#define CHANGE_ENDIAN_32(reg) do {              \
	unsigned int Reg = (reg);               \
	Reg  = ((REG_GET_BYTE0(Reg) << 24) |    \
	(REG_GET_BYTE1(Reg) << 16) |            \
	(REG_GET_BYTE2(Reg) << 8 ) |            \
	(REG_GET_BYTE3(Reg))) ;                 \
	(reg)   =   (Reg) ;                     \
}while(0)

#define GET_ENDIAN_32(reg)                      \
    ((REG_GET_BYTE0(reg) << 24) |               \
     (REG_GET_BYTE1(reg) << 16) |               \
     (REG_GET_BYTE2(reg) << 8 ) |               \
     (REG_GET_BYTE3(reg)))

#define GET_ENDIAN_16(reg)                      \
     (REG_GET_BYTE0(reg) << 8 ) |               \
     (REG_GET_BYTE1(reg))

#else
#define CHANGE_ENDIAN_32(reg)
#define GET_ENDIAN_32(reg)     (reg)
#define GET_ENDIAN_16(reg)     (reg)
#endif


#define GX_SET_BIT(reg,bit) do{                \
	gx_set_bit(bit, reg);                  \
}while(0)

#define GX_CLR_BIT(reg,bit) do {               \
	gx_clear_bit(bit, reg);                \
}while(0)

#define GX_GET_BIT(reg,bit) do {               \
	gx_test_bit(bit, reg);                 \
}while(0)

#define GX_SET_FEILD(reg,mask,val,offset)  do{ \
	unsigned int Reg = gx_ioread32(reg);   \
	Reg  &=  ~(mask);                      \
	Reg  |=  ((val) << (offset)) & (mask); \
	gx_iowrite32(Reg, reg);                \
}while(0)

#define GX_GET_FEILD(reg,mask,val, offset)\
	(val) = ((gx_ioread32(reg)&(mask))>>(offset))

#define GX_SET_VAL(reg,val)                    \
    gx_iowrite32(val, reg)

#define GX_SET_VAL_E(reg,val) do {             \
	unsigned int tmpVal = val;             \
	CHANGE_ENDIAN_32(tmpVal) ;             \
	gx_iowrite32(tmpVal, reg);             \
}while(0)

#define GX_GET_VAL_E(reg,val) do {             \
	val = gx_ioread32(reg);                \
	val = CHANGE_ENDIAN_32(val) ;          \
}while(0)

#define GX_CMD_SET_VAL_E(reg,val) do {         \
    unsigned int tmpVal = val;                 \
	CHANGE_ENDIAN_32(tmpVal) ;             \
	gx_iowrite32(tmpVal, reg);             \
}while(0)

#define GX_CMD_SET_VAL(reg,val) do {           \
    unsigned int tmpVal = val;                 \
	gx_iowrite32(tmpVal, reg);             \
}while(0)

#define GX_SET_FEILD_E(reg,mask,val,offset) do { \
	unsigned int tmpVal  = gx_ioread32(reg); \
	CHANGE_ENDIAN_32(tmpVal) ;               \
	tmpVal  &=  ~(mask);                     \
	tmpVal  |=  ((val) << (offset)) & (mask);\
	CHANGE_ENDIAN_32(tmpVal) ;               \
	gx_iowrite32(tmpVal, reg);               \
}while(0)

#define GX_GET_FEILD_E(reg,mask,val,offset) do {      \
	unsigned int tmpVal  = *(unsigned int*)(reg); \
	CHANGE_ENDIAN_32(tmpVal) ;                    \
	(val) = (tmpVal & (mask)) >> (offset);        \
}while(0)

#define GX_SET_BIT_E(reg,bit) do {               \
	unsigned int tmpVal  = gx_ioread32(reg); \
	CHANGE_ENDIAN_32(tmpVal);                \
	tmpVal |=(1<<(bit));                     \
	CHANGE_ENDIAN_32(tmpVal);                \
	gx_iowrite32(tmpVal, reg);               \
}while(0)

#define GX_CLR_BIT_E(reg,bit) do {               \
	unsigned int tmpVal  = gx_ioread32(reg); \
	CHANGE_ENDIAN_32(tmpVal);                \
	tmpVal  &= (~(1<<(bit)));                \
	CHANGE_ENDIAN_32(tmpVal);                \
	gx_iowrite32(tmpVal, reg);               \
}while(0)

#endif    /* __GXAV_BITOPS_H__ */


//reference from gxavdev->gx3201_vpu_internel.h
typedef enum {
	DCBA_HGFE = 0,
	EFGH_ABCD = 1,
	HGFE_DCBA = 2,
	ABCD_EFGH = 3,
	CDAB_GHEF = 4,
	FEHG_BADC = 5,
	GHEF_CDAB = 6,
	BADC_FEHG = 7,
}ByteSequence;

#define IS_REFERENCE_COLOR(color)              \
	(((color) >= GX_COLOR_FMT_CLUT1) && ((color) <= GX_COLOR_FMT_CLUT8))

//reference from gxavdev->gx3201_vpu_reg.h
/*****************************************************************************/
/*                                   OSD                                     */
/*****************************************************************************/
typedef struct {
	unsigned int    word1;
	unsigned int    word2;
	unsigned int    word3;
	unsigned int    word4;
	unsigned int    word5;
	unsigned int    word6;
	unsigned int    word7;
}OsdRegionHead;

#define OSD_ADDR_MASK             (0xFFFFFFFF)
#define OSD_BASE_LINE_MASK        (0x0001FFF)

#define OSD_FLICKER_TABLE_LEN     (64)

//OSD_CTRL
#define bVPU_OSD_EN               (0)
#define bVPU_OSD_VT_PHASE         (8)
#define bVPU_OSD_HZOOM            (0)
#define bVPU_OSD_VZOOM            (16)
#define bVPU_OSD_ZOOM_MODE_EN_IPS (25)
#define bVPU_OSD_HDOWN_SAMPLE_EN  (28)
#define bVPU_OSD_ANTI_FLICKER     (29)
#define bVPU_OSD_ANTI_FLICKER_CBCR (31)
#define mVPU_OSD_EN               (0x1<<bVPU_OSD_EN)
#define mVPU_OSD_VT_PHASE         (0x3FFF<<bVPU_OSD_VT_PHASE)
#define mVPU_OSD_VZOOM            (0x3FFF<<bVPU_OSD_VZOOM)
#define mVPU_OSD_HZOOM            (0x3FFF<<bVPU_OSD_HZOOM)
#define mVPU_OSD_HDOWN_SAMPLE_EN  (0x1<<bVPU_OSD_HDOWN_SAMPLE_EN)

/*
 * sequences ....... value
 * DCBA_HGFE .......   0
 * EFGH_ABCD .......   1
 * HGFE_DCBA .......   2
 * ABCD_EFGH .......   3
 *
 * reg: 0xa48000c8 [21:20]
 * understanding:
 * If points on a line are p0,p1,p2,p3...
 * And the color value of them are vale0, vale1, vale2, vale3, ...
 * VPU read 8 bytes each time.
 * 1)32 bpp VPU defined vale0 = (A<<24)|(B<<16)|(C<<8)|(D<<0) ...
 *      example: p0 = 0x11223344,
 *      gdb>>x /x &p0
 *      gdb>>0x11223344
 *      so we should config: ABCD_EFGH
 * 2)16 bpp VPU defined vale0 = (A<<8 )|(B<<0), vale1 = (C<<8)|(D<<0) ...
 *      example:  p0 = 0x1122, p1 = 0x3344
 *      gdb>>x /x &p0
 *      gdb>>0x33441122
 *      so we should config: CDAB_GHEF
 * 3)8  bpp VPU defined vale0 = (A<<0), vale1 = (B<<0), vale2 = (C<<0) ...
 *      example: p0 = 0x11, p1 = 0x22, p2 = 0x33, p3 = 0x44
 *      gdb>>x /x &p0
 *      gdb>>0x44332211
 *      so we should config: DCBA_HGFE
 */
#define bVPU_RW_BYTESEQ_HIGH	(28)
#define bVPU_RW_BYTESEQ_LOW		(20)
#define mVPU_RW_BYTESEQ_LOW		(0x7<<bVPU_RW_BYTESEQ_LOW)
#define VPU_SET_RW_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_RW_BYTESEQ_LOW, byte_seq&0x7, bVPU_RW_BYTESEQ_LOW);\
	if(byte_seq>>2)\
		REG_SET_BIT(&(reg), bVPU_RW_BYTESEQ_HIGH);\
	else\
		REG_CLR_BIT(&(reg), bVPU_RW_BYTESEQ_HIGH);\
}while(0)

#define bVPU_OSD_REGIONHEAD_BYTESEQ   (12)
#define mVPU_OSD_REGIONHEAD_BYTESEQ   (0x7<<bVPU_OSD_REGIONHEAD_BYTESEQ)
#define VPU_OSD_SET_REGIONHEAD_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_OSD_REGIONHEAD_BYTESEQ, byte_seq&0x7, bVPU_OSD_REGIONHEAD_BYTESEQ);\
}while(0)

#define REG_SET_BIT_ABCD_EFGH	GX_SET_BIT
#define REG_SET_BIT_DCBA_HGFE	GX_SET_BIT_E
#define REG_SET_BIT_CDAB_GHEF(reg, bit)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal |=(1<<(bit));\
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_SET_BIT(reg, bit) \
	do{\
		switch(byte_seq)\
		{\
		case ABCD_EFGH:\
			REG_SET_BIT_ABCD_EFGH(reg, bit);\
			break;\
		case DCBA_HGFE:\
			REG_SET_BIT_DCBA_HGFE(reg, bit);\
			break;\
		case CDAB_GHEF:\
			REG_SET_BIT_CDAB_GHEF(reg, bit);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_CLR_BIT_ABCD_EFGH	GX_CLR_BIT
#define REG_CLR_BIT_DCBA_HGFE	GX_CLR_BIT_E
#define REG_CLR_BIT_CDAB_GHEF(reg, bit)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal &= (~(1<<(bit)));\
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_CLR_BIT(reg, bit) \
	do{\
		switch(byte_seq)\
		{\
		case ABCD_EFGH:\
			REG_CLR_BIT_ABCD_EFGH(reg, bit);\
			break;\
		case DCBA_HGFE:\
			REG_CLR_BIT_DCBA_HGFE(reg, bit);\
			break;\
		case CDAB_GHEF:\
			REG_CLR_BIT_CDAB_GHEF(reg, bit);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_SET_FEILD_ABCD_EFGH	GX_SET_FEILD
#define REG_SET_FEILD_DCBA_HGFE	GX_SET_FEILD_E
#define REG_SET_FEILD_CDAB_GHEF(reg,mask,val,offset)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal &=  ~(mask);\
		tmpVal |=  ((val) << (offset)) & (mask);\
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_SET_FEILD(reg,mask,val,offset)\
	do{\
		switch(byte_seq)\
		{\
		case ABCD_EFGH:\
			REG_SET_FEILD_ABCD_EFGH(reg,mask,val,offset);\
			break;\
		case DCBA_HGFE:\
			REG_SET_FEILD_DCBA_HGFE(reg,mask,val,offset);\
			break;\
		case CDAB_GHEF:\
			REG_SET_FEILD_CDAB_GHEF(reg,mask,val,offset);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_GET_FEILD_ABCD_EFGH	GX_GET_FEILD
#define REG_GET_FEILD_DCBA_HGFE	GX_GET_FEILD_E
#define REG_GET_FEILD_CDAB_GHEF(reg,mask,val,offset)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		(val)  = (tmpVal & (mask)) >> (offset);\
	}while(0)
#define OSD_HEAD_GET_FEILD(reg,mask,val,offset)\
	do{\
		switch(byte_seq)\
		{\
		case ABCD_EFGH:\
			REG_GET_FEILD_ABCD_EFGH(reg,mask,val,offset);\
			break;\
		case DCBA_HGFE:\
			REG_GET_FEILD_DCBA_HGFE(reg,mask,val,offset);\
			break;\
		case CDAB_GHEF:\
			REG_GET_FEILD_CDAB_GHEF(reg,mask,val,offset);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define VPU_OSD_ENABLE(reg) do{\
	while(!REG_GET_BIT(&(reg),bVPU_OSD_EN)){\
		REG_SET_BIT(&(reg),bVPU_OSD_EN);\
	}\
}while(0)

#define VPU_OSD_DISABLE(reg) do{\
	while(REG_GET_BIT(&(reg),bVPU_OSD_EN)){\
		REG_CLR_BIT(&(reg),bVPU_OSD_EN);\
	}\
}while(0)

#define VPU_OSD_ZOOM_WIDTH (12)
#define VPU_OSD_ZOOM_NONE (1 << VPU_OSD_ZOOM_WIDTH)

#define VPU_OSD_SET_VZOOM(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_VZOOM,value,bVPU_OSD_VZOOM)
#define VPU_OSD_GET_VZOOM(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_VZOOM, bVPU_OSD_VZOOM)
#define VPU_OSD_SET_HZOOM(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_HZOOM,value,bVPU_OSD_HZOOM)
#define VPU_OSD_GET_HZOOM(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_HZOOM, bVPU_OSD_HZOOM)
#define VPU_OSD_SET_VTOP_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_VT_PHASE,value,bVPU_OSD_VT_PHASE)
#define VPU_OSD_SET_ZOOM_MODE(reg)  \
	REG_SET_BIT(&(reg),bVPU_OSD_ANTI_FLICKER_CBCR)
#define VPU_OSD_CLR_ZOOM_MODE(reg)  \
	REG_CLR_BIT(&(reg),bVPU_OSD_ANTI_FLICKER_CBCR)
#define VPU_OSD_SET_ZOOM_MODE_EN_IPS(reg) \
	REG_SET_BIT(&(reg),bVPU_OSD_ZOOM_MODE_EN_IPS)
#define VPU_OSD_CLR_ZOOM_MODE_EN_IPS(reg) \
	REG_CLR_BIT(&(reg),bVPU_OSD_ZOOM_MODE_EN_IPS)

#define VPU_OSD_H_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_OSD_HDOWN_SAMPLE_EN)
#define VPU_OSD_H_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_OSD_HDOWN_SAMPLE_EN)

#define VPU_OSD_PP_ANTI_FLICKER_ENABLE(reg)	 REG_SET_BIT(&(reg),bVPU_OSD_ANTI_FLICKER)
#define VPU_OSD_PP_ANTI_FLICKER_DISABLE(reg) REG_CLR_BIT(&(reg),bVPU_OSD_ANTI_FLICKER)

//OSD_FIRST_HEAD_PTR
#define bVPU_OSD_FIRST_HEAD       (0)
#define mVPU_OSD_FIRST_HEAD       (OSD_ADDR_MASK<<bVPU_OSD_FIRST_HEAD)

#define VPU_OSD_SET_FIRST_HEAD(reg,value)\
	REG_SET_FIELD(&(reg),mVPU_OSD_FIRST_HEAD,value,bVPU_OSD_FIRST_HEAD)

//OSD_PHASE_0
#define VPU_OSD_PHASE_0_ANTI_FLICKER_ENABLE (0x408040)
#define VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE (0x01FF00)
#define bVPU_OSD_PHASE_0       (0)
#define mVPU_OSD_PHASE_0       (0xFFFFFF<<bVPU_OSD_PHASE_0)
#define VPU_OSD_SET_PHASE_0(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_PHASE_0,value,bVPU_OSD_PHASE_0)
#define VPU_OSD_GET_PHASE_0(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_PHASE_0, bVPU_OSD_PHASE_0)

//OSD_ALPHA_5551
#define bVPU_OSD_ALPHA_5551 (0)
#define mVPU_OSD_ALPHA_5551	(0xFFFF)
#define VPU_OSD_SET_ALPHA_5551(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_OSD_ALPHA_5551,value,bVPU_OSD_ALPHA_5551)

//BUFFER_CTRL2
#define bVPU_OSD_REQ_DATA_LEN     (0)
#define bVPU_BUFF_STATE_DELAY     (16)
#define mVPU_OSD_REQ_DATA_LEN     (0x7FF<<bVPU_OSD_REQ_DATA_LEN)
#define mVPU_BUFF_STATE_DELAY     (0xFF<<bVPU_BUFF_STATE_DELAY)

#define VPU_OSD_SET_BUFFER_REQ_DATA_LEN(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_REQ_DATA_LEN,value,bVPU_OSD_REQ_DATA_LEN)

#define VPU_SET_BUFF_STATE_DELAY(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_BUFF_STATE_DELAY,value,bVPU_BUFF_STATE_DELAY)

//OSD_COLOR_KEY
#define VPU_OSD_SET_COLOR_KEY(reg,R,G,B,A) do{\
	unsigned int Reg ; \
	Reg = (((unsigned char)R)<<24)|(((unsigned char)G)<<16)|(((unsigned char)B)<<8)|((unsigned char)A);\
	reg = Reg ;\
}while(0)

//rFLICKER
#define VPU_OSD_SET_FLIKER_FLITER(reg,i,Value) \
	REG_SET_VAL(&(reg[i]),Value)

//OSD_POSITION
#define bVPU_OSD_POSITION_X (0)
#define bVPU_OSD_POSITION_Y (16)
#define mVPU_OSD_POSITION_X (0x3FF << bVPU_OSD_POSITION_X)
#define mVPU_OSD_POSITION_Y (0x3FF << bVPU_OSD_POSITION_Y)

#define VPU_OSD_SET_POSITION(reg,x,y) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_OSD_POSITION_X,x,bVPU_OSD_POSITION_X); \
		REG_SET_FIELD(&(reg),mVPU_OSD_POSITION_Y,y,bVPU_OSD_POSITION_Y); \
	}while (0)

//OSD_SIZE
#define bVPU_OSD_VIEW_SIZE_WIDTH (0)
#define bVPU_OSD_VIEW_SIZE_HIGH  (16)
#define mVPU_OSD_VIEW_SIZE_WIDTH (0x7FF << bVPU_OSD_VIEW_SIZE_WIDTH)
#define mVPU_OSD_VIEW_SIZE_HIGH  (0x7FF << bVPU_OSD_VIEW_SIZE_HIGH)
#define VPU_OSD_SET_VIEW_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_OSD_VIEW_SIZE_WIDTH,width,bVPU_OSD_VIEW_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_OSD_VIEW_SIZE_HIGH,high,bVPU_OSD_VIEW_SIZE_HIGH); \
	}while (0)
//OSD_HEAD_WORD1
#define bVPU_OSD_CLUT_SWITCH      (0)
#define bVPU_OSD_CLUT_LENGTH      (8)
#define bVPU_OSD_COLOR_MODE       (10)
#define bVPU_OSD_CLUT_UPDATA_EN   (13)
#define bVPU_OSD_FLIKER_FLITER_EN (14)
#define bVPU_OSD_COLOR_KEY_EN     (15)
#define bVPU_OSD_MIX_WEIGHT       (16)
#define bVPU_OSD_GLOBAL_ALPHA_EN  (23)
#define bVPU_OSD_TRUE_COLOR_MODE  (24)
#define bVPU_OSD_ARGB_CONVERT     (26)

#define mVPU_OSD_CLUT_SWITCH      (0xFF << bVPU_OSD_CLUT_SWITCH)
#define mVPU_OSD_CLUT_LENGTH      (0x3  << bVPU_OSD_CLUT_LENGTH)
#define mVPU_OSD_COLOR_MODE       (0x7  << bVPU_OSD_COLOR_MODE)
#define mVPU_OSD_CLUT_UPDATA_EN   (0x1  << bVPU_OSD_CLUT_UPDATA_EN)
#define mVPU_OSD_FLIKER_FLITER_EN (0x1  << bVPU_OSD_FLIKER_FLITER_EN)
#define mVPU_OSD_COLOR_KEY_EN     (0x1  << bVPU_OSD_COLOR_KEY_EN)
#define mVPU_OSD_MIX_WEIGHT       (0x7F << bVPU_OSD_MIX_WEIGHT)
#define mVPU_OSD_GLOBAL_ALPHA_EN  (0x1  << bVPU_OSD_GLOBAL_ALPHA_EN)
#define mVPU_OSD_TRUE_COLOR_MODE  (0x3  << bVPU_OSD_TRUE_COLOR_MODE)
#define mVPU_OSD_ARGB_CONVERT     (0x1  << bVPU_OSD_ARGB_CONVERT)

#define VPU_OSD_COLOR_KEY_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_COLOR_KEY_EN)

#define VPU_OSD_COLOR_KEY_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_COLOR_KEY_EN)

#define VPU_OSD_CLUT_UPDATA_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_CLUT_UPDATA_EN)

#define VPU_OSD_CLUT_UPDATA_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_CLUT_UPDATA_EN)

#define VPU_OSD_FLIKER_FLITER_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_FLIKER_FLITER_EN)

#define VPU_OSD_FLIKER_FLITER_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_FLIKER_FLITER_EN)

#define VPU_OSD_GLOBAL_ALHPA_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_GLOBAL_ALPHA_EN)

#define VPU_OSD_GLOBAL_ALHPA_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_GLOBAL_ALPHA_EN)

#define VPU_OSD_SET_MIX_WEIGHT(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_MIX_WEIGHT,value,bVPU_OSD_MIX_WEIGHT)

#define VPU_OSD_SET_COLOR_TYPE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_COLOR_MODE,value,bVPU_OSD_COLOR_MODE)

#define VPU_OSD_GET_COLOR_TYPE(reg,value) \
	OSD_HEAD_GET_FEILD(&(reg), mVPU_OSD_COLOR_MODE, value, bVPU_OSD_COLOR_MODE)

#define VPU_OSD_SET_TRUE_COLOR_MODE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_TRUE_COLOR_MODE,value,bVPU_OSD_TRUE_COLOR_MODE)

#define VPU_OSD_SET_CLUT_LENGTH(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_CLUT_LENGTH,value,bVPU_OSD_CLUT_LENGTH)

#define VPU_OSD_SET_CLUT_SWITCH(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_CLUT_SWITCH,value,bVPU_OSD_CLUT_SWITCH)

#define VPU_OSD_SET_ARGB_CONVERT(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_ARGB_CONVERT,value,bVPU_OSD_ARGB_CONVERT)

//OSD_HEAD_WORD2
#define bVPU_OSD_CLUT_PTR         (0)
#define mVPU_OSD_CLUT_PTR         (OSD_ADDR_MASK<<bVPU_OSD_CLUT_PTR)

#define VPU_OSD_SET_CLUT_PTR(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg), mVPU_OSD_CLUT_PTR,value,bVPU_OSD_CLUT_PTR)

//OSD_HEAD_WORD3
#define bVPU_OSD_LEFT             (0)
#define bVPU_OSD_RIGHT            (16)
#define mVPU_OSD_LEFT             (0x7FF<<bVPU_OSD_LEFT)
#define mVPU_OSD_RIGHT            (0x7FF<<bVPU_OSD_RIGHT)

#define VPU_OSD_SET_WIDTH(reg,left,right) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_LEFT,left,bVPU_OSD_LEFT); \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_RIGHT,right,bVPU_OSD_RIGHT)

//OSD_HEAD_WORD4
#define bVPU_OSD_TOP              (0)
#define bVPU_OSD_BOTTOM           (16)
#define mVPU_OSD_TOP              (0x7FF<<bVPU_OSD_TOP)
#define mVPU_OSD_BOTTOM           (0x7FF<<bVPU_OSD_BOTTOM)

#define VPU_OSD_SET_HEIGHT(reg,top,bottom) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_TOP,top,bVPU_OSD_TOP); \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_BOTTOM,bottom,bVPU_OSD_BOTTOM)

//OSD_HEAD_WORD5
#define bVPU_OSD_DATA_ADDR         (0)
#define mVPU_OSD_DATA_ADDR         (OSD_ADDR_MASK<<bVPU_OSD_DATA_ADDR)

#define VPU_OSD_SET_DATA_ADDR(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_DATA_ADDR,value,bVPU_OSD_DATA_ADDR)

 //OSD_HEAD_WORD6
#define bVPU_OSD_NEXT_PTR         (0)
#define bVPU_OSD_LIST_END         (31)

#define mVPU_OSD_NEXT_PTR         (OSD_ADDR_MASK<<bVPU_OSD_NEXT_PTR)
#define mVPU_OSD_LIST_END         (0x1<<bVPU_OSD_LIST_END)

#define VPU_OSD_LIST_END_ENABLE(reg) \
        OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_LIST_END)
#define VPU_OSD_LIST_END_DISABLE(reg) \
        OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_LIST_END)

#define VPU_OSD_SET_NEXT_PTR(reg,value) \
        OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_NEXT_PTR,value,bVPU_OSD_NEXT_PTR)

//OSD_HEAD_WORD7
#define bVPU_OSD_BASE_LINE         (0)
#define mVPU_OSD_BASE_LINE         (OSD_BASE_LINE_MASK<<bVPU_OSD_BASE_LINE)
#define bVPU_OSD_ALPHA_RATIO       (16)
#define mVPU_OSD_ALPHA_RATIO       (0xFF<<bVPU_OSD_ALPHA_RATIO)
#define bVPU_OSD_ALPHA_RATIO_EN    (24)
#define VPU_OSD_SET_BASE_LINE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_BASE_LINE,value,bVPU_OSD_BASE_LINE)
#define VPU_OSD_SET_ALPHA_RATIO_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_ALPHA_RATIO_EN)
#define VPU_OSD_SET_ALPHA_RATIO_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_ALPHA_RATIO_EN)
#define VPU_OSD_SET_ALPHA_RATIO_VALUE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_ALPHA_RATIO,value,bVPU_OSD_ALPHA_RATIO)

#endif

